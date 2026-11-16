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
#include "game/game_init.h"

#include <array>
#include <cassert>
#include <vector>

#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/out.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "game/audio/audio.h"
#include "game/dialog/dialog.h"
#include "game/dialog/display.h"
#include "game/dialog/lipsync.h"
#include "game/dialog/parser.h"
#include "game/dialog/speech.h"
#include "game/dynobj/all_dynamic_classes.h"
#include "game/dynobj/all_script_classes.h"
#include "game/file.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "game/script.h"
#include "game/script_api/exports.h"
#include "game/translation.h"
#include "game/ui/gui.h"
#include "game/view.h"
#include "game/world/character.h"
#include "game/world/hotspot.h"
#include "game/world/inventory_item.h"
#include "game/world/move_list.h"
#include "game/world/object.h"
#include "game/world/region.h"
#include "game/world/walk_behind.h"
#include "game/world/walkable_area.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/sprite_cache.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/dynobj/cc_ags_dynamic_object.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/script_runtime.h"
#include "util/memory_compat.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

CCStaticArray StaticHandlesArray;
// Managed handles of the audio clips' script objects, by clip index
std::vector<int> StaticAudioClipArray;


namespace AGS
{
namespace Engine
{

String GetGameInitErrorText(GameInitErrorType err)
{
    switch (err)
    {
    case kGameInitErr_NoError: return "No error.";
    case kGameInitErr_UnknownDataVersion: return "Unknown game data version";
    case kGameInitErr_NoFonts: return "No fonts specified to be used in this game.";
    case kGameInitErr_EntityInitFail: return "Failed to initialize game entities.";
    case kGameInitErr_PluginNameInvalid: return "Plugin name is invalid.";
    case kGameInitErr_ScriptInitFailed: return "Failed to initialize runtime scripts.";
    }
    return "Unknown error.";
}

// Initializes audio channels and clips and registers them in the script system
void InitAndRegisterAudioObjects(Game &game)
{
    for (int i = 0; i < game.numCompatGameChannels; ++i)
    {
        game.ScAudioChannels[i].id = i;
        ccRegisterPersistentObject(&game.ScAudioChannels[i], &ccDynamicAudio); // add internal ref
    }

    StaticAudioClipArray.resize(game.audioClips.size());
    for (size_t i = 0; i < game.audioClips.size(); ++i)
    {
        // A clip's ID is its index in the array
        game.audioClips[i].id = i;
        int handle = ccRegisterPersistentObject(&game.audioClips[i], &ccDynamicAudioClip); // add internal ref
        StaticAudioClipArray[i] = handle;
        if (!game.audioClips[i].scriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(game.audioClips[i].scriptName, &StaticAudioClipArray[i]);
    }
}

// Initializes characters and registers them in the script system
void InitAndRegisterCharacters(Game &game)
{
    // ensure at least 1 element, we must register buffer
    game.Handles.Characters.resize(std::max(1, game.numcharacters));

    for (int i = 0; i < game.numcharacters; ++i)
    {
        // TODO: move this to Character's "init" method
        game.Characters[i].MoveListID = MoveState::kNoMoveList;
        game.Characters[i].BlinkInterval = 140;
        game.Characters[i].BlinkTimer = game.Characters[i].BlinkInterval;
        game.Characters[i].ID = i;
        game.Characters[i].PreviousRoom = -1;
        game.Characters[i].Loop = 0;
        game.Characters[i].Frame = 0;
        game.Characters[i].WalkWait = -1;
        // register and save handle
        game.Handles.Characters[i] = ccRegisterPersistentObject(&game.Characters[i], &ccDynamicCharacter);

        // export the character's script object
        if (!game.Characters[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(game.Characters[i].ScriptName, &game.Handles.Characters[i]);
    }

    // the blend mode, graphic anchor and offset came with the definitions; the effective values follow from them
    for (int i = 0; i < game.numcharacters; ++i)
    {
        // TODO: move this to Character's "init" method
        game.Characters[i].UpdateEffectiveValues();
    }
}

// Initializes dialogs and registers them in the script system
void InitAndRegisterDialogs(Game &game)
{
    game.ScDialogs.resize(game.numdialog);
    // ensure at least 1 element, we must register buffer
    game.Handles.Dialogs.resize(std::max(1, game.numdialog));

    for (int i = 0; i < game.numdialog; ++i)
    {
        game.ScDialogs[i].id = i;
        // register and save handle
        game.Handles.Dialogs[i] = ccRegisterPersistentObject(&game.ScDialogs[i], &ccDynamicDialog);

        if (!game.Dialogs[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(game.Dialogs[i].ScriptName, &game.Handles.Dialogs[i]);
    }
}

// Registers all the controls of all GUIs in the script system, and exports the named ones;
// their handles are stored in Game::Handles.GuiControls
void RegisterAllGUIControls(Game &game)
{
    game.Handles.GuiControls.resize(game.numgui);
    for (int gui_id = 0; gui_id < game.numgui; ++gui_id)
    {
        auto const &gui = game.Guis[gui_id];
        game.Handles.GuiControls[gui_id] = std::vector<int>();
        game.Handles.GuiControls[gui_id].resize(gui.GetControlCount());
    }

    for (int gui_id = 0; gui_id < game.numgui; ++gui_id)
    {
        auto const &gui = game.Guis[gui_id];
        for (int ctrl_id = 0; ctrl_id < gui.GetControlCount(); ctrl_id++)
        {
            GUIControl *control = gui.GetControl(ctrl_id);
            IScriptObject *manager;
            switch (gui.GetControlType(ctrl_id))
            {
            case kGUIButton: manager = &ccDynamicGUIButton; break;
            case kGUILabel: manager = &ccDynamicGUILabel; break;
            case kGUIInvWindow: manager = &ccDynamicGUIInvWindow; break;
            case kGUISlider: manager = &ccDynamicGUISlider; break;
            case kGUITextBox: manager = &ccDynamicGUITextBox; break;
            case kGUIListBox: manager = &ccDynamicGUIListBox; break;
            default: manager = nullptr; break;
            }

            if (!manager)
                continue;
            game.Handles.GuiControls[gui_id][ctrl_id] =
                ccRegisterPersistentObject(control, manager); // add ref for engine
            if (!control->GetName().IsEmpty())
            {
                ccAddExternalScriptObjectHandle(control->GetName(), &game.Handles.GuiControls[gui_id][ctrl_id]);
            }
        }
    }
}

// Initializes GUIs (including their controls) and registers them in the script system
HError InitAndRegisterGUI(Game &game)
{
    game.ScGuis.resize(game.numgui);
    // ensure at least 1 element, we must register buffer
    game.Handles.Guis.resize(std::max(1, game.numgui));

    for (int gui_id = 0; gui_id < game.numgui; ++gui_id)
    {
        game.ScGuis[gui_id].id = -1;
    }

    GUIRefCollection gui_control_refs(game.GuiButtons, game.GuiInvWindows, game.GuiLabels, game.GuiListBoxes,
                                      game.GuiSliders, game.GuiTextBoxes);
    for (int gui_id = 0; gui_id < game.numgui; ++gui_id)
    {
        // link controls to their parent guis
        HError err = game.Guis[gui_id].RebuildArray(gui_control_refs);
        if (!err)
            return err;
        game.ScGuis[gui_id].id = gui_id;
        // register and save handle
        IScriptObject *manager = game.Guis[gui_id].IsTextWindow() ? &ccDynamicTextWindowGUI : &ccDynamicGUI;
        game.Handles.Guis[gui_id] = ccRegisterPersistentObject(&game.ScGuis[gui_id], manager);

        // export the gui script object
        if (!game.Guis[gui_id].GetName().IsEmpty())
            ccAddExternalScriptObjectHandle(game.Guis[gui_id].GetName(), &game.Handles.Guis[gui_id]);
    }
    // register and export all the GUI's controls at once
    RegisterAllGUIControls(game);

    return HError::None();
}

// Initializes inventory items and registers them in the script system
void InitAndRegisterInvItems(Game &game)
{
    game.ScInvItems.resize(game.numinvitems);
    // ensure at least 1 element, we must register buffer
    game.Handles.InvItems.resize(std::max<uint16_t>(1, game.numinvitems));

    for (int i = 0; i < game.numinvitems; ++i)
    {
        game.ScInvItems[i].id = i;
        // register and save handle
        game.Handles.InvItems[i] = ccRegisterPersistentObject(&game.ScInvItems[i], &ccDynamicInv);

        if (!game.invScriptNames[i].IsEmpty())
            ccAddExternalScriptObjectHandle(game.invScriptNames[i], &game.Handles.InvItems[i]);
    }
}

// Register any GUI controls that are not present in the managed pool.
// We assume that handle arrays are already created of proper size when the game was loaded,
// so we only need to fill missing elements.
static void RegisterMissingGUIControls(Game &game)
{
    for (int gui_id = 0; gui_id < game.numgui; ++gui_id)
    {
        auto const &gui = game.Guis[gui_id];
        for (int ctrl_id = 0; ctrl_id < gui.GetControlCount(); ctrl_id++)
        {
            GUIControl *control = gui.GetControl(ctrl_id);
            IScriptObject *manager;
            switch (gui.GetControlType(ctrl_id))
            {
            case kGUIButton: manager = &ccDynamicGUIButton; break;
            case kGUILabel: manager = &ccDynamicGUILabel; break;
            case kGUIInvWindow: manager = &ccDynamicGUIInvWindow; break;
            case kGUISlider: manager = &ccDynamicGUISlider; break;
            case kGUITextBox: manager = &ccDynamicGUITextBox; break;
            case kGUIListBox: manager = &ccDynamicGUIListBox; break;
            default: manager = nullptr; break;
            }

            if (manager)
            {
                game.Handles.GuiControls[gui_id][ctrl_id] =
                    ccRegisterPersistentObjectIfMissing(control, manager); // add ref for engine
            }
        }
    }
}

// Registers any persistent script objects that are missing from the managed pool,
// e.g. if restoring a save made before they were added to the game.
// We assume that handle arrays are already created of proper size when the game was loaded,
// so we only need to fill missing elements.
void RegisterMissingManagedObjects(Game &game)
{
    for (int i = 0; i < game.numCompatGameChannels; ++i)
    {
        ccRegisterPersistentObjectIfMissing(&game.ScAudioChannels[i], &ccDynamicAudio);
    }
    for (size_t i = 0; i < game.audioClips.size(); ++i)
    {
        StaticAudioClipArray[i] = ccRegisterPersistentObjectIfMissing(&game.audioClips[i], &ccDynamicAudioClip);
    }
    for (int i = 0; i < game.numcharacters; ++i)
    {
        game.Handles.Characters[i] = ccRegisterPersistentObjectIfMissing(&game.Characters[i], &ccDynamicCharacter);
    }
    for (int i = 0; i < game.numdialog; ++i)
    {
        game.Handles.Dialogs[i] = ccRegisterPersistentObjectIfMissing(&game.ScDialogs[i], &ccDynamicDialog);
    }
    for (int i = 0; i < game.numinvitems; ++i)
    {
        game.Handles.InvItems[i] = ccRegisterPersistentObjectIfMissing(&game.ScInvItems[i], &ccDynamicInv);
    }
    for (int i = 0; i < game.numgui; ++i)
    {
        game.Handles.Guis[i] = ccRegisterPersistentObjectIfMissing(&game.ScGuis[i], &ccDynamicGUI);
    }
    RegisterMissingGUIControls(game);
}

// Registers an array of 'count' room script entities of a given type (hotspots, objects and so on),
// and stores their handles in 'static_arr'
template <typename TScriptType>
void RegisterRoomEntities(std::vector<int> &static_arr, TScriptType scr_objs[], size_t count, AGSCCDynamicObject &mgr)
{
    static_arr.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        scr_objs[i].id = i;
        // register and save handle
        static_arr[i] = ccRegisterPersistentObject(&scr_objs[i], &mgr);
    }
}

// Initializes room entities and registers them in the script system
void InitAndRegisterRoomEntities()
{
    Game &game = Game::Current();
    RegisterRoomEntities(game.Handles.Hotspots, game.ScHotspots, MAX_ROOM_HOTSPOTS, ccDynamicHotspot);
    RegisterRoomEntities(game.Handles.Objects, game.ScObjects, MAX_ROOM_OBJECTS, ccDynamicObject);
    RegisterRoomEntities(game.Handles.Regions, game.ScRegions, MAX_ROOM_REGIONS, ccDynamicRegion);
    RegisterRoomEntities(game.Handles.WalkableAreas, game.ScWalkableAreas, MAX_WALK_AREAS, ccDynamicWalkarea);
    RegisterRoomEntities(game.Handles.WalkBehinds, game.ScWalkBehinds, MAX_WALK_BEHINDS, ccDynamicWalkbehind);
}

// Registers static entity arrays in the script system
void RegisterStaticArrays(Game &game)
{
    StaticHandlesArray.Create(sizeof(int32_t));

    // Game element arrays
    ccAddExternalScriptObject("character", game.Handles.Characters.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("dialog", game.Handles.Dialogs.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("gui", game.Handles.Guis.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("inventory", game.Handles.InvItems.data(), &StaticHandlesArray);
    // Room element arrays
    ccAddExternalScriptObject("hotspot", game.Handles.Hotspots.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("object", game.Handles.Objects.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("region", game.Handles.Regions.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("walkarea", game.Handles.WalkableAreas.data(), &StaticHandlesArray);
    ccAddExternalScriptObject("walkbehind", game.Handles.WalkBehinds.data(), &StaticHandlesArray);
}

// Initializes various game entities and registers them in the script system
HError InitAndRegisterGameEntities(Game &game, const LoadedGameEntities &ents)
{
    InitAndRegisterAudioObjects(game);
    InitAndRegisterCharacters(game);
    InitAndRegisterDialogs(game);
    HError err = InitAndRegisterGUI(game);
    if (!err)
        return err;
    InitAndRegisterInvItems(game);

    InitAndRegisterRoomEntities();

    RegisterStaticArrays(game);

    setup_player_character(game.playercharacter);
    ccAddExternalScriptObjectHandle("player", &game.ScPlayerCharPtr);
    return HError::None();
}

// Loads all the game fonts, then lets the font module fix them up
void LoadFonts(GameDef &game, GameDataVersion data_ver)
{
    set_gamedata_version(data_ver);

    // Supply eligible palette entries for 8-bit WFN grayscale rendering.
    std::array<bool, 256> wfn_palette_eligible = {};

    for (int i = 0; i < 256; ++i)
        wfn_palette_eligible[i] = (game.paluses[i] == PAL_GAMEWIDE);

    set_wfn_palette_eligibility(wfn_palette_eligible);

    for (int i = 0; i < game.numfonts; ++i)
    {
        load_game_font(i, game.fonts[i], data_ver);
    }

    // Additional fixups - after all the fonts are registered
    fixup_game_fonts();
}

// TODO: move lipsync dat reading (and writing?) out to a dedicated module.
enum LipsyncDatVersion
{
    // Version 4 had filename as a fixed-size 14-char array (including terminator)
    kLipSyncVersion_321 = 4, // TODO: find out exact version when this format was introduced
    // Version 3060200 has filename as a unlimited null-terminated string
    kLipSyncVersion_362 = 3060200
};

// Reads the speech lip sync data ("syncdata.dat") of the voice pack; leaves lip sync disabled if there is none,
// or if it has an unknown format
void LoadLipsyncData()
{
    Game &game = Game::Current();
    game.LipSync.Lines.clear();
    game.LipSync.NumLines = 0;
    game.LipSync.CurLine = -1;

    // a named voice pack has its files, the sync data included, under the pack's name
    auto speech_sync =
        EngineServices::Current().Assets->OpenAsset(Path::ConcatPaths(get_voice_assetpath(), "syncdata.dat"), "voice");
    if (!speech_sync)
        return;
    // the voice pack has lip sync data
    int lipsync_format = speech_sync->ReadInt32();
    if (lipsync_format < kLipSyncVersion_321 || lipsync_format > kLipSyncVersion_362)
    {
        Debug::Printf(kDbgMsg_Warn, "Unknown speech lip sync format (%d).\nLip sync disabled.", lipsync_format);
        return;
    }

    game.LipSync.NumLines = speech_sync->ReadInt32();
    game.LipSync.Lines.resize(game.LipSync.NumLines);
    for (int line_id = 0; line_id < game.LipSync.NumLines; line_id++)
    {
        game.LipSync.Lines[line_id].numPhonemes = speech_sync->ReadInt16();
        if (lipsync_format < kLipSyncVersion_362)
            game.LipSync.Lines[line_id].filename.ReadCount(speech_sync.get(), 14);
        else
            game.LipSync.Lines[line_id].filename.Read(speech_sync.get());
        game.LipSync.Lines[line_id].endtimeoffs.resize(game.LipSync.Lines[line_id].numPhonemes);
        speech_sync->ReadArrayOfInt32(game.LipSync.Lines[line_id].endtimeoffs.data(),
                                      game.LipSync.Lines[line_id].numPhonemes);
        game.LipSync.Lines[line_id].frame.resize(game.LipSync.Lines[line_id].numPhonemes);
        speech_sync->ReadArrayOfInt16(game.LipSync.Lines[line_id].frame.data(),
                                      game.LipSync.Lines[line_id].numPhonemes);
    }
    Debug::Printf(kDbgMsg_Info, "Lipsync data found and loaded");
}

// Reports the game resolution, and sets up the main and UI viewports and the script system's
// resolution variables from it
void InitGameResolution(Game &game, GameDataVersion /*data_ver*/)
{
    const Size game_size = game.GetGameRes();
    Debug::Printf(kDbgMsg_Info, "Game native resolution: %d x %d (%d bit)", game_size.Width, game_size.Height,
                  game.color_depth * 8);

    // Assign general game viewports
    Rect viewport = RectWH(game_size);
    game.Viewports.SetMainViewport(viewport);
    game.Viewports.SetUIViewport(viewport);

    // Assign ScriptSystem's resolution variables
    game.ScSystem.width = game.GetGameRes().Width;
    game.ScSystem.height = game.GetGameRes().Height;
    game.ScSystem.coldepth = game.GetColorDepth();
    game.ScSystem.viewport_width = game.Viewports.GetMainViewport().GetWidth();
    game.ScSystem.viewport_height = game.Viewports.GetMainViewport().GetHeight();
}

HGameInitError InitGameState(const LoadedGameEntities &ents, GameDataVersion data_ver)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    // the entities were loaded into the engine's game (load_game_file)
    assert(&ents.Game == &game);
    const ScriptAPIVersion base_api = (ScriptAPIVersion)game.options[OPT_BASESCRIPTAPI];
    const ScriptAPIVersion compat_api = (ScriptAPIVersion)game.options[OPT_SCRIPTCOMPATLEV];
    const char *base_api_name = GetScriptAPIName(base_api);
    const char *compat_api_name = GetScriptAPIName(compat_api);
    Debug::Printf(kDbgMsg_Info, "Requested script API: %s (%d), compat level: %s (%d)",
                  base_api >= 0 && base_api <= kScriptAPI_Current ? base_api_name : "unknown", base_api,
                  compat_api >= 0 && compat_api <= kScriptAPI_Current ? compat_api_name : "unknown", compat_api);
    // If the game was compiled using unsupported version of the script API,
    // we warn about potential incompatibilities but proceed further.
    if (game.options[OPT_BASESCRIPTAPI] > kScriptAPI_Current)
        services.Platform->DisplayAlert(
            "Warning: this game requests a higher version of AGS script API, it may not run correctly or run at all.");

    //
    // 1. Check that the loaded data is valid and compatible with the current
    // engine capabilities.
    //
    if (game.numfonts == 0)
        return new GameInitError(kGameInitErr_NoFonts);

    //
    // 2. Allocate and init game objects
    //
    services.Sprites.EnlargeTo(ents.SpriteCount - 1);
    // the live characters start as copies of their definitions; the managed pool, scripts and plugins hold their
    // addresses from InitAndRegisterCharacters on, so the list is not resized again until unload_game
    game.Characters.clear();
    game.Characters.reserve(game.Def().Characters.size());
    for (const auto &def : game.Def().Characters)
        game.Characters.emplace_back(def);
    // the GUIs, views and dialogs were read into the game (GameDef) by the loader
    GUI::Context.GameColorDepth = game.GetColorDepth();
    GUI::Context.Spriteset = &services.Sprites;
    GUIRefCollection gui_control_refs(game.GuiButtons, game.GuiInvWindows, game.GuiLabels, game.GuiListBoxes,
                                      game.GuiSliders, game.GuiTextBoxes);
    GUI::RebuildGUI(game.Guis, gui_control_refs);
    game.Play.audioclipProps.resize(game.audioClips.size());
    game.Play.charProps.resize(game.numcharacters);
    game.Play.dialogProps.resize(game.numdialog);
    game.Play.guiProps.resize(game.numgui);
    game.Play.guicontrolProps[kGUIButton].resize(game.GuiButtons.size());
    game.Play.guicontrolProps[kGUILabel].resize(game.GuiLabels.size());
    game.Play.guicontrolProps[kGUIInvWindow].resize(game.GuiInvWindows.size());
    game.Play.guicontrolProps[kGUISlider].resize(game.GuiSliders.size());
    game.Play.guicontrolProps[kGUITextBox].resize(game.GuiTextBoxes.size());
    game.Play.guicontrolProps[kGUIListBox].resize(game.GuiListBoxes.size());
    game.Play.invProps.resize(game.numinvitems);
    // Set the number of game channels (the same for all the supported game versions)
    game.numGameChannels = MAX_GAME_CHANNELS;
    game.numCompatGameChannels = MAX_GAME_CHANNELS;
    HError err = InitAndRegisterGameEntities(game, ents);
    if (!err)
        return new GameInitError(err, kGameInitErr_EntityInitFail);
    LoadFonts(game, data_ver);
    LoadLipsyncData();

    //
    // 3. Initialize certain runtime variables
    //
    // TODO: merge this with engine_init_game_settings()
    game.Paused = 0; // reset the game paused flag
    game.IfacePopped = -1;

    String svg_suffix;
    if (game.saveGameFileExtension[0] != 0)
        svg_suffix.Format(".%s", game.saveGameFileExtension);
    set_save_game_suffix(svg_suffix);

    game.Play.fade_effect = game.options[OPT_FADETYPE];
    game.Play.std_gui_textheight = get_font_height_outlined(0) + 1;
    SetGameTextLanguage(game.GameTextLanguage);
    if (game.GameInfo.count("dev_error_text") > 0)
        game.Play.SetGameErrorText(game.GameInfo["dev_error_text"]);
    SetBaseTextParser(CreateTextParser(game.dict.get(), true, services.TextLocaleName));
    SetTranslationTextParser(CreateTextParser(game.dict.get(), true, services.TextLocaleName));

    //
    // 4. Initialize runtime state of certain game objects
    //
    InitGameResolution(game, data_ver);
    prepare_gui_runtime(true /* startup */);
    calculate_reserved_channel_count();
    // Default viewport and camera, draw data, etc, should be created when resolution is set
    game.Viewports.CreatePrimaryViewportAndCamera();
    init_game_drawdata();

    //
    // 5. Register engine API exports
    // NOTE: we must do this before plugin start, because some plugins may
    // require access to script API at initialization time.
    //
    InitScriptExec();
    setup_script_exports(base_api, compat_api);

    //
    // 6. Start up plugins
    //
    pl_register_plugins(ents.PluginInfos, !services.Config.Override.NoPlugins);
    pl_startup_plugins();

    //
    // 7. Create script modules and link the scripts
    // NOTE: we must do this after plugins, because some plugins may export
    // script symbols too.
    //

    std::vector<ccScript *> script_modules;
    for (const auto &script : game.ScriptModules)
        script_modules.push_back(script.get());
    err =
        CreateRuntimeScripts(game.GlobalScript.get(), script_modules, game.ScriptModuleNames, game.DialogScript.get());
    if (!err)
        return new GameInitError(err, kGameInitErr_ScriptInitFailed);
    err = LinkGlobalScripts();
    if (!err)
        return new GameInitError(err, kGameInitErr_ScriptInitFailed);

    // Apply accessibility options, must be done last, because some
    // may override startup game settings.
    ApplyAccessibilityOptions(game.Play, services.Config);
    // Applies behavior options (the runtime behavior switches)
    ApplyBehaviorOptions(game, game.Play, services.Config);

    // Optionally dump joint RTTI into the log
    if (services.LogScriptRTTI && RuntimeScript::GetJointRTTI())
    {
        Debug::Printf(PrintRTTI(RuntimeScript::GetJointRTTI()->AsConstRTTI()));
    }

    return HGameInitError::None();
}

void ApplyAccessibilityOptions(GamePlayState &play, const EngineConfig &setup)
{
    if (setup.Access.SpeechSkipStyle != kSkipSpeechNone)
    {
        play.speech_skip_style = user_to_internal_skip_speech(setup.Access.SpeechSkipStyle);
    }
    if (setup.Access.TextSkipStyle != kSkipSpeechNone)
    {
        play.skip_display = setup.Access.TextSkipStyle;
    }
    if (setup.Access.TextReadSpeed > 0)
    {
        play.text_speed = setup.Access.TextReadSpeed;
        play.text_min_display_time_ms = Math::Clamp((int)(1000 * (15.f / setup.Access.TextReadSpeed)), 1000, 3000);
    }
    if (setup.Access.SpeechMode != kSpeech_None)
    {
        play.speech_mode = setup.Access.SpeechMode;
    }
    if (setup.Access.AlwaysWaitForText)
    {
        play.speech_always_wait_for_text = true;
    }
}

void ApplyBehaviorOptions(GameDef & /*game*/, GamePlayState &play, const EngineConfig &setup)
{
    // First designate the default behavior settings: games older than 3.6.3 had them off, which this
    // engine does not load any more, and 3.99+ data has them all on
    // TODO: the four switches start on and the overrides below only turn switches on, so they are always
    // on; remove the switches together with the old-behavior code that tests them
    std::array<bool, kNum_RBS> behavior_switches = {false};
    behavior_switches[kRBO_SmoothWalkTransition] = true;
    behavior_switches[kRBO_ApplyGUITextDirection] = true;
    behavior_switches[kRBO_ApplyDialogOptionTextAlignment] = true;
    behavior_switches[kRBO_NoTextPropertyAutoTranslate] = true;

    // Now apply overrides from config, *BUT* these only enable disabled options,
    // and never disable enabled ones (because that might break or "downgrade" modern games).
    for (size_t i = 0; i < kNum_RBS; ++i)
    {
        behavior_switches[i] |= setup.BehaviorOverrides[i];
    }
    play.SetRBSwitches(behavior_switches);

    GUI::Options.ApplyTextDirection = play.GetRBSwitches()[kRBO_ApplyGUITextDirection];
}

void PrintBehaviorOptions(const GameDef & /*game*/, const GamePlayState &play)
{
    // Runtime Behavior Switches
    {
        Debug::Printf("Runtime behavior switches:");
        const auto &rbs_values = play.GetRBSwitches();
        for (size_t i = kRBS_Dummy + 1; i < kNum_RBS; ++i)
        {
            assert(kRBSwitchNames[i] != nullptr);
            Debug::Printf("\t%s = %d", kRBSwitchNames[i], static_cast<int>(rbs_values[i]));
        }
    }
}

} // namespace Engine
} // namespace AGS
