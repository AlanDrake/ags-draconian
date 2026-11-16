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
//
// The plain entities of a game against their definitions: inventory items,
// mouse cursors, view frames, dialog options, audio types, audio clips, the game
// options and the game's values that a script changes. Every script function
// that writes one of them during play, read back through its getter; the audio
// clip's script handle; the save round trip of every saved field of the view
// frames and of the inventory items; the fix-ups that engine_init_game_settings
// makes at start; the values a started game has, which are its definition's;
// ViewFrame.YOffset, which gave the x offset.
// The game is made by hand, in a Game, a Room and an EngineServices of the
// fixture's own: no game file is read.
//
// Not tested: InitGameState (it loads fonts, plugins and scripts, and draws:
// the fixture starts the game with the parts of it that run without them, see
// StartGame); a cursor mode's view that has frames (Mouse.ChangeModeView then
// makes textures of the frames, which needs a graphics driver: the cursors use a
// view without frames); the option commands of a dialog script (they need a
// compiled dialog script); the marker that the inventory cursor draws on its
// image (it needs the sprite's image).
//
//=============================================================================
#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <allegro.h> // get_palette, set_palette_range, get_uformat, set_uformat

#include "game/audio_clip_def.h"
#include "game/audio_clip_type_def.h"
#include "game/dialog/dialog.h"
#include "game/dialog_def.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_view_frame.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/game_struct_defines.h"
#include "game/inventory_item_def.h"
#include "game/mouse_cursor_def.h"
#include "game/runtime_defines.h"
#include "game/save/savegame.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
#include "game/script.h"
#include "game/ui/mouse.h"
#include "game/view_def.h"
#include "game/world/character.h"
#include "game/world/inventory_item.h"
#include "game/world/room_context.h"
#include "game/world/view_frame.h"
#include "gfx/gfx_def.h"
#include "gtest/gtest.h"
#include "gui/guimain.h"
#include "main/engine.h"
#include "platform/base/engine_services.h"
#include "script/cc_common.h"
#include "script/cc_script.h"
#include "script/dynobj/cc_script_object.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/managed_object_pool.h"
#include "script/runtimescript.h"
#include "script/scriptexecutor.h"
#include "util/geometry.h"
#include "util/memorystream.h"
#include "util/stream.h"
#include "util/string.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace AGS
{
namespace Engine
{
// game_init.cpp: makes each audio clip's id its index and registers the audio channels and the clips with the managed
// pool, at their elements' addresses. Part of InitGameState; no header declares it.
void InitAndRegisterAudioObjects(Game &game);
} // namespace Engine
} // namespace AGS

namespace
{

const int kCharacterCount = 3;
const int kItemCount = 3;
// The cursors: a pointer, a look cursor with a mode view, the "use inventory" cursor, and one whose view number is
// past the game's views
const int kPointerCursor = 0;
const int kLookCursor = 1;
const int kUseInvCursor = 2;
const int kBadViewCursor = 3;
const int kCursorCount = 4;
// The views: one with two loops of two frames, one with a loop without frames (the cursors' mode view)
const int kFramedView = 0;
const int kEmptyView = 1;
const int kViewCount = 2;
const int kLoopCount = 2;
const int kFrameCount = 2;
const int kDialogCount = 2;
const int kOptionCount = 3;
const int kAudioTypeCount = 3;
const int kAudioClipCount = 3;
// A sprite that has a size in the sprite table, for the inventory cursor's hotspot alignment
const int kCursorSprite = 77;
const int kCursorSpriteWidth = 20;
const int kCursorSpriteHeight = 30;
// The sayChosenOption argument of Dialog.DisplayOptions that never says the option (SAYCHOSEN_NO in dialog.cpp)
const int kSayChosenNever = 3;

// The value that MakeDefinition gives a field of view frame n (0 to 3: the loop's index times the frame count, plus
// the frame's index); different for every frame and every field
int FramePic(int n)
{
    return 100 + n;
}
int FrameXOffset(int n)
{
    return 1 + n;
}
int FrameYOffset(int n)
{
    return 10 + n;
}
int FrameSpeed(int n)
{
    return 20 + n;
}
SpriteTransformFlags FrameFlags(int n)
{
    return (n % 2) == 0 ? kSprTf_None : kSprTf_FlipX;
}
int FrameSound(int n)
{
    return n < kAudioClipCount ? n : -1;
}

ScriptInvItem ItemObject(int id)
{
    ScriptInvItem item;
    item.id = id;
    return item;
}

ScriptDialog DialogObject(int id)
{
    ScriptDialog dialog;
    dialog.id = id;
    return dialog;
}

// A script executor and an empty global script of their own, while a test runs a function that runs the script
// event "on_event": the event finds no handler. The engine's executor is put back after.
class EmptyScripts
{
public:
    explicit EmptyScripts(Game &for_game) : _game(for_game), _wasExecutor(std::move(scriptExecutor))
    {
        scriptExecutor = std::make_unique<ScriptExecutor>();
        _script.scriptname = "GlobalScript.asc";
        _game.Scripts.Global = RuntimeScript::Create(&_script);
    }

    ~EmptyScripts()
    {
        _game.Scripts.Global.reset();
        scriptExecutor = std::move(_wasExecutor);
    }

    EmptyScripts(const EmptyScripts &) = delete;
    EmptyScripts &operator=(const EmptyScripts &) = delete;

    bool IsReady() const { return _game.Scripts.Global != nullptr; }

private:
    Game &_game;
    std::unique_ptr<ScriptExecutor> _wasExecutor;
    ccScript _script;
};

} // namespace

