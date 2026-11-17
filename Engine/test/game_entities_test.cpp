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
// views, dialogs and audio types by their definitions' field
// tables (the saved ones restored, the others the definition's), and of the items' and the dialogs' play state by
// theirs; the fix-up that engine_init_game_settings
// makes at start (the loader's are Common's GameLoader tests), and the whole
// game state it leaves, field by field, against a new GameState; the values a
// started game has, which are its definition's; ViewFrame.YOffset, which gave
// the x offset; the sprite table, which the sprite cache writes in the game's live
// copy (sizes, dynamic sprites, deleted sprites, flags), with Game::IsSpriteValid.
// The GUIs and their controls: a script setter of the GUI and of each type of
// control, the save round trip and the unload, against the live copies and the
// definitions; the live GUIs' links to the live controls.
// The custom properties of the characters, inventory items, dialogs, audio clips,
// GUIs and GUI controls through their script functions: the value set during
// play, else the definition's, else the schema's default; the save round trip,
// the restore and the unload; the bytes a save holds for them. Three script
// variables of the game state (game.disable_antialiasing,
// game.disable_dialog_parser, game.no_multiloop_repeat) and what they switch;
// the wait's skip type and the voice speech checks of the game state.
// The game is made by hand, in a Game, a Room and an EngineServices of the
// fixture's own: no game file is read.
//
// Not tested: InitGameState (it loads fonts, plugins and scripts, and draws:
// the fixture starts the game with the parts of it that run without them, see
// StartGame); a cursor mode's view that has frames (Mouse.ChangeModeView then
// makes textures of the frames, which needs a graphics driver: the cursors use a
// view without frames); the option commands of a dialog script (they need a
// compiled dialog script); the marker that the inventory cursor draws on its
// image (it needs the sprite's image); a custom property name that the schema
// does not have, or a text property read as a number (the engine quits the game:
// get_property_desc calls quitprintf, which ends the process); the dialog parser
// shown when game.disable_dialog_parser is 0 (the options are then shown, a
// blocking loop that needs a graphics driver and input).
//
//=============================================================================
#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <allegro.h> // get_palette, set_palette_range, get_uformat, set_uformat

