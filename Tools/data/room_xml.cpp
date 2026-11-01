//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "data/room_xml.h"
#include <algorithm>
#include <cctype>
#include "util/math.h"
#include "util/string_types.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::AGF;
using tinyxml2::XMLElement;

namespace AGS
{
namespace DataUtil
{

const char *ROOM_XML_FIRST_VERSION = "3.99.99.01";
const char *ROOM_XML_LATEST_VERSION = "4.0.0.34";
// Region light level and tint values moved to the 0-255 range (Room.XML_VERSION_VALUE_RANGES)
static const char *ROOM_XML_VERSION_VALUE_RANGES = "4.0.0.34";

// The editor reads at most this many items of each kind (Room.MAX_*)
static const size_t MAX_XML_OBJECTS = 256;
static const size_t MAX_XML_HOTSPOTS = 50;
static const size_t MAX_XML_WALKABLE_AREAS = 16;
static const size_t MAX_XML_WALK_BEHINDS = 16;
static const size_t MAX_XML_REGIONS = 16;

static const CstrArr<13> kBlendModeNames = {{"Normal", "Add", "Darken", "Lighten", "Multiply", "Screen", "Burn",
                                             "Subtract", "Exclusion", "Dodge", "Copy", "CopyRGB", "CopyAlpha"}};

//-----------------------------------------------------------------------------
// Value parsers, following the AGS.Types property setters
//-----------------------------------------------------------------------------

static bool ParseBlendMode(const char *text, int &value)
{
    return ParseXmlEnum(text, kBlendModeNames, value);
}

// Script names must be valid script identifiers (Utilities.ValidateScriptName)
static bool ParseScriptName(const char *text, String &value)
{
    for (const char *c = text; *c; ++c)
    {
        if ((c == text && std::isdigit(static_cast<unsigned char>(*c))) || !StrUtil::IsScriptWordChar(*c))
            return false;
    }
    value = text;
    return true;
}

// RoomObject.Image is never negative
static bool ParseImage(const char *text, int &value)
{
    if (!ParseXmlValue(text, value))
        return false;
    value = std::max(value, 0);
    return true;
}

// RoomObject.Opacity is clamped to 0-255
static bool ParseOpacity(const char *text, int &value)
{
    if (!ParseXmlValue(text, value))
        return false;
    value = Math::Clamp(value, 0, 255);
    return true;
}

// RoomRegion.TintSaturation cannot be 0: the editor refuses to load it
static bool ParseTintSaturation(const char *text, int &value)
{
    return ParseXmlValue(text, value) && value != 0;
}

// GraphicAnchor is "x, y" (GraphicAnchorTypeConverter): numbers that don't parse read as 0,
// and both are clamped to 0-1
static bool ParseGraphicAnchor(const char *text, Pointf &value)
{
    String s = text;
    s.Trim();
    if (s.IsEmpty())
    {
        value = Pointf(); // the converter gives null, which sets the default struct
        return true;
    }
    const auto parts = s.Split(',');
    if (parts.size() != 2)
        return false;
    float x = 0.f, y = 0.f;
    ParseXmlValue(parts[0].GetCStr(), x);
    ParseXmlValue(parts[1].GetCStr(), y);
    value = Pointf(Math::Clamp(x, 0.f, 1.f), Math::Clamp(y, 0.f, 1.f));
    return true;
}

//-----------------------------------------------------------------------------
// Readers for nested elements
//-----------------------------------------------------------------------------

static const XmlField<CustomPropertyValue> kCustomPropertyFields[] = {
    {"Name", Read<&CustomPropertyValue::Name>},
    {"Value", Read<&CustomPropertyValue::Value>},
};

// <Properties> with <CustomProperty> items; a malformed project may have duplicates,
// of which the editor keeps the first (names compare case-insensitively)
static HError ReadProperties(const XMLElement *elem, std::vector<CustomPropertyValue> &props)
{
    std::vector<CustomPropertyValue> items;
    HError err = ReadList(elem, "CustomProperty", items, kCustomPropertyFields);
    if (!err)
        return err;
    props.clear();
    for (auto &item : items)
    {
        const bool dup = std::any_of(props.begin(), props.end(), [&item](const CustomPropertyValue &p)
                                     { return p.Name.CompareNoCase(item.Name) == 0; });
        if (!dup)
            props.push_back(std::move(item));
    }
    return HError::None();
}

// <Interactions> with <Event Index= UID=>function</Event> items (Interactions.FromXml)
static HError ReadInteractions(const XMLElement *elem, InteractionsXml &inter)
{
    inter = {};
    for (const XMLElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if (std::strcmp(child->Name(), "Event") != 0)
        {
            if (std::strcmp(child->Name(), "ScriptModule") == 0)
                inter.ScriptModule = XmlDetail::ElementText(child);
            continue;
        }
        const int index = child->IntAttribute("Index", -1);
        const char *uid = child->Attribute("UID");
        const char *function = XmlDetail::ElementText(child);
        if (*function == 0)
            continue;
        if (uid && *uid)
            inter.ByUID[uid] = function;
        else if (index >= 0)
            inter.ByIndex[index] = function;
    }
    return HError::None();
}

// Transparency 0-100 of older formats, kept as Opacity 0-255 (TypesHelper.Trans100ToAlpha255)
static HError ReadTransparency(const XMLElement *elem, RoomObjectXml &obj)
{
    int transparency;
    if (!ParseXmlValue(XmlDetail::ElementText(elem), transparency))
        return XmlDetail::InvalidValue(elem);
    transparency = Math::Clamp(transparency, 0, 100);
    obj.Opacity = ((100 - transparency) * 255) / 100;
    return HError::None();
}

//-----------------------------------------------------------------------------
// Tables
//-----------------------------------------------------------------------------

static const XmlField<RoomObjectXml> kRoomObjectFields[] = {
    {"Baseline", Read<&RoomObjectXml::Baseline>},
    {"BlendMode", ReadWith<&RoomObjectXml::BlendMode, ParseBlendMode>},
    {"BlockingRectangle", Read<&RoomObjectXml::BlockingRectangle>},
    {"Clickable", Read<&RoomObjectXml::Clickable>},
    {"Description", Read<&RoomObjectXml::DisplayName>}, // before 4.0.0.34
    {"DisplayName", Read<&RoomObjectXml::DisplayName>},
    {"Enabled", Read<&RoomObjectXml::Enabled>},
    {"GraphicAnchor", ReadWith<&RoomObjectXml::GraphicAnchor, ParseGraphicAnchor>},
    {"GraphicOffset", Read<&RoomObjectXml::GraphicOffset>},
    {"Image", ReadWith<&RoomObjectXml::Image, ParseImage>},
    {"Interactions", ReadElement<&RoomObjectXml::Interactions, ReadInteractions>},
    {"Name", ReadWith<&RoomObjectXml::ScriptName, ParseScriptName>}, // before 4.0.0.34
    {"OnAnyClick", Read<&RoomObjectXml::OnAnyClick>},
    {"OnFrameEvent", Read<&RoomObjectXml::OnFrameEvent>},
    {"Opacity", ReadWith<&RoomObjectXml::Opacity, ParseOpacity>},
    {"Properties", ReadElement<&RoomObjectXml::Properties, ReadProperties>},
    {"ScriptName", ReadWith<&RoomObjectXml::ScriptName, ParseScriptName>},
    {"Solid", Read<&RoomObjectXml::Solid>},
    {"StartX", Read<&RoomObjectXml::StartX>},
    {"StartY", Read<&RoomObjectXml::StartY>},
    {"Transparency", ReadTransparency}, // before 4.0.0.34
    {"UseRoomAreaLighting", Read<&RoomObjectXml::UseRoomAreaLighting>},
    {"UseRoomAreaScaling", Read<&RoomObjectXml::UseRoomAreaScaling>},
    {"Visible", Read<&RoomObjectXml::Visible>},
};

static const XmlField<RoomHotspotXml> kRoomHotspotFields[] = {
    {"Description", Read<&RoomHotspotXml::DisplayName>}, // before 4.0.0.34
    {"DisplayName", Read<&RoomHotspotXml::DisplayName>},
    {"Interactions", ReadElement<&RoomHotspotXml::Interactions, ReadInteractions>},
    {"Name", ReadWith<&RoomHotspotXml::ScriptName, ParseScriptName>}, // before 4.0.0.34
    {"OnAnyClick", Read<&RoomHotspotXml::OnAnyClick>},
    {"OnMouseMove", Read<&RoomHotspotXml::OnMouseMove>},
    {"OnWalkOn", Read<&RoomHotspotXml::OnWalkOn>},
    {"Properties", ReadElement<&RoomHotspotXml::Properties, ReadProperties>},
    {"ScriptName", ReadWith<&RoomHotspotXml::ScriptName, ParseScriptName>},
    {"WalkToPoint", Read<&RoomHotspotXml::WalkToPoint>},
};

static const XmlField<RoomWalkableAreaXml> kRoomWalkableAreaFields[] = {
    {"AreaSpecificView", Read<&RoomWalkableAreaXml::AreaSpecificView>},
    {"FaceDirectionRatio", Read<&RoomWalkableAreaXml::FaceDirectionRatio>},
    {"MaxScalingLevel", Read<&RoomWalkableAreaXml::MaxScalingLevel>},
    {"MinScalingLevel", Read<&RoomWalkableAreaXml::MinScalingLevel>},
    {"Properties", ReadElement<&RoomWalkableAreaXml::Properties, ReadProperties>},
    {"ScalingLevel", Read<&RoomWalkableAreaXml::ScalingLevel>},
    {"ScriptName", ReadWith<&RoomWalkableAreaXml::ScriptName, ParseScriptName>},
    {"UseContinuousScaling", Read<&RoomWalkableAreaXml::UseContinuousScaling>},
};

static const XmlField<RoomWalkBehindXml> kRoomWalkBehindFields[] = {
    {"Baseline", Read<&RoomWalkBehindXml::Baseline>},
    {"ScriptName", ReadWith<&RoomWalkBehindXml::ScriptName, ParseScriptName>},
};

static const XmlField<RoomRegionXml> kRoomRegionFields[] = {
    {"BlueTint", Read<&RoomRegionXml::BlueTint>},
    {"GreenTint", Read<&RoomRegionXml::GreenTint>},
    {"Interactions", ReadElement<&RoomRegionXml::Interactions, ReadInteractions>}, // old indexed events only
    {"LightLevel", Read<&RoomRegionXml::LightLevel>},
    {"OnStanding", Read<&RoomRegionXml::OnStanding>},
    {"OnWalksOff", Read<&RoomRegionXml::OnWalksOff>},
    {"OnWalksOnto", Read<&RoomRegionXml::OnWalksOnto>},
    {"Properties", ReadElement<&RoomRegionXml::Properties, ReadProperties>},
    {"RedTint", Read<&RoomRegionXml::RedTint>},
    {"ScriptName", ReadWith<&RoomRegionXml::ScriptName, ParseScriptName>},
    {"TintLuminance", Read<&RoomRegionXml::TintLuminance>},
    {"TintSaturation", ReadWith<&RoomRegionXml::TintSaturation, ParseTintSaturation>},
    {"UseColourTint", Read<&RoomRegionXml::UseColourTint>},
};

static HError ReadObjects(const XMLElement *elem, std::vector<RoomObjectXml> &items)
{
    return ReadList(elem, "RoomObject", items, kRoomObjectFields, MAX_XML_OBJECTS);
}

static HError ReadHotspots(const XMLElement *elem, std::vector<RoomHotspotXml> &items)
{
    return ReadList(elem, "RoomHotspot", items, kRoomHotspotFields, MAX_XML_HOTSPOTS);
}

static HError ReadWalkableAreas(const XMLElement *elem, std::vector<RoomWalkableAreaXml> &items)
{
    return ReadList(elem, "RoomWalkableArea", items, kRoomWalkableAreaFields, MAX_XML_WALKABLE_AREAS);
}

static HError ReadWalkBehinds(const XMLElement *elem, std::vector<RoomWalkBehindXml> &items)
{
    return ReadList(elem, "RoomWalkBehind", items, kRoomWalkBehindFields, MAX_XML_WALK_BEHINDS);
}

static HError ReadRegions(const XMLElement *elem, std::vector<RoomRegionXml> &items)
{
    return ReadList(elem, "RoomRegion", items, kRoomRegionFields, MAX_XML_REGIONS);
}

static const XmlField<RoomXml> kRoomFields[] = {
    {"BackgroundAnimationDelay", Read<&RoomXml::BackgroundAnimationDelay>},
    {"BackgroundAnimationEnabled", Read<&RoomXml::BackgroundAnimationEnabled>},
    {"BackgroundCount", Read<&RoomXml::BackgroundCount>},
    {"BottomEdgeY", Read<&RoomXml::BottomEdgeY>},
    {"ColorDepth", Read<&RoomXml::ColorDepth>},
    {"Description", Read<&RoomXml::Description>},
    {"FaceDirectionRatio", Read<&RoomXml::FaceDirectionRatio>},
    {"GameID", Read<&RoomXml::GameID>},
    {"Height", Read<&RoomXml::Height>},
    {"Hotspots", ReadElement<&RoomXml::Hotspots, ReadHotspots>},
    {"Interactions", ReadElement<&RoomXml::Interactions, ReadInteractions>}, // old indexed events only
    {"LeftEdgeX", Read<&RoomXml::LeftEdgeX>},
    {"MaskResolution", Read<&RoomXml::MaskResolution>},
    {"Number", Read<&RoomXml::Number>},
    {"Objects", ReadElement<&RoomXml::Objects, ReadObjects>},
    {"OnAfterFadeIn", Read<&RoomXml::OnAfterFadeIn>},
    {"OnFirstTimeEnter", Read<&RoomXml::OnFirstTimeEnter>},
    {"OnLeave", Read<&RoomXml::OnLeave>},
    {"OnLeaveBottom", Read<&RoomXml::OnLeaveBottom>},
    {"OnLeaveLeft", Read<&RoomXml::OnLeaveLeft>},
    {"OnLeaveRight", Read<&RoomXml::OnLeaveRight>},
    {"OnLeaveTop", Read<&RoomXml::OnLeaveTop>},
    {"OnLoad", Read<&RoomXml::OnLoad>},
    {"OnRepExec", Read<&RoomXml::OnRepExec>},
    {"OnUnload", Read<&RoomXml::OnUnload>},
    {"PlayerCharacterView", Read<&RoomXml::PlayerCharacterView>},
    {"Properties", ReadElement<&RoomXml::Properties, ReadProperties>},
    {"Regions", ReadElement<&RoomXml::Regions, ReadRegions>},
    {"RightEdgeX", Read<&RoomXml::RightEdgeX>},
    {"ShowPlayerCharacter", Read<&RoomXml::ShowPlayerCharacter>},
    {"TopEdgeY", Read<&RoomXml::TopEdgeY>},
    {"WalkBehinds", ReadElement<&RoomXml::WalkBehinds, ReadWalkBehinds>},
    {"WalkableAreas", ReadElement<&RoomXml::WalkableAreas, ReadWalkableAreas>},
    {"Width", Read<&RoomXml::Width>},
    // Obsolete properties that older editors wrote, and the editor skips
    {"Messages", nullptr},
    {"MusicVolumeAdjustment", nullptr},
    {"PlayMusicOnRoomLoad", nullptr},
    {"SaveLoadEnabled", nullptr},
};

//-----------------------------------------------------------------------------
// Upgrades of older formats, as the editor does them when it loads a room
//-----------------------------------------------------------------------------

static String IndexedFunction(const std::map<int, String> &functions, int index)
{
    auto it = functions.find(index);
    return it != functions.end() ? it->second : String();
}

// Old interaction lists were indexed by event; the new ones are indexed by cursor ID
// (Interactions.RemapLegacyFunctionIndexes)
static void RemapIndexedFunctions(std::map<int, String> &functions, const std::vector<int> &event_for_cursor)
{
    std::map<int, String> remapped;
    for (size_t cursor = 0; cursor < event_for_cursor.size(); ++cursor)
    {
        auto it = functions.find(event_for_cursor[cursor]);
        if (it != functions.end())
            remapped[static_cast<int>(cursor)] = it->second;
    }
    functions = std::move(remapped);
}

// RoomObject.ConvertOldInteractionEvents
static void ConvertOldEvents(RoomObjectXml &obj)
{
    auto &functions = obj.Interactions.ByIndex;
    if (functions.empty())
        return;
    obj.OnAnyClick = IndexedFunction(functions, 4);
    // Walk, Look, Interact, Talk, UseInv, PickUp, Pointer, Wait, Mode8, Mode9
    RemapIndexedFunctions(functions, {-1, 0, 1, 2, 3, 5, -1, -1, 6, 7});
}

// RoomHotspot.ConvertOldInteractionEvents
static void ConvertOldEvents(RoomHotspotXml &hotspot)
{
    auto &functions = hotspot.Interactions.ByIndex;
    if (functions.empty())
        return;
    hotspot.OnWalkOn = IndexedFunction(functions, 0);
    hotspot.OnAnyClick = IndexedFunction(functions, 5);
    hotspot.OnMouseMove = IndexedFunction(functions, 6);
    RemapIndexedFunctions(functions, {-1, 1, 2, 4, 3, 7, -1, -1, 8, 9});
}

// RoomRegion.LoadAndConvertInteractionEvents
static void ConvertOldEvents(RoomRegionXml &region)
{
    const auto &functions = region.Interactions.ByIndex;
    if (!functions.empty())
    {
        region.OnStanding = IndexedFunction(functions, 0);
        region.OnWalksOnto = IndexedFunction(functions, 1);
        region.OnWalksOff = IndexedFunction(functions, 2);
    }
    region.Interactions = {};
}

// Room.LoadAndConvertInteractionEvents
static void ConvertOldEvents(RoomXml &room)
{
    const auto &functions = room.Interactions.ByIndex;
    if (!functions.empty())
    {
        room.OnLeaveLeft = IndexedFunction(functions, 0);
        room.OnLeaveRight = IndexedFunction(functions, 1);
        room.OnLeaveBottom = IndexedFunction(functions, 2);
        room.OnLeaveTop = IndexedFunction(functions, 3);
        room.OnFirstTimeEnter = IndexedFunction(functions, 4);
        room.OnLoad = IndexedFunction(functions, 5);
        room.OnRepExec = IndexedFunction(functions, 6);
        room.OnAfterFadeIn = IndexedFunction(functions, 7);
        room.OnLeave = IndexedFunction(functions, 8);
        room.OnUnload = IndexedFunction(functions, 9);
    }
    room.Interactions = {};
}

static void UpgradeRoom(RoomXml &room)
{
    ConvertOldEvents(room);
    for (auto &obj : room.Objects)
        ConvertOldEvents(obj);
    for (auto &hotspot : room.Hotspots)
        ConvertOldEvents(hotspot);
    for (auto &region : room.Regions)
        ConvertOldEvents(region);

    // RoomsComponent.UpgradeRoomFeatures
    if (CompareXmlVersions(room.Version.GetCStr(), ROOM_XML_VERSION_VALUE_RANGES) < 0)
    {
        // Region light level was 0-200 (100 = normal), tint saturation and luminance 0-100;
        // now they all use the full 0-255 range, and the light level is -255 to 255 (0 = normal)
        for (auto &region : room.Regions)
        {
            region.LightLevel = (region.LightLevel - 100) * 255 / 100;
            region.TintSaturation = region.TintSaturation * 255 / 100;
            region.TintLuminance = region.TintLuminance * 255 / 100;
        }
    }
}

//-----------------------------------------------------------------------------
// Reading
//-----------------------------------------------------------------------------

HError ReadRoomXml(const tinyxml2::XMLDocument &doc, RoomXml &room)
{
    const XMLElement *root = doc.RootElement();
    if (!root || std::strcmp(root->Name(), "Room") != 0)
        return new Error("Not a room data file: <Room> element not found");
    const char *version = root->Attribute("Version");
    if (!version)
        version = ROOM_XML_FIRST_VERSION; // as the editor assumes
    if (CompareXmlVersions(version, ROOM_XML_FIRST_VERSION) < 0)
        return new Error(String::FromFormat("Room format %s is older than any AGS 4 room format", version));
    if (CompareXmlVersions(version, ROOM_XML_LATEST_VERSION) > 0)
        return new Error(String::FromFormat("Room format %s is newer than these tools support (%s). Update the tools.",
                                            version, ROOM_XML_LATEST_VERSION));

    room = {};
    room.Version = version;
    HError err = ReadFields(root, room, kRoomFields);
    if (!err)
        return err;
    UpgradeRoom(room);
    return HError::None();
}

HError ReadRoomXmlFile(const String &filename, RoomXml &room)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename.GetCStr()) != tinyxml2::XML_SUCCESS)
        return new Error(String::FromFormat("Failed to read %s: %s", filename.GetCStr(), doc.ErrorStr()));
    HError err = ReadRoomXml(doc, room);
    if (!err)
        return new Error(err, String::FromFormat("Failed to read %s", filename.GetCStr()));
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
