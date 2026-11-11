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
#include "ac/global_game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "ac/audiocliptype.h"
#include "ac/character.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_display.h"
#include "ac/global_gui.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/inventoryitem.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/path_helper.h"
#include "ac/room.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/sys_events.h"
#include "ac/system.h"
#include "ac/view.h"
#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gui/guidialog.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/game_file.h"
#include "main/game_run.h"
#include "main/game_start.h"
#include "main/graphics_mode.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "platform/platform.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

extern int displayed_room;
extern int game_paused;
extern SpriteCache spriteset;
extern unsigned int load_new_game;
extern int load_new_game_restore;
extern GameSetupStruct game;
extern std::vector<ViewStruct> views;
extern RoomStatus *croom;
extern RoomStruct thisroom;
extern IGraphicsDriver *gfxDriver;
extern RGB palette[256];


void AbortGame()
{
    // make sure scripts stop at the next step
    AbortAllScripts();
}

void restart_game()
{
    can_run_delayed_command();
    if (is_inside_script())
    {
        get_executingscript()->QueueAction(PostScriptAction(ePSARestartGame, 0, "RestartGame"));
        return;
    }
    try_restore_save(RESTART_POINT_SAVE_GAME_NUMBER);
}

void CopySaveSlot(int old_slot, int new_slot)
{
    if (old_slot == new_slot)
        return; // cannot copy into itself

    String old_filename = get_save_game_path(old_slot);
    String new_filename = get_save_game_path(new_slot);
    File::CopyFile(old_filename, new_filename, true);
}

void MoveSaveSlot(int old_slot, int new_slot)
{
    if (old_slot == new_slot)
        return; // cannot move into itself

    String old_filename = get_save_game_path(old_slot);
    String new_filename = get_save_game_path(new_slot);
    File::RenameFile(old_filename, new_filename);
}

void RestoreGameSlot(int slot)
{
    if (displayed_room < 0)
        quit("!RestoreGameSlot: a game cannot be restored from within game_start");

    can_run_delayed_command();
    if (is_inside_script())
    {
        get_executingscript()->QueueAction(PostScriptAction(ePSARestoreGame, slot, "RestoreGameSlot"));
        return;
    }
    try_restore_save(slot);
}

void SaveGameSlot(int slot, const char *description, int sprite_id)
{
    VALIDATE_STRING(description);

    if (platform->GetDiskFreeSpaceMB(get_save_game_directory()) < 2)
    {
        Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
        return;
    }

    schedule_save_game(slot, description, sprite_id);
}

void SaveGameSlot2(int slot, const char *description)
{
    SaveGameSlot(slot, description, -1);
}

void DeleteSaveSlot(int slot)
{
    String save_filename = get_save_game_path(slot);
    File::DeleteFile(save_filename);
}

void PauseGame()
{
    game_paused++;
    debug_script_log("Game paused (%d)", game_paused);
}

void UnPauseGame()
{
    if (game_paused > 0)
    {
        game_paused--;
        debug_script_log("Game un-paused (%d)", game_paused);
    }
}


int IsGamePaused()
{
    if (game_paused > 0)
        return 1;
    return 0;
}

bool GetSaveSlotDescription(int slot, String &description)
{
    if (read_savedgame_description(get_save_game_path(slot), description))
        return true;
    description.Format("INVALID SLOT %d", slot);
    return false;
}

void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, bool get_description)
{
    if (top_index < bot_index)
        return; // empty range

    bot_index = std::min(999u, bot_index); // NOTE: slots are limited by 0..999 range
    top_index = std::min(999u, top_index);

    const String save_dir = get_save_game_directory();
    const String save_suffix = get_save_game_suffix();
    const String pattern = String::FromFormat("agssave.???%s", save_suffix.GetCStr());

    for (FindFile ff = FindFile::OpenFiles(save_dir, pattern); !ff.AtEnd(); ff.Next())
    {
        String slot_name = ff.Current();
        if (!save_suffix.IsEmpty())
            slot_name.ClipRight(save_suffix.GetLength());
        int save_slot = Path::GetFileExtension(slot_name).ToInt();
        // only list games between .XXX to .YYY (to allow hidden slots for special purposes)
        if (save_slot < 0 || static_cast<unsigned>(save_slot) < bot_index ||
            static_cast<unsigned>(save_slot) > top_index)
            continue;
        String description;
        if (get_description)
            GetSaveSlotDescription(save_slot, description);
        saves.push_back(SaveListItem(save_slot, description, ff.GetEntry().Time));
    }
}

