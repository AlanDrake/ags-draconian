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
// frames and of the inventory items, and of every field of the items, cursors,
// view frames, dialog options and audio types by their definitions' field
// tables (the saved ones restored, the others the definition's); the fix-up that engine_init_game_settings
// makes at start (the loader's are Common's GameLoader tests); the values a
// started game has, which are its definition's; ViewFrame.YOffset, which gave
// the x offset; the sprite table, which the sprite cache writes in the game's live
// copy (sizes, dynamic sprites, deleted sprites, flags), with Game::IsSpriteValid.
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
#include "game/game_init.h"
#include "game/game_state.h"
#include "game/game_struct_defines.h"
#include "game/inventory_item_def.h"
#include "game/mouse_cursor_def.h"
#include "game/render/dynamic_sprite.h"
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
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/sprite_cache.h"
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
#include "util/field_table.h"
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
// The cursors: a pointer, a look cursor with a mode view, the "use inventory" cursor, and one whose view number in the
// game file was past the game's views, which the loader made none (GameLoader.CursorViewPastTheViewsBecomesNone)
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
// The sprite file of StartWithSpriteFile: sprites 0..9, 16x16 32-bit, but kMissingSprite; the game file gives
// kKeepDepthSprite and kMissingSprite a flag (SPF_KEEPDEPTH)
const int kFileSpriteCount = 10;
const int kFileSpriteSize = 16;
const int kKeepDepthSprite = 3;
const int kMissingSprite = 5;

// The sprite file of StartWithSpriteFile, made in memory
std::vector<uint8_t> MakeSpriteFile()
{
    std::vector<uint8_t> file;
    std::vector<SpriteInfo> infos;
    SpriteCache source(infos, SpriteCache::Callbacks());
    for (int i = 0; i < kFileSpriteCount; ++i)
    {
        if (i != kMissingSprite)
            source.SetSprite(i, std::make_unique<Bitmap>(kFileSpriteSize, kFileSpriteSize, 32));
    }
    SpriteFileIndex index;
    HError err = source.SaveToFile(std::make_unique<Stream>(std::make_unique<VectorStream>(file, kStream_Write)), 0,
                                   kSprCompress_None, index);
    EXPECT_TRUE(err) << err->FullMessage().GetCStr();
    return file;
}