#include "game/anim_flow.h"
#include "game/audio/audio_clip.h"
#include "game/audio_clip_def.h"
#include "game/audio_clip_type_def.h"
#include "game/customproperties.h"
#include "game/dialog/dialog.h"
#include "game/dialog_def.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/dynobj/script_game.h"
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
#include "game/ui/button.h"
#include "game/ui/gui.h"
#include "game/ui/gui_control.h"
#include "game/ui/inv_window.h"
#include "game/ui/label.h"
#include "game/ui/list_box.h"
#include "game/ui/mouse.h"
#include "game/ui/slider.h"
#include "game/ui/text_box.h"
#include "game/view_def.h"
#include "game/world/character.h"
#include "game/world/inventory_item.h"
#include "game/world/object.h"
#include "game/world/room_context.h"
#include "game/world/view_frame.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/sprite_cache.h"
#include "gtest/gtest.h"
#include "gui/guibutton.h"
#include "gui/guicontrol.h"
#include "gui/guidefines.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "main/engine.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_defines.h"
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
#include "util/string_types.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace AGS
{
namespace Engine
{
// game_init.cpp: makes each audio clip's id its index and registers the audio channels and the clips with the managed
// pool, at their elements' addresses. Part of InitGameState; no header declares it.
void InitAndRegisterAudioObjects(Game &game);
// savegame.cpp: what a restore into the running game does before it reads the components (RestoreGameState): frees
// the state that the components do not overwrite, the custom property values set during play among it. No header
// declares it.
void DoBeforeRestore(PreservedParams &pp, SaveCmpSelection select_cmp);
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
// The GUIs: each has one control of each of the six types; GUI g's control of a type has the per-type id g, so the
// controls of GUI 1 are its controls 0 to 5 and the elements 1 of the six control lists
const int kGuiCount = 2;
const GUIControlType kControlTypes[] = {kGUIButton, kGUILabel, kGUIInvWindow, kGUISlider, kGUITextBox, kGUIListBox};
// The fields that the GUI tests give values of their own: one of the GUI and one of each type of control, all saved
enum GuiField
{
    kGuiX,
    kButtonTextColor,
    kLabelTextColor,
    kInvWindowItemWidth,
    kListBoxSelectedBgColor,
    kSliderMax,
    kTextBoxTextColor
};
// The custom properties of the game's schema: a number and a text, with their defaults. The definition gives element 1
// of every kind values of its own and the other elements none.
const char *const kNumberProperty = "Weight";
const char *const kTextProperty = "Colour";
const int kNumberDefault = 7;
const char *const kTextDefault = "grey";
const int kNumberOfElement1 = 11;
const char *const kTextOfElement1 = "red";
// The byte offsets of three variables of the script's "game" object (CCScriptGame, script_game.cpp)
const int kNoMultiLoopRepeatOffset = 72 * sizeof(int32_t);
const int kDisableAntialiasingOffset = 97 * sizeof(int32_t);
const int kDisableDialogParserOffset = 102 * sizeof(int32_t);

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

ScriptGUI GuiObject(int id)
{
    ScriptGUI gui;
    gui.id = id;
    return gui;
}

// Element i of the game's list of controls of that type: through a Game the live list, through a GameDef (Game::Def())
// the definition's
template <typename G>
auto ControlOf(G &game, GUIControlType type, int i)
    -> std::conditional_t<std::is_const_v<G>, const GUIControl *, GUIControl *>
{
    switch (type)
    {
    case kGUIButton: return &game.GuiButtons[i];
    case kGUILabel: return &game.GuiLabels[i];
    case kGUIInvWindow: return &game.GuiInvWindows[i];
    case kGUISlider: return &game.GuiSliders[i];
    case kGUITextBox: return &game.GuiTextBoxes[i];
    case kGUIListBox: return &game.GuiListBoxes[i];
    default: return nullptr;
    }
}

// Tells if the control is an element of one of the game's control lists ('game' names them as for ControlOf)
template <typename G>
bool IsControlOf(G &game, const GUIControl *control)
{
    auto in = [control](const auto &list)
    {
        return std::any_of(list.begin(), list.end(),
                           [control](const GUIControl &element) { return &element == control; });
    };
    return in(game.GuiButtons) || in(game.GuiLabels) || in(game.GuiInvWindows) || in(game.GuiSliders) ||
           in(game.GuiTextBoxes) || in(game.GuiListBoxes);
}

// The values of the fields of GuiField in GUI g and in its controls of the game's lists ('game' names them as for
// ControlOf), in the order of GuiField
template <typename G>
std::vector<int> GuiFieldValues(G &game, int g)
{
    return {game.Guis[g].GetX(),
            game.GuiButtons[g].GetTextColor(),
            game.GuiLabels[g].GetTextColor(),
            game.GuiInvWindows[g].GetItemWidth(),
            game.GuiListBoxes[g].GetSelectedBgColor(),
            game.GuiSliders[g].GetMaxValue(),
            game.GuiTextBoxes[g].GetTextColor()};
}

// The text a script function returned (nullptr as an empty text)
std::string TextOf(const char *text)
{
    return text ? text : "";
}

// The bytes of a property map as a save holds it (Properties::WriteValues)
std::vector<uint8_t> PropertyMapBytes(const StringIMap &map)
{
    std::vector<uint8_t> bytes;
    {
        Stream out(std::make_unique<VectorStream>(bytes, kStream_Write));
        Properties::WriteValues(map, &out);
    }
    return bytes;
}

// The data of the save's component of that name: after its opening tag, the component's header (its size, flags,
// format version, data size, uncompressed size, checksum: SavegameComponents::WriteComponent), then the data. Empty
// if the save does not have the component.
std::vector<uint8_t> ComponentData(const std::vector<uint8_t> &save, const std::string &name)
{
    const std::string tag = "<" + name + ">";
    const auto at = std::search(save.begin(), save.end(), tag.begin(), tag.end());
    if (at == save.end())
        return {};
    const soff_t header_pos = static_cast<soff_t>(at - save.begin()) + static_cast<soff_t>(tag.size());
    Stream in(std::make_unique<VectorStream>(save));
    in.Seek(header_pos, kSeekBegin);
    const int32_t header_size = in.ReadInt32();
    in.ReadInt32(); // flags
    in.ReadInt32(); // format version
    const int32_t data_size = in.ReadInt32();
    const size_t data_pos = static_cast<size_t>(header_pos + header_size);
    if (data_size < 0 || data_pos + data_size > save.size())
        return {};
    return std::vector<uint8_t>(save.begin() + data_pos, save.begin() + data_pos + data_size);
}

// One kind of game entity that has custom properties, as a script reaches it: how many the game has, the script
// functions that get and set a number and a text property of element i, the definition's values of element i, and the
// save component that holds the values set during play (none for the audio clips)
struct PropertyKind
{
    std::string Name;
    int Count = 0;
    std::function<int(int, const char *)> GetNumber;
    std::function<std::string(int, const char *)> GetText;
    std::function<bool(int, const char *, int)> SetNumber;
    std::function<bool(int, const char *, const char *)> SetText;
    std::function<const StringIMap &(const GameDef &, int)> Definition;
    SaveCmpSelection Component = kSaveCmp_None;
    const char *ComponentName = nullptr;
};

void AddGuiPropertyKinds(std::vector<PropertyKind> &kinds);

// The kinds of game entities that have custom properties: characters, inventory items, dialogs, audio clips, GUIs and
// the six types of GUI control
std::vector<PropertyKind> PropertyKinds()
{
    std::vector<PropertyKind> kinds;
    {
        PropertyKind kind;
        kind.Name = "character";
        kind.Count = kCharacterCount;
        kind.GetNumber = [](int i, const char *name)
        { return Character_GetProperty(&Game::Current().Characters[i], name); };
        kind.GetText = [](int i, const char *name)
        { return TextOf(Character_GetTextProperty(&Game::Current().Characters[i], name)); };
        kind.SetNumber = [](int i, const char *name, int value)
        { return Character_SetProperty(&Game::Current().Characters[i], name, value); };
        kind.SetText = [](int i, const char *name, const char *value)
        { return Character_SetTextProperty(&Game::Current().Characters[i], name, value); };
        kind.Definition = [](const GameDef &def, int i) -> const StringIMap & { return def.CharacterProperties[i]; };
        kind.Component = kSaveCmp_Characters;
        kind.ComponentName = "Characters";
        kinds.push_back(kind);
    }
    {
        PropertyKind kind;
        kind.Name = "inventory item";
        kind.Count = kItemCount;
        kind.GetNumber = [](int i, const char *name)
        {
            ScriptInvItem item = ItemObject(i);
            return InventoryItem_GetProperty(&item, name);
        };
        kind.GetText = [](int i, const char *name)
        {
            ScriptInvItem item = ItemObject(i);
            return TextOf(InventoryItem_GetTextProperty(&item, name));
        };
        kind.SetNumber = [](int i, const char *name, int value)
        {
            ScriptInvItem item = ItemObject(i);
            return InventoryItem_SetProperty(&item, name, value);
        };
        kind.SetText = [](int i, const char *name, const char *value)
        {
            ScriptInvItem item = ItemObject(i);
            return InventoryItem_SetTextProperty(&item, name, value);
        };
        kind.Definition = [](const GameDef &def, int i) -> const StringIMap &
        { return def.InventoryItemProperties[i]; };
        kind.Component = kSaveCmp_InvItems;
        kind.ComponentName = "Inventory Items";
        kinds.push_back(kind);
    }
    {
        PropertyKind kind;
        kind.Name = "dialog";
        kind.Count = kDialogCount;
        kind.GetNumber = [](int i, const char *name)
        {
            ScriptDialog dialog = DialogObject(i);
            return Dialog_GetProperty(&dialog, name);
        };
        kind.GetText = [](int i, const char *name)
        {
            ScriptDialog dialog = DialogObject(i);
            return TextOf(Dialog_GetTextProperty(&dialog, name));
        };
        kind.SetNumber = [](int i, const char *name, int value)
        {
            ScriptDialog dialog = DialogObject(i);
            return Dialog_SetProperty(&dialog, name, value);
        };
        kind.SetText = [](int i, const char *name, const char *value)
        {
            ScriptDialog dialog = DialogObject(i);
            return Dialog_SetTextProperty(&dialog, name, value);
        };
        kind.Definition = [](const GameDef &def, int i) -> const StringIMap & { return def.DialogProperties[i]; };
        kind.Component = kSaveCmp_Dialogs;
        kind.ComponentName = "Dialogs";
        kinds.push_back(kind);
    }
    {
        PropertyKind kind;
        kind.Name = "audio clip";
        kind.Count = kAudioClipCount;
        kind.GetNumber = [](int i, const char *name)
        { return AudioClip_GetProperty(&Game::Current().AudioClips[i], name); };
        kind.GetText = [](int i, const char *name)
        { return TextOf(AudioClip_GetTextProperty(&Game::Current().AudioClips[i], name)); };
        kind.SetNumber = [](int i, const char *name, int value)
        { return AudioClip_SetProperty(&Game::Current().AudioClips[i], name, value); };
        kind.SetText = [](int i, const char *name, const char *value)
        { return AudioClip_SetTextProperty(&Game::Current().AudioClips[i], name, value); };
        kind.Definition = [](const GameDef &def, int i) -> const StringIMap & { return def.AudioClipProperties[i]; };
        kinds.push_back(kind);
    }
    AddGuiPropertyKinds(kinds);
    return kinds;
}

// The GUIs and the six types of GUI control, for PropertyKinds
void AddGuiPropertyKinds(std::vector<PropertyKind> &kinds)
{
    {
        PropertyKind kind;
        kind.Name = "GUI";
        kind.Count = kGuiCount;
        kind.GetNumber = [](int i, const char *name)
        {
            ScriptGUI gui = GuiObject(i);
            return GUI_GetProperty(&gui, name);
        };
        kind.GetText = [](int i, const char *name)
        {
            ScriptGUI gui = GuiObject(i);
            return TextOf(GUI_GetTextProperty(&gui, name));
        };
        kind.SetNumber = [](int i, const char *name, int value)
        {
            ScriptGUI gui = GuiObject(i);
            return GUI_SetProperty(&gui, name, value);
        };
        kind.SetText = [](int i, const char *name, const char *value)
        {
            ScriptGUI gui = GuiObject(i);
            return GUI_SetTextProperty(&gui, name, value);
        };
        kind.Definition = [](const GameDef &def, int i) -> const StringIMap & { return def.GuiProperties[i]; };
        kind.Component = kSaveCmp_GUI;
        kind.ComponentName = "GUI";
        kinds.push_back(kind);
    }
    for (GUIControlType type : kControlTypes)
    {
        PropertyKind kind;
        kind.Name = "GUI control of type " + std::to_string(type);
        kind.Count = kGuiCount;
        kind.GetNumber = [type](int i, const char *name)
        { return GUIControl_GetProperty(ControlOf(Game::Current(), type, i), name); };
        kind.GetText = [type](int i, const char *name)
        { return TextOf(GUIControl_GetTextProperty(ControlOf(Game::Current(), type, i), name)); };
        kind.SetNumber = [type](int i, const char *name, int value)
        { return GUIControl_SetProperty(ControlOf(Game::Current(), type, i), name, value); };
        kind.SetText = [type](int i, const char *name, const char *value)
        { return GUIControl_SetTextProperty(ControlOf(Game::Current(), type, i), name, value); };
        kind.Definition = [type](const GameDef &def, int i) -> const StringIMap &
        { return def.GuiControlProperties[type][i]; };
        kind.Component = kSaveCmp_GUI;
        kind.ComponentName = "GUI";
        kinds.push_back(kind);
    }
}

// The kinds whose values set during play a save holds: all but the audio clips
std::vector<PropertyKind> SavedPropertyKinds()
{
    std::vector<PropertyKind> kinds = PropertyKinds();
    kinds.erase(std::remove_if(kinds.begin(), kinds.end(),
                               [](const PropertyKind &kind) { return kind.Component == kSaveCmp_None; }),
                kinds.end());
    return kinds;
}

bool IsGuiControlKind(const PropertyKind &kind)
{
    return kind.Name.find("GUI control") == 0;
}

// The components that hold the values of the custom properties set during play
const SaveCmpSelection kPropertyComponents =
    static_cast<SaveCmpSelection>(kSaveCmp_Characters | kSaveCmp_InvItems | kSaveCmp_Dialogs | kSaveCmp_GUI);

// Element i of the kind has the schema's defaults
void ExpectDefaults(const PropertyKind &kind, int i)
{
    SCOPED_TRACE(testing::Message() << kind.Name << " " << i);
    EXPECT_EQ(kNumberDefault, kind.GetNumber(i, kNumberProperty));
    EXPECT_EQ(kTextDefault, kind.GetText(i, kTextProperty));
}

// Element i of the kind has the values that the definition gives element 1, or the defaults
void ExpectDefinitionsValues(const PropertyKind &kind, int i)
{
    if (i != 1)
    {
        ExpectDefaults(kind, i);
        return;
    }
    SCOPED_TRACE(testing::Message() << kind.Name << " " << i);
    EXPECT_EQ(kNumberOfElement1, kind.GetNumber(i, kNumberProperty));
    EXPECT_EQ(kTextOfElement1, kind.GetText(i, kTextProperty));
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
// The tests that call engine_init_game_settings reset the globals it sets (RunEngineInitGameSettings). StartGame sets
// GUI::Options.ClipControls while it links the GUIs, and puts it back. RestoreIntoTheRunningGame runs DoBeforeRestore,
// which empties the engine's overlays, drawing caches, button animations and managed pool, all empty in these tests.
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
    GameDef &Definition() { return _def; }

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

        MakeGuis(def);
        MakeCustomProperties(def);
    }

    // The GUIs of MakeDefinition, as the loader reads them: each GUI's references to its controls in the six control
    // lists (kGuiCount, kControlTypes), not linked to them (StartGame links the live copies, GUI::RebuildGUI); and in
    // GUI g and its controls the values GuiValue(g, field) of the fields of GuiField
    static void MakeGuis(GameDef &def)
    {
        def.GuiCount = kGuiCount;
        def.Guis.clear();
        def.Guis.resize(kGuiCount);
        def.GuiButtons.clear();
        def.GuiButtons.resize(kGuiCount);
        def.GuiLabels.clear();
        def.GuiLabels.resize(kGuiCount);
        def.GuiInvWindows.clear();
        def.GuiInvWindows.resize(kGuiCount);
        def.GuiSliders.clear();
        def.GuiSliders.resize(kGuiCount);
        def.GuiTextBoxes.clear();
        def.GuiTextBoxes.resize(kGuiCount);
        def.GuiListBoxes.clear();
        def.GuiListBoxes.resize(kGuiCount);
        for (int g = 0; g < kGuiCount; ++g)
        {
            def.Guis[g].SetID(g);
            for (GUIControlType type : kControlTypes)
                def.Guis[g].AddControl(type, g, nullptr);
            def.Guis[g].SetX(GuiValue(g, kGuiX));
            def.GuiButtons[g].SetTextColor(GuiValue(g, kButtonTextColor));
            def.GuiLabels[g].SetTextColor(GuiValue(g, kLabelTextColor));
            def.GuiInvWindows[g].SetItemWidth(GuiValue(g, kInvWindowItemWidth));
            def.GuiListBoxes[g].SetSelectedBgColor(GuiValue(g, kListBoxSelectedBgColor));
            def.GuiSliders[g].SetMaxValue(GuiValue(g, kSliderMax));
            def.GuiTextBoxes[g].SetTextColor(GuiValue(g, kTextBoxTextColor));
        }
    }

    // The value of a field of GuiField in GUI g or in its control of the field's type: base, plus ten times the field,
    // plus g. MakeGuis gives the definitions the values of base 10.
    static int GuiValue(int g, int field, int base = 10) { return base + 10 * field + g; }

    // Sets the fields of GuiField in GUI g and its controls to GuiValue(g, field, base), through the script functions,
    // on the elements that the script reaches: the GUI by its index, each control by its address in the live list
    static void SetGuiValues(int g, int base)
    {
        Game &game = Game::Current();
        ScriptGUI gui = GuiObject(g);
        GUI_SetX(&gui, GuiValue(g, kGuiX, base));
        Button_SetTextColor(&game.GuiButtons[g], GuiValue(g, kButtonTextColor, base));
        Label_SetTextColor(&game.GuiLabels[g], GuiValue(g, kLabelTextColor, base));
        InvWindow_SetItemWidth(&game.GuiInvWindows[g], GuiValue(g, kInvWindowItemWidth, base));
        ListBox_SetSelectedBackColor(&game.GuiListBoxes[g], GuiValue(g, kListBoxSelectedBgColor, base));
        Slider_SetMax(&game.GuiSliders[g], GuiValue(g, kSliderMax, base));
        TextBox_SetTextColor(&game.GuiTextBoxes[g], GuiValue(g, kTextBoxTextColor, base));
    }

    // GUI g and its controls, as the script functions give them, have the values that SetGuiValues(g, base) sets
    static void ExpectGuiValues(int g, int base)
    {
        SCOPED_TRACE(testing::Message() << "GUI " << g);
        Game &game = Game::Current();
        ScriptGUI gui = GuiObject(g);
        EXPECT_EQ(GuiValue(g, kGuiX, base), GUI_GetX(&gui));
        EXPECT_EQ(GuiValue(g, kButtonTextColor, base), Button_GetTextColor(&game.GuiButtons[g]));
        EXPECT_EQ(GuiValue(g, kLabelTextColor, base), Label_GetTextColor(&game.GuiLabels[g]));
        EXPECT_EQ(GuiValue(g, kInvWindowItemWidth, base), InvWindow_GetItemWidth(&game.GuiInvWindows[g]));
        EXPECT_EQ(GuiValue(g, kListBoxSelectedBgColor, base), ListBox_GetSelectedBackColor(&game.GuiListBoxes[g]));
        EXPECT_EQ(GuiValue(g, kSliderMax, base), Slider_GetMax(&game.GuiSliders[g]));
        EXPECT_EQ(GuiValue(g, kTextBoxTextColor, base), TextBox_GetTextColor(&game.GuiTextBoxes[g]));
    }

    // Live GUI g and its controls have their definitions' values, and the GUI is linked to the live controls
    static void ExpectGuiTakesTheDefinition(int g)
    {
        Game &game = Game::Current();
        EXPECT_EQ(GuiFieldValues(game.Def(), g), GuiFieldValues(game, g)) << "GUI " << g;
        ExpectGuiLinkedToTheLiveControls(g);
    }

    // Each control of live GUI g is the element of the live list that the GUI's reference names, which has the GUI as
    // its parent; none is an element of the definition's lists
    static void ExpectGuiLinkedToTheLiveControls(int g)
    {
        SCOPED_TRACE(testing::Message() << "GUI " << g);
        Game &game = Game::Current();
        const GameDef &game_def = game.Def();
        const GUIMain &gui = game.Guis[g];
        const auto &refs = gui.GetControlRefs();
        ASSERT_EQ(refs.size(), static_cast<size_t>(gui.GetControlCount()));
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            SCOPED_TRACE(testing::Message() << "control " << i);
            const GUIControl *control = gui.GetControl(i);
            EXPECT_EQ(ControlOf(game, refs[i].first, refs[i].second), control);
            EXPECT_FALSE(IsControlOf(game_def, control));
            ASSERT_NE(nullptr, control);
            EXPECT_EQ(g, control->GetParentID());
            EXPECT_EQ(i, control->GetID());
        }
    }

    // The custom property schema of MakeDefinition (kNumberProperty, kTextProperty) and the definition's values of
    // every kind of entity: element 1 has values of its own, the others none
    static void MakeCustomProperties(GameDef &def)
    {
        def.CustomPropertySchema.clear();
        PropertyDesc number;
        number.Name = kNumberProperty;
        number.Type = kPropertyInteger;
        number.DefaultValue = String::FromFormat("%d", kNumberDefault);
        def.CustomPropertySchema[number.Name] = number;
        PropertyDesc text;
        text.Name = kTextProperty;
        text.Type = kPropertyString;
        text.DefaultValue = kTextDefault;
        def.CustomPropertySchema[text.Name] = text;

        auto give_values = [](std::vector<StringIMap> &lists, size_t count)
        {
            lists.assign(count, StringIMap());
            lists[1][kNumberProperty] = String::FromFormat("%d", kNumberOfElement1);
            lists[1][kTextProperty] = kTextOfElement1;
        };
        give_values(def.CharacterProperties, kCharacterCount);
        give_values(def.InventoryItemProperties, kItemCount);
        give_values(def.DialogProperties, kDialogCount);
        give_values(def.AudioClipProperties, kAudioClipCount);
        give_values(def.GuiProperties, kGuiCount);
        for (GUIControlType type : kControlTypes)
            give_values(def.GuiControlProperties[type], kGuiCount);
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
        // the first loop runs into the second: an animation of the two loops is one sequence
        def.Views[kFramedView].Loops[0].Flags = LOOPFLAG_RUNNEXTLOOP;
        def.Views[kEmptyView].Initialize(1);
        def.Views[kEmptyView].Loops[0].Initialize(0);
    }

    // Does what InitGameState does for these entities that a test can run without fonts, plugins, scripts and a
    // graphics driver: the live copies of the definitions (StartFromDefinitions, as InitGameState), the live GUIs linked
    // to the live controls (GUI::RebuildGUI; with the controls' graphics clipped to their rectangles, so that no font or
    // sprite is measured), the player, the GUIs' lists of the custom property values set during play sized as
    // InitGameState sizes them (a list that has values keeps them: InitGameState runs on a game that unload_game
    // emptied; the other entities hold theirs in their live copies, which start without any), the audio clips' ids and
    // their registration with the managed pool
    void StartGame()
    {
        Game &game = *_game;
        const GameDef &game_def = game.Def();
        StartFromDefinitions(game);
        for (int i = 0; i < game_def.CharacterCount; ++i)
            game.Characters[i].ID = i;
        const bool clip_controls = GUI::Options.ClipControls;
        GUI::Options.ClipControls = true;
        GUIRefCollection gui_control_refs(game.GuiButtons, game.GuiInvWindows, game.GuiLabels, game.GuiListBoxes,
                                          game.GuiSliders, game.GuiTextBoxes);
        HError err = GUI::RebuildGUI(game.Guis, gui_control_refs);
        GUI::Options.ClipControls = clip_controls;
        EXPECT_TRUE(err) << err->FullMessage().GetCStr();
        game.Play.GuiProperties.resize(game_def.GuiCount);
        game.Play.GuiControlProperties[kGUIButton].resize(game.GuiButtons.size());
        game.Play.GuiControlProperties[kGUILabel].resize(game.GuiLabels.size());
        game.Play.GuiControlProperties[kGUIInvWindow].resize(game.GuiInvWindows.size());
        game.Play.GuiControlProperties[kGUISlider].resize(game.GuiSliders.size());
        game.Play.GuiControlProperties[kGUITextBox].resize(game.GuiTextBoxes.size());
        game.Play.GuiControlProperties[kGUIListBox].resize(game.GuiListBoxes.size());
        game.Play.DefaultAudioTypeVolumes.assign(game.AudioClipTypes.size(), -1);
        InitAndRegisterAudioObjects(game);
        setup_player_character(game.PlayerCharacterID);
    }

    // Starts the game again from its definition: the managed pool emptied, the definition made again, the game
    // started. No test sets a custom property of a GUI before it: StartGame keeps the GUIs' values set during play, as
    // InitGameState, which runs on a game that unload_game emptied (UnloadedGameKeepsNoPropertyValues); the other
    // entities' values go with their live copies.
    void RestartGame()
    {
        pool.Reset();
        MakeDefinition(Definition());
        StartGame();
    }

    // Restores the components of the save into the running game as RestoreGameState does before it runs the game
    // again: DoBeforeRestore (which frees what the components do not overwrite: the custom property values set during
    // play among it, FreePropertyOverrides in savegame.cpp), then the components. DoBeforeRestore reads the global script's
    // data size: the game has an empty one meanwhile (EmptyScripts).
    HSaveError RestoreIntoTheRunningGame(const std::vector<uint8_t> &buf, SaveCmpSelection cmp)
    {
        Game &game = *_game;
        EmptyScripts scripts(game);
        PreservedParams pp;
        DoBeforeRestore(pp, cmp);
        Stream in(std::make_unique<VectorStream>(buf));
        RestoredData r_data;
        return SavegameComponents::ReadAll(&in, kSvgVersion_Current, cmp, pp, r_data);
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

    // Changes the fields of a role in the parts of an element (AlterFields): its definition's (Def) and, unless State is
    // void, its play state's
    template <typename Def, typename State, typename T>
    static void AlterParts(T &element, FieldRole role)
    {
        AlterFields<Def>(element, role);
        if constexpr (!std::is_void_v<State>)
            AlterFields<State>(element, role);
    }

    // The fields in which two elements differ, a line each (DiffFields), by the tables of their parts as AlterParts
    template <typename Def, typename State, typename T>
    static std::vector<String> DiffParts(const T &a, const T &b)
    {
        std::vector<String> lines = DiffFields<Def>(a, b);
        if constexpr (!std::is_void_v<State>)
        {
            for (const String &line : DiffFields<State>(a, b))
                lines.push_back(line);
        }
        return lines;
    }

    // The round trip of the live elements of one kind (elements(game) lists them, by address), by the rows of their
    // definition's table (Def) and of their play state's (State; void if the element has none): in each element the
    // saved fields are altered (AlterFields: what the game changed during play), which gives the expected element, and
    // then the fields from the definition; keep, if given, restores after each what the engine keeps true between two
    // fields. The component cmp is saved, the game started again from its definition
    // (RestartGame) and the save restored. Returns the fields of the restored elements that differ from the expected
    // ones: a saved field that the save lost, or a field from the definition that the start did not give again. The
    // start gives the definition's values back (StartFromDefinitions, from InitGameState), not the restore, which
    // reads only the saved fields into the live elements; nothing writes the other fields during play, so a restore
    // into a running game finds the definition's values there too.
    template <typename T, typename Def, typename State, typename Elements>
    std::string RoundTripPartsLosses(SaveCmpSelection cmp, Elements elements, void (*keep)(T &) = nullptr)
    {
        Game &game = Game::Current();
        std::vector<T> expected;
        for (T *element : elements(game))
        {
            const T started = *element;
            AlterParts<Def, State>(*element, FieldRole::Saved);
            if (keep)
                keep(*element);
            expected.push_back(*element);
            EXPECT_FALSE((DiffParts<Def, State>(started, *element).empty()))
                << "the element has no saved field to alter";
            AlterParts<Def, State>(*element, FieldRole::FromDefinition);
            if (keep)
                keep(*element);
            EXPECT_FALSE((DiffParts<Def, State>(expected.back(), *element).empty()))
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
            for (const String &line : DiffParts<Def, State>(expected[i], *restored[i]))
                lost += "element " + std::to_string(i) + ": " + line.GetCStr() + "\n";
        }
        return lost;
    }

    // The same for elements whose type is their definition's, with no play state
    template <typename T, typename Elements>
    std::string RoundTripLosses(SaveCmpSelection cmp, Elements elements, void (*keep)(T &) = nullptr)
    {
        return RoundTripPartsLosses<T, T, void>(cmp, elements, keep);
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
        ASSERT_EQ(made.Guis.size(), def.Guis.size());
        ASSERT_EQ(made.GuiButtons.size(), def.GuiButtons.size());
        ASSERT_EQ(made.GuiLabels.size(), def.GuiLabels.size());
        ASSERT_EQ(made.GuiInvWindows.size(), def.GuiInvWindows.size());
        ASSERT_EQ(made.GuiListBoxes.size(), def.GuiListBoxes.size());
        ASSERT_EQ(made.GuiSliders.size(), def.GuiSliders.size());
        ASSERT_EQ(made.GuiTextBoxes.size(), def.GuiTextBoxes.size());
        for (int g = 0; g < static_cast<int>(made.Guis.size()); ++g)
        {
            SCOPED_TRACE(testing::Message() << "GUI " << g);
            EXPECT_EQ(GuiFieldValues(made, g), GuiFieldValues(def, g));
            EXPECT_EQ(made.Guis[g].GetControlRefs(), def.Guis[g].GetControlRefs());
        }
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
        ASSERT_EQ(def.Guis.size(), game.Guis.size());
        EXPECT_EQ(def.GuiButtons.size(), game.GuiButtons.size());
        EXPECT_EQ(def.GuiLabels.size(), game.GuiLabels.size());
        EXPECT_EQ(def.GuiInvWindows.size(), game.GuiInvWindows.size());
        EXPECT_EQ(def.GuiListBoxes.size(), game.GuiListBoxes.size());
        EXPECT_EQ(def.GuiSliders.size(), game.GuiSliders.size());
        EXPECT_EQ(def.GuiTextBoxes.size(), game.GuiTextBoxes.size());
        for (int g = 0; g < static_cast<int>(game.Guis.size()); ++g)
            ExpectGuiTakesTheDefinition(g);
    }

    EngineServices _services;
    // The game's definition, which the test writes as the loader does, and the game played from it
    GameDef _def;
    std::unique_ptr<Game> _game = std::make_unique<Game>(_def);
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
    EXPECT_EQ(1, game.Play.SwapPortraitSide);

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
// definition is the definition's again (RoundTripPartsLosses); the rows of InventoryItemDef and InventoryItemState (the
// custom property values set during play) say which field is which, so a field added to either struct is checked here
// without a line of this test changing
TEST_F(GameEntities, EveryInventoryItemFieldIsRestored)
{
    EXPECT_EQ("", (RoundTripPartsLosses<InventoryItem, InventoryItemDef, InventoryItemState>(
                      kSaveCmp_InvItems,
                      [](Game &running)
                      {
                          std::vector<InventoryItem *> items;
                          for (InventoryItem &item : running.InventoryItems)
                              items.push_back(&item);
                          return items;
                      })));
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

// The same for the framed view, by ViewDef's rows: the saved fields are its frames' (ViewFrameDef's rows); the counts,
// the loops' flags and the rest are the definition's (the save holds the counts only to compare them with the game's).
// The counts stay the lists' lengths, which AlterFields does not change and which the save's writer follows.
TEST_F(GameEntities, EveryViewFrameFieldIsRestored)
{
    EXPECT_EQ("", RoundTripLosses<View>(
                      kSaveCmp_Views, [](Game &running) { return std::vector<View *>{&running.Views[kFramedView]}; },
                      [](View &view)
                      {
                          view.LoopCount = static_cast<int>(view.Loops.size());
                          for (ViewLoop &loop : view.Loops)
                              loop.FrameCount = static_cast<int>(loop.Frames.size());
                      }));
    ExpectDefinitionAsMade();
}

// The same for every dialog, by DialogDef's rows: the saved fields are its options' (DialogOptionDef's rows). The save
// holds an option's text only while its flags have DFLG_TEXTSET, which the engine sets whenever the text differs from
// the original (Dialog.SetOptionText): the altered option gets the flag, as the engine would give it.
TEST_F(GameEntities, EveryDialogOptionFieldIsRestored)
{
    EXPECT_EQ("", (RoundTripPartsLosses<Dialog, DialogDef, void>(
                      kSaveCmp_Dialogs,
                      [](Game &running)
                      {
                          std::vector<Dialog *> dialogs;
                          for (Dialog &dialog : running.Dialogs)
                              dialogs.push_back(&dialog);
                          return dialogs;
                      },
                      [](Dialog &dialog)
                      {
                          for (DialogOption &option : dialog.Options)
                              option.Flags |= DFLG_TEXTSET;
                      })));
    ExpectDefinitionAsMade();
}

// Every saved field of the play state of every dialog (DialogState's rows: the custom property values set during play)
// comes back from a save into a game started again; DialogDef's rows are the test above.
TEST_F(GameEntities, EveryDialogStateFieldIsRestored)
{
    Game &game = Game::Current();
    std::vector<Dialog> expected;
    for (Dialog &dialog : game.Dialogs)
    {
        const Dialog started = dialog;
        AlterFields<DialogState>(dialog, FieldRole::Saved);
        EXPECT_FALSE(DiffFields<DialogState>(started, dialog).empty()) << "the dialog has no saved field to alter";
        expected.push_back(dialog);
    }
    ASSERT_FALSE(expected.empty());
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Dialogs, saved, error)) << error;

    RestartGame();
    HSaveError err = Restore(saved, kSaveCmp_Dialogs);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    ASSERT_EQ(expected.size(), game.Dialogs.size());
    std::string lost;
    for (size_t i = 0; i < expected.size(); ++i)
    {
        for (const String &line : DiffFields<DialogState>(expected[i], game.Dialogs[i]))
            lost += "dialog " + std::to_string(i) + ": " + line.GetCStr() + "\n";
    }
    EXPECT_EQ("", lost);
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
    const GameDef &game_def = game.Def();
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
        EXPECT_EQ(c.Delay, game.Play.GlobalSpeechAnimationDelay);
        EXPECT_EQ(c.Option, GetGameOption(OPT_GLOBALTALKANIMSPD));
        EXPECT_EQ(c.Loaded, game_def.Options[OPT_GLOBALTALKANIMSPD]);
    }
}

