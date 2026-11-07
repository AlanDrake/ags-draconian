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
#include <cstring>
#include <vector>

#include "data/room_build.h"
#include "data/room_xml.h"
#include "gfx/image_file.h"
#include "gtest/gtest.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace
{

HError ReadRoom(const char *xml, RoomXml &room)
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml) != tinyxml2::XML_SUCCESS)
        return new Error("XML parse error");
    return ReadRoomXml(doc, room);
}

// PixelBuffer does not clear its memory
PixelBuffer ClearBuffer(int width, int height, PixelFormat fmt)
{
    PixelBuffer buf(width, height, fmt);
    std::memset(buf.GetData(), 0, buf.GetDataSize());
    return buf;
}

RoomBuildGame MakeGame()
{
    RoomBuildGame game;
    game.UniqueID = 1234;
    game.DefaultRoomMaskResolution = 1;
    // cursors 1, 2 and 8 create events, as Look, Interact and Usermode1
    game.InteractionEvents = {{1, "Curs0002"}, {2, "Curs0003"}, {8, "Curs0009"}};
    CustomPropertySchemaItem prop;
    prop.Name = "Weight";
    game.PropertySchema.push_back(prop);
    return game;
}

} // namespace

TEST(RoomXml, UpgradesOlderFormats)
{
    // Format 4.0.0.27: Transparency instead of Opacity, Name and Description instead of
    // ScriptName and DisplayName, region values in the old ranges
    RoomXml room;
    HError err = ReadRoom("<Room Version=\"4.0.0.27\"><Number>3</Number><Width>320</Width><Objects><RoomObject>"
                          "<Name>oDoor</Name><Description>A door</Description><Transparency>40</Transparency>"
                          "<GraphicAnchor>0.5, 2</GraphicAnchor><BlendMode>Screen</BlendMode>"
                          "</RoomObject></Objects><Regions><RoomRegion><LightLevel>150</LightLevel>"
                          "<TintSaturation>50</TintSaturation><TintLuminance>100</TintLuminance></RoomRegion>"
                          "</Regions></Room>",
                          room);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(3, room.Number);
    ASSERT_EQ(1u, room.Objects.size());
    EXPECT_STREQ("oDoor", room.Objects[0].ScriptName.GetCStr());
    EXPECT_STREQ("A door", room.Objects[0].DisplayName.GetCStr());
    EXPECT_EQ(153, room.Objects[0].Opacity); // (100 - 40) * 255 / 100
    EXPECT_FLOAT_EQ(0.5f, room.Objects[0].GraphicAnchor.X);
    EXPECT_FLOAT_EQ(1.f, room.Objects[0].GraphicAnchor.Y); // clamped
    EXPECT_EQ(5, room.Objects[0].BlendMode);
    ASSERT_EQ(1u, room.Regions.size());
    EXPECT_EQ(127, room.Regions[0].LightLevel); // (150 - 100) * 255 / 100
    EXPECT_EQ(127, room.Regions[0].TintSaturation);
    EXPECT_EQ(255, room.Regions[0].TintLuminance);

    // The current format is read as is
    RoomXml current;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.34\"><Regions><RoomRegion><LightLevel>-20</LightLevel>"
                         "</RoomRegion></Regions></Room>",
                         current));
    EXPECT_EQ(-20, current.Regions[0].LightLevel);
}

TEST(RoomXml, RejectsWhatTheEditorRejects)
{
    RoomXml room;
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.34\"><Unheard>1</Unheard></Room>", room));
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.99\" />", room));
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.34\"><Objects><RoomObject><ScriptName>1st</ScriptName>"
                          "</RoomObject></Objects></Room>",
                          room));
    EXPECT_FALSE(ReadRoom("<Room Version=\"4.0.0.34\"><Regions><RoomRegion><TintSaturation>0</TintSaturation>"
                          "</RoomRegion></Regions></Room>",
                          room));
    // obsolete properties are skipped, as the editor does
    EXPECT_TRUE(ReadRoom("<Room Version=\"4.0.0.34\"><SaveLoadEnabled>True</SaveLoadEnabled></Room>", room));
}