// The two sprite tables hold the same sizes and flags
void ExpectSameSpriteInfos(const std::vector<SpriteInfo> &expected, const std::vector<SpriteInfo> &actual)
{
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        SCOPED_TRACE(testing::Message() << "sprite " << i);
        EXPECT_EQ(expected[i].Width, actual[i].Width);
        EXPECT_EQ(expected[i].Height, actual[i].Height);
        EXPECT_EQ(expected[i].ColorDepth, actual[i].ColorDepth);
        EXPECT_EQ(expected[i].Flags, actual[i].Flags);
    }
}

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
        MakeDefinition(Definition());
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

    // The game's definition (or another one, to compare with): of each entity two or more, with values of their own
    static void MakeDefinition(GameDef &def)
    {
        def.CharacterCount = kCharacterCount;
        def.Characters.clear();
        def.Characters.resize(kCharacterCount);
        for (int i = 0; i < kCharacterCount; ++i)
        {
            def.Characters[i].ScriptName = String::FromFormat("cPerson%d", i);
            def.Characters[i].Name = String::FromFormat("Person %d", i);
            def.Characters[i].View = -1; // engine_init_game_settings precaches a view: that needs a graphics driver
        }
        def.PlayerCharacterID = 1;

        def.InventoryItemCount = kItemCount;
        def.InventoryItems.clear();
        def.InventoryItems.resize(kItemCount);
        const FrameAlignment item_aligns[kItemCount] = {kAlignNone, kAlignBottomRight, kAlignMiddleCenter};
        for (int i = 0; i < kItemCount; ++i)
        {
            auto &item = def.InventoryItems[i];
            item.Name = String::FromFormat("Item %d", i);
            item.Sprite = 20 + i;
            item.CursorSprite = 40 + i;
            item.CursorHotspotX = 3 + i;
            item.CursorHotspotY = 5 + i;
            item.CursorHotspotAlignment = item_aligns[i];
        }

        def.MouseCursorCount = kCursorCount;
        def.MouseCursors.clear();
        def.MouseCursors.resize(kCursorCount);
        const CursorRole roles[kCursorCount] = {kCursorRole_Pointer, kCursorRole_Look, kCursorRole_UseInv,
                                                kCursorRole_None};
        const short views[kCursorCount] = {-1, kEmptyView, -1, -1};
        for (int i = 0; i < kCursorCount; ++i)
        {
            auto &cur = def.MouseCursors[i];
            cur.Role = roles[i];
            cur.Sprite = 30 + i;
            cur.HotspotX = static_cast<short>(1 + i);
            cur.HotspotY = static_cast<short>(2 + i);
            cur.View = views[i];
            cur.AnimationDelay = 5 + i;
            cur.Flags = static_cast<uint8_t>(MCF_STANDARD | (i == kLookCursor ? MCF_EVENT : 0));
        }

        MakeViews(def);

        def.DialogCount = kDialogCount;
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
            type.ID = i;
            type.ReservedChannels = 1;
            type.VolumeReductionWhileSpeechPlaying = 20 + i;
            type.CrossfadeSpeed = 10 + i;
        }
        def.AudioClips.clear();
        def.AudioClips.resize(kAudioClipCount);
        for (int i = 0; i < kAudioClipCount; ++i)
        {
            auto &clip = def.AudioClips[i];
            clip.ID = i; // the loader makes a clip's id its index (GameLoader.AudioClipIdIsItsIndex)
            clip.FileName = String::FromFormat("clip%d.ogg", i);
            clip.Type = static_cast<uint8_t>(i);
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
    static void MakeViews(GameDef &def)
    {
        def.ViewCount = kViewCount;
        def.Views.clear();
        def.Views.resize(kViewCount);
        def.Views[kFramedView].Initialize(kLoopCount);
        for (int loop = 0; loop < kLoopCount; ++loop)
        {
            def.Views[kFramedView].Loops[loop].Initialize(kFrameCount);
            for (int frame = 0; frame < kFrameCount; ++frame)
            {
                const int n = loop * kFrameCount + frame;
                auto &vf = def.Views[kFramedView].Loops[loop].Frames[frame];
                vf.Sprite = FramePic(n);
                vf.OffsetX = static_cast<int16_t>(FrameXOffset(n));
                vf.OffsetY = static_cast<int16_t>(FrameYOffset(n));
                vf.Delay = static_cast<int16_t>(FrameSpeed(n));
                vf.Flags = FrameFlags(n);
                vf.LinkedAudio = FrameSound(n);
                vf.EventName = String::FromFormat("step%d", n);
            }
        }
        def.Views[kEmptyView].Initialize(1);
        def.Views[kEmptyView].Loops[0].Initialize(0);
    }

    // Does what InitGameState does for these entities that a test can run without fonts, plugins, scripts and a
    // graphics driver: the live copies of the definitions (StartFromDefinitions, as InitGameState), the player, the
    // custom property lists, the audio clips' ids and their registration with the managed pool
    void StartGame()
    {
        Game &game = *_game;
        StartFromDefinitions(game);
        for (int i = 0; i < game.CharacterCount; ++i)
            game.Characters[i].ID = i;
        game.Play.charProps.clear();
        game.Play.charProps.resize(game.CharacterCount);
        game.Play.invProps.clear();
        game.Play.invProps.resize(game.InventoryItemCount);
        game.Play.dialogProps.clear();
        game.Play.dialogProps.resize(game.DialogCount);
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
        MakeDefinition(Definition());
        StartGame();
    }

    // The definition of StartWithSpriteFile: MakeDefinition's, with the game file's flag SPF_KEEPDEPTH for
    // kKeepDepthSprite and kMissingSprite
    static void MakeSpriteDefinition(GameDef &def)
    {
        MakeDefinition(def);
        def.SpriteInfos[kKeepDepthSprite].Flags = SPF_KEEPDEPTH;
        def.SpriteInfos[kMissingSprite].Flags = SPF_KEEPDEPTH;
    }

    // The sprite table of MakeSpriteDefinition, made apart: what the definition's table must stay
    static std::vector<SpriteInfo> LoadedSpriteInfos()
    {
        GameDef made;
        MakeSpriteDefinition(made);
        return made.SpriteInfos;
    }

    // Starts the game again from its definition, with its sprites as the engine has them: the fixture's sprite cache
    // attached to the live sprite table with the engine's callbacks (init_sprite_cache, at engine start), the
    // definition with the game file's flags (MakeSpriteDefinition), the game started (StartGame, then the cache
    // enlarged to the game's sprite count, as InitGameState), then the sprite file opened (as engine_init_sprites)
    void StartWithSpriteFile()
    {
        EngineServices &services = EngineServices::Current();
        init_sprite_cache();
        pool.Reset();
        MakeSpriteDefinition(Definition());
        StartGame();
        services.Sprites.EnlargeTo(static_cast<sprkey_t>(Definition().SpriteInfos.size()) - 1);
        services.Sprites.Reset();
        if (_spriteFile.empty())
            _spriteFile = MakeSpriteFile();
        HError err =
            services.Sprites.InitFile(std::make_unique<Stream>(std::make_unique<VectorStream>(_spriteFile)), nullptr);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    }

    // The started game's sprite table after StartWithSpriteFile: the sprite file's sizes, the game file's flags; the
    // missing sprite is an empty slot, its flag dropped by the cache; the sprites past the file's are the definition's
    static void ExpectSpritesStartedWithTheFile()
    {
        Game &game = Game::Current();
        const GameDef &def = game.Def();
        ASSERT_EQ(def.SpriteInfos.size(), game.SpriteInfos.size());
        for (int i = 0; i < static_cast<int>(game.SpriteInfos.size()); ++i)
        {
            SCOPED_TRACE(testing::Message() << "sprite " << i);
            const SpriteInfo &info = game.SpriteInfos[i];
            if (i < kFileSpriteCount && i != kMissingSprite)
            {
                EXPECT_EQ(kFileSpriteSize, info.Width);
                EXPECT_EQ(kFileSpriteSize, info.Height);
                EXPECT_EQ(32, info.ColorDepth);
                EXPECT_EQ(def.SpriteInfos[i].Flags, info.Flags);
            }
            else if (i == kMissingSprite)
            {
                EXPECT_FALSE(info.IsValid());
                EXPECT_EQ(0u, info.Flags);
            }
            else
            {
                EXPECT_EQ(def.SpriteInfos[i].Width, info.Width);
                EXPECT_EQ(def.SpriteInfos[i].Height, info.Height);
                EXPECT_EQ(def.SpriteInfos[i].Flags, info.Flags);
            }
        }
        EXPECT_EQ(static_cast<uint32_t>(SPF_KEEPDEPTH), game.SpriteInfos[kKeepDepthSprite].Flags);
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

    // The round trip of the live elements of one kind (elements(game) lists them, by address), by the rows of their
    // definition's table: in each element the saved fields are altered (AlterFields: what the game changed during play;
    // keep, if given, then restores what the engine keeps true between two fields), which gives the expected element,
    // and then the fields from the definition. The component cmp is saved, the game started again from its definition
    // (RestartGame) and the save restored. Returns the fields of the restored elements that differ from the expected
    // ones: a saved field that the save lost, or a field from the definition that the start did not give again. The
    // start gives the definition's values back (StartFromDefinitions, from InitGameState), not the restore, which
    // reads only the saved fields into the live elements; nothing writes the other fields during play, so a restore
    // into a running game finds the definition's values there too.
    template <typename T, typename Elements>
    std::string RoundTripLosses(SaveCmpSelection cmp, Elements elements, void (*keep)(T &) = nullptr)
    {
        Game &game = Game::Current();
        std::vector<T> expected;
        for (T *element : elements(game))
        {
            const T started = *element;
            AlterFields(*element, FieldRole::Saved);
            if (keep)
                keep(*element);
            expected.push_back(*element);
            EXPECT_FALSE(DiffFields(started, *element).empty()) << "the element has no saved field to alter";
            AlterFields(*element, FieldRole::FromDefinition);
            EXPECT_FALSE(DiffFields(expected.back(), *element).empty())
                << "the element has no field from the definition";
        }
        EXPECT_FALSE(expected.empty());
        std::vector<uint8_t> saved;
        std::string error;
        if (!Save(cmp, saved, error))
            return "the save failed: " + error;

        RestartGame();
        HSaveError err = Restore(saved, cmp);
        if (!err)
            return std::string("the restore failed: ") + err->FullMessage().GetCStr();
        const std::vector<T *> restored = elements(game);
        if (restored.size() != expected.size())
            return "the game has another count of elements";
        std::string lost;
        for (size_t i = 0; i < expected.size(); ++i)
        {
            for (const String &line : DiffFields(expected[i], *restored[i]))
                lost += "element " + std::to_string(i) + ": " + line.GetCStr() + "\n";
        }
        return lost;
    }

    // Item i, as its getters give it, against its definition
    static void ExpectItemTakesTheDefinition(int i)
    {
        SCOPED_TRACE(testing::Message() << "item " << i);
        const InventoryItemDef &def = Game::Current().Def().InventoryItems[i];
        ScriptInvItem item = ItemObject(i);
        EXPECT_EQ(def.Sprite, InventoryItem_GetGraphic(&item));
        EXPECT_EQ(def.CursorSprite, InventoryItem_GetCursorGraphic(&item));
        EXPECT_EQ(def.CursorHotspotX, InventoryItem_GetCursorHotspotX(&item));
        EXPECT_EQ(def.CursorHotspotY, InventoryItem_GetCursorHotspotY(&item));
        EXPECT_EQ(static_cast<int>(def.CursorHotspotAlignment), InventoryItem_GetCursorHotspotAlignment(&item));
        EXPECT_STREQ(def.Name.GetCStr(), InventoryItem_GetName_New(&item));
    }

    // Cursor i against its definition: the graphic through its getter, the fields without one directly
    static void ExpectCursorTakesTheDefinition(int i)
    {
        SCOPED_TRACE(testing::Message() << "cursor " << i);
        Game &game = Game::Current();
        const MouseCursorDef &def = game.Def().MouseCursors[i];
        EXPECT_EQ(def.Sprite, Mouse_GetModeGraphic(i));
        EXPECT_EQ(def.HotspotX, game.MouseCursors[i].HotspotX);
        EXPECT_EQ(def.HotspotY, game.MouseCursors[i].HotspotY);
        EXPECT_EQ(def.View, game.MouseCursors[i].View);
        EXPECT_EQ(def.AnimationDelay, game.MouseCursors[i].AnimationDelay);
        EXPECT_EQ(def.Flags, game.MouseCursors[i].Flags);
    }

    // Frame of the framed view against its definition, through the getters
    static void ExpectFrameTakesTheDefinition(int loop, int frame)
    {
        SCOPED_TRACE(testing::Message() << "loop " << loop << " frame " << frame);
        const ViewFrameDef &def = Game::Current().Def().Views[kFramedView].Loops[loop].Frames[frame];
        ScriptViewFrame svf(kFramedView, loop, frame);
        EXPECT_EQ(def.Sprite, ViewFrame_GetGraphic(&svf));
        EXPECT_EQ(def.OffsetX, ViewFrame_GetXOffset(&svf));
        EXPECT_EQ(def.OffsetY, ViewFrame_GetYOffset(&svf));
        EXPECT_EQ(def.Delay, ViewFrame_GetSpeed(&svf));
        EXPECT_EQ(static_cast<int>(GfxDef::GetFlipFromFlags(def.Flags)), ViewFrame_GetFlipped(&svf));
        EXPECT_EQ(def.LinkedAudio < 0 ? nullptr : Game_GetAudioClip(def.LinkedAudio), ViewFrame_GetLinkedAudio(&svf));
        EXPECT_STREQ(def.EventName.GetCStr(), ViewFrame_GetEventName(&svf));
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
        EXPECT_EQ(def.CrossfadeSpeed, Game_GetAudioTypeCrossfadeSpeed(i));
        EXPECT_EQ(def.VolumeReductionWhileSpeechPlaying, Game_GetAudioTypeSpeechVolumeDrop(i));
    }

    // The game's definition (Game::Def()) is still the one MakeDefinition made: what the game changes, setters and
    // restores, goes to the live copies. Every field that a setter or a restore writes in a live copy is compared.
    static void ExpectDefinitionAsMade()
    {
        GameDef made;
        MakeDefinition(made);
        const GameDef &def = Game::Current().Def();
        ASSERT_EQ(made.InventoryItems.size(), def.InventoryItems.size());
        for (size_t i = 0; i < made.InventoryItems.size(); ++i)
        {
            SCOPED_TRACE(testing::Message() << "item " << i);
            EXPECT_STREQ(made.InventoryItems[i].Name.GetCStr(), def.InventoryItems[i].Name.GetCStr());
            EXPECT_EQ(made.InventoryItems[i].Sprite, def.InventoryItems[i].Sprite);
            EXPECT_EQ(made.InventoryItems[i].CursorSprite, def.InventoryItems[i].CursorSprite);
            EXPECT_EQ(made.InventoryItems[i].CursorHotspotX, def.InventoryItems[i].CursorHotspotX);
            EXPECT_EQ(made.InventoryItems[i].CursorHotspotY, def.InventoryItems[i].CursorHotspotY);
            EXPECT_EQ(made.InventoryItems[i].CursorHotspotAlignment, def.InventoryItems[i].CursorHotspotAlignment);
        }
        ASSERT_EQ(made.MouseCursors.size(), def.MouseCursors.size());
        for (size_t i = 0; i < made.MouseCursors.size(); ++i)
        {
            SCOPED_TRACE(testing::Message() << "cursor " << i);
            EXPECT_EQ(made.MouseCursors[i].Sprite, def.MouseCursors[i].Sprite);
            EXPECT_EQ(made.MouseCursors[i].HotspotX, def.MouseCursors[i].HotspotX);
            EXPECT_EQ(made.MouseCursors[i].HotspotY, def.MouseCursors[i].HotspotY);
            EXPECT_EQ(made.MouseCursors[i].View, def.MouseCursors[i].View);
            EXPECT_EQ(made.MouseCursors[i].AnimationDelay, def.MouseCursors[i].AnimationDelay);
            EXPECT_EQ(made.MouseCursors[i].Flags, def.MouseCursors[i].Flags);
        }
        ASSERT_EQ(made.Views.size(), def.Views.size());
        for (size_t v = 0; v < made.Views.size(); ++v)
        {
            ASSERT_EQ(made.Views[v].LoopCount, def.Views[v].LoopCount);
            for (int loop = 0; loop < made.Views[v].LoopCount; ++loop)
            {
                ASSERT_EQ(made.Views[v].Loops[loop].FrameCount, def.Views[v].Loops[loop].FrameCount);
                for (int frame = 0; frame < made.Views[v].Loops[loop].FrameCount; ++frame)
                {
                    SCOPED_TRACE(testing::Message() << "view " << v << " loop " << loop << " frame " << frame);
                    const ViewFrameDef &made_vf = made.Views[v].Loops[loop].Frames[frame];
                    const ViewFrameDef &vf = def.Views[v].Loops[loop].Frames[frame];
                    EXPECT_EQ(made_vf.Sprite, vf.Sprite);
                    EXPECT_EQ(made_vf.OffsetX, vf.OffsetX);
                    EXPECT_EQ(made_vf.OffsetY, vf.OffsetY);
                    EXPECT_EQ(made_vf.Delay, vf.Delay);
                    EXPECT_EQ(made_vf.Flags, vf.Flags);
                    EXPECT_EQ(made_vf.LinkedAudio, vf.LinkedAudio);
                    EXPECT_STREQ(made_vf.EventName.GetCStr(), vf.EventName.GetCStr());
                }
            }
        }
        ASSERT_EQ(made.Dialogs.size(), def.Dialogs.size());
        for (size_t d = 0; d < made.Dialogs.size(); ++d)
        {
            ASSERT_EQ(made.Dialogs[d].Options.size(), def.Dialogs[d].Options.size());
            for (size_t o = 0; o < made.Dialogs[d].Options.size(); ++o)
            {
                SCOPED_TRACE(testing::Message() << "dialog " << d << " option " << o);
                EXPECT_STREQ(made.Dialogs[d].Options[o].Text.GetCStr(), def.Dialogs[d].Options[o].Text.GetCStr());
                EXPECT_EQ(made.Dialogs[d].Options[o].Flags, def.Dialogs[d].Options[o].Flags);
            }
        }
        ASSERT_EQ(made.AudioClipTypes.size(), def.AudioClipTypes.size());
        for (size_t i = 0; i < made.AudioClipTypes.size(); ++i)
        {
            SCOPED_TRACE(testing::Message() << "audio type " << i);
            EXPECT_EQ(made.AudioClipTypes[i].CrossfadeSpeed, def.AudioClipTypes[i].CrossfadeSpeed);
            EXPECT_EQ(made.AudioClipTypes[i].VolumeReductionWhileSpeechPlaying,
                      def.AudioClipTypes[i].VolumeReductionWhileSpeechPlaying);
        }
        ASSERT_EQ(made.AudioClips.size(), def.AudioClips.size());
        for (size_t i = 0; i < made.AudioClips.size(); ++i)
            EXPECT_EQ(made.AudioClips[i].ID, def.AudioClips[i].ID) << "clip " << i;
        for (int opt = 0; opt < GameDef::MAX_OPTIONS; ++opt)
            EXPECT_EQ(made.Options[opt], def.Options[opt]) << "option " << opt;
        EXPECT_EQ(made.PlayerCharacterID, def.PlayerCharacterID);
        EXPECT_EQ(made.DialogBulletSprite, def.DialogBulletSprite);
        EXPECT_EQ(made.InventoryCursorHotspotSprite, def.InventoryCursorHotspotSprite);
        EXPECT_EQ(made.InventoryCursorHotspotColor, def.InventoryCursorHotspotColor);
        EXPECT_EQ(made.InventoryCursorHotspotCrossColor, def.InventoryCursorHotspotCrossColor);
        EXPECT_EQ(made.DefaultLipSyncFrame, def.DefaultLipSyncFrame);
        ExpectSameSpriteInfos(made.SpriteInfos, def.SpriteInfos);
    }

    // The started game's items, cursors, view frames, dialog options, audio types, audio clips, options and values,
    // through the getters where there are some, against its definition
    static void ExpectStartedFromTheDefinitions()
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
        EXPECT_EQ(def.Views[kEmptyView].LoopCount, game.Views[kEmptyView].LoopCount);
        EXPECT_EQ(def.Views[kEmptyView].Loops[0].FrameCount, game.Views[kEmptyView].Loops[0].FrameCount);
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
            EXPECT_STREQ(def.AudioClips[i].FileName.GetCStr(), Game_GetAudioClip(i)->FileName.GetCStr());
            EXPECT_EQ(def.AudioClips[i].Type, Game_GetAudioClip(i)->Type);
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
        ExpectSameSpriteInfos(def.SpriteInfos, game.SpriteInfos);
    }

    EngineServices _services;
    std::unique_ptr<Game> _game = std::make_unique<Game>();
    Room _room;
    // The sprite file of StartWithSpriteFile, made at its first call
    std::vector<uint8_t> _spriteFile;
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
    ExpectDefinitionAsMade();
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
    EXPECT_EQ(8, game.MouseCursors[kLookCursor].HotspotX);
    EXPECT_EQ(9, game.MouseCursors[kLookCursor].HotspotY);

    Mouse_ChangeModeView(kLookCursor, 0, 11);
    EXPECT_EQ(-1, game.MouseCursors[kLookCursor].View);
    EXPECT_EQ(11, game.MouseCursors[kLookCursor].AnimationDelay);
    Mouse_ChangeModeView(kLookCursor, kEmptyView + 1, SCR_NO_VALUE);
    EXPECT_EQ(kEmptyView, game.MouseCursors[kLookCursor].View);
    EXPECT_EQ(11, game.MouseCursors[kLookCursor].AnimationDelay);

    Mouse_DisableCursorMode(kLookCursor);
    EXPECT_EQ(0, Mouse_IsModeEnabled(kLookCursor));
    EXPECT_EQ(MCF_STANDARD | MCF_EVENT | MCF_DISABLED, game.MouseCursors[kLookCursor].Flags);
    Mouse_EnableCursorMode(kLookCursor);
    EXPECT_EQ(1, Mouse_IsModeEnabled(kLookCursor));
    EXPECT_EQ(MCF_STANDARD | MCF_EVENT, game.MouseCursors[kLookCursor].Flags);

    ExpectCursorTakesTheDefinition(kUseInvCursor);
    ExpectCursorTakesTheDefinition(kBadViewCursor);
    ExpectDefinitionAsMade();
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
    EXPECT_EQ(4 + kCursorSpriteWidth, game.MouseCursors[kUseInvCursor].HotspotX);
    EXPECT_EQ(6 + kCursorSpriteHeight, game.MouseCursors[kUseInvCursor].HotspotY);

    InventoryItem_SetCursorHotspotX(&item1, 2);
    EXPECT_EQ(2 + kCursorSpriteWidth, game.MouseCursors[kUseInvCursor].HotspotX);
    EXPECT_EQ(6 + kCursorSpriteHeight, game.MouseCursors[kUseInvCursor].HotspotY);
    ExpectCursorTakesTheDefinition(kLookCursor);
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
}