// Fixture: a game made by hand (MakeDefinition), started as far as a test can start one (StartGame), in a Game, a Room
// and an EngineServices of its own, made the current ones. The engine globals it touches, all reset in TearDown:
// - Game::Current(), Room::Current(), EngineServices::Current(): the fixture's own
// - the managed pool (pool): the audio clips and the script strings that the getters return
// - the script error state (cc_get_error)
// The tests that call engine_init_game_settings reset the globals it sets (RunEngineInitGameSettings).
class GameEntities : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _wasServices = EngineServices::SetCurrent(&_services);
        _wasGame = Game::SetCurrent(_game.get());
        _wasRoom = Room::SetCurrent(&_room);
        MakeDefinition();
        StartGame();
    }

    void TearDown() override
    {
        pool.Reset();
        cc_clear_error();
        Room::SetCurrent(_wasRoom);
        Game::SetCurrent(_wasGame);
        EngineServices::SetCurrent(_wasServices);
    }

    // The game's definition, which the test writes as the game file reader would (the engine reads it through
    // Game::Def(), which is const)
    GameDef &Definition() { return *_game; }

    // The game's definition: of each entity two or more, with values of their own
    void MakeDefinition()
    {
        GameDef &def = Definition();
        def.numcharacters = kCharacterCount;
        def.Characters.clear();
        def.Characters.resize(kCharacterCount);
        for (int i = 0; i < kCharacterCount; ++i)
        {
            def.Characters[i].ScriptName = String::FromFormat("cPerson%d", i);
            def.Characters[i].Name = String::FromFormat("Person %d", i);
            def.Characters[i].View = -1; // engine_init_game_settings precaches a view: that needs a graphics driver
        }
        def.PlayerCharacterID = 1;

        def.numinvitems = kItemCount;
        def.InventoryItems.clear();
        def.InventoryItems.resize(kItemCount);
        const FrameAlignment item_aligns[kItemCount] = {kAlignNone, kAlignBottomRight, kAlignMiddleCenter};
        for (int i = 0; i < kItemCount; ++i)
        {
            auto &item = def.InventoryItems[i];
            item.name = String::FromFormat("Item %d", i);
            item.pic = 20 + i;
            item.cursorPic = 40 + i;
            item.hotx = 3 + i;
            item.hoty = 5 + i;
            item.hotAlign = item_aligns[i];
        }

        def.numcursors = kCursorCount;
        def.MouseCursors.clear();
        def.MouseCursors.resize(kCursorCount);
        const CursorRole roles[kCursorCount] = {kCursorRole_Pointer, kCursorRole_Look, kCursorRole_UseInv,
                                                kCursorRole_None};
        const short views[kCursorCount] = {-1, kEmptyView, -1, kViewCount};
        for (int i = 0; i < kCursorCount; ++i)
        {
            auto &cur = def.MouseCursors[i];
            cur.role = roles[i];
            cur.pic = 30 + i;
            cur.hotx = static_cast<short>(1 + i);
            cur.hoty = static_cast<short>(2 + i);
            cur.view = views[i];
            cur.animdelay = 5 + i;
            cur.flags = static_cast<uint8_t>(MCF_STANDARD | (i == kLookCursor ? MCF_EVENT : 0));
        }

        MakeViews();

        def.numdialog = kDialogCount;
        def.Dialogs.clear();
        def.Dialogs.resize(kDialogCount);
        for (int d = 0; d < kDialogCount; ++d)
        {
            def.Dialogs[d].ScriptName = String::FromFormat("dTopic%d", d);
            def.Dialogs[d].Options.resize(kOptionCount);
            for (int o = 0; o < kOptionCount; ++o)
            {
                auto &opt = def.Dialogs[d].Options[o];
                opt.OriginalText = String::FromFormat("Dialog %d option %d", d, o);
                opt.Text = opt.OriginalText;
            }
            // the first option on, the second off, the third on and not said when chosen; in the second dialog the
            // first option was chosen already
            def.Dialogs[d].Options[0].Flags = DFLG_ON | (d == 1 ? DFLG_HASBEENCHOSEN : 0);
            def.Dialogs[d].Options[2].Flags = DFLG_ON | DFLG_NOREPEAT;
        }

        def.AudioClipTypes.clear();
        def.AudioClipTypes.resize(kAudioTypeCount);
        for (int i = 0; i < kAudioTypeCount; ++i)
        {
            auto &type = def.AudioClipTypes[i];
            type.id = i;
            type.reservedChannels = 1;
            type.volume_reduction_while_speech_playing = 20 + i;
            type.crossfadeSpeed = 10 + i;
        }
        def.AudioClips.clear();
        def.AudioClips.resize(kAudioClipCount);
        for (int i = 0; i < kAudioClipCount; ++i)
        {
            auto &clip = def.AudioClips[i];
            clip.id = 10 + i; // the id the game file holds, which the engine replaces with the index
            clip.fileName = String::FromFormat("clip%d.ogg", i);
            clip.type = static_cast<uint8_t>(i);
        }

        std::fill(std::begin(def.Options), std::end(def.Options), 0);
        def.Options[OPT_DEBUGMODE] = 1;
        def.Options[OPT_PORTRAITSIDE] = 1;
        def.Options[OPT_DIALOGNUMBERED] = 1;
        def.Options[OPT_GLOBALTALKANIMSPD] = 5;
        def.Options[OPT_LIPSYNCTEXT] = 1;
        def.DialogBulletSprite = 14;
        def.InventoryCursorHotspotSprite = 17;
        def.InventoryCursorHotspotColor = 0x00112233;
        def.InventoryCursorHotspotCrossColor = 0x00445566;
        def.DefaultLipSyncFrame = 4;

        def.SpriteInfos.clear();
        def.SpriteInfos.resize(100);
        def.SpriteInfos[kCursorSprite] = SpriteInfo(kCursorSpriteWidth, kCursorSpriteHeight, 32, 0u);
        def.ResolveStandardModes();
    }

    // The views of MakeDefinition: in the framed view, frame n of FramePic(n)... with an event name "step<n>"
    void MakeViews()
    {
        GameDef &def = Definition();
        def.numviews = kViewCount;
        def.Views.clear();
        def.Views.resize(kViewCount);
        def.Views[kFramedView].Initialize(kLoopCount);
        for (int loop = 0; loop < kLoopCount; ++loop)
        {
            def.Views[kFramedView].loops[loop].Initialize(kFrameCount);
            for (int frame = 0; frame < kFrameCount; ++frame)
            {
                const int n = loop * kFrameCount + frame;
                auto &vf = def.Views[kFramedView].loops[loop].frames[frame];
                vf.pic = FramePic(n);
                vf.xoffs = static_cast<int16_t>(FrameXOffset(n));
                vf.yoffs = static_cast<int16_t>(FrameYOffset(n));
                vf.speed = static_cast<int16_t>(FrameSpeed(n));
                vf.flags = FrameFlags(n);
                vf.sound = FrameSound(n);
                vf.event_name = String::FromFormat("step%d", n);
            }
        }
        def.Views[kEmptyView].Initialize(1);
        def.Views[kEmptyView].loops[0].Initialize(0);
    }

    // Does what InitGameState does for these entities that a test can run without fonts, plugins, scripts and a
    // graphics driver: the live characters and the player, the custom property lists, the audio clips' ids and
    // their registration with the managed pool
    void StartGame()
    {
        Game &game = *_game;
        game.Characters.clear();
        game.Characters.reserve(game.Def().Characters.size());
        for (const auto &def : game.Def().Characters)
            game.Characters.emplace_back(def);
        for (int i = 0; i < game.numcharacters; ++i)
            game.Characters[i].ID = i;
        game.Play.charProps.clear();
        game.Play.charProps.resize(game.numcharacters);
        game.Play.invProps.clear();
        game.Play.invProps.resize(game.numinvitems);
        game.Play.dialogProps.clear();
        game.Play.dialogProps.resize(game.numdialog);
        game.Play.audioclipProps.clear();
        game.Play.audioclipProps.resize(game.AudioClips.size());
        game.Play.default_audio_type_volumes.assign(game.AudioClipTypes.size(), -1);
        InitAndRegisterAudioObjects(game);
        setup_player_character(game.PlayerCharacterID);
    }

    // Starts the game again from its definition: the managed pool emptied, the definition made again, the game
    // started
    void RestartGame()
    {
        pool.Reset();
        MakeDefinition();
        StartGame();
    }

    // Runs engine_init_game_settings, and puts back the globals it sets outside the fixture's contexts: the GUI
    // options, the GUI data version, Allegro's palette and text format. The GUI's colour depth is set for it, as
    // InitGameState sets it before (GUI::GetStandardColor asserts it).
    static void RunEngineInitGameSettings()
    {
        const auto gui_options = GUI::Options;
        const GameDataVersion gui_data_version = GUI::DataVersion;
        const int gui_color_depth = GUI::Context.GameColorDepth;
        const int text_format = get_uformat();
        PALETTE palette;
        get_palette(palette);

        GUI::Context.GameColorDepth = 32;
        engine_init_game_settings();

        set_palette_range(palette, 0, PAL_SIZE - 1, 0);
        set_uformat(text_format);
        GUI::Context.GameColorDepth = gui_color_depth;
        GUI::DataVersion = gui_data_version;
        GUI::Options = gui_options;
    }

    static bool Save(SaveCmpSelection cmp, std::vector<uint8_t> &buf, std::string &error)
    {
        buf.clear();
        Stream out(std::make_unique<VectorStream>(buf, kStream_Write));
        HSaveError err = SavegameComponents::WriteAllCommon(&out, cmp, false);
        if (!err)
            error = err->FullMessage().GetCStr();
        return static_cast<bool>(err);
    }

    static HSaveError Restore(const std::vector<uint8_t> &buf, SaveCmpSelection cmp)
    {
        Stream in(std::make_unique<VectorStream>(buf));
        PreservedParams pp;
        RestoredData r_data;
        return SavegameComponents::ReadAll(&in, kSvgVersion_Current, cmp, pp, r_data);
    }

    // Item i, as its getters give it, against its definition
    static void ExpectItemTakesTheDefinition(int i)
    {
        SCOPED_TRACE(testing::Message() << "item " << i);
        const InventoryItemDef &def = Game::Current().Def().InventoryItems[i];
        ScriptInvItem item = ItemObject(i);
        EXPECT_EQ(def.pic, InventoryItem_GetGraphic(&item));
        EXPECT_EQ(def.cursorPic, InventoryItem_GetCursorGraphic(&item));
        EXPECT_EQ(def.hotx, InventoryItem_GetCursorHotspotX(&item));
        EXPECT_EQ(def.hoty, InventoryItem_GetCursorHotspotY(&item));
        EXPECT_EQ(static_cast<int>(def.hotAlign), InventoryItem_GetCursorHotspotAlignment(&item));
        EXPECT_STREQ(def.name.GetCStr(), InventoryItem_GetName_New(&item));
    }

    // Cursor i against its definition: the graphic through its getter, the fields without one directly
    static void ExpectCursorTakesTheDefinition(int i)
    {
        SCOPED_TRACE(testing::Message() << "cursor " << i);
        Game &game = Game::Current();
        const MouseCursorDef &def = game.Def().MouseCursors[i];
        EXPECT_EQ(def.pic, Mouse_GetModeGraphic(i));
        EXPECT_EQ(def.hotx, game.MouseCursors[i].hotx);
        EXPECT_EQ(def.hoty, game.MouseCursors[i].hoty);
        EXPECT_EQ(def.view, game.MouseCursors[i].view);
        EXPECT_EQ(def.animdelay, game.MouseCursors[i].animdelay);
        EXPECT_EQ(def.flags, game.MouseCursors[i].flags);
    }

    // Frame of the framed view against its definition, through the getters
    static void ExpectFrameTakesTheDefinition(int loop, int frame)
    {
        SCOPED_TRACE(testing::Message() << "loop " << loop << " frame " << frame);
        const ViewFrameDef &def = Game::Current().Def().Views[kFramedView].loops[loop].frames[frame];
        ScriptViewFrame svf(kFramedView, loop, frame);
        EXPECT_EQ(def.pic, ViewFrame_GetGraphic(&svf));
        EXPECT_EQ(def.xoffs, ViewFrame_GetXOffset(&svf));
        EXPECT_EQ(def.yoffs, ViewFrame_GetYOffset(&svf));
        EXPECT_EQ(def.speed, ViewFrame_GetSpeed(&svf));
        EXPECT_EQ(static_cast<int>(GfxDef::GetFlipFromFlags(def.flags)), ViewFrame_GetFlipped(&svf));
        EXPECT_EQ(def.sound < 0 ? nullptr : Game_GetAudioClip(def.sound), ViewFrame_GetLinkedAudio(&svf));
        EXPECT_STREQ(def.event_name.GetCStr(), ViewFrame_GetEventName(&svf));
    }

    // Option o (0-based) of dialog d against its definition, through the getters, and its flags
    static void ExpectOptionTakesTheDefinition(int d, int o)
    {
        SCOPED_TRACE(testing::Message() << "dialog " << d << " option " << o);
        Game &game = Game::Current();
        const DialogOptionDef &def = game.Def().Dialogs[d].Options[o];
        ScriptDialog dialog = DialogObject(d);
        const int expected_state = (def.Flags & DFLG_OFFPERM) ? 2 : (def.Flags & DFLG_ON) ? 1 : 0;
        EXPECT_EQ(expected_state, Dialog_GetOptionState(&dialog, o + 1));
        EXPECT_EQ((def.Flags & DFLG_HASBEENCHOSEN) ? 1 : 0, Dialog_HasOptionBeenChosen(&dialog, o + 1));
        EXPECT_STREQ(def.Text.GetCStr(), Dialog_GetOptionText(&dialog, o + 1));
        EXPECT_EQ(def.Flags, game.Dialogs[d].Options[o].Flags);
    }

    // Audio type i against its definition, through the getters
    static void ExpectAudioTypeTakesTheDefinition(int i)
    {
        SCOPED_TRACE(testing::Message() << "audio type " << i);
        const AudioClipTypeDef &def = Game::Current().Def().AudioClipTypes[i];
        EXPECT_EQ(def.crossfadeSpeed, Game_GetAudioTypeCrossfadeSpeed(i));
        EXPECT_EQ(def.volume_reduction_while_speech_playing, Game_GetAudioTypeSpeechVolumeDrop(i));
    }

    EngineServices _services;
    std::unique_ptr<Game> _game = std::make_unique<Game>();
    Room _room;
    EngineServices *_wasServices = nullptr;
    Game *_wasGame = nullptr;
    Room *_wasRoom = nullptr;
};