TEST(RoomXml, ConvertsOldIndexedEvents)
{
    // Old object interactions were indexed by event: 4 was "any click", 0 "look at"
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.34\"><Objects><RoomObject><Interactions>"
                         "<Event Index=\"0\">oLook</Event><Event Index=\"4\">oAny</Event></Interactions>"
                         "</RoomObject></Objects><Interactions><Event Index=\"6\">roomRep</Event></Interactions>"
                         "<OnRepExec>ignored</OnRepExec></Room>",
                         room));
    const auto &obj = room.Objects[0];
    EXPECT_STREQ("oAny", obj.OnAnyClick.GetCStr());
    ASSERT_EQ(1u, obj.Interactions.ByIndex.size());
    EXPECT_STREQ("oLook", obj.Interactions.ByIndex.at(1).GetCStr()); // now by cursor ID: Look is cursor 1
    EXPECT_STREQ("roomRep", room.OnRepExec.GetCStr());
}

TEST(RoomBuild, InteractionsAreIndexedByCursor)
{
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.34\"><GameID>1234</GameID><Objects><RoomObject><Interactions>"
                         "<Event Index=\"2\" UID=\"Curs0003\">oUse</Event>"
                         "<Event Index=\"8\" UID=\"Curs0009\">oMode8</Event>"
                         "<Event Index=\"5\" UID=\"Curs0006\">oStale</Event></Interactions>"
                         "<OnAnyClick>oAny</OnAnyClick></RoomObject></Objects><Hotspots><RoomHotspot /></Hotspots>"
                         "</Room>",
                         room));
    RoomData data;
    ASSERT_TRUE(ConvertRoomXml(room, MakeGame(), data));
    const auto &handlers = data.Objects[0].Interactions.GetHandlers();
    // slots 0..8, by cursor ID; functions of cursors without an event are dropped
    ASSERT_EQ(9u, handlers.size());
    EXPECT_STREQ("", handlers[1].FunctionName.GetCStr());
    EXPECT_STREQ("oUse", handlers[2].FunctionName.GetCStr());
    EXPECT_STREQ("", handlers[5].FunctionName.GetCStr());
    EXPECT_STREQ("oMode8", handlers[8].FunctionName.GetCStr());
    EXPECT_STREQ("oAny", data.Objects[0].GetEvents().GetHandler(kRoomObjectEvent_AnyClick).FunctionName.GetCStr());
    // no functions: no table at all
    EXPECT_EQ(0u, data.Hotspots[0].Interactions.GetHandlers().size());
    EXPECT_EQ(1u, data.HotspotCount);
}

TEST(RoomBuild, FixupsBeforeSaving)
{
    RoomXml room;
    ASSERT_TRUE(ReadRoom("<Room Version=\"4.0.0.34\"><GameID>1</GameID><Properties>"
                         "<CustomProperty><Name>weight</Name><Value>5</Value></CustomProperty>"
                         "<CustomProperty><Name>Gone</Name><Value>1</Value></CustomProperty></Properties>"
                         "<WalkableAreas><RoomWalkableArea><UseContinuousScaling>True</UseContinuousScaling>"
                         "<MinScalingLevel>150</MinScalingLevel><MaxScalingLevel>50</MaxScalingLevel>"
                         "</RoomWalkableArea></WalkableAreas></Room>",
                         room));
    RoomData data;
    ASSERT_TRUE(ConvertRoomXml(room, MakeGame(), data));
    EXPECT_EQ(1234, data.GameID);          // room taken from another game
    ASSERT_EQ(1u, data.Properties.size()); // only what the schema has, with its spelling
    EXPECT_STREQ("5", data.Properties["Weight"].GetCStr());
    EXPECT_EQ(-50, data.WalkAreas[0].ScalingFar); // min and max were swapped
    EXPECT_EQ(50, data.WalkAreas[0].ScalingNear);

    // another game's room with a different mask resolution needs the editor
    room.MaskResolution = 2;
    EXPECT_FALSE(ConvertRoomXml(room, MakeGame(), data));
}

TEST(RoomBuild, TextProperties)
{
    EXPECT_STREQ("caf\xC3\xA9 \xE2\x82\xAC", ConvertTextProperty("caf\xC3\xA9 \xE2\x82\xAC", 65001).GetCStr());
    EXPECT_STREQ("caf\xE9 \x80 ?", ConvertTextProperty("caf\xC3\xA9 \xE2\x82\xAC \xE2\x98\x83", 1252).GetCStr());
    EXPECT_STREQ("a\nb \\[c", ConvertTextProperty("a\\nb \\[c", 65001).GetCStr());
}