// ViewFrame.YOffset gives the frame's y offset (it gave the x offset, as upstream's ags4 still does)
TEST_F(GameEntities, ViewFrameYOffsetIsTheYOffset)
{
    ScriptViewFrame svf(kFramedView, 0, 1);
    ViewFrame_SetXOffset(&svf, 3);
    ViewFrame_SetYOffset(&svf, 7);
    EXPECT_EQ(3, ViewFrame_GetXOffset(&svf));
    EXPECT_EQ(7, ViewFrame_GetYOffset(&svf));
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
}

// Each audio clip is its own script object: the managed pool holds the address of its element, which
// Game.AudioClips[i] returns, and a clip's script handle written to a save and read back is that address again. A
// clip's id is its index.
TEST_F(GameEntities, AudioClipsAreScriptObjectsAtTheirElements)
{
    Game &game = Game::Current();
    std::vector<int32_t> handles(kAudioClipCount);
    for (int i = 0; i < kAudioClipCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "clip " << i);
        AudioClip *clip = &game.AudioClips[i];
        EXPECT_EQ(i, clip->ID);
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
    ExpectDefinitionAsMade();
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
    ExpectDefinitionAsMade();
}

// The saved fields of the mouse cursors come back from a save into the live cursors of a game started again
TEST_F(GameEntities, MouseCursorsComeBackFromASave)
{
    Game &game = Game::Current();
    Mouse_ChangeCursorGraphic(kLookCursor, 81);
    Mouse_ChangeCursorHotspot(kLookCursor, 6, 7);
    Mouse_ChangeModeView(kLookCursor, 0, 12);
    Mouse_DisableCursorMode(kBadViewCursor);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Cursors, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_Cursors);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(81, Mouse_GetModeGraphic(kLookCursor));
    EXPECT_EQ(6, game.MouseCursors[kLookCursor].HotspotX);
    EXPECT_EQ(7, game.MouseCursors[kLookCursor].HotspotY);
    EXPECT_EQ(-1, game.MouseCursors[kLookCursor].View);
    EXPECT_EQ(12, game.MouseCursors[kLookCursor].AnimationDelay);
    EXPECT_EQ(0, Mouse_IsModeEnabled(kBadViewCursor));
    ExpectCursorTakesTheDefinition(kPointerCursor);
    ExpectCursorTakesTheDefinition(kUseInvCursor);
    ExpectDefinitionAsMade();
}