namespace
{

// A value as the field table prints it, with each run of more than three equal items of a list ("[a, b, ...]") written
// once with its count: "[0 x200]". Another value is returned as it is.
std::string CollapseRuns(const std::string &value)
{
    if (value.size() < 2 || value.front() != '[' || value.back() != ']')
        return value;
    // the items, split at the commas of the list's own level
    std::vector<std::string> items;
    int depth = 0;
    bool quoted = false;
    size_t from = 1;
    for (size_t i = 1; i + 1 < value.size(); ++i)
    {
        const char c = value[i];
        if (c == '"')
            quoted = !quoted;
        else if (quoted)
            continue;
        else if (c == '[' || c == '{' || c == '(')
            depth++;
        else if (c == ']' || c == '}' || c == ')')
            depth--;
        else if (c == ',' && depth == 0)
        {
            items.push_back(value.substr(from, i - from));
            from = i + 2; // past ", "
        }
    }
    if (from < value.size() - 1)
        items.push_back(value.substr(from, value.size() - 1 - from));

    std::string text = "[";
    for (size_t i = 0; i < items.size();)
    {
        size_t run = 1;
        while (i + run < items.size() && items[i + run] == items[i])
            run++;
        if (text.size() > 1)
            text += ", ";
        if (run > 3)
        {
            text += items[i] + " x" + std::to_string(run);
        }
        else
        {
            for (size_t k = 0; k < run; ++k)
                text += (k > 0 ? ", " : "") + items[i];
        }
        i += run;
    }
    return text + "]";
}

// The fields of a game state, one line each, as PrintFields prints them ("Name: value"), the lists' runs collapsed
// (CollapseRuns); the opaque fields are left out, as PrintFields leaves them
std::string StateLines(const GameState &play)
{
    std::string text;
    for (const FieldRow<GameState> &row : GameState::Fields())
    {
        if (row.Role == FieldRole::Opaque)
            continue;
        text += std::string(row.Name) + ": " + CollapseRuns(row.Ops->Print(row.Address(play)).GetCStr()) + "\n";
    }
    return text;
}

// The game state that engine_init_game_settings leaves in the fixture's game: what a new game has at its start. A
// change of a member initializer of GameState, or of a value the start gives, changes this text.
constexpr const char kStartedGameState[] = R"(UsedMode: 0
UserInterfaceDisabledCount: 0
GlobalScriptTimer: -1
DebugMode: 1
MessageTimeLeft: -1
UsedInventory: 0
TextReadingSpeed: 15
GlobalSpeechAnimationDelay: 5
InventoryItemWidth: 40
InventoryItemHeight: 22
TextShadowColor: -16777216
SwapPortraitSide: 0
SpeechTextWindowGui: -1
FollowingRoomDelay: 150
SkipDisplay: 3
NoMultiLoopRepeat: 0
InventoryActivated: 0
NoTextBackgroundWhenVoice: 0
MaxDialogOptionWidth: 180
NoHighColorFadeIn: 0
BackgroundSpeechGameSpeed: 0
KeepBackgroundSpeechOnDisplay: 0
UnfactorSpeechFromTextLength: 0
SpeechMusicDrop: 60
RunningCutsceneSkipStyle: 0
FastForward: 0
GameSpeedModifier: 0
PreviousGameData: 0
ReplayHotkeyUnused: -1
DialogOptionsPaddingX: 3
DialogOptionsPaddingY: 0
NarratorSpeech: 1
LipSyncSpeed: 15
SpeechAnimationStopTimeMargin: 10
DisableAntialiasing: 0
TextReadingSpeedModifier: 0
TextAlignment: 1
SpeechBubbleWidth: 100
MinDialogOptionWidth: 0
DisableDialogParser: 0
TopBarBackColor: -11513776
TopBarTextColor: -16777216
TopBarBorderColor: -11513776
TopBarBorderWidth: 1
TopBarY: 25
ScreenshotWidth: 160
ScreenshotHeight: 100
TopBarFont: -1
SpeechTextAlignment: 2
AutoUseWalkToPoints: 1
GreyOutInventory: 0
SpeechSkipKey: 0
AbortKey: 324
FadeColorRed: 0
FadeColorGreen: 0
FadeColorBlue: 0
ShowSingleDialogOption: 0
KeepScreenDuringInstantTransition: 0
ReadDialogOptionColor: -1
DialogRequestResult: 0
IsSpeechCustomPortraitPlacement: 0
SpeechPortraitOffsetX: 0
SpeechPortraitY: 0
SpeechDisplayPostTimeMs: 0
DialogOptionsHighlightColor: -176
RandomSeed: 0
PlayerRegionID: 0
IsScreenFadedOut: 0
CheckInteractionOnly: 0
BgAnimWait: 0
WaitCounter: 0
WaitSkippedBy: 0
WaitSkippedByData: 0
MouseBounds: (0, 0)-(-1, -1)
FadeEffect: 0
AudioMasterVolume: 100
ScreenFlip: 0
SpeechVoiceMode: 1
SpeechSkipStyle: 31
AlwaysWaitForSpeechText: 0
SpeechOverlayZOrder: 2147483647
SpeechVolume: 255
NormalFont: 0
SpeechFont: 1
StandardGuiTextHeight: 0
WaitSkipType: 0
SwapPortraitLastCharacter: -1
SwapPortraitLastLastCharacter: -1
ConversationDepth: 0
ScreenTint: -1
ParsedWords: []
BadParsedWord: ""
RoomChanges: 0
IsMouseInputEnabled: 1
IsMouseCursorShown: 1
ShakeScreenDelay: 0
ShakeScreenAmount: 0
ShakeScreenLength: 0
AmbientTintRed: 0
AmbientTintGreen: 0
AmbientTintBlue: 0
AmbientTintLevel: 0
AmbientTintLight: 0
IsAmbientTintEnabled: 0
SkipUntilCharacterStops: -1
LastNamedLocation: (0, -1)
LocationNameSaveCursor: (0, -1)
RestoreCursorModeTo: -1
RestoreCursorImageTo: -1
CrossfadingOutChannel: 0
CrossfadeStep: 0
CrossfadeOutVolumePerStep: 0
CrossfadeInitialVolumeOut: 0
CrossfadingInChannel: 0
CrossfadeInVolumePerStep: 0
CrossfadeFinalVolumeIn: 0
NewMusicQueueSize: 0
NewMusicQueue: [{audioClipIndex: -1, priority: 0, repeat: 0} x10]
LastParserEntry: [0 x200]
GameName: ""
GroundLevelAreasDisabled: 0
NextScreenTransition: -1
GammaAdjustment: 100
TemporarilyTurnedOffCharacter: -1
GuiDrawOrder: []
DoOnceTokens: []
MinimumTextDisplayTimeMs: 1000
IgnoreUserInputAfterTextTimeoutMs: 500
DefaultAudioTypeVolumes: [-1, -1, -1]
DialogOptionsGuiX: -1
DialogOptionsGuiY: -1
DialogOptionsTextAlignment: 1
DialogOptionsFont: -2
DialogOptionsZOrder: 2147483647
FaceDirectionRatio: 1
GuiProperties: [{}, {}]
GuiControlProperties: [[], [{}, {}] x6]
HasSpeechVoice: 0
IsSpeechVoiceBlocking: 0
IsSpeechInPostState: 0
OverlayCount: 0
CompleteOverlayID: 0
TextOverlayID: 0
SpeechTextScriptHandle: 0
SpeechPortraitScriptHandle: 0
ShakeScreenOffsetY: 0
GameErrorText: ""
RBSwitches: [0, 1 x4]
ScreenShaderID: 0
ScreenShaderHandle: 0
CursorShaderID: 0
CursorShaderHandle: 0
IgnoreUserInputUntilTime: 0
ScriptTimers: {}
)";

