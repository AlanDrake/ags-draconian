using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor.Panes.Room
{
    public enum RoomLayerNodeKind
    {
        /// <summary>
        /// The tree's root: the room itself.
        /// </summary>
        Room,
        /// <summary>
        /// A room editor layer.
        /// </summary>
        Layer,
        /// <summary>
        /// An item of a layer.
        /// </summary>
        Item,
        /// <summary>
        /// The group of a layer's unused items, at the end of the layer.
        /// </summary>
        UnusedGroup
    }

    /// <summary>
    /// A node of the room editor's layer tree. Visibility and lock are not stored here:
    /// <see cref="RoomLayerTree"/> reads and writes them on the layer.
    /// </summary>
    public sealed class RoomLayerNode
    {
        private readonly List<RoomLayerNode> _children = new List<RoomLayerNode>();

        internal RoomLayerNode(RoomLayerNodeKind kind, string key, string text, IRoomLayer layer,
            string itemID, RoomLayerItemState itemState, RoomLayerNode parent)
        {
            Kind = kind;
            Key = key;
            Text = text;
            Layer = layer;
            ItemID = itemID;
            ItemState = itemState;
            Parent = parent;
            parent?._children.Add(this);
        }

        public RoomLayerNodeKind Kind { get; }
        /// <summary>
        /// Identifies the node across rebuilds: the layer's Name, plus the item ID for an item.
        /// An item keeps its key when it moves in or out of the unused group, or is renamed.
        /// </summary>
        public string Key { get; }
        public string Text { get; }
        /// <summary>
        /// The layer of this node; null for the root.
        /// </summary>
        public IRoomLayer Layer { get; }
        /// <summary>
        /// The ID of the item in its layer's DesignItems; null unless this is an item.
        /// </summary>
        public string ItemID { get; }
        public RoomLayerItemState ItemState { get; }
        public RoomLayerNode Parent { get; }
        public IReadOnlyList<RoomLayerNode> Children { get { return _children; } }

        public bool HasVisibleToggle
        {
            get
            {
                return Kind == RoomLayerNodeKind.Layer ||
                    (Kind == RoomLayerNodeKind.Item && Layer.SupportVisibleItems);
            }
        }

        public bool HasLockToggle
        {
            get { return Kind == RoomLayerNodeKind.Layer || Kind == RoomLayerNodeKind.Item; }
        }

        public override string ToString()
        {
            return Text;
        }
    }

    /// <summary>
    /// The room editor's layer tree without its UI: the room, its layers and their items,
    /// the visible and locked state of each, and the rule that only one layer with a mask
    /// is visible at a time. A view draws the nodes and calls back here.
    /// </summary>
    public sealed class RoomLayerTree
    {
        public const string ROOT_KEY = "Room";
        private const char KEY_SEPARATOR = '/';
        private const string UNUSED_GROUP_ID = "#unused";

        private readonly List<IRoomLayer> _layers;
        private readonly Dictionary<string, RoomLayerNode> _nodes = new Dictionary<string, RoomLayerNode>();

        public RoomLayerTree(IEnumerable<IRoomLayer> layers)
        {
            _layers = new List<IRoomLayer>(layers);
            Rebuild();
        }

        public RoomLayerNode Root { get; private set; }

        /// <summary>
        /// Builds the nodes again from the layers' current items, names and item states.
        /// Nodes from an earlier build are not updated: look them up again by key.
        /// </summary>
        public void Rebuild()
        {
            _nodes.Clear();
            Root = Add(new RoomLayerNode(RoomLayerNodeKind.Room, ROOT_KEY, ROOT_KEY, null, null,
                RoomLayerItemState.Normal, null));
            foreach (IRoomLayer layer in _layers)
            {
                RoomLayerNode layerNode = Add(new RoomLayerNode(RoomLayerNodeKind.Layer, layer.Name,
                    layer.DisplayName, layer, null, RoomLayerItemState.Normal, Root));
                List<string> unused = new List<string>();
                foreach (string id in layer.DesignItems.Keys)
                {
                    RoomLayerItemState state = layer.GetItemState(id);
                    if (state == RoomLayerItemState.Unused)
                        unused.Add(id);
                    else
                        AddItem(layer, id, state, layerNode);
                }
                if (unused.Count == 0)
                    continue;
                RoomLayerNode group = Add(new RoomLayerNode(RoomLayerNodeKind.UnusedGroup,
                    MakeKey(layer, UNUSED_GROUP_ID), UnusedGroupText(unused.Count), layer, null,
                    RoomLayerItemState.Normal, layerNode));
                foreach (string id in unused)
                    AddItem(layer, id, RoomLayerItemState.Unused, group);
            }
        }

        public static string UnusedGroupText(int count)
        {
            return string.Format("Unused ({0})", count);
        }

        /// <summary>
        /// Finds the node with this key, or returns null.
        /// </summary>
        public RoomLayerNode Find(string key)
        {
            RoomLayerNode node;
            if (key != null && _nodes.TryGetValue(key, out node))
                return node;
            return null;
        }

        /// <summary>
        /// Finds the node with this key; if it is gone, its layer's node; if that is gone too,
        /// the root. Used to keep the selection over a rebuild.
        /// </summary>
        public RoomLayerNode FindNearest(string key)
        {
            RoomLayerNode node = Find(key);
            if (node != null)
                return node;
            if (key != null)
            {
                int separator = key.IndexOf(KEY_SEPARATOR);
                if (separator > 0)
                    node = Find(key.Substring(0, separator));
            }
            return node ?? Root;
        }

        /// <summary>
        /// Gets the node of the node's layer; null for the root.
        /// </summary>
        public RoomLayerNode GetLayerNode(RoomLayerNode node)
        {
            return node.Layer == null ? null : Find(node.Layer.Name);
        }

        public bool IsVisible(RoomLayerNode node)
        {
            if (node.Kind == RoomLayerNodeKind.Layer)
                return node.Layer.Visible;
            DesignTimeProperties props = GetItemProperties(node);
            return props == null || props.Visible;
        }

        public bool IsLocked(RoomLayerNode node)
        {
            if (node.Kind == RoomLayerNodeKind.Layer)
                return node.Layer.Locked;
            DesignTimeProperties props = GetItemProperties(node);
            return props != null && props.Locked;
        }

        /// <summary>
        /// Shows or hides a layer or item. Showing a layer with a mask hides the other layers
        /// with a mask. Returns whether anything changed; a node without a visible toggle
        /// changes nothing.
        /// </summary>
        public bool SetVisible(RoomLayerNode node, bool visible)
        {
            if (!node.HasVisibleToggle || IsVisible(node) == visible)
                return false;
            if (node.Kind == RoomLayerNodeKind.Layer)
            {
                node.Layer.Visible = visible;
                if (visible && node.Layer.MaskToDraw != RoomAreaMaskType.None)
                {
                    foreach (IRoomLayer other in _layers)
                    {
                        if (other == node.Layer || other.MaskToDraw == RoomAreaMaskType.None || !other.Visible)
                            continue;
                        other.Visible = false;
                        other.Modified = true;
                    }
                }
            }
            else
            {
                DesignTimeProperties props = GetItemProperties(node);
                if (props == null)
                    return false;
                props.Visible = visible;
            }
            node.Layer.Modified = true;
            return true;
        }

        /// <summary>
        /// Locks or unlocks a layer or item. Returns whether anything changed; a node without
        /// a lock toggle changes nothing.
        /// </summary>
        public bool SetLocked(RoomLayerNode node, bool locked)
        {
            if (!node.HasLockToggle || IsLocked(node) == locked)
                return false;
            if (node.Kind == RoomLayerNodeKind.Layer)
            {
                node.Layer.Locked = locked;
            }
            else
            {
                DesignTimeProperties props = GetItemProperties(node);
                if (props == null)
                    return false;
                props.Locked = locked;
            }
            node.Layer.Modified = true;
            return true;
        }

        /// <summary>
        /// Gets the node's path as the room's design-time user file stores the selection:
        /// "Room", "Room/&lt;layer display name&gt;", "Room/&lt;layer display name&gt;/&lt;item ID&gt;".
        /// The unused group is stored as its layer.
        /// </summary>
        public string GetPath(RoomLayerNode node)
        {
            if (node.Layer == null)
                return ROOT_KEY;
            string path = ROOT_KEY + KEY_SEPARATOR + node.Layer.DisplayName;
            if (node.Kind == RoomLayerNodeKind.Item)
                path += KEY_SEPARATOR + node.ItemID;
            return path;
        }

        /// <summary>
        /// Finds the node of a path made by <see cref="GetPath"/>, or by the editor before the
        /// layer tree. A layer is matched by display name or by name. Returns the deepest node
        /// found, or null if the path does not start at the root.
        /// </summary>
        public RoomLayerNode FindByPath(string path)
        {
            if (string.IsNullOrEmpty(path))
                return null;
            string[] parts = path.Split(KEY_SEPARATOR);
            if (parts[0] != ROOT_KEY)
                return null;
            if (parts.Length < 2)
                return Root;
            IRoomLayer layer = _layers.Find(l => l.DisplayName == parts[1]) ??
                _layers.Find(l => l.Name == parts[1]);
            if (layer == null)
                return Root;
            RoomLayerNode layerNode = Find(layer.Name);
            if (parts.Length < 3)
                return layerNode;
            return Find(MakeKey(layer, parts[2])) ?? layerNode;
        }

        private RoomLayerNode Add(RoomLayerNode node)
        {
            _nodes.Add(node.Key, node);
            return node;
        }

        private void AddItem(IRoomLayer layer, string id, RoomLayerItemState state, RoomLayerNode parent)
        {
            Add(new RoomLayerNode(RoomLayerNodeKind.Item, MakeKey(layer, id), layer.GetItemName(id) ?? id,
                layer, id, state, parent));
        }

        private static string MakeKey(IRoomLayer layer, string id)
        {
            return layer.Name + KEY_SEPARATOR + id;
        }

        private static DesignTimeProperties GetItemProperties(RoomLayerNode node)
        {
            if (node.Kind != RoomLayerNodeKind.Item)
                return null;
            DesignTimeProperties props;
            node.Layer.DesignItems.TryGetValue(node.ItemID, out props);
            return props;
        }
    }
}