// The dialog options' states, "chosen" flags and texts come back from a save into the live dialogs of a game started
// again
TEST_F(GameEntities, DialogOptionsComeBackFromASave)
{
    ScriptDialog dialog0 = DialogObject(0);
    ScriptDialog dialog1 = DialogObject(1);
    Dialog_SetOptionState(&dialog0, 1, 0);
    Dialog_SetOptionState(&dialog0, 2, 2);
    Dialog_SetHasOptionBeenChosen(&dialog0, 3, true);
    Dialog_SetOptionText(&dialog1, 2, "Ask about the harbour");
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Dialogs, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_Dialogs);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(0, Dialog_GetOptionState(&dialog0, 1));
    EXPECT_EQ(2, Dialog_GetOptionState(&dialog0, 2));
    EXPECT_EQ(1, Dialog_HasOptionBeenChosen(&dialog0, 3));
    EXPECT_STREQ("Ask about the harbour", Dialog_GetOptionText(&dialog1, 2));
    ExpectOptionTakesTheDefinition(1, 0);
    ExpectOptionTakesTheDefinition(1, 2);
    ExpectDefinitionAsMade();
}

// The audio types' crossfade speeds and speech volume drops come back from a save into the live types of a game started
// again
TEST_F(GameEntities, AudioTypesComeBackFromASave)
{
    Game_SetAudioTypeCrossfadeSpeed(1, 44);
    Game_SetAudioTypeSpeechVolumeDrop(2, 55);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Audio, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_Audio);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(44, Game_GetAudioTypeCrossfadeSpeed(1));
    EXPECT_EQ(55, Game_GetAudioTypeSpeechVolumeDrop(2));
    ExpectAudioTypeTakesTheDefinition(0);
    ExpectDefinitionAsMade();
}