TEST(RoomBuild, ScriptHeader)
{
    RoomScriptNames names;
    names.ObjectNames = {"oDoor", ""};
    names.HotspotNames = {"", "hWall"};
    names.RegionNames = {"rDark"};
    names.WalkableAreaNames = {"wFloor"};
    names.WalkBehindNames = {"bTree"};
    EXPECT_STREQ("import readonly Object *oDoor;\n"
                 "import readonly Hotspot *hWall;\n"
                 "import readonly Region *rDark;\n"
                 "import readonly WalkableArea *wFloor;\n"
                 "import readonly Walkbehind *bTree;\n",
                 MakeRoomScriptHeader(names).GetCStr());
}

TEST(RoomBuild, WalkableAreaExtents)
{
    RoomData data;
    data.Height = 10;
    data.WalkAreaMaskBuf = ClearBuffer(4, 6, kPxFmt_Indexed8);
    data.WalkAreaMaskBuf.GetLine(2)[1] = 3;
    data.WalkAreaMaskBuf.GetLine(4)[3] = 3;
    data.WalkAreaMaskBuf.GetLine(5)[0] = 200; // not an area
    FinishRoomData(data);
    EXPECT_EQ(2, data.WalkAreas[3].Top);
    EXPECT_EQ(4, data.WalkAreas[3].Bottom);
    EXPECT_EQ(10, data.WalkAreas[5].Top); // no pixels: room height and 0
    EXPECT_EQ(0, data.WalkAreas[5].Bottom);
}

TEST(RoomBuild, LoadsImages)
{
    const String dir = Path::ConcatPaths(Directory::GetCurrentDirectory(), "room_build_test");
    Directory::CreateDirectory(dir);
    RGB pal[256] = {};
    PixelBuffer bg = ClearBuffer(4, 4, kPxFmt_A8R8G8B8);
    reinterpret_cast<uint32_t *>(bg.GetLine(0))[0] = 0x00123456; // fully transparent
    ASSERT_TRUE(ImageFile::SaveImage(bg, false, nullptr, Path::ConcatPaths(dir, "background0.png")));
    PixelBuffer mask = ClearBuffer(4, 4, kPxFmt_Indexed8);
    mask.GetLine(1)[1] = 20; // beyond the 16 regions
    for (const char *name : {"hotspots", "walkbehinds", "walkableareas", "regions"})
        ASSERT_TRUE(ImageFile::SaveImage(mask, false, pal, Path::ConcatPaths(dir, String::FromFormat("%s.png", name))));

    RoomXml room;
    room.BackgroundCount = 2; // background1.png is missing: the editor uses the ones it finds
    RoomData data;
    std::vector<String> warnings;
    HError err = LoadRoomImages(dir, room, MakeGame(), data, warnings);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(1u, data.BgFrameCount);
    EXPECT_EQ(4, data.BackgroundBPP);
    EXPECT_EQ(0xFF123456u, reinterpret_cast<const uint32_t *>(data.BgFrames[0].GraphicBuf.GetLine(0))[0]); // opaque
    EXPECT_EQ(20, data.HotspotMaskBuf.GetLine(1)[1]); // within the 50 hotspots
    EXPECT_EQ(0, data.RegionMaskBuf.GetLine(1)[1]);   // cleared
    EXPECT_EQ(3u, warnings.size());

    File::DeleteFile(Path::ConcatPaths(dir, "hotspots.png"));
    EXPECT_FALSE(LoadRoomImages(dir, room, MakeGame(), data, warnings));

    for (const char *name : {"background0", "walkbehinds", "walkableareas", "regions"})
        File::DeleteFile(Path::ConcatPaths(dir, String::FromFormat("%s.png", name)));
    Directory::DeleteDirectory(dir);
}