// The fields in which the game state after engine_init_game_settings differs from a new GameState (DiffFields, "Name:
// new != started"), in the fixture's game: the values that the game and the configuration decide, which a start from
// the definition still has to give. A value of the game that equals the initializer in this game is not listed
// (GameName, ShowSingleDialogOption, FaceDirectionRatio), nor is one that only the configuration's accessibility
// options change, which the fixture leaves unset (TextReadingSpeed, MinimumTextDisplayTimeMs, SpeechVoiceMode,
// SkipDisplay).
constexpr const char kStartedGameStateFromTheGame[] = R"(DebugMode: 0 != 1
GlobalSpeechAnimationDelay: 0 != 5
TextShadowColor: 0 != -16777216
SpeechTextWindowGui: 0 != -1
NarratorSpeech: 0 != 1
TopBarBackColor: 0 != -11513776
TopBarTextColor: 0 != -16777216
TopBarBorderColor: 0 != -11513776
DialogOptionsHighlightColor: 0 != -176
SpeechSkipStyle: 0 != 31
DefaultAudioTypeVolumes: [] != [-1, -1, -1]
GuiProperties: [] != [{}, {}]
GuiControlProperties: [[], [], [], [], [], [], []] != [[], [{}, {}], [{}, {}], [{}, {}], [{}, {}], [{}, {}], [{}, {}]]
RBSwitches: [0, 0, 0, 0, 0] != [0, 1, 1, 1, 1]
)";

} // namespace