// The four setters of an item write its graphic, its cursor graphic, its name and its cursor hotspot; each of the three
// hotspot setters keeps the other two values. The other items keep their values.
TEST_F(GameEntities, InventoryItemSettersWriteTheItem)
{
    ScriptInvItem item1 = ItemObject(1);
    InventoryItem_SetGraphic(&item1, 61);
    InventoryItem_SetCursorGraphic(&item1, 62);
    InventoryItem_SetName(&item1, "Renamed item");
    EXPECT_EQ(61, InventoryItem_GetGraphic(&item1));
    EXPECT_EQ(62, InventoryItem_GetCursorGraphic(&item1));
    EXPECT_STREQ("Renamed item", InventoryItem_GetName_New(&item1));

    InventoryItem_SetCursorHotspotX(&item1, 13);
    EXPECT_EQ(13, InventoryItem_GetCursorHotspotX(&item1));
    EXPECT_EQ(6, InventoryItem_GetCursorHotspotY(&item1));
    EXPECT_EQ(static_cast<int>(kAlignBottomRight), InventoryItem_GetCursorHotspotAlignment(&item1));
    InventoryItem_SetCursorHotspotY(&item1, 14);
    EXPECT_EQ(13, InventoryItem_GetCursorHotspotX(&item1));
    EXPECT_EQ(14, InventoryItem_GetCursorHotspotY(&item1));
    EXPECT_EQ(static_cast<int>(kAlignBottomRight), InventoryItem_GetCursorHotspotAlignment(&item1));
    InventoryItem_SetCursorHotspotAlignment(&item1, kAlignMiddleCenter);
    EXPECT_EQ(13, InventoryItem_GetCursorHotspotX(&item1));
    EXPECT_EQ(14, InventoryItem_GetCursorHotspotY(&item1));
    EXPECT_EQ(static_cast<int>(kAlignMiddleCenter), InventoryItem_GetCursorHotspotAlignment(&item1));

    ExpectItemTakesTheDefinition(0);
    ExpectItemTakesTheDefinition(2);
}