namespace
{

const char *const kRoomImageNames[] = {"background0", "background1",   "hotspots",
                                       "walkbehinds", "walkableareas", "regions"};

// A room directory with the four masks; the tests add the backgrounds
String MakeRoomDir(const char *name)
{
    const String dir = Path::ConcatPaths(Directory::GetCurrentDirectory(), name);
    Directory::CreateDirectory(dir);
    RGB pal[256] = {};
    const PixelBuffer mask = ClearBuffer(2, 2, kPxFmt_Indexed8);
    for (const char *mask_name : {"hotspots", "walkbehinds", "walkableareas", "regions"})
    {
        ImageFile::SaveImage(mask, false, pal, Path::ConcatPaths(dir, String::FromFormat("%s.png", mask_name)));
    }
    return dir;
}

void DeleteRoomDir(const String &dir)
{
    for (const char *name : kRoomImageNames)
    {
        File::DeleteFile(Path::ConcatPaths(dir, String::FromFormat("%s.png", name)));
    }
    Directory::DeleteDirectory(dir);
}

// The game palette: slots 0 and 1 are game-wide, slot 2 belongs to the backgrounds
std::vector<PaletteEntryData> MakeGamePalette()
{
    std::vector<PaletteEntryData> pal(3);
    pal[0] = {kPaletteColourType_Gamewide, 0, 0, 0};
    pal[1] = {kPaletteColourType_Gamewide, 10, 20, 30};
    pal[2] = {kPaletteColourType_Background, 1, 1, 1};
    return pal;
}

void ExpectColour(const RGB &colour, int r, int g, int b)
{
    EXPECT_EQ(r, colour.r);
    EXPECT_EQ(g, colour.g);
    EXPECT_EQ(b, colour.b);
}

} // namespace

TEST(RoomBuild, EightBitBackgroundsTakeTheGamewideColours)
{
    const String dir = MakeRoomDir("room_build_test_8bit");
    RGB pal[256] = {};
    pal[1] = {100, 110, 120, 255};
    pal[2] = {200, 210, 220, 255};
    PixelBuffer bg = ClearBuffer(2, 2, kPxFmt_Indexed8);
    bg.GetLine(0)[0] = 1;
    bg.GetLine(0)[1] = 2;
    ASSERT_TRUE(ImageFile::SaveImage(bg, false, pal, Path::ConcatPaths(dir, "background0.png")));
    pal[2] = {50, 60, 70, 255};
    ASSERT_TRUE(ImageFile::SaveImage(bg, false, pal, Path::ConcatPaths(dir, "background1.png")));

    RoomXml room;
    room.ColorDepth = 8;
    room.BackgroundCount = 2;
    RoomBuildGame game = MakeGame();
    game.ColorDepth = 8;
    game.Palette = MakeGamePalette();
    RoomData data;
    std::vector<String> warnings;
    HError err = LoadRoomImages(dir, room, game, data, warnings);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    // The pixels stay; the game-wide slots have the game's colours, the others the image's
    EXPECT_EQ(1, data.BackgroundBPP);
    EXPECT_EQ(2u, data.BgFrameCount);
    EXPECT_EQ(1, data.BgFrames[0].GraphicBuf.GetLine(0)[0]);
    EXPECT_EQ(2, data.BgFrames[0].GraphicBuf.GetLine(0)[1]);
    ExpectColour(data.Palette[1], 10, 20, 30);
    ExpectColour(data.Palette[2], 200, 210, 220);
    // each frame has its palette
    ExpectColour(data.BgFrames[0].Palette[2], 200, 210, 220);
    ExpectColour(data.BgFrames[1].Palette[1], 10, 20, 30);
    ExpectColour(data.BgFrames[1].Palette[2], 50, 60, 70);

    // In a 32-bit game the editor does the same to a room saved as 8-bit, then converts it
    game.ColorDepth = 32;
    ASSERT_TRUE(LoadRoomImages(dir, room, game, data, warnings));
    EXPECT_EQ(4, data.BackgroundBPP);
    const uint32_t *line = reinterpret_cast<const uint32_t *>(data.BgFrames[0].GraphicBuf.GetLine(0));
    EXPECT_EQ(0xFF0A141Eu, line[0]); // game-wide: 10, 20, 30
    EXPECT_EQ(0xFFC8D2DCu, line[1]); // the image's: 200, 210, 220
    // and not to a room saved as 32-bit
    room.ColorDepth = 32;
    ASSERT_TRUE(LoadRoomImages(dir, room, game, data, warnings));
    EXPECT_EQ(0xFF646E78u, reinterpret_cast<const uint32_t *>(data.BgFrames[0].GraphicBuf.GetLine(0))[0]);

    // A true-colour background cannot go into an 8-bit game
    ASSERT_TRUE(ImageFile::SaveImage(ClearBuffer(2, 2, kPxFmt_A8R8G8B8), false, nullptr,
                                     Path::ConcatPaths(dir, "background0.png")));
    game.ColorDepth = 8;
    EXPECT_FALSE(LoadRoomImages(dir, room, game, data, warnings));
    DeleteRoomDir(dir);
}