// The whole game state after engine_init_game_settings, field by field, against the text kept above. RandomSeed (the
// clock) and IgnoreUserInputUntilTime (a point in real time) are given fixed values before the print.
TEST_F(GameEntities, StartedGameStateIsAsPinned)
{
    Game &game = Game::Current();
    RunEngineInitGameSettings();
    game.Play.RandomSeed = 0;
    game.Play.IgnoreUserInputUntilTime = {};
    EXPECT_EQ(kStartedGameState, StateLines(game.Play));
}

// A new GameState is the started one but for the values that the game and the configuration decide: the member
// initializers hold the constants of a new game. RandomSeed and IgnoreUserInputUntilTime (the clock) are made equal.
TEST_F(GameEntities, NewGameStateDiffersFromTheStartedOneByTheGame)
{
    Game &game = Game::Current();
    RunEngineInitGameSettings();
    GameState fresh;
    fresh.RandomSeed = game.Play.RandomSeed;
    fresh.IgnoreUserInputUntilTime = game.Play.IgnoreUserInputUntilTime;
    std::string lines;
    for (const String &line : DiffFields(fresh, game.Play))
        lines += std::string(line.GetCStr()) + "\n";
    EXPECT_EQ(kStartedGameStateFromTheGame, lines);
}

// A started game's items, cursors, view frames, dialog options, audio types, options and values are its definition's:
// what a live copy of each must start as
TEST_F(GameEntities, StartedGameTakesTheDefinitions)
{
    ExpectStartedFromTheDefinitions();
    ExpectDefinitionAsMade();
}