// The Mouse functions write a cursor mode's graphic (of the current mode too), hotspot, view and animation delay, and
// its flag of a disabled mode; a view of 0 is none, a delay of SCR_NO_VALUE keeps the delay. The other cursors keep
// their values.
TEST_F(GameEntities, MouseCursorSettersWriteTheCursor)
{
    Game &game = Game::Current();
    Mouse_ChangeCursorGraphic(kLookCursor, 71);
    EXPECT_EQ(71, Mouse_GetModeGraphic(kLookCursor));
    ASSERT_EQ(kPointerCursor, game.Cursor.Mode);
    Mouse_ChangeCursorGraphic(kPointerCursor, 72);
    EXPECT_EQ(72, Mouse_GetModeGraphic(kPointerCursor));

    Mouse_ChangeCursorHotspot(kLookCursor, 8, 9);
    EXPECT_EQ(8, game.MouseCursors[kLookCursor].hotx);
    EXPECT_EQ(9, game.MouseCursors[kLookCursor].hoty);

    Mouse_ChangeModeView(kLookCursor, 0, 11);
    EXPECT_EQ(-1, game.MouseCursors[kLookCursor].view);
    EXPECT_EQ(11, game.MouseCursors[kLookCursor].animdelay);
    Mouse_ChangeModeView(kLookCursor, kEmptyView + 1, SCR_NO_VALUE);
    EXPECT_EQ(kEmptyView, game.MouseCursors[kLookCursor].view);
    EXPECT_EQ(11, game.MouseCursors[kLookCursor].animdelay);

    Mouse_DisableCursorMode(kLookCursor);
    EXPECT_EQ(0, Mouse_IsModeEnabled(kLookCursor));
    EXPECT_EQ(MCF_STANDARD | MCF_EVENT | MCF_DISABLED, game.MouseCursors[kLookCursor].flags);
    Mouse_EnableCursorMode(kLookCursor);
    EXPECT_EQ(1, Mouse_IsModeEnabled(kLookCursor));
    EXPECT_EQ(MCF_STANDARD | MCF_EVENT, game.MouseCursors[kLookCursor].flags);

    ExpectCursorTakesTheDefinition(kUseInvCursor);
    ExpectCursorTakesTheDefinition(kBadViewCursor);
}