TEST(RoomBuild, GamewideColoursStopAtTheImagePalette)
{
    // An 8-bit PNG, 2x2, pixels 0 1 / 1 0, with a palette of two colours: 10 20 30 and 40 50 60
    static const uint8_t png[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x03, 0x00, 0x00, 0x00, 0x45, 0x68, 0xFD, 0x16, 0x00, 0x00, 0x00,
        0x06, 0x50, 0x4C, 0x54, 0x45, 0x0A, 0x14, 0x1E, 0x28, 0x32, 0x3C, 0xD5, 0x1B, 0xB4, 0xE9, 0x00, 0x00, 0x00,
        0x0C, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0x60, 0x60, 0x04, 0x42, 0x00, 0x00, 0x0C, 0x00, 0x03, 0x15,
        0x9E, 0x18, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };
    const String dir = MakeRoomDir("room_build_test_pal");
    {
        auto out = File::CreateFile(Path::ConcatPaths(dir, "background0.png"));
        ASSERT_TRUE(out != nullptr);
        out->Write(png, sizeof(png));
    }

    RoomXml room;
    room.ColorDepth = 8;
    room.BackgroundCount = 1;
    RoomBuildGame game = MakeGame();
    game.ColorDepth = 8;
    game.Palette.resize(4);
    game.Palette[1] = {kPaletteColourType_Gamewide, 1, 2, 3};
    game.Palette[3] = {kPaletteColourType_Gamewide, 7, 8, 9};
    game.Palette[0].ColourType = kPaletteColourType_Background;
    RoomData data;
    std::vector<String> warnings;
    HError err = LoadRoomImages(dir, room, game, data, warnings);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(1, data.BgFrames[0].GraphicBuf.GetLine(0)[1]);
    ExpectColour(data.Palette[0], 10, 20, 30); // the image's
    ExpectColour(data.Palette[1], 1, 2, 3);    // game-wide
    ExpectColour(data.Palette[3], 0, 0, 0);    // game-wide, but past the image's palette: left black
    DeleteRoomDir(dir);
}

// agsbuild keeps a built room while the key of the settings it was built with is the same: every
// setting changes the key
TEST(RoomBuild, EverySettingChangesTheGameKey)
{
    RoomBuildGame base = MakeGame();
    base.Palette.resize(1);
    const String base_key = MakeRoomBuildGameKey(base);
    EXPECT_EQ(base_key, MakeRoomBuildGameKey(base));

    std::vector<RoomBuildGame> changed(17, base);
    changed[0].UniqueID++;
    changed[1].DefaultRoomMaskResolution = 2;
    changed[2].ColorDepth = 8;
    changed[3].TextCodePage = 1252;
    changed[4].Palette.resize(2);
    changed[5].InteractionEvents[0].first = 3;
    changed[6].InteractionEvents[0].second = "Curs0004";
    changed[7].InteractionEvents.pop_back();
    changed[8].PropertySchema[0].Name = "Height";
    changed[9].PropertySchema[0].Description = "How heavy";
    changed[10].PropertySchema[0].DefaultValue = "1";
    changed[11].PropertySchema[0].Type = AGS::Common::kPropertyInteger;
    changed[12].PropertySchema[0].AppliesToHotspots = false;
    changed[13].PropertySchema[0].Translated = true;
    changed[14].PropertySchema.emplace_back();
    changed[15].Palette[0].ColourType = kPaletteColourType_Locked;
    changed[16].Palette[0].Blue = 1;
    for (size_t i = 0; i < changed.size(); ++i)
    {
        EXPECT_NE(base_key, MakeRoomBuildGameKey(changed[i])) << "change " << i;
    }
    // Values are told apart where they end: "1", "23" is not "12", "3"
    RoomBuildGame a = base, b = base;
    a.PropertySchema[0].Name = "1";
    a.PropertySchema[0].Description = "23";
    b.PropertySchema[0].Name = "12";
    b.PropertySchema[0].Description = "3";
    EXPECT_NE(MakeRoomBuildGameKey(a), MakeRoomBuildGameKey(b));
}