// The script functions of a GUI and of each type of control write the live GUI and controls: they give back what was
// set, the other GUI and its controls keep the definitions' values, and the definitions keep theirs
TEST_F(GameEntities, GuiSettersWriteTheLiveGuisAndControls)
{
    SetGuiValues(1, 200);
    ExpectGuiValues(1, 200);
    ExpectGuiTakesTheDefinition(0);
    ExpectGuiLinkedToTheLiveControls(1);
    ExpectDefinitionAsMade();
}

// The saved fields of the GUIs and of their controls come back from a save into the live GUIs and controls of a game
// started again, which stay linked to each other; the definitions keep their values
TEST_F(GameEntities, GuisComeBackFromASave)
{
    SetGuiValues(0, 200);
    SetGuiValues(1, 300);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GUI, saved, error)) << error;

    RestartGame();
    ExpectGuiTakesTheDefinition(1);
    HSaveError err = Restore(saved, kSaveCmp_GUI);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    ExpectGuiValues(0, 200);
    ExpectGuiValues(1, 300);
    for (int g = 0; g < kGuiCount; ++g)
        ExpectGuiLinkedToTheLiveControls(g);
    ExpectDefinitionAsMade();
}

// A started game's GUIs point at the live controls only, also when the definition's GUIs are linked to the
// definition's controls (the GUIs are copied with their pointers, StartFromDefinitions, and linked again after,
// GUI::RebuildGUI); the definition's GUIs keep pointing at the definition's controls
TEST_F(GameEntities, StartedGuisPointAtTheLiveControlsOnly)
{
    Game &game = Game::Current();
    const GameDef &game_def = game.Def();
    GameDef &def = Definition();
    GUIRefCollection def_control_refs(def.GuiButtons, def.GuiInvWindows, def.GuiLabels, def.GuiListBoxes,
                                      def.GuiSliders, def.GuiTextBoxes);
    for (GUIMain &gui : def.Guis)
        ASSERT_TRUE(gui.RebuildArray(def_control_refs));
    pool.Reset();
    StartGame();

    for (int g = 0; g < kGuiCount; ++g)
    {
        ExpectGuiLinkedToTheLiveControls(g);
        const GUIMain &def_gui = game_def.Guis[g];
        for (int i = 0; i < def_gui.GetControlCount(); ++i)
            EXPECT_TRUE(IsControlOf(game_def, def_gui.GetControl(i))) << "GUI " << g << " control " << i;
    }
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
    SetGuiValues(1, 200);

    unload_game();
    EXPECT_TRUE(game.Guis.empty());
    EXPECT_TRUE(game.GuiButtons.empty());
    EXPECT_TRUE(game.GuiLabels.empty());
    EXPECT_TRUE(game.GuiInvWindows.empty());
    EXPECT_TRUE(game.GuiListBoxes.empty());
    EXPECT_TRUE(game.GuiSliders.empty());
    EXPECT_TRUE(game.GuiTextBoxes.empty());
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
    const GameDef &game_def = game.Def();
    StartWithSpriteFile();
    ExpectSpritesStartedWithTheFile();
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game_def.SpriteInfos);

    const int slot = add_dynamic_sprite(std::make_unique<Bitmap>(24, 12, 32));
    ASSERT_GT(slot, 0);
    EXPECT_EQ(24, game.SpriteInfos[slot].Width);
    EXPECT_EQ(12, game.SpriteInfos[slot].Height);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC), game.SpriteInfos[slot].Flags);

    attach_dynsprite_surface(slot, 0);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC | SPF_SURFACEACQUIRED), game.SpriteInfos[slot].Flags);
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game_def.SpriteInfos);
    on_dynsprite_surface_release(slot, false);
    EXPECT_EQ(static_cast<uint32_t>(SPF_DYNAMICALLOC), game.SpriteInfos[slot].Flags);

    free_dynamic_sprite(slot);
    EXPECT_FALSE(game.SpriteInfos[slot].IsValid());
    EXPECT_EQ(0u, game.SpriteInfos[slot].Flags);

    EngineServices::Current().Sprites.DeleteSprite(kKeepDepthSprite);
    EXPECT_FALSE(game.SpriteInfos[kKeepDepthSprite].IsValid());
    EXPECT_EQ(0u, game.SpriteInfos[kKeepDepthSprite].Flags);
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game_def.SpriteInfos);
}

// Game::IsSpriteValid reads the live table: a sprite of the sprite file and a dynamic sprite are valid, a free slot, a
// sprite missing from the sprite file and a number outside the table are not. GameDef::IsSpriteValid (Def()) reads
// the definition's table, which has no sizes.
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
    const GameDef &game_def = game.Def();
    StartWithSpriteFile();
    const int slot = add_dynamic_sprite(std::make_unique<Bitmap>(24, 12, 32));
    ASSERT_GT(slot, 0);
    attach_dynsprite_surface(slot, 0);
    detach_dynsprite_surface(slot); // forgets the surface, keeps the flag
    ASSERT_NE(0u, game.SpriteInfos[slot].Flags & SPF_SURFACEACQUIRED);
    EngineServices::Current().Sprites.DeleteSprite(kKeepDepthSprite);

    unload_game();
    EXPECT_TRUE(game.SpriteInfos.empty());
    EXPECT_TRUE(game_def.SpriteInfos.empty());

    StartWithSpriteFile();
    ExpectSpritesStartedWithTheFile();
    ExpectSameSpriteInfos(LoadedSpriteInfos(), game_def.SpriteInfos);
}

// A custom property of an element that nothing set has the definition's value, and the schema's default where the
// definition has none: element 1 of every kind has the definition's values, the others the defaults
TEST_F(GameEntities, PropertiesAreTheDefinitionsOrTheSchemasDefault)
{
    for (const PropertyKind &kind : PropertyKinds())
    {
        for (int i = 0; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);
    }
}

// A value set during play hides the definition's value and the schema's default, for a number and a text property;
// it is the element's own: the other elements of the kind keep theirs, and the definition's values stay as loaded. A
// property's name is not case-sensitive.
TEST_F(GameEntities, PropertySetDuringPlayIsTheElementsOwn)
{
    const GameDef &game_def = Game::Current().Def();
    GameDef made;
    MakeDefinition(made);
    for (const PropertyKind &kind : PropertyKinds())
    {
        SCOPED_TRACE(kind.Name);
        EXPECT_TRUE(kind.SetNumber(1, kNumberProperty, 12));
        EXPECT_TRUE(kind.SetText(1, kTextProperty, "blue"));
        EXPECT_EQ(12, kind.GetNumber(1, kNumberProperty));
        EXPECT_EQ("blue", kind.GetText(1, kTextProperty));
        ExpectDefaults(kind, 0);

        EXPECT_TRUE(kind.SetNumber(0, kNumberProperty, 3));
        EXPECT_EQ(3, kind.GetNumber(0, kNumberProperty));
        EXPECT_EQ(kTextDefault, kind.GetText(0, kTextProperty));
        EXPECT_EQ(12, kind.GetNumber(1, kNumberProperty));
        for (int i = 2; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);

        EXPECT_TRUE(kind.SetNumber(1, "WEIGHT", 13));
        EXPECT_EQ(13, kind.GetNumber(1, "weight"));
        EXPECT_EQ("blue", kind.GetText(1, "COLOUR"));

        for (int i = 0; i < kind.Count; ++i)
            EXPECT_EQ(kind.Definition(made, i), kind.Definition(game_def, i)) << "element " << i;
    }
}