// The "use inventory" cursor shows the player's active item: when the item's cursor graphic or hotspot changes, the
// cursor takes the graphic and the hotspot aligned in the graphic as the item says (update_inv_cursor)
TEST_F(GameEntities, UseInvCursorTakesTheActiveItemsGraphicAndHotspot)
{
    Game &game = Game::Current();
    // no hotspot marker, which would be drawn on the cursor's image
    Game_SetInventoryCursorHotspotColor(0);
    Game_SetInventoryCursorHotspotGraphic(0);
    Mouse_SetCursor(kUseInvCursor);
    game.PlayerChar->ActiveInventory = 1;

    ScriptInvItem item1 = ItemObject(1);
    InventoryItem_SetCursorGraphic(&item1, kCursorSprite);
    EXPECT_EQ(kCursorSprite, Mouse_GetModeGraphic(kUseInvCursor));
    // item 1's hotspot (4, 6), aligned to the bottom right of the sprite
    EXPECT_EQ(4 + kCursorSpriteWidth, game.MouseCursors[kUseInvCursor].hotx);
    EXPECT_EQ(6 + kCursorSpriteHeight, game.MouseCursors[kUseInvCursor].hoty);

    InventoryItem_SetCursorHotspotX(&item1, 2);
    EXPECT_EQ(2 + kCursorSpriteWidth, game.MouseCursors[kUseInvCursor].hotx);
    EXPECT_EQ(6 + kCursorSpriteHeight, game.MouseCursors[kUseInvCursor].hoty);
    ExpectCursorTakesTheDefinition(kLookCursor);
}

// The seven setters of a view frame write the frame; the getters give back what was set. The other frames keep their
// values.
TEST_F(GameEntities, ViewFrameSettersWriteTheFrame)
{
    ScriptViewFrame svf(kFramedView, 1, 1);
    ViewFrame_SetGraphic(&svf, 91);
    EXPECT_EQ(91, ViewFrame_GetGraphic(&svf));
    ViewFrame_SetSpeed(&svf, 4);
    EXPECT_EQ(4, ViewFrame_GetSpeed(&svf));
    ViewFrame_SetXOffset(&svf, -6);
    EXPECT_EQ(-6, ViewFrame_GetXOffset(&svf));
    ViewFrame_SetYOffset(&svf, 8);
    EXPECT_EQ(8, ViewFrame_GetYOffset(&svf));
    EXPECT_EQ(-6, ViewFrame_GetXOffset(&svf));
    ViewFrame_SetFlipped(&svf, kFlip_Both);
    EXPECT_EQ(static_cast<int>(kFlip_Both), ViewFrame_GetFlipped(&svf));
    ViewFrame_SetLinkedAudio(&svf, Game_GetAudioClip(2));
    EXPECT_EQ(Game_GetAudioClip(2), ViewFrame_GetLinkedAudio(&svf));
    ViewFrame_SetEventName(&svf, "thud");
    EXPECT_STREQ("thud", ViewFrame_GetEventName(&svf));
    ViewFrame_SetLinkedAudio(&svf, nullptr);
    EXPECT_EQ(nullptr, ViewFrame_GetLinkedAudio(&svf));

    ExpectFrameTakesTheDefinition(0, 0);
    ExpectFrameTakesTheDefinition(0, 1);
    ExpectFrameTakesTheDefinition(1, 0);
}

// ViewFrame.YOffset gives the frame's y offset (it gave the x offset, as upstream's ags4 still does)
TEST_F(GameEntities, ViewFrameYOffsetIsTheYOffset)
{
    ScriptViewFrame svf(kFramedView, 0, 1);
    ViewFrame_SetXOffset(&svf, 3);
    ViewFrame_SetYOffset(&svf, 7);
    EXPECT_EQ(3, ViewFrame_GetXOffset(&svf));
    EXPECT_EQ(7, ViewFrame_GetYOffset(&svf));
}