// Every saved field of every inventory item comes back from a save into a game started again, and every field from the
// definition is the definition's again (RoundTripLosses); the rows of InventoryItemDef say which field is which, so a
// field added to the struct is checked here without a line of this test changing
TEST_F(GameEntities, EveryInventoryItemFieldIsRestored)
{
    EXPECT_EQ("", RoundTripLosses<InventoryItem>(kSaveCmp_InvItems,
                                                 [](Game &running)
                                                 {
                                                     std::vector<InventoryItem *> items;
                                                     for (InventoryItem &item : running.InventoryItems)
                                                         items.push_back(&item);
                                                     return items;
                                                 }));
    ExpectDefinitionAsMade();
}

// The same for every mouse cursor (MouseCursorDef's rows)
TEST_F(GameEntities, EveryMouseCursorFieldIsRestored)
{
    EXPECT_EQ("", RoundTripLosses<MouseCursor>(kSaveCmp_Cursors,
                                               [](Game &running)
                                               {
                                                   std::vector<MouseCursor *> cursors;
                                                   for (MouseCursor &cursor : running.MouseCursors)
                                                       cursors.push_back(&cursor);
                                                   return cursors;
                                               }));
    ExpectDefinitionAsMade();
}

// The same for every frame of every loop of the framed view (ViewFrameDef's rows); the loops' and the views' own fields
// are the definition's (the save holds their counts only to compare them with the game's)
TEST_F(GameEntities, EveryViewFrameFieldIsRestored)
{
    EXPECT_EQ("", RoundTripLosses<ViewFrame>(kSaveCmp_Views,
                                             [](Game &running)
                                             {
                                                 std::vector<ViewFrame *> frames;
                                                 for (ViewLoop &loop : running.Views[kFramedView].Loops)
                                                 {
                                                     for (int frame = 0; frame < loop.FrameCount; ++frame)
                                                         frames.push_back(&loop.Frames[frame]);
                                                 }
                                                 return frames;
                                             }));
    ExpectDefinitionAsMade();
}

