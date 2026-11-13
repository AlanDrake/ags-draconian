using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Tells which room areas (hotspots, walkable areas, walk-behinds, regions) are in use,
    /// for the layer tree's "Unused" group. Area 0 means "no area" and is always listed.
    /// </summary>
    public static class RoomAreaUsage
    {
        /// <summary>
        /// The number of entries in the result of <see cref="FindDrawnAreas"/>: one per 8-bit value.
        /// </summary>
        public const int MASK_VALUES = 256;

        /// <summary>
        /// Tells for each area number whether an 8-bit mask has at least one pixel of it.
        /// The padding at the end of each row is not read.
        /// </summary>
        public static bool[] FindDrawnAreas(byte[] pixels, int width, int height, int stride)
        {
            if (width < 0 || height < 0 || stride < width || (long)stride * height > pixels.Length)
                throw new ArgumentException("The pixel buffer is smaller than its dimensions.");
            bool[] drawn = new bool[MASK_VALUES];
            for (int y = 0; y < height; ++y)
            {
                int row = y * stride;
                for (int x = 0; x < width; ++x)
                    drawn[pixels[row + x]] = true;
            }
            return drawn;
        }

        /// <summary>
        /// Tells whether an area's properties are all at their defaults, by comparing its XML
        /// with a new area's: a property added to the area classes later is compared too.
        /// </summary>
        public static bool HasDefaultProperties(Room room, RoomAreaMaskType mask, int area)
        {
            IToXml item;
            IToXml fresh;
            // A new hotspot or region subscribes its Interactions to the global schema
            Interactions freshInteractions = null;
            switch (mask)
            {
                case RoomAreaMaskType.Hotspots:
                    RoomHotspot hotspot = room.Hotspots[area];
                    RoomHotspot freshHotspot = new RoomHotspot(room) { ID = area };
                    // Older editors named every hotspot slot "hHotspot<n>", "Hotspot <n>" (or
                    // "HOTSPOT<n>"); most rooms still have these names on the slots nobody used
                    if (hotspot.ScriptName == LegacyHotspotScriptName(area))
                        freshHotspot.ScriptName = hotspot.ScriptName;
                    if (IsLegacyHotspotDisplayName(hotspot.DisplayName, area))
                        freshHotspot.DisplayName = hotspot.DisplayName;
                    item = hotspot;
                    fresh = freshHotspot;
                    freshInteractions = freshHotspot.Interactions;
                    break;
                case RoomAreaMaskType.WalkableAreas:
                    item = room.WalkableAreas[area];
                    fresh = new RoomWalkableArea(room) { ID = area };
                    break;
                case RoomAreaMaskType.WalkBehinds:
                    item = room.WalkBehinds[area];
                    fresh = new RoomWalkBehind(room) { ID = area };
                    break;
                case RoomAreaMaskType.Regions:
                    item = room.Regions[area];
                    RoomRegion freshRegion = new RoomRegion(room) { ID = area };
                    fresh = freshRegion;
#pragma warning disable 0612 // obsolete for scripts, but a region still subscribes through it
                    freshInteractions = freshRegion.Interactions;
#pragma warning restore 0612
                    break;
                default:
                    throw new ArgumentException($"Illegal mask type, mask {mask} has no areas");
            }
            try
            {
                return item.ToXmlDocument().OuterXml == fresh.ToXmlDocument().OuterXml;
            }
            finally
            {
                // Unsubscribe, or the schema would keep every area made here
                if (freshInteractions != null)
                    freshInteractions.Schema = null;
            }
        }

        public static string LegacyHotspotScriptName(int area)
        {
            return "hHotspot" + area.ToString(System.Globalization.CultureInfo.InvariantCulture);
        }

        /// <summary>
        /// Tells whether a hotspot description is an older editor's default for this slot:
        /// "Hotspot &lt;n&gt;", in any case, with or without the space.
        /// </summary>
        public static bool IsLegacyHotspotDisplayName(string displayName, int area)
        {
            if (string.IsNullOrEmpty(displayName))
                return false;
            string number = area.ToString(System.Globalization.CultureInfo.InvariantCulture);
            return string.Equals(displayName, "Hotspot " + number, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(displayName, "Hotspot" + number, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// How an area is listed: area 0 and drawn areas normally, an area with properties but
        /// no pixels marked "nothing drawn", the others in the "Unused" group.
        /// </summary>
        public static RoomLayerItemState GetItemState(int area, bool drawn, bool defaultProperties)
        {
            if (area == 0 || drawn)
                return RoomLayerItemState.Normal;
            return defaultProperties ? RoomLayerItemState.Unused : RoomLayerItemState.NothingDrawn;
        }
    }
}