static void SortSaveList(std::vector<SaveListItem> &saves, ScriptSaveGameSortStyle save_sort,
                         ScriptSortDirection sort_dir)
{
    const bool ascending = (sort_dir != kScSortDescending) || (save_sort == kScSaveGameSort_None);
    switch (save_sort)
    {
    case kScSaveGameSort_Number:
        if (ascending)
            std::sort(saves.begin(), saves.end(), SaveItemCmpByNumber());
        else
            std::sort(saves.rbegin(), saves.rend(), SaveItemCmpByNumber());
        break;
    case kScSaveGameSort_Time:
        if (ascending)
            std::sort(saves.begin(), saves.end(), SaveItemCmpByTime());
        else
            std::sort(saves.rbegin(), saves.rend(), SaveItemCmpByTime());
        break;
    case kScSaveGameSort_Description:
    {
        SaveItemCmpByDesc cmp_by_desc(StrUtil::GetStrCmpImplFor(true, false, play.GetTextLocaleName().GetCStr()));
        if (ascending)
            std::sort(saves.begin(), saves.end(), cmp_by_desc);
        else
            std::sort(saves.rbegin(), saves.rend(), cmp_by_desc);
        break;
    }
    default: break;
    }
}

void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, bool get_description,
                  ScriptSaveGameSortStyle save_sort, ScriptSortDirection sort_dir)
{
    FillSaveList(saves, bot_index, top_index, get_description);
    SortSaveList(saves, save_sort, sort_dir);
}

void FillSaveList(const std::vector<int> &slots, std::vector<SaveListItem> &saves, bool get_description,
                  ScriptSaveGameSortStyle save_sort, ScriptSortDirection sort_dir)
{
    for (const auto &slot : slots)
    {
        if (slot < 0 || slot > TOP_SAVESLOT)
            continue; // unsupported save slot
        String path = get_save_game_path(slot);
        if (!File::IsFile(path))
            continue; // file does not exist
        String description;
        if (get_description)
            GetSaveSlotDescription(slot, description);
        saves.push_back(SaveListItem(slot, description, File::GetFileTime(path)));
    }

    SortSaveList(saves, save_sort, sort_dir);
}

int GetLastSaveSlot()
{
    std::vector<SaveListItem> saves;
    FillSaveList(saves, 0, RESTART_POINT_SAVE_GAME_NUMBER - 1, false /* no desc */);
    if (saves.size() == 0)
        return -1;
    std::sort(saves.rbegin(), saves.rend(), SaveItemCmpByTime()); // sort by time in reverse
    return saves[0].Slot;
}