// The same for every option of every dialog (DialogOptionDef's rows). The save holds an option's text only while its
// flags have DFLG_TEXTSET, which the engine sets whenever the text differs from the original (Dialog.SetOptionText):
// the altered option gets the flag, as the engine would give it.
TEST_F(GameEntities, EveryDialogOptionFieldIsRestored)
{
    EXPECT_EQ("", RoundTripLosses<DialogOption>(
                      kSaveCmp_Dialogs,
                      [](Game &running)
                      {
                          std::vector<DialogOption *> options;
                          for (Dialog &dialog : running.Dialogs)
                          {
                              for (DialogOption &option : dialog.Options)
                                  options.push_back(&option);
                          }
                          return options;
                      },
                      [](DialogOption &option) { option.Flags |= DFLG_TEXTSET; }));
    ExpectDefinitionAsMade();
}

// The same for every audio type (AudioClipTypeDef's rows)
TEST_F(GameEntities, EveryAudioTypeFieldIsRestored)
{
    EXPECT_EQ("", RoundTripLosses<AudioClipType>(kSaveCmp_Audio,
                                                 [](Game &running)
                                                 {
                                                     std::vector<AudioClipType *> types;
                                                     for (AudioClipType &type : running.AudioClipTypes)
                                                         types.push_back(&type);
                                                     return types;
                                                 }));
    ExpectDefinitionAsMade();
}