// Dialog.SetOptionState (the path of SetDialogOption) turns an option off, on, and off for good, after which it stays
// off; Dialog.SetHasOptionBeenChosen sets and clears the "chosen" flag. The other dialog keeps its options.
TEST_F(GameEntities, DialogOptionStatesFollowTheScript)
{
    ScriptDialog dialog0 = DialogObject(0);
    Dialog_SetOptionState(&dialog0, 1, 0);
    EXPECT_EQ(0, Dialog_GetOptionState(&dialog0, 1));
    Dialog_SetOptionState(&dialog0, 1, 1);
    EXPECT_EQ(1, Dialog_GetOptionState(&dialog0, 1));
    Dialog_SetOptionState(&dialog0, 2, 2);
    EXPECT_EQ(2, Dialog_GetOptionState(&dialog0, 2));
    Dialog_SetOptionState(&dialog0, 2, 1);
    EXPECT_EQ(2, Dialog_GetOptionState(&dialog0, 2));

    Dialog_SetHasOptionBeenChosen(&dialog0, 3, true);
    EXPECT_EQ(1, Dialog_HasOptionBeenChosen(&dialog0, 3));
    EXPECT_EQ(1, Dialog_GetOptionState(&dialog0, 3));
    Dialog_SetHasOptionBeenChosen(&dialog0, 3, false);
    EXPECT_EQ(0, Dialog_HasOptionBeenChosen(&dialog0, 3));
    EXPECT_EQ(DFLG_ON | DFLG_NOREPEAT, Game::Current().Dialogs[0].Options[2].Flags);

    for (int o = 0; o < kOptionCount; ++o)
        ExpectOptionTakesTheDefinition(1, o);
}

// An option that is run (the player chose it) is marked chosen, and keeps its state (run_dialog_option)
TEST_F(GameEntities, RunOptionIsMarkedChosen)
{
    Game &game = Game::Current();
    EmptyScripts scripts(game);
    ASSERT_TRUE(scripts.IsReady()) << cc_get_error().ErrorString.GetCStr();
    ScriptDialog dialog0 = DialogObject(0);
    ASSERT_EQ(0, Dialog_HasOptionBeenChosen(&dialog0, 3));

    EXPECT_EQ(0, run_dialog_option(0, 2, kSayChosenNever, false));
    EXPECT_EQ(1, Dialog_HasOptionBeenChosen(&dialog0, 3));
    EXPECT_EQ(DFLG_ON | DFLG_NOREPEAT | DFLG_HASBEENCHOSEN, game.Dialogs[0].Options[2].Flags);
    ExpectOptionTakesTheDefinition(0, 0);
    ExpectOptionTakesTheDefinition(0, 1);
}

// Dialog.SetOptionText changes the text, and marks the option as having a text of its own (which a save keeps) while
// the text differs from the original
TEST_F(GameEntities, DialogOptionTextIsSetAndMarked)
{
    Game &game = Game::Current();
    ScriptDialog dialog1 = DialogObject(1);
    Dialog_SetOptionText(&dialog1, 2, "Ask about the weather");
    EXPECT_STREQ("Ask about the weather", Dialog_GetOptionText(&dialog1, 2));
    EXPECT_EQ(DFLG_TEXTSET, game.Dialogs[1].Options[1].Flags);

    Dialog_SetOptionText(&dialog1, 2, "Dialog 1 option 1");
    EXPECT_STREQ("Dialog 1 option 1", Dialog_GetOptionText(&dialog1, 2));
    EXPECT_EQ(0, game.Dialogs[1].Options[1].Flags);
    ExpectOptionTakesTheDefinition(1, 0);
    ExpectOptionTakesTheDefinition(1, 2);
}

// Game.SetAudioTypeCrossfadeSpeed (clamped to 0..100) and Game.SetAudioTypeSpeechVolumeDrop write the audio type
TEST_F(GameEntities, AudioTypeSettersWriteTheType)
{
    Game_SetAudioTypeCrossfadeSpeed(1, 7);
    EXPECT_EQ(7, Game_GetAudioTypeCrossfadeSpeed(1));
    Game_SetAudioTypeCrossfadeSpeed(1, 150);
    EXPECT_EQ(100, Game_GetAudioTypeCrossfadeSpeed(1));
    Game_SetAudioTypeSpeechVolumeDrop(1, 33);
    EXPECT_EQ(33, Game_GetAudioTypeSpeechVolumeDrop(1));
    EXPECT_EQ(100, Game_GetAudioTypeCrossfadeSpeed(1));

    ExpectAudioTypeTakesTheDefinition(0);
    ExpectAudioTypeTakesTheDefinition(2);
}

// SetGameOption changes an option that a script may change and returns the old value; a restricted option keeps its
// value, and the call returns it
TEST_F(GameEntities, GameOptionsSetByTheScript)
{
    Game &game = Game::Current();
    EXPECT_EQ(1, SetGameOption(OPT_PORTRAITSIDE, PORTRAIT_ALTERNATE));
    EXPECT_EQ(PORTRAIT_ALTERNATE, GetGameOption(OPT_PORTRAITSIDE));
    EXPECT_EQ(1, game.Play.swap_portrait_side);

    EXPECT_EQ(1, SetGameOption(OPT_DEBUGMODE, 0));
    EXPECT_EQ(1, GetGameOption(OPT_DEBUGMODE));
    EXPECT_EQ(1, GetGameOption(OPT_DIALOGNUMBERED));
}