int RunAGSGame(const String &new_game, unsigned int mode, int data)
{
    can_run_delayed_command();

    int allowed_modes = RAGMODE_LOADNOW;

    if ((mode & (~allowed_modes)) != 0)
        quit("!RunAGSGame: mode value unknown");

    if (editor_debugging_initialized)
    {
        quit("!RunAGSGame cannot be used while running the game from within the AGS Editor. You must build the game "
             "EXE and run it from there to use this function.");
    }

    if ((mode & RAGMODE_LOADNOW) == 0)
    {
        ResPaths.GamePak.Path = PathFromInstallDir(new_game);
        ResPaths.GamePak.Name = new_game;
        play.takeover_data = data;
        load_new_game_restore = -1;

        if (is_inside_script())
        {
            get_executingscript()->QueueAction(PostScriptAction(ePSARunAGSGame, mode | RAGMODE_LOADNOW, "RunAGSGame"));
            scriptExecutor->Abort();
        }
        else
        {
            load_new_game = mode | RAGMODE_LOADNOW;
        }

        return 0;
    }

    unload_old_room();
    displayed_room = -10;

#if defined(AGS_AUTO_WRITE_USER_CONFIG)
    save_config_file(); // save current user config in case engine fails to run new game
#endif                  // AGS_AUTO_WRITE_USER_CONFIG
    unload_game();

    // Adjust config (NOTE: normally, RunAGSGame would need a redesign to allow separate config etc per each game)
    usetup.Translation = ""; // reset to default, prevent from trying translation file of game A in game B

    AssetMgr->RemoveAllLibraries();

    // TODO: this repeats the engine's startup steps (main/engine.cpp); share them
    if (AssetMgr->AddLibrary(ResPaths.GamePak.Path) != Common::kAssetNoError)
        quitprintf("!RunAGSGame: unable to load new game file '%s'", ResPaths.GamePak.Path.GetCStr());
    engine_assign_assetpaths();

    show_preload();

    HError err = load_game_file();
    if (!err)
        quitprintf("!RunAGSGame: error loading new game file:\n%s", err->FullMessage().GetCStr());
    // Assign the default bitmap color depth, equals this game's color depth
    Bitmap::SetColorDepth(game.GetColorDepth());

    err = engine_init_sprites();
    if (!err)
        quitprintf("!RunAGSGame: error loading new sprites:\n%s", err->FullMessage().GetCStr());

    engine_init_game_settings();
    play.screen_is_faded_out = true;

    if (load_new_game_restore >= 0)
    {
        try_restore_save(load_new_game_restore, true);
        load_new_game_restore = -1;
    }
    else
        start_game();

    return 0;
}

void QuitGame(int dialog)
{
    if (dialog)
    {
        int dialog_result;
        setup_for_dialog(play.normal_font);
        dialog_result = quitdialog();
        restore_after_dialog();
        if (dialog_result == 0)
            return;
    }
    quit("|You have exited.");
}

void SetRestartPoint()
{
    schedule_save_game(RESTART_POINT_SAVE_GAME_NUMBER, "Restart Game Auto-Save");
}

void SetGameSpeed(int new_speed)
{
    game.options[OPT_GAMEFPS] = new_speed; // save for the reference
    int fixed_speed = new_speed + play.game_speed_modifier;
    fixed_speed = Math::Clamp(fixed_speed, 10, 1000);
    set_game_speed(fixed_speed);
    debug_script_log("Game speed set to %d (requested %d)", fixed_speed, new_speed);
}

int GetGameSpeed()
{
    return game.options[OPT_GAMEFPS];
}