// The saved options, the player character, the dialog bullet, the inventory cursor's hotspot marker and the default
// lip-sync frame come back from a save into the live values of a game started again
TEST_F(GameEntities, GameOptionsAndValuesComeBackFromASave)
{
    Game &game = Game::Current();
    SetGameOption(OPT_PORTRAITSIDE, PORTRAIT_ALTERNATE);
    SetGameOption(OPT_DIALOGGAP, 9);
    SetGameOption(OPT_LIPSYNCTEXT, 0);
    DialogOptions_SetBulletGraphic(65);
    Game_SetInventoryCursorHotspotGraphic(66);
    Game_SetInventoryCursorHotspotColor(0x00010203);
    Game_SetInventoryCursorHotspotCrossColor(0x00040506);
    ASSERT_LT(game.DisplayedRoom, 0); // no room is loaded: the new player stays where it is
    Character_SetAsPlayer(&game.Characters[2]);
    game.DefaultLipSyncFrame = 7; // no script function sets it: only a restore writes it
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GameState, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_GameState);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(PORTRAIT_ALTERNATE, GetGameOption(OPT_PORTRAITSIDE));
    EXPECT_EQ(9, GetGameOption(OPT_DIALOGGAP));
    EXPECT_EQ(0, GetGameOption(OPT_LIPSYNCTEXT));
    EXPECT_EQ(1, GetGameOption(OPT_DIALOGNUMBERED));
    EXPECT_EQ(65, DialogOptions_GetBulletGraphic());
    EXPECT_EQ(66, Game_GetInventoryCursorHotspotGraphic());
    EXPECT_EQ(0x00010203, Game_GetInventoryCursorHotspotColor());
    EXPECT_EQ(0x00040506, Game_GetInventoryCursorHotspotCrossColor());
    EXPECT_EQ(2, game.PlayerCharacterID);
    EXPECT_EQ(7, game.DefaultLipSyncFrame);
    ExpectDefinitionAsMade();
}

// The game file's global speech animation delay holds a flag and a delay: at start a value of 0 or more is the delay,
// with the option on (1); a negative value -v is the delay v - 1, with the option off (0)
// (engine_init_game_settings). The live option changes, the definition keeps the value of the game file.
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
        pool.Reset();
        MakeDefinition(Definition());
        Definition().Options[OPT_GLOBALTALKANIMSPD] = c.Loaded;
        StartGame();
        RunEngineInitGameSettings();
        EXPECT_EQ(c.Delay, game.Play.talkanim_speed);
        EXPECT_EQ(c.Option, GetGameOption(OPT_GLOBALTALKANIMSPD));
        EXPECT_EQ(c.Loaded, game.Def().Options[OPT_GLOBALTALKANIMSPD]);
    }
}

// A started game's items, cursors, view frames, dialog options, audio types, options and values are its definition's:
// what a live copy of each must start as
TEST_F(GameEntities, StartedGameTakesTheDefinitions)
{
    ExpectStartedFromTheDefinitions();
    ExpectDefinitionAsMade();
}

// unload_game keeps nothing of the game's live copies: the lists are empty, the options and values those of a game
// without a definition; the next game, started from its definition, takes the definition's values, not the changes
// that the first one made
TEST_F(GameEntities, UnloadedGameKeepsNoLiveCopies)
{
    Game &game = Game::Current();
    ScriptInvItem item1 = ItemObject(1);
    InventoryItem_SetGraphic(&item1, 91);
    Mouse_ChangeCursorGraphic(kLookCursor, 92);
    ScriptViewFrame svf(kFramedView, 1, 0);
    ViewFrame_SetGraphic(&svf, 93);
    ScriptDialog dialog0 = DialogObject(0);
    Dialog_SetOptionState(&dialog0, 1, 0);
    Game_SetAudioTypeCrossfadeSpeed(1, 94);
    SetGameOption(OPT_DIALOGGAP, 95);
    DialogOptions_SetBulletGraphic(96);
    Game_SetInventoryCursorHotspotGraphic(97);

    unload_game();
    EXPECT_TRUE(game.Characters.empty());
    EXPECT_TRUE(game.InventoryItems.empty());
    EXPECT_TRUE(game.MouseCursors.empty());
    EXPECT_TRUE(game.AudioClips.empty());
    EXPECT_TRUE(game.AudioClipTypes.empty());
    EXPECT_TRUE(game.Views.empty());
    EXPECT_TRUE(game.Dialogs.empty());
    EXPECT_TRUE(game.SpriteInfos.empty());
    for (int opt = 0; opt < GameDef::MAX_OPTIONS; ++opt)
        EXPECT_EQ(0, game.Options[opt]) << "option " << opt;
    EXPECT_EQ(-1, game.PlayerCharacterID);
    EXPECT_EQ(0, game.DialogBulletSprite);
    EXPECT_EQ(0, game.InventoryCursorHotspotSprite);
    EXPECT_EQ(0, game.InventoryCursorHotspotColor);
    EXPECT_EQ(0, game.InventoryCursorHotspotCrossColor);
    EXPECT_EQ(0, game.DefaultLipSyncFrame);

    MakeDefinition(Definition());
    StartGame();
    ExpectStartedFromTheDefinitions();
    ExpectDefinitionAsMade();
}