// The dialog bullet, the inventory cursor's hotspot sprite, colour and cross colour, and the player character, as the
// script sets them
TEST_F(GameEntities, GameValuesSetByTheScript)
{
    Game &game = Game::Current();
    DialogOptions_SetBulletGraphic(55);
    EXPECT_EQ(55, DialogOptions_GetBulletGraphic());
    Game_SetInventoryCursorHotspotGraphic(56);
    EXPECT_EQ(56, Game_GetInventoryCursorHotspotGraphic());
    Game_SetInventoryCursorHotspotColor(0x00AABBCC);
    EXPECT_EQ(0x00AABBCC, Game_GetInventoryCursorHotspotColor());
    Game_SetInventoryCursorHotspotCrossColor(0x00DDEEFF);
    EXPECT_EQ(0x00DDEEFF, Game_GetInventoryCursorHotspotCrossColor());

    ASSERT_LT(game.DisplayedRoom, 0); // no room is loaded: the new player stays where it is
    Character_SetAsPlayer(&game.Characters[2]);
    EXPECT_EQ(2, game.PlayerCharacterID);
    EXPECT_EQ(&game.Characters[2], game.PlayerChar);
}

// Each audio clip is its own script object: the managed pool holds the address of its element, which
// Game.AudioClips[i] returns, and a clip's script handle written to a save and read back is that address again. A
// clip's id is its index, whatever the game file said.
TEST_F(GameEntities, AudioClipsAreScriptObjectsAtTheirElements)
{
    Game &game = Game::Current();
    std::vector<int32_t> handles(kAudioClipCount);
    for (int i = 0; i < kAudioClipCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "clip " << i);
        AudioClip *clip = &game.AudioClips[i];
        EXPECT_EQ(i, clip->id);
        EXPECT_EQ(clip, Game_GetAudioClip(i));
        handles[i] = ccGetObjectHandleFromAddress(clip);
        EXPECT_GT(handles[i], 0);
        EXPECT_EQ(clip, ccGetObjectAddressFromHandle(handles[i]));
    }
    EXPECT_EQ(nullptr, Game_GetAudioClip(-1));
    EXPECT_EQ(nullptr, Game_GetAudioClip(kAudioClipCount));

    IScriptObject &manager = ccDynamicAudioClip;
    std::vector<uint8_t> saved(16);
    const int size = manager.Serialize(&game.AudioClips[2], saved.data(), static_cast<int>(saved.size()));
    ASSERT_EQ(4, size);
    saved.resize(static_cast<size_t>(size));
    pool.Reset();
    ASSERT_EQ(nullptr, pool.HandleToAddress(handles[2]));
    Stream in(std::make_unique<VectorStream>(saved));
    ccDynamicAudioClip.Unserialize(handles[2], &in, saved.size());
    EXPECT_EQ(&game.AudioClips[2], ccGetObjectAddressFromHandle(handles[2]));
}

// Every saved field of every frame of a view comes back from a save into a game started again: the frames set through
// the seven setters, saved, the game started again from its definition, restored
TEST_F(GameEntities, ViewFramesComeBackFromASave)
{
    for (int loop = 0; loop < kLoopCount; ++loop)
    {
        for (int frame = 0; frame < kFrameCount; ++frame)
        {
            const int n = loop * kFrameCount + frame;
            ScriptViewFrame svf(kFramedView, loop, frame);
            ViewFrame_SetGraphic(&svf, 200 + n);
            ViewFrame_SetXOffset(&svf, -30 - n);
            ViewFrame_SetYOffset(&svf, 40 + n);
            ViewFrame_SetSpeed(&svf, 50 + n);
            ViewFrame_SetFlipped(&svf, (n % 2) == 0 ? kFlip_Vertical : kFlip_Both);
            ViewFrame_SetLinkedAudio(&svf, Game_GetAudioClip((n + 1) % kAudioClipCount));
            ViewFrame_SetEventName(&svf, String::FromFormat("saved%d", n).GetCStr());
        }
    }
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Views, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_Views);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int loop = 0; loop < kLoopCount; ++loop)
    {
        for (int frame = 0; frame < kFrameCount; ++frame)
        {
            SCOPED_TRACE(testing::Message() << "loop " << loop << " frame " << frame);
            const int n = loop * kFrameCount + frame;
            ScriptViewFrame svf(kFramedView, loop, frame);
            EXPECT_EQ(200 + n, ViewFrame_GetGraphic(&svf));
            EXPECT_EQ(-30 - n, ViewFrame_GetXOffset(&svf));
            EXPECT_EQ(40 + n, ViewFrame_GetYOffset(&svf));
            EXPECT_EQ(50 + n, ViewFrame_GetSpeed(&svf));
            EXPECT_EQ(static_cast<int>((n % 2) == 0 ? kFlip_Vertical : kFlip_Both), ViewFrame_GetFlipped(&svf));
            EXPECT_EQ(Game_GetAudioClip((n + 1) % kAudioClipCount), ViewFrame_GetLinkedAudio(&svf));
            EXPECT_STREQ(String::FromFormat("saved%d", n).GetCStr(), ViewFrame_GetEventName(&svf));
        }
    }
}