// The older functions that copy a character's or an inventory item's text property into a buffer make the same lookup
TEST_F(GameEntities, PropertyTextIntoABufferIsTheSameLookup)
{
    Game &game = Game::Current();
    char buffer[MAX_MAXSTRLEN];
    Character_GetPropertyText(&game.Characters[0], kTextProperty, buffer);
    EXPECT_STREQ(kTextDefault, buffer);
    Character_GetPropertyText(&game.Characters[1], kTextProperty, buffer);
    EXPECT_STREQ(kTextOfElement1, buffer);
    Character_SetTextProperty(&game.Characters[1], kTextProperty, "blue");
    Character_GetPropertyText(&game.Characters[1], kTextProperty, buffer);
    EXPECT_STREQ("blue", buffer);

    ScriptInvItem item0 = ItemObject(0);
    ScriptInvItem item1 = ItemObject(1);
    InventoryItem_GetPropertyText(&item0, kTextProperty, buffer);
    EXPECT_STREQ(kTextDefault, buffer);
    InventoryItem_GetPropertyText(&item1, kTextProperty, buffer);
    EXPECT_STREQ(kTextOfElement1, buffer);
    InventoryItem_SetTextProperty(&item1, kTextProperty, "blue");
    InventoryItem_GetPropertyText(&item1, kTextProperty, buffer);
    EXPECT_STREQ("blue", buffer);
}

// The values set during play come back from a save into the running game, for every kind that a save holds: the
// values set after the save are replaced by the saved ones
TEST_F(GameEntities, PropertiesSetDuringPlayComeBackFromASave)
{
    const std::vector<PropertyKind> kinds = SavedPropertyKinds();
    for (const PropertyKind &kind : kinds)
    {
        kind.SetNumber(0, kNumberProperty, 20);
        kind.SetNumber(1, kNumberProperty, 21);
        kind.SetText(1, kTextProperty, "saved");
    }
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kPropertyComponents, saved, error)) << error;
    for (const PropertyKind &kind : kinds)
    {
        kind.SetNumber(0, kNumberProperty, 30);
        kind.SetNumber(1, kNumberProperty, 31);
        kind.SetText(1, kTextProperty, "changed");
    }

    HSaveError err = RestoreIntoTheRunningGame(saved, kPropertyComponents);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (const PropertyKind &kind : kinds)
    {
        SCOPED_TRACE(kind.Name);
        EXPECT_EQ(20, kind.GetNumber(0, kNumberProperty));
        EXPECT_EQ(kTextDefault, kind.GetText(0, kTextProperty));
        EXPECT_EQ(21, kind.GetNumber(1, kNumberProperty));
        EXPECT_EQ("saved", kind.GetText(1, kTextProperty));
        for (int i = 2; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);
    }
    ExpectDefinitionAsMade();
}

// A restore into the running game takes away a value set after the save where the save held none for that property:
// the element has the definition's value or the schema's default again (DoBeforeRestore clears the values, then the
// components read the saved values into the maps). For every kind but the GUI controls (the test below); the audio
// clips' values, which no save holds, are cleared too.
TEST_F(GameEntities, RestoreTakesAwayPropertiesSetAfterTheSave)
{
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kPropertyComponents, saved, error)) << error;
    std::vector<PropertyKind> kinds = PropertyKinds();
    kinds.erase(std::remove_if(kinds.begin(), kinds.end(), IsGuiControlKind), kinds.end());
    for (const PropertyKind &kind : kinds)
    {
        for (int i = 0; i < kind.Count; ++i)
        {
            kind.SetNumber(i, kNumberProperty, 40 + i);
            kind.SetText(i, kTextProperty, "after");
        }
    }

    HSaveError err = RestoreIntoTheRunningGame(saved, kPropertyComponents);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (const PropertyKind &kind : kinds)
    {
        for (int i = 0; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);
    }
}

// The same for the six types of GUI control. A value set after the save stayed after the restore until
// GameState::FreeProperties (called by DoBeforeRestore) cleared the GUI controls' lists (GuiControlProperties) too: the
// "GUI" component only adds the saved values to a map (Properties::ReadValues does not clear it). Upstream's
// FreeProperties still leaves them.
TEST_F(GameEntities, RestoreTakesAwayGuiControlPropertiesSetAfterTheSave)
{
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kPropertyComponents, saved, error)) << error;
    std::vector<PropertyKind> kinds = PropertyKinds();
    kinds.erase(
        std::remove_if(kinds.begin(), kinds.end(), [](const PropertyKind &kind) { return !IsGuiControlKind(kind); }),
        kinds.end());
    ASSERT_EQ(std::size(kControlTypes), kinds.size());
    for (const PropertyKind &kind : kinds)
    {
        for (int i = 0; i < kind.Count; ++i)
        {
            kind.SetNumber(i, kNumberProperty, 40 + i);
            kind.SetText(i, kTextProperty, "after");
        }
    }

    HSaveError err = RestoreIntoTheRunningGame(saved, kPropertyComponents);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (const PropertyKind &kind : kinds)
    {
        for (int i = 0; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);
    }
}

// No save holds the audio clips' values set during play (on purpose, game_state.h says): they change no byte of the
// components that hold the other kinds' values, the audio and the game state. A restore clears them, whichever
// components it restores (DoBeforeRestore): the clips have the definition's values or the schema's defaults again.
TEST_F(GameEntities, AudioClipPropertiesAreNotSavedAndARestoreClearsThem)
{
    const SaveCmpSelection cmp =
        static_cast<SaveCmpSelection>(kPropertyComponents | kSaveCmp_Audio | kSaveCmp_GameState);
    std::vector<uint8_t> without, with;
    std::string error;
    ASSERT_TRUE(Save(cmp, without, error)) << error;
    for (int i = 0; i < kAudioClipCount; ++i)
    {
        AudioClip_SetProperty(&Game::Current().AudioClips[i], kNumberProperty, 50 + i);
        AudioClip_SetTextProperty(&Game::Current().AudioClips[i], kTextProperty, "saved");
    }
    ASSERT_TRUE(Save(cmp, with, error)) << error;
    EXPECT_TRUE(without == with);

    HSaveError err = RestoreIntoTheRunningGame(with, kSaveCmp_Characters);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    const std::vector<PropertyKind> kinds = PropertyKinds();
    const auto clips =
        std::find_if(kinds.begin(), kinds.end(), [](const PropertyKind &kind) { return kind.Name == "audio clip"; });
    ASSERT_NE(kinds.end(), clips);
    for (int i = 0; i < kAudioClipCount; ++i)
        ExpectDefinitionsValues(*clips, i);
}

// A game that has a value set during play on two elements of every kind that a save holds, saved, restored into the
// running game and saved again gives the same bytes
TEST_F(GameEntities, PropertiesSavedRestoredAndSavedAgainGiveTheSameBytes)
{
    // one value per element: a map of several values is written in the hash map's iteration order
    // (Properties::WriteValues), which a restored map need not have again, so only the set of entries is the same
    for (const PropertyKind &kind : SavedPropertyKinds())
    {
        kind.SetNumber(0, kNumberProperty, 60);
        kind.SetText(1, kTextProperty, "saved");
    }
    std::vector<uint8_t> first, second;
    std::string error;
    ASSERT_TRUE(Save(kPropertyComponents, first, error)) << error;
    HSaveError err = RestoreIntoTheRunningGame(first, kPropertyComponents);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    ASSERT_TRUE(Save(kPropertyComponents, second, error)) << error;
    EXPECT_EQ(first.size(), second.size());
    EXPECT_TRUE(first == second);
}

// In its save component, the values set during play on an element are one property map (Properties::WriteValues) in
// place of the empty map that the element has without them; nothing else in the component changes. For every kind
// that a save holds.
TEST_F(GameEntities, PropertiesSetDuringPlayAreAPropertyMapInTheirComponent)
{
    const std::vector<uint8_t> empty_map = PropertyMapBytes(StringIMap());
    StringIMap values;
    values[kNumberProperty] = "21";
    values[kTextProperty] = "saved";
    const std::vector<uint8_t> map = PropertyMapBytes(values);
    // the kinds one after the other: a kind's save with and without its values also holds the values of the kinds
    // before it, the same in both
    for (const PropertyKind &kind : SavedPropertyKinds())
    {
        SCOPED_TRACE(kind.Name);
        std::vector<uint8_t> without, with;
        std::string error;
        ASSERT_TRUE(Save(kind.Component, without, error)) << error;
        kind.SetNumber(1, kNumberProperty, 21);
        kind.SetText(1, kTextProperty, "saved");
        ASSERT_TRUE(Save(kind.Component, with, error)) << error;

        const std::vector<uint8_t> data_without = ComponentData(without, kind.ComponentName);
        const std::vector<uint8_t> data_with = ComponentData(with, kind.ComponentName);
        ASSERT_FALSE(data_without.empty());
        ASSERT_EQ(data_without.size() + map.size() - empty_map.size(), data_with.size());
        const auto differ = std::mismatch(data_without.begin(), data_without.end(), data_with.begin());
        ASSERT_NE(data_without.end(), differ.first);
        // the maps differ first in their count, after the format version (an int32)
        const size_t at = static_cast<size_t>(differ.first - data_without.begin()) - sizeof(int32_t);
        std::vector<uint8_t> expected(data_without.begin(), data_without.begin() + at);
        EXPECT_TRUE(std::equal(empty_map.begin(), empty_map.end(), data_without.begin() + at));
        expected.insert(expected.end(), map.begin(), map.end());
        expected.insert(expected.end(), data_without.begin() + at + empty_map.size(), data_without.end());
        EXPECT_TRUE(expected == data_with);
    }
}