// The sprite cache writes the game's live sprite table: the sprite file's sizes, a dynamic sprite made during play, the
// flag of a drawing surface acquired on it, its deletion and a game sprite deleted from the cache go there; the
// definition's table keeps the game file's flags and no sizes
TEST_F(GameEntities, SpriteCacheWritesTheLiveSpriteTable)
{
    Game &game = Game::Current();
    StartWithSpriteFile();
    ExpectSpritesStartedWithTheFile();
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game.Def().SpriteInfos);

    const int slot = add_dynamic_sprite(std::make_unique<Bitmap>(24, 12, 32));
    ASSERT_GT(slot, 0);
    EXPECT_EQ(24, game.SpriteInfos[slot].Width);
    EXPECT_EQ(12, game.SpriteInfos[slot].Height);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC), game.SpriteInfos[slot].Flags);

    attach_dynsprite_surface(slot, 0);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC | SPF_SURFACEACQUIRED), game.SpriteInfos[slot].Flags);
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game.Def().SpriteInfos);
    on_dynsprite_surface_release(slot, false);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC), game.SpriteInfos[slot].Flags);

    free_dynamic_sprite(slot);
    EXPECT_FALSE(game.SpriteInfos[slot].IsValid());
    EXPECT_EQ(0u, game.SpriteInfos[slot].Flags);

    EngineServices::Current().Sprites.DeleteSprite(kKeepDepthSprite);
    EXPECT_FALSE(game.SpriteInfos[kKeepDepthSprite].IsValid());
    EXPECT_EQ(0u, game.SpriteInfos[kKeepDepthSprite].Flags);
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game.Def().SpriteInfos);
}

// Game::IsSpriteValid reads the live table: a sprite of the sprite file and a dynamic sprite are valid, a free slot, a
// sprite missing from the sprite file and a number outside the table are not. GameDef::IsSpriteValid, which it
// hides, reads the definition's table, which has no sizes.
TEST_F(GameEntities, SpriteValidityIsTheLiveTables)
{
    Game &game = Game::Current();
    StartWithSpriteFile();
    EXPECT_TRUE(game.IsSpriteValid(kKeepDepthSprite));
    EXPECT_FALSE(game.Def().IsSpriteValid(kKeepDepthSprite));
    EXPECT_FALSE(game.IsSpriteValid(kMissingSprite));
    EXPECT_FALSE(game.IsSpriteValid(50)); // a slot of the table without a sprite
    EXPECT_FALSE(game.IsSpriteValid(-1));
    EXPECT_FALSE(game.IsSpriteValid(static_cast<int>(game.SpriteInfos.size())));

    const int slot = add_dynamic_sprite(std::make_unique<Bitmap>(8, 8, 32));
    ASSERT_GT(slot, 0);
    EXPECT_TRUE(game.IsSpriteValid(slot));
    free_dynamic_sprite(slot);
    EXPECT_FALSE(game.IsSpriteValid(slot));
}

// A dynamic sprite comes back from a save into the live sprite table of a game started again, with its flags; the
// definition's table stays as loaded
TEST_F(GameEntities, DynamicSpritesComeBackFromASaveIntoTheLiveTable)
{
    Game &game = Game::Current();
    StartWithSpriteFile();
    const int slot = add_dynamic_sprite(std::make_unique<Bitmap>(24, 12, 32));
    ASSERT_GT(slot, 0);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_DynamicSprites, saved, error)) << error;

    StartWithSpriteFile();
    ASSERT_FALSE(game.SpriteInfos[slot].IsValid());
    HSaveError err = Restore(saved, kSaveCmp_DynamicSprites);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(24, game.SpriteInfos[slot].Width);
    EXPECT_EQ(12, game.SpriteInfos[slot].Height);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC), game.SpriteInfos[slot].Flags);
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game.Def().SpriteInfos);
    free_dynamic_sprite(slot);
}

// After unload_game and a second start the live sprite table is the definition's again, with the sprite file's sizes:
// nothing is left of the first game's dynamic sprite, deleted sprite and acquired surface's flag
TEST_F(GameEntities, SecondStartTakesTheSpriteTableAgain)
{
    Game &game = Game::Current();
    StartWithSpriteFile();
    const int slot = add_dynamic_sprite(std::make_unique<Bitmap>(24, 12, 32));
    ASSERT_GT(slot, 0);
    attach_dynsprite_surface(slot, 0);
    detach_dynsprite_surface(slot); // forgets the surface, keeps the flag
    ASSERT_NE(0u, game.SpriteInfos[slot].Flags & SPF_SURFACEACQUIRED);
    EngineServices::Current().Sprites.DeleteSprite(kKeepDepthSprite);

    unload_game();
    EXPECT_TRUE(game.SpriteInfos.empty());
    EXPECT_TRUE(game.Def().SpriteInfos.empty());

    StartWithSpriteFile();
    ExpectSpritesStartedWithTheFile();
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game.Def().SpriteInfos);
}