// The six saved fields of an inventory item come back from a save into a game started again, the cursor hotspot's
// alignment included (SavegameContent.CharactersAndItemsAreRestored leaves it out)
TEST_F(GameEntities, InventoryItemsComeBackFromASave)
{
    const FrameAlignment aligns[kItemCount] = {kAlignTopRight, kAlignBottomLeft, kAlignBottomCenter};
    for (int i = 0; i < kItemCount; ++i)
    {
        ScriptInvItem item = ItemObject(i);
        InventoryItem_SetGraphic(&item, 300 + i);
        InventoryItem_SetCursorGraphic(&item, 310 + i);
        InventoryItem_SetCursorHotspotX(&item, 320 + i);
        InventoryItem_SetCursorHotspotY(&item, 330 + i);
        InventoryItem_SetCursorHotspotAlignment(&item, aligns[i]);
        InventoryItem_SetName(&item, String::FromFormat("Saved item %d", i).GetCStr());
    }
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_InvItems, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_InvItems);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int i = 0; i < kItemCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "item " << i);
        ScriptInvItem item = ItemObject(i);
        EXPECT_EQ(300 + i, InventoryItem_GetGraphic(&item));
        EXPECT_EQ(310 + i, InventoryItem_GetCursorGraphic(&item));
        EXPECT_EQ(320 + i, InventoryItem_GetCursorHotspotX(&item));
        EXPECT_EQ(330 + i, InventoryItem_GetCursorHotspotY(&item));
        EXPECT_EQ(static_cast<int>(aligns[i]), InventoryItem_GetCursorHotspotAlignment(&item));
        EXPECT_STREQ(String::FromFormat("Saved item %d", i).GetCStr(), InventoryItem_GetName_New(&item));
    }
}

// At start a cursor whose view number is not one of the game's views gets none (engine_init_game_settings: "they typed
// an invalid view number in the editor"); the others keep theirs
TEST_F(GameEntities, CursorViewPastTheViewsBecomesNoneAtStart)
{
    Game &game = Game::Current();
    ASSERT_EQ(kViewCount, game.MouseCursors[kBadViewCursor].view);
    RunEngineInitGameSettings();
    EXPECT_EQ(-1, game.MouseCursors[kBadViewCursor].view);
    EXPECT_EQ(kEmptyView, game.MouseCursors[kLookCursor].view);
    EXPECT_EQ(-1, game.MouseCursors[kPointerCursor].view);
    EXPECT_EQ(-1, game.MouseCursors[kUseInvCursor].view);
}

// The game file's global speech animation delay holds a flag and a delay: at start a value of 0 or more is the delay,
// with the option on (1); a negative value -v is the delay v - 1, with the option off (0)
// (engine_init_game_settings)
TEST_F(GameEntities, GlobalSpeechAnimationDelayIsSplitAtStart)
{
    Game &game = Game::Current();
    struct Case
    {
        int Loaded;
        int Delay;
        int Option;
    };
    for (const Case &c : {Case{5, 5, 1}, Case{0, 0, 1}, Case{-3, 2, 0}})
    {
        SCOPED_TRACE(testing::Message() << "loaded " << c.Loaded);
        Definition().Options[OPT_GLOBALTALKANIMSPD] = c.Loaded;
        RunEngineInitGameSettings();
        EXPECT_EQ(c.Delay, game.Play.talkanim_speed);
        EXPECT_EQ(c.Option, GetGameOption(OPT_GLOBALTALKANIMSPD));
    }
}

// A started game's items, cursors, view frames, dialog options, audio types, options and values are its definition's:
// what a live copy of each must start as
TEST_F(GameEntities, StartedGameTakesTheDefinitions)
{
    Game &game = Game::Current();
    const GameDef &def = game.Def();
    for (int i = 0; i < kItemCount; ++i)
        ExpectItemTakesTheDefinition(i);
    for (int i = 0; i < kCursorCount; ++i)
        ExpectCursorTakesTheDefinition(i);
    for (int loop = 0; loop < kLoopCount; ++loop)
    {
        for (int frame = 0; frame < kFrameCount; ++frame)
            ExpectFrameTakesTheDefinition(loop, frame);
    }
    EXPECT_EQ(def.Views[kEmptyView].numLoops, game.Views[kEmptyView].numLoops);
    EXPECT_EQ(def.Views[kEmptyView].loops[0].numFrames, game.Views[kEmptyView].loops[0].numFrames);
    for (int d = 0; d < kDialogCount; ++d)
    {
        for (int o = 0; o < kOptionCount; ++o)
            ExpectOptionTakesTheDefinition(d, o);
    }
    for (int i = 0; i < kAudioTypeCount; ++i)
        ExpectAudioTypeTakesTheDefinition(i);
    for (int i = 0; i < kAudioClipCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "clip " << i);
        EXPECT_STREQ(def.AudioClips[i].fileName.GetCStr(), Game_GetAudioClip(i)->fileName.GetCStr());
        EXPECT_EQ(def.AudioClips[i].type, Game_GetAudioClip(i)->type);
    }
    for (int opt = OPT_DEBUGMODE; opt <= OPT_HIGHESTOPTION; ++opt)
        EXPECT_EQ(def.Options[opt], GetGameOption(opt)) << "option " << opt;
    EXPECT_EQ(def.Options[OPT_LIPSYNCTEXT], GetGameOption(OPT_LIPSYNCTEXT));
    EXPECT_EQ(def.DialogBulletSprite, DialogOptions_GetBulletGraphic());
    EXPECT_EQ(def.InventoryCursorHotspotSprite, Game_GetInventoryCursorHotspotGraphic());
    EXPECT_EQ(def.InventoryCursorHotspotColor, Game_GetInventoryCursorHotspotColor());
    EXPECT_EQ(def.InventoryCursorHotspotCrossColor, Game_GetInventoryCursorHotspotCrossColor());
    EXPECT_EQ(def.PlayerCharacterID, game.PlayerCharacterID);
    EXPECT_EQ(&game.Characters[def.PlayerCharacterID], game.PlayerChar);
    EXPECT_EQ(def.DefaultLipSyncFrame, game.DefaultLipSyncFrame);
}