// unload_game keeps nothing of the values set during play: after it, the game started again from its definition has
// the definition's values and the schema's defaults, for every kind
TEST_F(GameEntities, UnloadedGameKeepsNoPropertyValues)
{
    for (const PropertyKind &kind : PropertyKinds())
    {
        for (int i = 0; i < kind.Count; ++i)
        {
            kind.SetNumber(i, kNumberProperty, 70 + i);
            kind.SetText(i, kTextProperty, "first game");
        }
    }

    unload_game();
    MakeDefinition(Definition());
    StartGame();
    for (const PropertyKind &kind : PropertyKinds())
    {
        for (int i = 0; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);
    }
}

// The characters, inventory items, dialogs and audio clips hold their values set during play (PropertyOverrides): the
// live copies that StartFromDefinitions makes from the definitions have none, whatever the elements they replace had
// (RestartGame, which starts the game without unload_game)
TEST_F(GameEntities, LiveCopiesStartWithoutPropertyValues)
{
    Game &game = Game::Current();
    std::vector<PropertyKind> kinds = PropertyKinds();
    kinds.erase(std::remove_if(kinds.begin(), kinds.end(),
                               [](const PropertyKind &kind) { return kind.Name == "GUI" || IsGuiControlKind(kind); }),
                kinds.end());
    ASSERT_EQ(4u, kinds.size());
    for (const PropertyKind &kind : kinds)
    {
        for (int i = 0; i < kind.Count; ++i)
            kind.SetNumber(i, kNumberProperty, 80 + i);
    }

    RestartGame();
    for (const Character &ch : game.Characters)
        EXPECT_TRUE(ch.PropertyOverrides.empty());
    for (const InventoryItem &item : game.InventoryItems)
        EXPECT_TRUE(item.PropertyOverrides.empty());
    for (const Dialog &dialog : game.Dialogs)
        EXPECT_TRUE(dialog.PropertyOverrides.empty());
    for (const AudioClip &clip : game.AudioClips)
        EXPECT_TRUE(clip.PropertyOverrides.empty());
    for (const PropertyKind &kind : kinds)
    {
        for (int i = 0; i < kind.Count; ++i)
            ExpectDefinitionsValues(kind, i);
    }
}

// The script's game.no_multiloop_repeat, game.disable_antialiasing and game.disable_dialog_parser read back what the
// script wrote: 0, 1 and any other number (an int behind each today; the engine tests each for 0 only, so a number
// other than 0 acts as 1)
TEST_F(GameEntities, ScriptFlagsReadBackWhatTheScriptWrote)
{
    Game &game = Game::Current();
    for (const int offset : {kNoMultiLoopRepeatOffset, kDisableAntialiasingOffset, kDisableDialogParserOffset})
    {
        for (const int value : {0, 1, 5, -1, 0})
        {
            GameStaticManager.WriteInt32(&game.Play, offset, value);
            EXPECT_EQ(value, GameStaticManager.ReadInt32(&game.Play, offset)) << "offset " << offset;
        }
    }
}

// game.disable_antialiasing: 0 lets the sprites be antialiased where the player's setup asks for it, any other number
// does not (GameState::ShouldAASprites, which the drawing code asks)
TEST_F(GameEntities, DisableAntialiasingTurnsSpriteAntialiasingOff)
{
    Game &game = Game::Current();
    EngineServices &services = EngineServices::Current();
    for (const bool setup : {true, false})
    {
        services.Config.AntialiasSprites = setup;
        for (const int value : {0, 1, 5, -1})
        {
            GameStaticManager.WriteInt32(&game.Play, kDisableAntialiasingOffset, value);
            EXPECT_EQ(setup && value == 0, game.Play.ShouldAASprites()) << "setup " << setup << ", value " << value;
        }
    }
}

// game.disable_dialog_parser: a number other than 0 hides the parser of a dialog that shows one, so that a dialog with
// a single option on chooses it at once without showing the options. (With 0 the options and the parser would be
// shown, a blocking loop: not reached here.) A dialog without a parser chooses its single option at once whatever the
// value.
TEST_F(GameEntities, DisableDialogParserLetsASingleOptionBeChosenAtOnce)
{
    Game &game = Game::Current();
    ScriptDialog dialog0 = DialogObject(0);
    ScriptDialog dialog1 = DialogObject(1);
    // the dialogs' third options off: the first is the only one on
    Dialog_SetOptionState(&dialog0, 3, 0);
    Dialog_SetOptionState(&dialog1, 3, 0);
    game.Dialogs[0].Flags |= DTFLG_SHOWPARSER; // as the game file gives it
    ASSERT_EQ(0, game.Play.ShowSingleDialogOption);
    for (const int value : {1, 5, -1})
    {
        GameStaticManager.WriteInt32(&game.Play, kDisableDialogParserOffset, value);
        EXPECT_EQ(value == 0, game.Play.IsDialogParserEnabled()) << "value " << value;
        EXPECT_EQ(0, show_dialog_options(0, false)) << "value " << value;
    }
    for (const int value : {0, 1})
    {
        GameStaticManager.WriteInt32(&game.Play, kDisableDialogParserOffset, value);
        EXPECT_EQ(0, show_dialog_options(1, false)) << "value " << value;
    }
}

// game.no_multiloop_repeat: with 0 a repeated animation of a loop that the loop before runs into starts again from the
// first loop of the sequence; any other number starts it again from its own loop (CycleViewAnim, which animates the
// characters, objects, overlays and buttons)
TEST_F(GameEntities, NoMultiLoopRepeatKeepsTheRepeatInItsLoop)
{
    Game &game = Game::Current();
    ASSERT_TRUE(game.Views[kFramedView].Loops[0].RunNextLoop());
    for (const int value : {0, 1, 5, -1})
    {
        SCOPED_TRACE(testing::Message() << "value " << value);
        GameStaticManager.WriteInt32(&game.Play, kNoMultiLoopRepeatOffset, value);
        EXPECT_EQ(value == 0, game.Play.ShouldRepeatMultiLoop());
        uint16_t loop = 1;
        uint16_t frame = kFrameCount - 1;
        AnimFlowParams anim(kAnimFlow_Repeat, kAnimDirForward);
        EXPECT_TRUE(CycleViewAnim(game, kFramedView, loop, frame, anim));
        EXPECT_EQ(value == 0 ? 0 : 1, loop);
        EXPECT_EQ(0, frame);
    }
}

// The kinds of input that may skip the current wait are the running game's (GameState::GetWaitSkipType), which
// Game.BlockingWaitSkipType gives as the script's InputType flags
TEST_F(GameEntities, WaitSkipTypeIsTheRunningGames)
{
    Game &game = Game::Current();
    game.Play.WaitSkipType = SKIP_KEYPRESS | SKIP_MOUSECLICK;
    EXPECT_EQ(SKIP_KEYPRESS | SKIP_MOUSECLICK, game.Play.GetWaitSkipType());
    EXPECT_EQ((SKIP_KEYPRESS | SKIP_MOUSECLICK) << SKIP_RESULT_TYPE_SHIFT, Game_BlockingWaitSkipType());
    game.Play.WaitSkipType = SKIP_NONE;
    EXPECT_EQ(SKIP_NONE, game.Play.GetWaitSkipType());
}

// Voice speech is played when the voice files are there and the game is not skipping a cutscene, and, for a blocking
// speech, the speech mode is not text only (GameState::ShouldPlayVoiceSpeech, ShouldPlayVoiceSpeechNonBlocking)
TEST_F(GameEntities, VoiceSpeechPlaysWithTheVoiceFilesAndNoSkipping)
{
    Game &game = Game::Current();
    EngineServices &services = EngineServices::Current();
    struct Case
    {
        bool VoiceAvail;
        int FastForward;
        SpeechMode Mode;
        bool Blocking;
        bool NonBlocking;
    };
    for (const Case &c : {Case{true, 0, kSpeech_VoiceText, true, true}, Case{true, 0, kSpeech_VoiceOnly, true, true},
                          Case{true, 0, kSpeech_TextOnly, false, true}, Case{true, 1, kSpeech_VoiceText, false, false},
                          Case{false, 0, kSpeech_VoiceText, false, false}})
    {
        SCOPED_TRACE(testing::Message() << "voice " << c.VoiceAvail << ", fast forward " << c.FastForward << ", mode "
                                        << c.Mode);
        services.VoiceAvail = c.VoiceAvail;
        game.Play.FastForward = c.FastForward;
        game.Play.SpeechVoiceMode = c.Mode;
        EXPECT_EQ(c.Blocking, game.Play.ShouldPlayVoiceSpeech());
        EXPECT_EQ(c.NonBlocking, game.Play.ShouldPlayVoiceSpeechNonBlocking());
    }
}