int SetGameOption(int opt, int new_value)
{
    if (((opt < OPT_DEBUGMODE) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    {
        debug_script_error("SetGameOption: invalid option specified: %d", opt);
        return 0;
    }

    // Handle forbidden options
    const auto restricted_opts = GameSetupStructBase::GetRestrictedOptions();
    for (auto r_opt : restricted_opts)
    {
        if (r_opt == opt)
        {
            debug_script_warn("SetGameOption: option %d cannot be modified at runtime", opt);
            return game.options[opt];
        }
    }

    // Test if option already has this value
    if (game.options[opt] == new_value)
        return game.options[opt];

    const int old_value = game.options[opt];
    game.options[opt] = new_value;

    // Update the game in accordance to the new option value
    switch (opt)
    {
    case OPT_ANTIGLIDE:
        for (int i = 0; i < game.numcharacters; i++)
        {
            if (new_value)
                game.chars[i].flags |= CHF_ANTIGLIDE;
            else
                game.chars[i].flags &= ~CHF_ANTIGLIDE;
        }
        break;
    case OPT_DISABLEOFF:
        GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.options[OPT_DISABLEOFF]);
        // If interface was disabled at this time then also update GUIs, as visual style could've changed
        update_gui_disabled_status();
        // Also update all disabled controls (like buttons), in case they need to update their disabled effect
        GUIE::MarkDisabledGUIForUpdate();
        break;
    case OPT_FIXEDINVCURSOR:
        // Update current "use inv" cursor if necessary
        if (is_current_cursor_look(kCursorRole_UseInv) && (playerchar->activeinv >= 0))
        {
            update_inv_cursor(playerchar->activeinv);
            set_cursor_look(cur_cursor);
        }
        break;
    case OPT_ANTIALIASFONTS: adjust_fonts_for_render_mode(new_value != 0); break;
    case OPT_RIGHTLEFTWRITE: GUIE::MarkForTranslationUpdate(); break;
    case OPT_DUPLICATEINV: UpdateInventory(); break;
    case OPT_PORTRAITSIDE: play.swap_portrait_side = (new_value == PORTRAIT_ALTERNATE); break;
    case OPT_DISPLAYSINGLEDIALOGOPTION: play.show_single_dialog_option = new_value; break;
    default: break; // do nothing else
    }

    return old_value;
}

int GetGameOption(int opt)
{
    if (((opt < OPT_DEBUGMODE) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    {
        debug_script_warn("GetGameOption: invalid option specified: %d", opt);
        return 0;
    }

    return game.options[opt];
}

void SkipUntilCharacterStops(int char_id)
{
    if (!AssertCharacter("SkipUntilCharacterStops", char_id))
        return;
    if (game.chars[char_id].room != displayed_room)
        quitprintf("!SkipUntilCharacterStops: character %s is not in current room %d (it is in room %d)",
                   game.chars[char_id].scrname.GetCStr(), displayed_room, game.chars[char_id].room);

    // if they are not currently moving, do nothing
    if (!game.chars[char_id].walking)
        return;

    if (is_in_cutscene())
        quit("!SkipUntilCharacterStops: cannot be used within a cutscene");

    initialize_skippable_cutscene();
    play.fast_forward = 2;
    play.skip_until_char_stops = char_id;
}

void EndSkippingUntilCharStops()
{
    // not currently skipping, so ignore
    if (play.skip_until_char_stops < 0)
        return;

    stop_fast_forwarding();
    play.skip_until_char_stops = -1;
}

void StartCutscene(int skip_mode)
{
    static ScriptPosition last_cutscene_script_pos;

    if (is_in_cutscene())
    {
        quitprintf("!StartCutscene: already in a cutscene; previous started in \"%s\", line %d",
                   last_cutscene_script_pos.Section.GetCStr(), last_cutscene_script_pos.Line);
    }

    if ((skip_mode < 1) || (skip_mode > 6))
        quit("!StartCutscene: invalid argument, must be 1 to 5.");

    get_script_position(last_cutscene_script_pos);

    // make sure they can't be skipping and cutsceneing at the same time
    EndSkippingUntilCharStops();

    play.in_cutscene = skip_mode;
    initialize_skippable_cutscene();
}

void SkipCutscene()
{
    if (is_in_cutscene())
        start_skipping_cutscene();
}

int EndCutscene()
{
    if (!is_in_cutscene())
        quit("!EndCutscene: not in a cutscene");

    int result = play.fast_forward;
    play.in_cutscene = 0;
    // Stop it fast-forwarding
    stop_fast_forwarding();

    // make sure that the screen redraws
    invalidate_screen();

    // Return whether the player skipped it
    return result;
}

void ShowInputBox(const char *msg, char *buffer)
{
    VALIDATE_STRING(buffer);
    String input = ShowInputBoxImpl(msg);
    snprintf(buffer, MAX_MAXSTRLEN, "%s", input.GetCStr());
}

String ShowInputBoxImpl(const char *msg)
{
    setup_for_dialog(play.normal_font);
    String input = enterstringwindow(get_translation(msg));
    restore_after_dialog();
    return input;
}

int GetLocationType(int x, int y, int hit_options)
{
    return GetLocationTypeImpl(nullptr, x, y, hit_options, false /* dont click-through gui */,
                               false /* not allow hotspot0 */);
}

int GetLocationType2(int x, int y)
{
    return GetLocationTypeImpl(nullptr, x, y, kHit_Interactable, false /* dont click-through gui */,
                               false /* not allow hotspot0 */);
}

// Finds out what is located under the cursor;
// returns location's name and "location reference".
static const char *GetLocationNameAndRef(SceneLocationRef &loc_ref, int x, int y, int hit_options)
{
    // If no room loaded yet, then return nothing
    if (displayed_room < 0)
    {
        loc_ref = {};
        return "";
    }

    int loc_id = -1;
    // GetLocationType takes screen coords
    const int loc_type =
        GetLocationTypeImpl(&loc_id, x, y, hit_options, false /* dont click-through gui */, true /* allow hotspot0 */);
    loc_ref = SceneLocationRef(loc_type, loc_id);
    if (loc_type == LOCTYPE_NOTHING)
    {
        return "";
    }

    // Get if we're above any interactable location
    // on character
    if (loc_type == LOCTYPE_CHAR)
    {
        return game.chars[loc_id].name.GetCStr();
    }
    // on object
    else if (loc_type == LOCTYPE_OBJ)
    {
        return croom->obj[loc_id].name.GetCStr();
    }
    // on hotspot (except hotspot 0 which means "empty space")
    else if (loc_type == LOCTYPE_HOTSPOT)
    {
        if (loc_id > 0)
        {
            return croom->hotspot[loc_id].Name.GetCStr();
        }
    }
    else if (loc_type == LOCTYPE_INVITEM)
    {
        return game.invinfo[loc_id].name.GetCStr();
    }
    return "";
}

const char *GetLocationName(int x, int y, int hit_options)
{
    SceneLocationRef loc_ref;
    const char *loc_name = GetLocationNameAndRef(loc_ref, x, y, hit_options);

    // If it's a new location, different from the last time we checked,
    // then update "@OVERHOTSPOT@" label(s), and save the last index
    if (play.get_loc_name_last_time != loc_ref)
    {
        GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
        play.get_loc_name_last_time = loc_ref;
    }

    return loc_name;
}

int IsKeyPressed(int keycode)
{
    return ags_iskeydown(static_cast<eAGSKeyCode>(keycode));
}

bool IsAnyKeyPressed()
{
    return ags_isanykeydown();
}

void *GetPressedKeys()
{
    const auto &keys = ags_getkeysdown();
    DynObjectRef arr = DynamicArrayHelpers::CreateArray(sizeof(int), keys.size());
    if (arr)
    {
        for (size_t i = 0; i < keys.size(); ++i)
            *(static_cast<uint32_t *>(arr.Obj()) + i) =
                AGSKeyToScriptKey(sdl_key_to_ags_key(keys[i].sym, keys[i].scancode, 0u /* ignore mod */));
    }
    return arr.Obj();
}

int SaveScreenShot4(const char *filename, int width, int height, int layers)
{
    String filepath = filename;
    String ext = Path::GetFileExtension(filepath);
    if (ext.IsEmpty())
    {
        ext = "bmp";
        filepath = Path::ReplaceExtension(filepath, ext);
    }
    if (!filepath.StartsWith("$"))
    {
        filepath = Path::ConcatPaths(GameSavedgamesDirToken, filepath);
    }

    std::unique_ptr<Stream> out = ResolveScriptPathAndOpen(filepath, kFile_CreateAlways, kStream_Write);
    if (!out)
        return 0;

    std::unique_ptr<Bitmap> shot = create_game_screenshot(width, height, layers);
    if (!shot)
        return 0; // out of mem or invalid parameters
    return BitmapHelper::SaveBitmap(shot.get(), true /* screenshots are opaque */, palette, out.get(), ext) ? 1 : 0;
}

int SaveScreenShot1(const char *filename)
{
    return SaveScreenShot4(filename, 0, 0, RENDER_BATCH_ALL);
}

void SetMultitasking(int mode)
{
    if ((mode < 0) | (mode > 1))
        quit("!SetMultitasking: invalid mode parameter");
    // Save requested setting
    usetup.RunInBackground = mode != 0;

    // Account for the override config option (must be checked first!)
    if ((usetup.Override.Multitasking >= 0) && (mode != usetup.Override.Multitasking))
    {
        Debug::Printf("SetMultitasking: overridden by user config: %d -> %d", mode, usetup.Override.Multitasking);
        mode = usetup.Override.Multitasking;
    }

    // Must run on background if debugger is connected
    if ((mode == 0) && (editor_debugging_initialized != 0))
    {
        Debug::Printf("SetMultitasking: overridden by the external debugger: %d -> 1", mode);
        mode = 1;
    }

    // Regardless, don't allow background running if exclusive full screen
    if ((mode == 1) && gfxDriver && gfxDriver->GetDisplayMode().IsRealFullscreen())
    {
        Debug::Printf("SetMultitasking: overridden by fullscreen: %d -> 0", mode);
        mode = 0;
    }

    // Install engine callbacks for switching in and out the window
    Debug::Printf(kDbgMsg_Info, "Multitasking mode set: %d", mode);
    if (mode == 0)
    {
        sys_set_background_mode(false);
        sys_evt_set_focus_callbacks(display_switch_in_resume, display_switch_out_suspend);
    }
    else
    {
        sys_set_background_mode(true);
        sys_evt_set_focus_callbacks(display_switch_in, display_switch_out);
    }
}

void RoomProcessClick(int x, int y, int mood)
{
    int loc_id = -1;
    int loc_type = GetLocationTypeImpl(&loc_id, x, y, kHit_Interactable, true /* click-through gui */,
                                       false /* not allow hotspot0 */);
    VpPoint vpt = play.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return;
    x = vpt.first.X;
    y = vpt.first.Y;

    if (game.HasCursorRole(mood, kCursorRole_Walk) && (game.options[OPT_NOWALKMODE] == 0))
    {
        int hotspot_id = GetHotspotIDAtRoom(x, y, kHit_Interactable);
        if (hotspot_id < 1)
            ;
        else if (thisroom.Hotspots[hotspot_id].WalkTo.X < 1)
            ;
        else if (play.auto_use_walkto_points == 0)
            ;
        else
        {
            x = thisroom.Hotspots[hotspot_id].WalkTo.X;
            y = thisroom.Hotspots[hotspot_id].WalkTo.Y;
            debug_script_log("Move to walk-to point hotspot %d", hotspot_id);
        }
        walk_character(playerchar, x, y, false /* walk areas */);
        return;
    }
    play.usedmode = mood;

    if (loc_type == 0)
    {
        // click on nothing -> hotspot 0
        loc_id = 0;
        loc_type = LOCTYPE_HOTSPOT;
    }

    if (loc_type == LOCTYPE_CHAR)
    {
        if (check_click_on_character(x, y, mood))
            return;
    }
    else if (loc_type == LOCTYPE_OBJ)
    {
        if (check_click_on_object(x, y, mood))
            return;
    }
    else if (loc_type == LOCTYPE_HOTSPOT)
        RunHotspotInteraction(loc_id, mood);
}

int IsInteractionAvailable(int x, int y, int mood)
{
    int loc_id = -1;
    int loc_type = GetLocationTypeImpl(&loc_id, x, y, kHit_Interactable, true /* click-through gui */,
                                       false /* not allow hotspot0 */);
    VpPoint vpt = play.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return 0;
    x = vpt.first.X;
    y = vpt.first.Y;

    // You can always walk places
    if (game.HasCursorRole(mood, kCursorRole_Walk) && (game.options[OPT_NOWALKMODE] == 0))
        return 1;

    play.check_interaction_only = 1;

    if (loc_type == 0)
    {
        // click on nothing -> hotspot 0
        loc_id = 0;
        loc_type = LOCTYPE_HOTSPOT;
    }

    if (loc_type == LOCTYPE_CHAR)
    {
        check_click_on_character(x, y, mood);
    }
    else if (loc_type == LOCTYPE_OBJ)
    {
        check_click_on_object(x, y, mood);
    }
    else if (loc_type == LOCTYPE_HOTSPOT)
        RunHotspotInteraction(loc_id, mood);

    int check_result = play.check_interaction_only;
    play.check_interaction_only = 0;

    if (check_result == 2)
        return 1;

    return 0;
}

void _sc_AbortGame(const char *text)
{
    quit(String::FromFormat("!?%s", text));
}

// Wait* implementation for games with a base script API below 3.6.0 (OPT_BASESCRIPTAPI), which had a number of
// accidental effects that some old games depend on.
static int WaitImplPre360(int skip_type, int num_loops)
{
    // Negative nloops was forbidden in < 3.6.0, but here we just return instantly
    if (num_loops < 0)
    {
        return 0;
    }

    // < 3.6.0 implementation of Wait* did not have a value range check when assigning
    // int32 parameter to a int16 wait counter.
    // Values above INT16_MAX caused wait counter overflow. If the resulting int16 value
    // appeared negative, then Wait would skip after the very first update,
    // returning "1", as if it was skipped by a player input (this was not intentional,
    // but pure accident due to a bad program logic). If the resulting int16 value
    // appeared positive, then game would wait for that amount normally.
    // We simulate this here, because, unfortunately, it's required to keep some old games run correctly.
    bool force_result_1 = false;
    if (num_loops > INT16_MAX)
    {
        int16_t overflown_value = static_cast<int16_t>(num_loops);
        if (overflown_value < 0)
        {
            num_loops = 1;
            force_result_1 = true;
        }
        else
        {
            num_loops = overflown_value;
        }
    }

    if (num_loops == 0)
    {
        return 0;
    }

    play.wait_counter = static_cast<int16_t>(Math::Clamp<int>(num_loops, -1, INT16_MAX));
    play.wait_skipped_by = SKIP_NONE;
    play.wait_skipped_by_data = 0;
    play.key_skip_wait = skip_type;

    GameLoopUntilValueIsZero(&play.wait_counter);

    if (force_result_1)
        return 1;

    // < 3.6.0 return 1 if skipped by user input, otherwise 0
    return ((play.wait_skipped_by & (SKIP_KEYPRESS | SKIP_MOUSECLICK)) != 0) ? 1 : 0;
}

int WaitImpl(int skip_type, int num_loops)
{
    // if skipping cutscene and expecting user input: don't wait at all
    if (play.fast_forward && ((skip_type & ~SKIP_AUTOTIMER) != 0))
        return 0;

    if (game.options[OPT_BASESCRIPTAPI] < kScriptAPI_v360)
    {
        return WaitImplPre360(skip_type, num_loops);
    }

    // Zero loops make no sense, so return instantly
    if (num_loops == 0)
    {
        return 0;
    }

    play.wait_counter = static_cast<int16_t>(Math::Clamp<int>(num_loops, -1, INT16_MAX));
    play.wait_skipped_by = SKIP_NONE;
    play.wait_skipped_by_data = 0;
    play.key_skip_wait = skip_type;

    GameLoopUntilValueIsZero(&play.wait_counter);

    // >= 3.6.0 return skip (input) type flags with keycode
    return play.GetWaitSkipResult();
}

void scrWait(int num_loops)
{
    WaitImpl(SKIP_AUTOTIMER, num_loops);
}

int WaitKey(int num_loops)
{
    return WaitImpl(SKIP_KEYPRESS | SKIP_AUTOTIMER, num_loops);
}

int WaitMouse(int num_loops)
{
    return WaitImpl(SKIP_MOUSECLICK | SKIP_AUTOTIMER, num_loops);
}

int WaitMouseKey(int num_loops)
{
    return WaitImpl(SKIP_KEYPRESS | SKIP_MOUSECLICK | SKIP_AUTOTIMER, num_loops);
}

int WaitInput(int input_flags, int num_loops)
{
    return WaitImpl((input_flags >> SKIP_RESULT_TYPE_SHIFT) | SKIP_AUTOTIMER, num_loops);
}

void SkipWait0()
{
    SkipWait(0);
}

void SkipWait(int result_value)
{
    play.SetWaitSkipResult(SKIP_NONE /* by script */, result_value);
}
