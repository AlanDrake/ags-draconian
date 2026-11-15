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
#include "game/game.h"

#include <algorithm>
#include <cstdio>
#include <queue>

#include "data/data_helpers.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "device/sys_events.h"
#include "font/fonts.h"
#include "game/audio/audio.h"
#include "game/audio/audio_channel.h"
#include "game/dialog/dialog.h"
#include "game/dialog/display.h"
#include "game/dialog/lipsync.h"
#include "game/dialog/speech.h"
#include "game/dynobj/all_dynamic_classes.h"
#include "game/dynobj/script_camera.h"
#include "game/dynobj/script_motion_path.h"
#include "game/event.h"
#include "game/game_setup_struct.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/render/dynamic_sprite.h"
#include "game/render/overlay.h"
#include "game/save/savegame.h"
#include "game/script.h"
#include "game/system.h"
#include "game/translation.h"
#include "game/ui/button.h"
#include "game/ui/csci_dialog.h"
#include "game/ui/gui.h"
#include "game/ui/gui_dialog.h"
#include "game/ui/mouse.h"
#include "game/video/video.h"
#include "game/view.h"
#include "game/world/character.h"
#include "game/world/hotspot.h"
#include "game/world/inventory_item.h"
#include "game/world/object.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/room_status.h"
#include "game/world/walk_behind.h"
#include "game/world/walkable_area.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/game_start.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "platform/base/sys_main.h"
#include "plugin/plugin_engine.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/script_runtime.h"
#include "script/scriptexecutor.h"
#include "util/common.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/keycode.h"
#include "util/path.h"
#include "util/path_helper.h"
#include "util/string_compat.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

//=============================================================================
GamePlayState play;
GameSetupStruct game;
RoomStatus troom; // state of the current room if it is a temporary one (number >= MAX_ROOMS), e.g. an intro room
RoomObject *objs = nullptr;
RoomStatus *croom = nullptr;
RoomStruct thisroom;

GameDataVersion loaded_game_file_version = kGameVersion_Undefined;
int frames_per_second = 40;
int displayed_room = -10;
EnterNewRoomState in_new_room = kEnterRoom_None, new_room_was = kEnterRoom_None;
int new_room_pos = 0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;
int new_room_loop = SCR_NO_VALUE;
bool new_room_placeonwalkable = false;

AGS::Common::SpriteCache::Callbacks spritecallbacks = {nullptr, initialize_sprite, post_init_sprite, nullptr};
SpriteCache spriteset(game.SpriteInfos, spritecallbacks);

std::vector<GUIMain> guis;
std::vector<GUIButton> guibuts;
std::vector<GUIInvWindow> guiinv;
std::vector<GUILabel> guilabels;
std::vector<GUIListBox> guilist;
std::vector<GUISlider> guislider;
std::vector<GUITextBox> guitext;

CCGUIControl ccDynamicGUIControl;
CCGUIButton ccDynamicGUIButton;
CCGUIInvWindow ccDynamicGUIInvWindow;
CCGUILabel ccDynamicGUILabel;
CCGUIListBox ccDynamicGUIListBox;
CCGUISlider ccDynamicGUISlider;
CCGUITextBox ccDynamicGUITextBox;
CCCharacter ccDynamicCharacter;
CCHotspot ccDynamicHotspot;
CCRegion ccDynamicRegion;
CCWalkableArea ccDynamicWalkarea;
CCWalkbehind ccDynamicWalkbehind;
CCInventory ccDynamicInv;
CCGUI ccDynamicGUI;
CCTextWindowGUI ccDynamicTextWindowGUI;
CCObject ccDynamicObject;
CCDialog ccDynamicDialog;
CCAudioClip ccDynamicAudioClip;
CCAudioChannel ccDynamicAudio;

std::vector<ScriptDialog> scrDialog;
std::vector<ScriptGUI> scrGui;
std::vector<ScriptInvItem> scrInv;
ScriptHotspot scrHotspot[MAX_ROOM_HOTSPOTS];
ScriptObject scrObj[MAX_ROOM_OBJECTS];
ScriptRegion scrRegion[MAX_ROOM_REGIONS];
ScriptWalkableArea scrWalkarea[MAX_WALK_AREAS];
ScriptWalkbehind scrWalkbehind[MAX_WALK_BEHINDS];


std::vector<ViewStruct> views;
std::vector<CharacterExtras> charextra;
// MoveLists of the moving characters and room objects, addressed by their index; NOTE: slot 0 is never used,
// as 0 means "no movelist". A slot freed by remove_movelist is queued in movelist_free_ids and used again.
std::vector<MoveList> movelists;
std::queue<uint32_t> movelist_free_ids;

//=============================================================================

String saveGameDirectory = "./";

String saveGameSuffix;

int game_paused = 0;

unsigned int load_new_game = 0;
int load_new_game_restore = -1;
SaveCmpSelection load_new_game_restore_cmp = kSaveCmp_All;

//=============================================================================
// Audio
//=============================================================================

bool AssertAudioType(const char *api_name, int audio_type)
{
    if ((audio_type < 0) || ((size_t)audio_type >= game.audioClipTypes.size()))
    {
        debug_script_error("%s: invalid audio type: %d, valid range is 0..%d", api_name, audio_type,
                           static_cast<int>(game.audioClipTypes.size()) - 1);
        return false;
    }
    return true;
}

void Game_StopAudio(int audio_type)
{
    if ((audio_type != SCR_NO_VALUE) && !AssertAudioType("Game.StopAudio", audio_type))
        return;
    for (int chan_id = 0; chan_id < game.numGameChannels; chan_id++)
    {
        if (audio_type == SCR_NO_VALUE)
        {
            stop_or_fade_out_channel(chan_id);
        }
        else
        {
            int playing_type = AudioChannel_GetPlayingType(&scrAudioChannel[chan_id]);
            if (playing_type == audio_type)
                stop_or_fade_out_channel(chan_id);
        }
    }

    remove_clips_of_type_from_queue(audio_type);
}

int Game_IsAudioPlaying(int audio_type)
{
    if ((audio_type != SCR_NO_VALUE) && !AssertAudioType("Game.IsAudioPlaying", audio_type))
        return 0;

    if (play.fast_forward)
        return 0;

    for (int chan_id = 0; chan_id < game.numGameChannels; chan_id++)
    {
        int playing_type = AudioChannel_GetPlayingType(&scrAudioChannel[chan_id]);
        if (playing_type >= 0)
        {
            if ((playing_type == audio_type) || (audio_type == SCR_NO_VALUE))
            {
                return 1;
            }
        }
    }
    return 0;
}

int Game_GetAudioTypeCrossfadeSpeed(int audio_type)
{
    if (!AssertAudioType("Game.GetAudioTypeCrossfadeSpeed", audio_type))
        return 0;

    return game.audioClipTypes[audio_type].crossfadeSpeed;
}

void Game_SetAudioTypeCrossfadeSpeed(int audio_type, int crossfade_speed)
{
    if (!AssertAudioType("Game.SetAudioTypeCrossfadeSpeed", audio_type))
        return;
    if (crossfade_speed < 0 || crossfade_speed > 100)
    {
        debug_script_warn("Game.SetAudioTypeCrossfadeSpeed: value %d is out of valid range (0..100)", crossfade_speed);
        crossfade_speed = Math::Clamp(crossfade_speed, 0, 100);
    }

    Debug::Printf("Game.SetAudioTypeCrossfadeSpeed: type: %d, speed: %d", audio_type, crossfade_speed);
    game.audioClipTypes[audio_type].crossfadeSpeed = crossfade_speed;
}

int Game_GetAudioTypeSpeechVolumeDrop(int audio_type)
{
    if (!AssertAudioType("Game.GetAudioTypeSpeechVolumeDrop", audio_type))
        return 0;

    return game.audioClipTypes[audio_type].volume_reduction_while_speech_playing;
}

void Game_SetAudioTypeSpeechVolumeDrop(int audio_type, int volume_drop)
{
    if (!AssertAudioType("Game.SetAudioTypeSpeechVolumeDrop", audio_type))
        return;

    Debug::Printf("Game.SetAudioTypeSpeechVolumeDrop: type: %d, drop: %d", audio_type, volume_drop);
    game.audioClipTypes[audio_type].volume_reduction_while_speech_playing = volume_drop;
    update_volume_drop_if_voiceover();
}

int Game_GetAudioTypeVolume(int audio_type)
{
    if (!AssertAudioType("Game.GetAudioTypeVolume", audio_type))
        return -1;
    return play.default_audio_type_volumes[audio_type];
}

void Game_SetAudioTypeVolume(int audio_type, int volume, int change_type)
{
    if (!AssertAudioType("Game.SetAudioTypeVolume", audio_type))
        return;
    if ((volume < 0) || (volume > 100))
    {
        debug_script_warn("Game.SetAudioTypeVolume: invalid volume %d, valid range is 0..100", volume);
        volume = Math::Clamp(volume, 0, 100);
    }

    const char *change_str[3]{"existing", "future", "all"};
    Debug::Printf("Game.SetAudioTypeVolume: type: %d, volume: %d, change: %s", audio_type, volume,
                  change_str[change_type - VOL_CHANGEEXISTING]);
    if ((change_type == VOL_CHANGEEXISTING) || (change_type == VOL_BOTH))
    {
        for (int i = 0; i < game.numGameChannels; ++i)
        {
            int playing_type = AudioChannel_GetPlayingType(&scrAudioChannel[i]);
            if (playing_type == audio_type)
            {
                auto *playback = AudioChans::GetChannel(i);
                if (playback)
                {
                    playback->set_volume100(volume);
                }
            }
        }
    }

    if ((change_type == VOL_SETFUTUREDEFAULT) || (change_type == VOL_BOTH))
    {
        play.default_audio_type_volumes[audio_type] = volume;

        // update queued clip volumes
        update_queued_clips_volume(audio_type, volume);
    }
}

//=============================================================================
// Game state, save games
//=============================================================================

int Game_GetDialogCount()
{
    return game.numdialog;
}

void set_debug_mode(bool on)
{
    play.debug_mode = on ? 1 : 0;
}

void set_game_speed(int new_fps)
{
    frames_per_second = new_fps;
    setTimerFps(new_fps, isTimerFpsMaxed()); // keep the current maxed mode (on/off)
}

float get_game_speed()
{
    return frames_per_second;
}

// The cursor look from before setup_for_dialog
int oldmouse;
void setup_for_dialog(int use_font)
{
    cbuttfont = use_font;
    acdialog_font = use_font;
    oldmouse = cur_cursor;
    set_cursor_look(game.GetModePointer());
}
void restore_after_dialog()
{
    set_cursor_look(oldmouse);
    invalidate_screen();
}


String get_save_game_directory()
{
    return saveGameDirectory;
}

String get_save_game_suffix()
{
    return saveGameSuffix;
}

void set_save_game_suffix(const String &suffix)
{
    saveGameSuffix = suffix;
}

String get_save_game_filename(int slot)
{
    return String::FromFormat("agssave.%03d%s", slot, saveGameSuffix.GetCStr());
}

String get_save_game_path(int slot)
{
    return Path::ConcatPaths(saveGameDirectory, get_save_game_filename(slot));
}

bool get_save_filepath_and_slot(const String &input_savepath, String &filepath, int &slot)
{
    if (StrUtil::StringToInt(input_savepath, slot, -1) == StrUtil::kNoError)
    {
        filepath = get_save_game_path(slot);
        return true;
    }

    const String filename = Path::GetFilename(input_savepath);
    if (filename.CompareLeftNoCase("agssave.") == 0)
    {
        if (sscanf(filename.GetCStr(), "agssave.%03d", &slot) == 1)
        {
            if (Path::IsOnlyFilename(input_savepath) && !File::IsFile(input_savepath))
            {
                filepath = Path::ConcatPaths(saveGameDirectory, input_savepath);
            }
            else
            {
                filepath = input_savepath;
            }
            return true;
        }
    }
    return false;
}

// Convert a path possibly containing path tags into acceptable save path
// NOTE that the game script may issue an order to change the save directory to
// a dir of a new name. While we let this work, we also try to keep these
// inside same parent location, would that be a common system directory,
// or a custom one set by a player in config.
static bool MakeSaveGameDir(const String &new_dir, FSLocation &fs_dir)
{
    fs_dir = FSLocation();
    // don't allow absolute paths
    if (!Path::IsRelativePath(new_dir))
        return false;

    String fixed_new_dir = new_dir;
    if (new_dir.CompareLeft(UserSavedgamesRootToken, UserSavedgamesRootToken.GetLength()) == 0)
    {
        fixed_new_dir.ClipLeft(UserSavedgamesRootToken.GetLength());
    }
    else
    {
        debug_script_warn("Attempt to explicitly set savegame location relative to the game installation directory "
                          "('%s') denied;\nPath will be remapped to the user documents directory: '%s'",
                          new_dir.GetCStr(), fs_dir.FullDir.GetCStr());
    }

    // Resolve the new dir relative to the user data parent dir
    fs_dir = GetGameUserDataDir().Concat(fixed_new_dir);
    return true;
}

static bool SetSaveGameDirectory(const FSLocation &fs_dir)
{
    if (!Directory::CreateAllDirectories(fs_dir.BaseDir, fs_dir.SubDir))
    {
        debug_script_warn("SetSaveGameDirectory: failed to create all subdirectories: %s", fs_dir.FullDir.GetCStr());
        return false;
    }

    String new_save_dir = fs_dir.FullDir;
    if (!File::TestCreateFile(Path::ConcatPaths(new_save_dir, "agstmp.tmp")))
        return false;

    // copy the Restart Game file, if applicable
    String old_restart_path =
        Path::ConcatPaths(saveGameDirectory, get_save_game_filename(RESTART_POINT_SAVE_GAME_NUMBER));
    if (File::IsFile(old_restart_path))
    {
        String new_restart_path =
            Path::ConcatPaths(new_save_dir, get_save_game_filename(RESTART_POINT_SAVE_GAME_NUMBER));
        File::CopyFile(old_restart_path, new_restart_path, true);
    }
    saveGameDirectory = new_save_dir;
    return true;
}

void SetDefaultSaveDirectory()
{
    // Request a default save location, and assign it as a save dir
    FSLocation fs_dir = GetGameUserDataDir();
    SetSaveGameDirectory(fs_dir);
}

int Game_SetSaveGameDirectory(const char *new_folder)
{
    // First resolve the script path (it may contain tokens)
    FSLocation fs_dir;
    if (!MakeSaveGameDir(new_folder, fs_dir))
        return 0;
    // If resolved successfully, try to assign the new dir
    return SetSaveGameDirectory(fs_dir) ? 1 : 0;
}

const char *Game_GetSaveSlotDescription(int slot)
{
    String description;
    if (read_savedgame_description(get_save_game_path(slot), description))
    {
        return CreateNewScriptString(description);
    }
    return nullptr;
}

ScriptDateTime *Game_GetSaveSlotTime(int slot)
{
    time_t file_time = File::GetFileTime(get_save_game_path(slot));
    ScriptDateTime *date_time = ScriptDateTime::FromStdTime(file_time);
    ccRegisterManagedObject(date_time, date_time);
    return date_time;
}

void restore_game_dialog()
{
    restore_game_dialog2(1, LEGACY_TOP_BUILTINDIALOGSAVESLOT);
}

void restore_game_dialog2(int min_slot, int max_slot)
{
    EngineServices &services = EngineServices::Current();
    // Optionally override the max slot
    max_slot = services.Config.Override.MaxSaveSlot > 0 ? services.Config.Override.MaxSaveSlot : max_slot;

    can_run_delayed_command();
    if (is_inside_script())
    {
        get_executingscript()->QueueAction(PostScriptAction(ePSARestoreGameDialog,
                                                            (min_slot & 0xFFFF) | (max_slot & 0xFFFF) << 16,
                                                            play.normal_font, "RestoreGameDialog"));
        return;
    }
    do_restore_game_dialog(min_slot, max_slot, play.normal_font);
}

bool do_restore_game_dialog(int min_slot, int max_slot, int use_font)
{
    setup_for_dialog(use_font);
    int load_slot = loadgamedialog(min_slot, max_slot);
    restore_after_dialog();
    if (load_slot >= 0)
        try_restore_save(load_slot);
    return load_slot >= 0;
}

void save_game_dialog()
{
    save_game_dialog2(1, LEGACY_TOP_BUILTINDIALOGSAVESLOT);
}

void save_game_dialog2(int min_slot, int max_slot)
{
    EngineServices &services = EngineServices::Current();
    // Optionally override the max slot
    max_slot = services.Config.Override.MaxSaveSlot > 0 ? services.Config.Override.MaxSaveSlot : max_slot;

    can_run_delayed_command();
    if (is_inside_script())
    {
        get_executingscript()->QueueAction(PostScriptAction(
            ePSASaveGameDialog, (min_slot & 0xFFFF) | (max_slot & 0xFFFF) << 16, play.normal_font, "SaveGameDialog"));
        return;
    }
    do_save_game_dialog(min_slot, max_slot, play.normal_font);
}

bool do_save_game_dialog(int min_slot, int max_slot, int use_font)
{
    setup_for_dialog(use_font);
    int save_slot = savegamedialog(min_slot, max_slot);
    restore_after_dialog();
    if (save_slot >= 0)
        save_game(save_slot, get_gui_dialog_buffer());
    return save_slot >= 0;
}

void free_do_once_tokens()
{
    play.do_once_tokens.clear();
}

void shutdown_game_state()
{
    // Try every possible game state that is represented by a global object
    shutdown_dialog_state();
    ShutGameWaitState();
}

// Free all the memory associated with the game
void unload_game()
{
    dispose_game_drawdata();
    // NOTE: fonts should be freed prior to stopping plugins,
    // as plugins may provide font renderer interface.
    free_all_fonts();
    close_translation();

    // NOTE: script objects must be freed prior to stopping plugins,
    // in case there are managed objects provided by plugins.
    ccRemoveAllSymbols();
    ccUnregisterAllObjects();
    pl_stop_plugins();

    FreeAllScripts();
    ShutdownScriptExec();

    charextra.clear();
    movelists.clear();
    movelist_free_ids = std::queue<uint32_t>();
    views.clear();
    splipsync.clear();
    numLipLines = 0;
    curLipLine = -1;

    dialog.clear();
    scrDialog.clear();

    guis.clear();
    scrGui.clear();

    get_overlays().Clear();

    ResetRoomStates();

    dispose_room_pathfinder();

    // Free game state and game struct
    play = GamePlayState();
    game = GameSetupStruct();

    // Reset all resource caches
    // IMPORTANT: this is hard reset, including locked items
    spriteset.Reset();
    soundcache_clear();
}

int Game_GetInventoryCursorHotspotGraphic()
{
    return game.inv_hot_sprite;
}

void Game_SetInventoryCursorHotspotGraphic(int graphic)
{
    if (game.inv_hot_sprite != graphic)
    {
        game.inv_hot_sprite = graphic;
        // Update current "use inv" cursor if necessary
        if (is_current_cursor_look(kCursorRole_UseInv))
            set_cursor_look(cur_cursor, true);
    }
}

int Game_GetInventoryCursorHotspotColor()
{
    return game.inv_hot_color;
}

void Game_SetInventoryCursorHotspotColor(int color)
{
    if (game.inv_hot_color != color)
    {
        game.inv_hot_color = color;
        if (is_current_cursor_look(kCursorRole_UseInv))
            set_cursor_look(cur_cursor, true);
    }
}

int Game_GetInventoryCursorHotspotCrossColor()
{
    return game.inv_hot_cross_color;
}

void Game_SetInventoryCursorHotspotCrossColor(int color)
{
    if (game.inv_hot_cross_color != color)
    {
        game.inv_hot_cross_color = color;
        if (is_current_cursor_look(kCursorRole_UseInv))
            set_cursor_look(cur_cursor, true);
    }
}

int Game_GetInventoryItemCount()
{
    return game.numinvitems;
}

bool Game_GetIsPaused()
{
    return game_paused > 0;
}

int Game_GetFontCount()
{
    return game.numfonts;
}

int Game_GetMouseCursorCount()
{
    return game.numcursors;
}

int Game_GetCharacterCount()
{
    return game.numcharacters;
}

int Game_GetGUICount()
{
    return game.numgui;
}

int Game_GetViewCount()
{
    return game.numviews;
}

bool Game_GetUseActiveInventoryGraphicForCursor()
{
    return game.options[OPT_FIXEDINVCURSOR] == 0; // OPT_FIXEDINVCURSOR is inverse
}

void Game_SetUseActiveInventoryGraphicForCursor(bool use_inv_graphic)
{
    game.options[OPT_FIXEDINVCURSOR] = use_inv_graphic ? 0 : 1; // OPT_FIXEDINVCURSOR is inverse
    // Update current "use inv" cursor if necessary
    if (is_current_cursor_look(kCursorRole_UseInv) && (playerchar->activeinv >= 0))
    {
        update_inv_cursor(playerchar->activeinv);
        set_cursor_look(cur_cursor);
    }
}

int Game_GetTopSpriteNumber()
{
    return spriteset.GetTopmostSprite();
}

int Game_GetSpriteWidth(int sprite_num)
{
    if (!spriteset.DoesSpriteExist(sprite_num))
        return 0;

    return game.SpriteInfos[sprite_num].Width;
}

int Game_GetSpriteHeight(int sprite_num)
{
    if (!spriteset.DoesSpriteExist(sprite_num))
        return 0;

    return game.SpriteInfos[sprite_num].Height;
}

int Game_GetSpriteDepth(int sprite_num)
{
    if (!spriteset.DoesSpriteExist(sprite_num))
        return 0;

    // If a sprite's color depth information is missing, then assume it is eq to a game's color depth
    return game.SpriteInfos[sprite_num].ColorDepth > 0 ? game.SpriteInfos[sprite_num].ColorDepth : game.GetColorDepth();
}

bool Game_GetIsSpriteDynamic(int sprite_num)
{
    if (sprite_num < 0)
        return false;

    if (!spriteset.DoesSpriteExist(sprite_num))
        return false;

    return game.SpriteInfos[sprite_num].IsDynamicSprite();
}

ScriptFileSortStyle ValidateFileSort(const char *api_name, int file_sort)
{
    if (file_sort < kScFileSort_None || file_sort > kScFileSort_Time)
    {
        debug_script_warn("%s: invalid file sort style (%d)", api_name, file_sort);
        return kScFileSort_None;
    }
    return static_cast<ScriptFileSortStyle>(file_sort);
}

ScriptSaveGameSortStyle ValidateSaveGameSort(const char *api_name, int save_sort)
{
    if (save_sort < kScSaveGameSort_None || save_sort > kScSaveGameSort_Description)
    {
        debug_script_warn("%s: invalid save game sort style (%d)", api_name, save_sort);
        return kScSaveGameSort_None;
    }
    return static_cast<ScriptSaveGameSortStyle>(save_sort);
}

ScriptSortStyle ValidateSortStyle(const char *api_name, int sort_style)
{
    if (sort_style < kScNotSorted || sort_style > kScSorted)
    {
        debug_script_warn("%s: invalid sort style (%d)", api_name, sort_style);
        return kScNotSorted;
    }
    return static_cast<ScriptSortStyle>(sort_style);
}

ScriptSortDirection ValidateSortDirection(const char *api_name, int sort_dir)
{
    if (sort_dir < kScSortNone || sort_dir > kScSortDescending)
    {
        debug_script_warn("%s: invalid sorting direction (%d)", api_name, sort_dir);
        return kScSortNone;
    }
    return static_cast<ScriptSortDirection>(sort_dir);
}

ScriptStringComparison ValidateStringComparison(const char *api_name, int compare_style)
{
    if (compare_style < kScCaseInsensitive || compare_style > kScCaseSensitiveLocaleAware)
    {
        debug_script_warn("%s: invalid case sensitivity (%d)", api_name, compare_style);
        return kScCaseInsensitive;
    }
    return static_cast<ScriptStringComparison>(compare_style);
}

bool ValidateSaveSlotRange(const char *api_name, int &min_slot, int &max_slot)
{
    int do_max_slot = std::min(max_slot, TOP_SAVESLOT);
    int do_min_slot = std::min(do_max_slot, std::max(0, min_slot));
    if (do_max_slot - do_min_slot + 1 <= 0)
    {
        debug_script_warn("%s: empty or invalid slots range requested (requested: %d..%d, valid range %d..%d)",
                          api_name, min_slot, max_slot, 0, TOP_SAVESLOT);
        return false;
    }
    min_slot = do_min_slot;
    max_slot = do_max_slot;
    return true;
}

GraphicFlip ValidateFlip(const char *api_name, int flip)
{
    if ((flip < kFlip_None) || (flip > kFlip_Both))
    {
        debug_script_warn("%s: invalid flip parameter (%d)", api_name, flip);
        return kFlip_None;
    }
    return static_cast<GraphicFlip>(flip);
}

void AssertView(const char *api_name, const char *obj_name, int view)
{
    // NOTE: we assume (here and below) that the view is already in an internal 0-based range.
    // but when printing an error we will use (view + 1) for compliance with the script API.
    if ((view < 0) || (view >= game.numviews))
        quitprintf("!%s (%s): invalid view %d (range is 1..%d)", api_name, obj_name, view + 1, game.numviews);
}

void AssertViewHasLoops(const char *api_name, const char *obj_name, int view)
{
    AssertView(api_name, obj_name, view);
    if (views[view].numLoops == 0)
        quitprintf("!%s (%s): view %d does not have any loops.", api_name, obj_name, view + 1);
}

void AssertLoop(const char *api_name, const char *obj_name, int view, int loop)
{
    AssertViewHasLoops(api_name, obj_name, view);
    if ((loop < 0) || (loop >= views[view].numLoops))
        quitprintf("!%s (%s): invalid loop number %d for view %d (range is 0..%d).", api_name, obj_name, loop, view + 1,
                   views[view].GetLastLoop());
}

void AssertFrame(const char *api_name, const char *obj_name, int view, int loop, int frame)
{
    AssertLoop(api_name, obj_name, view, loop);
    if (views[view].loops[loop].numFrames == 0)
        quitprintf("!%s (%s): view %d loop %d does not have any frames", api_name, obj_name, view + 1, loop);
    if ((frame < 0) || (frame >= views[view].loops[loop].numFrames))
        quitprintf("!%s (%s): invalid frame number %d for view %d loop %d (range is 0..%d)", api_name, obj_name, frame,
                   view + 1, loop, views[view].loops[loop].GetLastFrame());
}

int Game_GetLoopCountForView(int view)
{
    view--; // convert to 0-based
    AssertView("Game.GetLoopCountForView", "", view);
    return views[view].numLoops;
}

int Game_GetRunNextSettingForLoop(int view, int loop)
{
    view--; // convert to 0-based
    AssertLoop("Game.GetRunNextSettingForLoop", "", view, loop);
    return (views[view].loops[loop].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int view, int loop)
{
    view--; // convert to 0-based
    AssertLoop("Game.GetFrameCountForLoop", "", view, loop);
    return views[view].loops[loop].numFrames;
}

ScriptViewFrame *Game_GetViewFrame(int view, int loop, int frame)
{
    view--; // convert to 0-based
    AssertFrame("Game.GetViewFrame", "", view, loop, frame);
    ScriptViewFrame *sdt = new ScriptViewFrame(view, loop, frame);
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int Game_DoOnceOnly(const char *token)
{
    if (play.do_once_tokens.count(String::Wrapper(token)) > 0)
        return 0;
    play.do_once_tokens.insert(token);
    return 1;
}

void Game_ResetDoOnceOnly()
{
    free_do_once_tokens();
}

int Game_GetTextReadingSpeed()
{
    return play.text_speed;
}

void Game_SetTextReadingSpeed(int new_text_speed)
{
    if (new_text_speed < 1)
        quitprintf("!Game.TextReadingSpeed: %d is an invalid speed", new_text_speed);

    if (EngineServices::Current().Config.Access.TextReadSpeed <= 0)
        play.text_speed = new_text_speed;
}

int Game_GetTickCounter()
{
    // Loop counter is uint32, but the script values are int32;
    // convert uint32 value into a int32 wrapped in the range of 0 - INT32_MAX.
    uint32_t loop_counter = get_loop_counter();
    return loop_counter <= INT32_MAX ? static_cast<int32_t>(loop_counter)
                                     : static_cast<int32_t>(loop_counter - INT32_MAX);
}

int Game_GetMinimumTextDisplayTimeMs()
{
    return play.text_min_display_time_ms;
}

void Game_SetMinimumTextDisplayTimeMs(int new_text_min_time)
{
    if (EngineServices::Current().Config.Access.TextReadSpeed <= 0)
        play.text_min_display_time_ms = new_text_min_time;
}

int Game_GetIgnoreUserInputAfterTextTimeoutMs()
{
    return play.ignore_user_input_after_text_timeout_ms;
}

void Game_SetIgnoreUserInputAfterTextTimeoutMs(int new_value_ms)
{
    play.ignore_user_input_after_text_timeout_ms = new_value_ms;
}

const char *Game_GetFileName()
{
    return CreateNewScriptString(EngineServices::Current().ResPaths.GamePak.Name);
}

const char *Game_GetName()
{
    return CreateNewScriptString(play.game_name);
}

void Game_SetName(const char *new_name)
{
    play.game_name = new_name;
    EngineServices::Current().Platform->SetWindowTitle(play.game_name);
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Gamename);
}

int Game_GetSkippingCutscene()
{
    if (play.fast_forward)
    {
        return 1;
    }
    return 0;
}

int Game_GetInSkippableCutscene()
{
    if (play.in_cutscene)
    {
        return 1;
    }
    return 0;
}

int Game_GetColorFromRGBA(int red, int green, int blue, int alpha)
{
    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) || (blue < 0) || (blue > 255) || (alpha < 0) ||
        (alpha > 255))
    {
        debug_script_warn("GetColorFromRGB: colour values must be 0-255");
        red = Math::Clamp(red, 0, 255);
        green = Math::Clamp(green, 0, 255);
        blue = Math::Clamp(blue, 0, 255);
        alpha = Math::Clamp(alpha, 0, 255);
    }

    if (game.color_depth == 1)
    {
        return makecol8(red, green, blue);
    }

    return makeacol32(red, green, blue, alpha);
}

int Game_GetColorFromRGB(int red, int green, int blue)
{
    return Game_GetColorFromRGBA(red, green, blue, 255);
}

const char *Game_InputBox(const char *msg)
{
    String input = ShowInputBoxImpl(msg);
    return CreateNewScriptString(input);
}

const char *Game_GetLocationName(int x, int y, int hit_options)
{
    return CreateNewScriptString(GetLocationName(x, y, hit_options));
}

const char *Game_GetLocationName2(int x, int y)
{
    return Game_GetLocationName(x, y, kHit_Interactable);
}

int Game_GetSpeechFont()
{
    return play.speech_font;
}

int Game_GetNormalFont()
{
    return play.normal_font;
}

void Game_SetSpeechFont(int font)
{
    font = ValidateFontNumber("SetSpeechFont", font);
    play.speech_font = font;
}

void Game_SetNormalFont(int font)
{
    font = ValidateFontNumber("SetNormalFont", font);
    play.normal_font = font;
}

int Game_GetSpeed()
{
    return GetGameSpeed();
}

void Game_SetSpeed(int speed)
{
    SetGameSpeed(speed);
}

const char *Game_GetTranslationFilename()
{
    return CreateNewScriptString(get_translation_name());
}

int Game_ChangeTranslation(const char *new_filename)
{
    EngineServices &services = EngineServices::Current();
    if ((new_filename == nullptr) || (new_filename[0] == 0))
    { // switch back to default translation
        close_translation();
        services.Config.Translation = "";
        GUIE::MarkForTranslationUpdate();
        return 1;
    }

    String old_translation_name = get_translation_name();
    if (!init_translation(new_filename, old_translation_name))
        return 0; // failed, kept previous translation

    if (game.GameInfo.count("dev_error_text") > 0)
        play.SetGameErrorText(get_translation(game.GameInfo["dev_error_text"].GetCStr()));
    services.Config.Translation = new_filename;
    GUIE::MarkForTranslationUpdate();
    return 1;
}

const char *Game_GetSpeechVoxFilename()
{
    return CreateNewScriptString(get_voicepak_name());
}

bool Game_ChangeSpeechVox(const char *new_filename)
{
    play.voice_avail = init_voicepak(new_filename);
    if (!play.voice_avail)
    {
        // if failed (and was not default)- fallback to default
        if (strlen(new_filename) > 0)
            play.voice_avail = init_voicepak();
        return false;
    }
    return true;
}

int Game_GetAudioClipCount()
{
    return game.audioClips.size();
}

ScriptAudioClip *Game_GetAudioClip(int index)
{
    if (index < 0 || (size_t)index >= game.audioClips.size())
        return nullptr;
    return &game.audioClips[index];
}

ScriptCamera *Game_GetCamera()
{
    return play.GetScriptCamera(0);
}

int Game_GetCameraCount()
{
    return play.GetRoomCameraCount();
}

ScriptCamera *Game_GetAnyCamera(int index)
{
    return play.GetScriptCamera(index);
}

void Game_SimulateKeyPress(int key, int mod)
{
    eAGSKeyCode mod_key = eAGSKeyCodeNone;
    eAGSKeyMod mod_ex = eAGSModNone;
    // Support combo-keys, split them into key + mod and pass as separate events.
    // If game is running in the old-key mode, then they will become re-combined again on receival.
    if (key >= eAGSKeyCodeCtrlA && key <= eAGSKeyCodeCtrlZ)
    {
        key = key - eAGSKeyCodeCtrlA + eAGSKeyCodeA;
        mod_key = eAGSKeyCodeLCtrl;
        mod_ex = eAGSModCtrl;
    }
    else if (key >= eAGSKeyCodeAltA && key <= eAGSKeyCodeAltZ)
    {
        key = AGS_EXT_KEY_TOALPHA(key);
        mod_key = eAGSKeyCodeLAlt;
        mod_ex = eAGSModAlt;
    }

    if (mod_key > 0)
    {
        ags_simulate_keydown(mod_key);
        ags_simulate_keypress(static_cast<eAGSKeyCode>(key), static_cast<eAGSKeyMod>(mod | mod_ex));
        ags_simulate_keyup(mod_key);
    }
    else
    {
        ags_simulate_keypress(static_cast<eAGSKeyCode>(key), static_cast<eAGSKeyMod>(mod));
    }
}

void Game_SimulateKeyPressOld(int key)
{
    Game_SimulateKeyPress(key, 0);
}

int Game_BlockingWaitCounter()
{
    return play.GetWaitCounter();
}

int Game_BlockingWaitSkipped()
{
    return play.GetWaitSkipResult();
}

int Game_BlockingWaitSkipType()
{
    // Convert to the InputType flags
    return play.GetWaitSkipType() << SKIP_RESULT_TYPE_SHIFT;
}

int Game_InBlockingAction()
{
    return IsInBlockingAction();
}

int Game_InBlockingWait()
{
    // A design mistake in 3.6.2: IsInBlockingWait name was used to report any blocking action state
    return game.options[OPT_BASESCRIPTAPI] >= kScriptAPI_v363 ? play.IsInWait() : IsInBlockingAction();
}

void Game_Pause()
{
    PauseGame();
}

void Game_Resume()
{
    UnPauseGame();
}

ScriptAudioChannel *Game_PlayVoiceClip(CharacterInfo *ch, int sound_id, bool as_speech)
{
    if (!play_voice_nonblocking(ch ? ch->index_id : -1 /* narrator */, sound_id, as_speech))
        return nullptr;
    return &scrAudioChannel[SCHAN_SPEECH];
}

ScriptAudioChannel *Game_PlayVoiceClipAsType(CharacterInfo *ch, int sound_id, int audio_type, int chan_id, int priority,
                                             int repeat)
{
    if (audio_type <= AUDIOTYPE_SPEECH || static_cast<uint32_t>(audio_type) >= game.audioClipTypes.size())
        quitprintf("!AudioClip.PlayAsType: invalid audio type %d, the range is %u - %u", audio_type,
                   AUDIOTYPE_SPEECH + 1, static_cast<uint32_t>(game.audioClipTypes.size() - 1));
    if (chan_id != SCR_NO_VALUE && (chan_id < NUM_SPEECH_CHANS || chan_id >= game.numGameChannels))
        quitprintf("!AudioClip.PlayAsType: invalid channel %d, the range is %d - %d", chan_id, NUM_SPEECH_CHANS,
                   game.numGameChannels - 1);
    if (chan_id == SCR_NO_VALUE)
        chan_id = -1;

    if (!play_voice_clip_as_type(ch ? ch->index_id : -1 /* narrator */, sound_id, audio_type, chan_id, priority,
                                 repeat))
        return nullptr;
    return &scrAudioChannel[SCHAN_SPEECH];
}

void Game_PrecacheSprite(int sprite_num)
{
    const auto tp_start = FastClock::now();
    spriteset.PrecacheSprite(sprite_num);
    const auto tp_filedone = FastClock::now();
    texturecache_precache(sprite_num);
    const auto tp_texturedone = FastClock::now();

    const auto dur1 = ToMilliseconds(tp_filedone - tp_start);
    const auto dur2 = ToMilliseconds(tp_texturedone - tp_filedone);
    const auto dur_t = ToMilliseconds(tp_texturedone - tp_start);
    Debug::Printf("Precache sprite %d; file->mem = %" PRId64 " ms, bm->tx = %" PRId64 " ms, total = %" PRId64 " ms",
                  sprite_num, dur1, dur2, dur_t);
}

void Game_PrecacheView(int view, int first_loop, int last_loop)
{
    precache_view(view - 1 /* to 0-based view index */, first_loop, last_loop, true);
}

float Game_GetFaceDirectionRatio()
{
    return play.face_dir_ratio;
}

void Game_SetFaceDirectionRatio(float ratio)
{
    play.face_dir_ratio = ratio;
}

int Game_GetRoomCount()
{
    return game.roomNumbers.size();
}

int Game_GetRoomNumber(int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= game.roomNumbers.size())
        return -1;
    return game.roomNumbers[index];
}

const char *Game_GetRoomName(int room_number)
{
    auto it_room = game.roomNames.find(room_number);
    if (it_room == game.roomNames.end())
        return nullptr;

    return CreateNewScriptString(it_room->second);
}

void *Game_GetSaveSlots(int min_slot, int max_slot, int save_sort, int sort_dir)
{
    if (!ValidateSaveSlotRange("Game.GetSaveSlots", min_slot, max_slot))
        return CCDynamicArray::CreateOld(0, sizeof(int32_t), false).Obj();
    save_sort = ValidateSaveGameSort("Game.GetSaveSlots", save_sort);
    sort_dir = ValidateSortDirection("Game.GetSaveSlots", sort_dir);

    std::vector<SaveListItem> saves;
    FillSaveList(saves, min_slot, max_slot, false /* no desc */, (ScriptSaveGameSortStyle)save_sort,
                 (ScriptSortDirection)sort_dir);

    DynObjectRef arr = CCDynamicArray::CreateOld(saves.size(), sizeof(int32_t), false);
    int32_t *arr_ptr = static_cast<int32_t *>(arr.Obj());
    for (const auto &save : saves)
        *(arr_ptr++) = save.Slot;
    return arr.Obj();
}

void prescan_saves(int *dest_arr, size_t dest_count, int min_slot, int max_slot, int file_sort, int sort_dir);

void Game_ScanSaveSlots(void *dest_arr, int min_slot, int max_slot, int save_sort, int sort_dir, int user_param)
{
    const auto &array_header = CCDynamicArray::GetHeader(dest_arr);
    if (array_header.GetElemCount() == 0u)
    {
        debug_script_warn("Game.ScanSaveSlots: empty array provided, skip execution");
        return;
    }

    if (!ValidateSaveSlotRange("Game.ScanSaveSlots", min_slot, max_slot))
        return;
    save_sort = ValidateSaveGameSort("Game.ScanSaveSlots", save_sort);
    sort_dir = ValidateSortDirection("Game.ScanSaveSlots", sort_dir);

    can_run_delayed_command();
    if (is_inside_script())
    {
        int handle = ccGetObjectHandleFromAddress(dest_arr);
        ccAddObjectReference(handle); // add internal handle to prevent disposal
        get_executingscript()->QueueAction(PostScriptAction(ePSAScanSaves, handle, min_slot, max_slot, save_sort,
                                                            sort_dir, user_param, "ScanSaveSlots"));
        return;
    }

    prescan_saves(static_cast<int *>(dest_arr), array_header.GetElemCount(), min_slot, max_slot, save_sort, sort_dir);
}

//=============================================================================

std::unique_ptr<Common::Bitmap> create_game_screenshot(int width, int height, int layers)
{
    const Rect &viewport = play.GetMainViewport();
    if (width <= 0)
        width = viewport.GetWidth();
    if (height <= 0)
        height = viewport.GetHeight();

    // NOTE: if there will be a difference between script constants and internal
    // constants of Render Layers, or any necessity to adjust these, then convert flags here.
    std::unique_ptr<Bitmap> shot;
    if (layers != 0)
        shot.reset(CopyScreenIntoBitmap(width, height, &viewport, false, ~layers));
    else
        shot.reset(new Bitmap(width, height));
    return shot;
}

static std::unique_ptr<Common::Bitmap> create_savegame_screenshot()
{
    return create_game_screenshot(play.screenshot_width, play.screenshot_height, game.options[OPT_SAVESCREENSHOTLAYER]);
}

void save_game(int slot, const String &description, std::unique_ptr<Bitmap> &&image)
{
    pl_run_plugin_hooks(kPluginEvt_PreSaveGame, 0);

    String save_path = get_save_game_path(slot);
    if (!image && (game.options[OPT_SAVESCREENSHOT] != 0))
        image = create_savegame_screenshot();

    HSaveError err = SaveGame(
        save_path, description, image.get(),
        (SaveCmpSelection)(kSaveCmp_All & ~(game.options[OPT_SAVECOMPONENTSIGNORE] & kSaveCmp_ScriptIgnoreMask)),
        EngineServices::Current().Config.CompressSaves);
    if (!err)
    {
        // FIXME: on a failed save the player gets this hardcoded English message and the script gets no event, so a
        // game can neither translate nor replace it. Either pass the error code to kScriptEvent_GameSaved, or have a
        // separate event for errors.
        Display("ERROR: Unable to open savegame file for writing!");
        Debug::Printf(kDbgMsg_Error, "Save game failed: %s", err->FullMessage().GetCStr());
        return;
    }

    // call "After Save" event callback
    run_on_event(kScriptEvent_GameSaved, slot);
}

void schedule_save_game(int slot, const String &description, int sprite_num)
{
    // dont allow save in rep_exec_always, because we dont save
    // the state of blocked scripts
    can_run_delayed_command();

    // Make a sprite copy, as save process may be scheduled and asynchronous (in theory)
    std::unique_ptr<Bitmap> image;
    if (sprite_num >= 0)
        image.reset(BitmapHelper::CreateBitmapCopy(spriteset[sprite_num]));

    if (is_inside_script())
    {
        get_executingscript()->QueueAction(
            PostScriptAction(ePSASaveGame, slot, "SaveGameSlot", description, std::move(image)));
        return;
    }

    save_game(slot, description, std::move(image));
}

// Counts the restore attempts, so that a script run that was in progress can tell that one happened (script.cpp)
int gameHasBeenRestored = 0;
// The script position (eip) from before load_game, put back after a successful restore
int oldeip;

bool read_savedgame_description(const String &filename, String &description)
{
    if (!DoesSavegameExist(filename))
    {
        Debug::Printf("Save does not exist: %s", filename.GetCStr());
        return false;
    }

    SavegameDescription desc;
    HSaveError err = ReadSaveDescription(filename, desc, kSvgDesc_UserText);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's description.\n%s", err->FullMessage().GetCStr());
        return false;
    }
    description = desc.UserText;
    return true;
}

std::unique_ptr<Bitmap> read_savedgame_screenshot(const String &filename)
{
    if (!DoesSavegameExist(filename))
    {
        Debug::Printf("Save does not exist: %s", filename.GetCStr());
        return {};
    }

    SavegameDescription desc;
    HSaveError err = ReadSaveDescription(filename, desc, kSvgDesc_UserImage);
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Unable to read save's screenshot.\n%s", err->FullMessage().GetCStr());
        return {};
    }
    if (desc.UserImage)
    {
        desc.UserImage.reset(
            PrepareSpriteForUse(desc.UserImage.release(), true /* to game depth */, true /* force opaque */));
        return std::move(desc.UserImage);
    }
    return {};
}


// Test if the game file contains expected GUID / legacy id
bool test_game_guid(const String &filepath, const String &guid, int legacy_id)
{
    std::unique_ptr<AssetManager> asset_mgr(new AssetManager());
    if (asset_mgr->AddLibrary(filepath) != kAssetNoError)
        return false;
    MainGameSource src;
    if (!OpenMainGameFileFromDefaultAsset(src, asset_mgr.get()))
        return false;
    GameSetupStruct loaded_game;
    PreReadGameData(loaded_game, std::move(src.InputStream), src.DataVersion, src.CompiledWith);
    if (!guid.IsEmpty())
        return guid.CompareNoCase(loaded_game.guid) == 0;
    return legacy_id == loaded_game.uniqueid;
}

// Restores the game from the save file. If the save belongs to another game (by GUID, or by the main file name when
// there is none), schedules running that game and restoring the save there, and returns no error. Sets
// data_overwritten once RestoreSavegame has run, after which an error means the game state is lost.
HSaveError load_game(const String &path, int slot, bool startup, bool &data_overwritten)
{
    EngineServices &services = EngineServices::Current();
    data_overwritten = false;
    gameHasBeenRestored++;

    oldeip = get_our_eip();
    set_our_eip(2050);

    SavegameDescription desc;
    desc.Slot = slot;
    HSaveError err = ReadSaveDescription(path, desc, (SavegameDescElem)(kSvgDesc_EnvInfo | kSvgDesc_UserText));
    // saved in incompatible environment
    if (!err)
        return err;
    // The bitmaps in a save (dynamic sprites, room backgrounds, drawing surfaces) are read at the color depth they were
    // saved with, so a save made at another depth is refused.
    // TODO: convert the bitmaps while restoring, if saves must survive a change of the game's color depth
    else if (desc.ColorDepth != game.GetColorDepth())
        return new SavegameError(kSvgErr_DifferentColorDepth, "Running: %d-bit, saved in: %d-bit.",
                                 game.GetColorDepth(), desc.ColorDepth);

    // Match game GUIDs and test whether save was made for the different game
    if (!desc.GameGuid.IsEmpty() || desc.LegacyID != 0)
    {
        if (desc.GameGuid.Compare(game.guid) != 0 && desc.LegacyID != game.uniqueid)
        {
            // Try to find wanted game's data using game id
            String game_file = FindGameData(services.ResPaths.DataDir, [&desc](const String &filepath)
                                            { return test_game_guid(filepath, desc.GameGuid, desc.LegacyID); });
            if (!game_file.IsEmpty())
            {
                RunAGSGame(game_file.GetCStr(), 0, 0);
                load_new_game_restore = slot;
                return HSaveError::None();
            }
            return new SavegameError(kSvgErr_GameGuidMismatch);
        }
    }
    // A save without a game GUID and legacy ID: match the game data by its file name. Saves before
    // 3.5.1 had neither (they are refused now); a 4.0 save has none only if its game has none.
    // TODO: remove the filename test if every game is given a GUID
    else if (desc.MainDataFilename.Compare(services.ResPaths.GamePak.Name))
    {
        String game_file = Path::ConcatPaths(services.ResPaths.DataDir, desc.MainDataFilename);
        if (IsMainGameLibrary(game_file))
        {
            RunAGSGame(desc.MainDataFilename.GetCStr(), 0, 0);
            load_new_game_restore = slot;
            return HSaveError::None();
        }
        // if it does not exist, continue loading savedgame in current game, and pray for the best
        Common::Debug::Printf(kDbgMsg_Warn,
                              "WARNING: the saved game '%s' references game file '%s' (title: '%s'), but it cannot be "
                              "found in the current directory. Trying to restore in the running game instead.",
                              path.GetCStr(), desc.MainDataFilename.GetCStr(), desc.GameTitle.GetCStr());
    }

    // Do the actual game state restore
    SaveRestoreFeedback feedback;
    err = RestoreSavegame(
        path,
        RestoreGameStateOptions(
            (SaveCmpSelection)(kSaveCmp_All & ~(game.options[OPT_SAVECOMPONENTSIGNORE] & kSaveCmp_ScriptIgnoreMask)),
            startup),
        feedback);
    data_overwritten = true;

    // Handle restoration error
    if (!err)
    {
        // We must detect a case when the save contains less data and requires
        // a clean game reset before trying to restore again
        if (feedback.RetryWithClearGame)
        {
            // Schedule same game file re-run and save restore
            RunAGSGame(services.ResPaths.GamePak.Path, 0, 0);
            load_new_game_restore = slot;
            load_new_game_restore_cmp = feedback.RetryWithoutComponents;
            return HSaveError::None();
        }
        // Else bail out with error
        return err;
    }

    set_our_eip(oldeip);
    // ensure input queue is reset
    ags_clear_input_queue();
    // call "After Restore" event callback
    run_on_event(kScriptEvent_GameRestored, slot);
    return HSaveError::None();
}

bool try_restore_save(int slot, bool startup)
{
    return try_restore_save(get_save_game_path(slot), slot, startup);
}

bool try_restore_save(const Common::String &path, int slot, bool startup)
{
    bool data_overwritten;
    Debug::Printf(kDbgMsg_Info, "Restoring saved game '%s'", path.GetCStr());
    HSaveError err = load_game(path, slot, startup, data_overwritten);
    if (!err)
    {
        String error = String::FromFormat("Unable to restore the saved game.\n%s", err->FullMessage().GetCStr());
        Debug::Printf(kDbgMsg_Error, "%s", error.GetCStr());
        // currently AGS cannot properly revert to stable state if some of the
        // game data was released or overwritten by the data from save file,
        // this is why we tell engine to shutdown if that happened.
        if (data_overwritten)
            quitprintf("%s", error.GetCStr());
        else if (!startup)
            Display("%s", error.GetCStr());
        return false;
    }
    return true;
}

void prescan_save_slots(int dest_arr_handle, int min_slot, int max_slot, int save_sort, int sort_dir, int user_param)
{
    void *dest_arr;
    IScriptObject *manager;
    ccGetObjectAddressAndManagerFromHandle(dest_arr_handle, dest_arr, manager);
    if (!dest_arr || strcmp(manager->GetType(), CCDynamicArray::TypeName) != 0)
    {
        debug_script_warn("Game.PrescanSaveSlots: internal error: destination array not available, has been disposed?");
        return;
    }

    prescan_saves(static_cast<int *>(dest_arr), CCDynamicArray::GetHeader(dest_arr).GetElemCount(), min_slot, max_slot,
                  save_sort, sort_dir);
    ccReleaseObjectReference(dest_arr_handle); // release internal handle

    run_on_event(kScriptEvent_SavesScanComplete, user_param);
}

void prescan_saves(int *dest_arr, size_t dest_count, int min_slot, int max_slot, int save_sort, int sort_dir)
{
    // Gather existing list of saves in the requested range
    // ...and sort this list according to the parameters
    std::vector<SaveListItem> saves;
    FillSaveList(saves, min_slot, max_slot, false /* no desc */, (ScriptSaveGameSortStyle)save_sort,
                 (ScriptSortDirection)sort_dir);

    // Prescan saves from the sorted list, and fill the destination array
    int *dst_ptr = dest_arr;
    for (const auto &save : saves)
    {
        if (dst_ptr == dest_arr + dest_count)
            break;

        HSaveError err;
        SavegameSource src;
        SavegameDescription desc;
        desc.Slot = save.Slot;
        err = OpenSavegame(get_save_game_path(save.Slot), src, desc,
                           (SavegameDescElem)(kSvgDesc_FileFormat | kSvgDesc_EnvInfo | kSvgDesc_UserText));
        if (!err)
        {
            debug_script_log("Prescan save slot %d: failed to open save: %s", save.Slot, err->FullMessage().GetCStr());
            continue;
        }
        // Same test as in load_game: a save made at another color depth cannot be restored
        else if (desc.ColorDepth != game.GetColorDepth())
        {
            debug_script_log("Prescan save slot %d: mismatching game color depth (game: %d-bit, save: %d-bit)",
                             save.Slot, game.GetColorDepth(), desc.ColorDepth);
            continue;
        }

        // Do the save prescan
        err = PrescanSaveState(
            src.InputStream.get(), src.Version, desc,
            RestoreGameStateOptions((SaveCmpSelection)(kSaveCmp_All & ~(game.options[OPT_SAVECOMPONENTSIGNORE] &
                                                                        kSaveCmp_ScriptIgnoreMask)),
                                    false));

        if (!err)
        {
            debug_script_log("Prescan save slot %d: incompatible: %s", save.Slot, err->FullMessage().GetCStr());
            continue;
        }

        // Put valid slot into dest arr
        *(dst_ptr++) = save.Slot;
    }

    // Fill remaining destination array with "no slot" index
    for (; dst_ptr != dest_arr + dest_count; ++dst_ptr)
        *dst_ptr = -1;
}

bool is_in_cutscene()
{
    return play.in_cutscene > 0;
}

CutsceneSkipStyle get_cutscene_skipstyle()
{
    return static_cast<CutsceneSkipStyle>(play.in_cutscene);
}

void start_skipping_cutscene()
{
    play.fast_forward = 1;
    // if a drop-down icon bar is up, remove it as it will pause the game
    if (ifacepopped >= 0)
        remove_popup_interface(ifacepopped);

    // if a text message is currently displayed, remove it
    if (play.text_overlay_on > 0)
    {
        remove_screen_overlay(play.text_overlay_on);
        play.SetWaitSkipResult(SKIP_AUTOTIMER);
    }
}

bool check_skip_cutscene_keypress(int key)
{
    if (IsAGSServiceKey(static_cast<eAGSKeyCode>(key)))
        return false;
    CutsceneSkipStyle skip = get_cutscene_skipstyle();
    if (skip == eSkipSceneAnyKey || skip == eSkipSceneKeyMouse ||
        (key == eAGSKeyCodeEscape && (skip == eSkipSceneEscOnly || skip == eSkipSceneEscOrRMB)))
    {
        start_skipping_cutscene();
        return true;
    }
    return false;
}

bool check_skip_cutscene_mclick(int mouse_button)
{
    CutsceneSkipStyle skip = get_cutscene_skipstyle();
    if (skip == eSkipSceneMouse || skip == eSkipSceneKeyMouse ||
        (mouse_button == kMouseRight && skip == eSkipSceneEscOrRMB))
    {
        start_skipping_cutscene();
        return true;
    }
    return false;
}

bool check_skip_cutscene_gamepad(int gamepad_button)
{
    CutsceneSkipStyle skip = get_cutscene_skipstyle();
    if (gamepad_button > 0 && skip == eSkipSceneAnyKey)
    {
        start_skipping_cutscene();
        return true;
    }
    return false;
}

void initialize_skippable_cutscene()
{
    // TODO: move related variable init here
}

void stop_fast_forwarding()
{
    play.fast_forward = 0;
    setpal();

    // Restore actual volume of sounds
    for (int chan_id = 0; chan_id < TOTAL_AUDIO_CHANNELS; chan_id++)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(chan_id);
        if (playback)
        {
            playback->set_mute(false);
        }
    }
}

int GetLocationTypeImpl(int *locobj_index, int x, int y, int hit_options, bool click_through_gui, bool allow_hotspot0)
{
    if (locobj_index)
        *locobj_index = -1;

    if (!click_through_gui)
    {
        int gui_id = GetGUIAt(x, y, hit_options);
        if (gui_id >= 0)
        {
            // On GUI, test if we're above an inventory item
            int item_id = GetInvAt(x, y, hit_options, hit_options);
            if (item_id >= 0)
            {
                if (locobj_index)
                    *locobj_index = item_id;
                return LOCTYPE_INVITEM;
            }
            return LOCTYPE_NOTHING;
        }
    }

    // Find out if we're inside the room viewport
    const VpPoint vp_point = play.ScreenToRoom(x, y);
    const Point room_pt = vp_point.first;
    const int viewport_id = vp_point.second;
    if ((viewport_id < 0) || (!RectWH(0, 0, thisroom.Width, thisroom.Height).IsInside(room_pt)))
    {
        return LOCTYPE_NOTHING;
    }

    // check characters, objects and walkbehinds, work out which is
    // foremost visible to the player
    x = room_pt.X;
    y = room_pt.Y;
    int char_id = GetCharIDAtRoom(x, y, hit_options);
    int hotspot_id = GetHotspotIDAtRoom(x, y, hit_options);
    int obj_id = GetObjectIDAtRoom(x, y, hit_options);

    int wb_baseline = 0;
    int wb_id = thisroom.WalkBehindMask->GetPixel(x, y);
    if (wb_id > 0 && wb_id < MAX_WALK_BEHINDS) // wb == 0 is a no-area
        wb_baseline = croom->walkbehind_base[wb_id];

    int winner = LOCTYPE_NOTHING;
    // if it's an Ignore Walkbehinds object, then ignore the walkbehind
    if ((obj_id >= 0) && ((objs[obj_id].flags & OBJF_NOWALKBEHINDS) != 0))
        wb_baseline = 0;
    if ((char_id >= 0) && ((game.chars[char_id].flags & CHF_NOWALKBEHINDS) != 0))
        wb_baseline = 0;

    if ((char_id >= 0) && (obj_id >= 0))
    {
        if ((wb_baseline > obj_lowest_yp) && (wb_baseline > char_lowest_yp))
            winner = LOCTYPE_HOTSPOT;
        else if (obj_lowest_yp > char_lowest_yp)
            winner = LOCTYPE_OBJ;
        else
            winner = LOCTYPE_CHAR;
    }
    else if (char_id >= 0)
    {
        if (wb_baseline > char_lowest_yp)
            winner = LOCTYPE_HOTSPOT;
        else
            winner = LOCTYPE_CHAR;
    }
    else if (obj_id >= 0)
    {
        if (wb_baseline > obj_lowest_yp)
            winner = LOCTYPE_HOTSPOT;
        else
            winner = LOCTYPE_OBJ;
    }

    if (winner == 0)
    {
        if (hotspot_id >= 0)
            winner = LOCTYPE_HOTSPOT;
    }

    if ((winner == LOCTYPE_HOTSPOT) && (!allow_hotspot0) && (hotspot_id == 0))
        winner = LOCTYPE_NOTHING;

    if (locobj_index)
    {
        if (winner == LOCTYPE_HOTSPOT)
            *locobj_index = hotspot_id;
        else if (winner == LOCTYPE_CHAR)
            *locobj_index = char_id;
        else if (winner == LOCTYPE_OBJ)
            *locobj_index = obj_id;
    }
    return winner;
}

// Called whenever game loses input focus
void display_switch_out()
{
    Debug::Printf("Switching out from the game");
    EngineServices::Current().SwitchedAway = true;
    ags_clear_input_state();
    // Always unlock mouse when switching out from the game
    Mouse::UnlockFromWindow();
}

// Called when game loses input focus and must pause until focus is returned
void display_switch_out_suspend()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf("Suspending the game on switch out");
    services.SwitchingAwayFromGame++;
    services.GameUpdateSuspend = true;
    display_switch_out();

    services.Platform->PauseApplication();

    video_single_pause();
    // Pause all the sounds
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(i);
        if (playback)
        {
            playback->pause();
        }
    }

    services.SwitchingAwayFromGame--;
}

// Called whenever game gets input focus
void display_switch_in()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf("Switching back into the game");
    ags_clear_input_state();
    // If fullscreen, or auto lock option is set, lock mouse to the game window
    if ((scsystem.windowed == 0) || services.Config.MouseAutoLock)
        Mouse::TryLockToWindow();
    services.SwitchedAway = false;
}

// Called when game gets input focus and must resume after pause
void display_switch_in_resume()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf("Resuming the game on switch in");
    display_switch_in();

    // Resume all the sounds
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(i);
        if (playback)
        {
            playback->resume();
        }
    }
    video_single_resume();

    // release render targets if switching back to the full screen mode;
    // unfortunately, otherwise Direct3D fails to reset device when restoring fullscreen.
    if (services.GfxDriver && services.GfxDriver->GetDisplayMode().IsRealFullscreen())
        release_drawobj_rendertargets();
    // clear the screen if necessary
    if (services.GfxDriver && services.GfxDriver->UsesMemoryBackBuffer())
        services.GfxDriver->ClearRectangle(0, 0, game.GetGameRes().Width - 1, game.GetGameRes().Height - 1, nullptr);

    services.Platform->ResumeApplication();
    services.GameUpdateSuspend = false;
}

void game_sprite_updated(int sprite_num, bool deleted)
{
    // Notify draw system about dynamic sprite change
    notify_sprite_changed(sprite_num, deleted);
}

void precache_view(int view, int first_loop, int last_loop, bool with_sounds)
{
    if (view < 0)
        return;
    if (first_loop > last_loop)
        return;

    first_loop = Math::Clamp(first_loop, 0, views[view].GetLastLoop());
    last_loop = Math::Clamp(last_loop, 0, views[view].GetLastLoop());

    // Record cache sizes and timestamps, for diagnostic purposes
    const size_t sprite_cache_before = spriteset.GetCacheSize();
    const size_t texture_cache_before = texturecache_get_size();
    int total_frames = 0, total_sounds = 0;

    int64_t dur_sprite_load = 0, dur_texture_make = 0, dur_sound_load = 0;
    for (int i = first_loop; i <= last_loop; ++i)
    {
        for (int j = 0; j < views[view].loops[i].numFrames; ++j, ++total_frames)
        {
            const auto &frame = views[view].loops[i].frames[j];
            const auto tp_detail1 = FastClock::now();
            spriteset.PrecacheSprite(frame.pic);
            const auto tp_detail2 = FastClock::now();
            texturecache_precache(frame.pic);
            const auto tp_detail3 = FastClock::now();

            if (with_sounds && frame.sound >= 0)
            {
                ScriptAudioClip *clip = &game.audioClips[frame.sound];
                auto asset_path = get_audio_clip_assetpath(clip->bundlingType, clip->fileName);
                soundcache_precache(asset_path);
                dur_sound_load += ToMilliseconds(FastClock::now() - tp_detail3);
                total_sounds++;
            }
            dur_sprite_load += ToMilliseconds(tp_detail2 - tp_detail1);
            dur_texture_make += ToMilliseconds(tp_detail3 - tp_detail2);
        }
    }

    if (total_frames == 0)
    {
        Debug::Printf("Precache view %d (loops %d-%d): %d frames found in the view, skip", view, first_loop, last_loop,
                      total_frames);
        return;
    }

    // Print gathered time and size info
    size_t sprite_cache_after = spriteset.GetCacheSize();
    size_t texture_cache_after = texturecache_get_size();
    Debug::Printf("Precache view %d (loops %d-%d) with %d frames, total = %" PRId64 " ms, average file->mem = %" PRId64
                  " ms, bm->tx = %" PRId64 " ms,"
                  "\n\t\tloaded %d sounds = %" PRId64 " ms",
                  view, first_loop, last_loop, total_frames, dur_sprite_load + dur_texture_make + dur_sound_load,
                  dur_sprite_load / total_frames, dur_texture_make / total_frames, total_sounds, dur_sound_load);
    Debug::Printf("\tSprite cache: %zu -> %zu KB, texture cache: %zu -> %zu KB", sprite_cache_before / 1024u,
                  sprite_cache_after / 1024u, texture_cache_before / 1024u, texture_cache_after / 1024u);
}

bool load_game_font(int at_index, const FontInfo &font_info, GameDataVersion /*data_ver*/)
{
    if (font_info.Filename.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Warn, "ERROR: Unable to load font %d, because no source filename is assigned", at_index);
        return false;
    }

    if (!load_font_size(at_index, font_info))
    {
        Debug::Printf(
            kDbgMsg_Error,
            "ERROR: Unable to load font %d using %s, file does not exist or no renderer could load a matching file.",
            at_index, font_info.FileName.IsEmpty() ? "available asset(s)" : font_info.FileName.GetCStr());
        // Replace this font using the font 0's file to let display the text at least
        bool result = false;
        if (at_index != 0 && is_font_loaded(0))
            result = load_font_size(at_index, get_font_file(0), font_info);
        if (!result)
            return false;
    }

    return true;
}
void fixup_game_fonts()
{
    // TODO: this workaround may not be needed any more with the FreeType the engine is built with; test with the
    // LucasFan font before removing it
    for (int i = 0; i < game.numfonts; ++i)
    {
        if (!is_bitmap_font(i))
        {
            // Check for the LucasFan font since it comes with an outline font that
            // is drawn incorrectly with Freetype versions > 2.1.3.
            // A simple workaround is to disable outline fonts for it and use
            // automatic outline drawing.
            const int outline_font = get_font_outline(i);
            if (outline_font >= 0)
            {
                const char *name = get_font_name(i);
                const char *outline_name = get_font_name(outline_font);
                if ((ags_stricmp(name, "LucasFan-Font") == 0) && (ags_stricmp(outline_name, "Arcade") == 0))
                    set_font_outline(i, FONT_OUTLINE_AUTO);
            }
        }
    }
}

uint32_t add_movelist(MoveList &&move_list)
{
    // Find a free ID
    uint32_t index = 0;
    if (movelist_free_ids.size() > 0)
    {
        index = movelist_free_ids.front();
        movelist_free_ids.pop();
    }
    else if (movelists.size() == UINT32_MAX)
    {
        return 0u;
    }
    else
    {
        index = std::max<uint32_t>(1, movelists.size());
    }

    return add_movelist(std::move(move_list), index);
}

uint32_t add_movelist(MoveList &&move_list, uint32_t at_index)
{
    movelists.resize(std::max<size_t>(at_index + 1, movelists.size()));
    movelists[at_index] = std::move(move_list);
    return at_index;
}

void remove_movelist(uint32_t index)
{
    if (index == 0 || index >= movelists.size() || movelists[index].IsEmpty())
        return; // requested non-existing movelist

    // Don't erase vector elements, instead set invalid and record free index
    movelists[index] = {};
    movelist_free_ids.push(index);
}

MoveList *get_movelist(uint32_t index)
{
    if (index == 0 || index >= movelists.size() || movelists[index].IsEmpty())
        return nullptr; // requested non-existing movelist

    return &movelists[index];
}

void restore_movelists()
{
    // Will have to readjust free ids records, as movelists may be restored in any random slots
    // NOTE: valid movelist ids begin with 1!
    movelist_free_ids = std::queue<uint32_t>();
    for (uint32_t i = 1; i < movelists.size(); ++i)
    {
        if (movelists[i].IsEmpty())
        {
            movelist_free_ids.push(i);
        }
    }
}

std::vector<MoveList> &get_movelists()
{
    return movelists;
}

void release_script_movelist(int handle)
{
    ScriptMotionPath *script_path = static_cast<ScriptMotionPath *>(ccGetObjectAddressFromHandle(handle));
    if (!script_path)
        return;
    script_path->Invalidate();
    ccReleaseObjectReference(handle);
}

//=============================================================================
//
// GameInfo API
//
//=============================================================================

const char *GameInfo_GetTitle()
{
    return CreateNewScriptString(game.GameInfo["title"]);
}

const char *GameInfo_GetDescription()
{
    return CreateNewScriptString(game.GameInfo["description"]);
}

const char *GameInfo_GetDeveloperName()
{
    return CreateNewScriptString(game.GameInfo["dev_name"]);
}

const char *GameInfo_GetDeveloperURL()
{
    return CreateNewScriptString(game.GameInfo["dev_url"]);
}

const char *GameInfo_GetGenre()
{
    return CreateNewScriptString(game.GameInfo["genre"]);
}

ScriptDateTime *GameInfo_GetReleaseDate()
{
    String release_date = game.GameInfo["release_date"];
    if (release_date.IsEmpty())
        return nullptr; // not available
    std::vector<String> parts = release_date.Split('.');
    if (parts.size() == 0 || parts.size() > 3)
        return nullptr; // malformed date
    int year = 0, month = 0, day = 0;
    year = StrUtil::StringToInt(parts[parts.size() - 1], -1);
    if (parts.size() > 1)
        month = StrUtil::StringToInt(parts[parts.size() - 2], -1);
    if (parts.size() > 2)
        day = StrUtil::StringToInt(parts[parts.size() - 3], -1);
    if (year < 0 || month < 0 || day < 0)
        return nullptr; // malformed date
    return DateTime_CreateFromDate(year, month, day, 0, 0, 0);
}

const char *GameInfo_GetVersion()
{
    return CreateNewScriptString(game.GameInfo["version"]);
}

#include "debug/out.h"
#include "script/script_runtime.h"

bool AssertTimerID(const char *api_name, int tnum)
{
    if (tnum < 0)
    {
        debug_script_error("%s: invalid timer id %d, must be a number >= 0", api_name, tnum);
        return false;
    }
    return true;
}

void SetTimer(int tnum, int timeout)
{
    if (!AssertTimerID("SetTimer", tnum))
        return;

    if (timeout > 0)
    {
        play.StartScriptTimer(tnum, timeout);
    }
    else if (timeout == 0)
    {
        play.StopScriptTimer(tnum);
    }
    else
    {
        debug_script_warn("SetTimer: invalid timeout %d for timer id %d, timeout must be a positive number to start "
                          "the timer, or 0 to stop the timer",
                          timeout, tnum);
    }
}

int GetTimerPos(int tnum)
{
    if (!AssertTimerID("GetTimerPos", tnum))
        return -1;

    return play.GetScriptTimerPos(tnum);
}

int IsTimerExpired(int tnum)
{
    if (!AssertTimerID("IsTimerExpired", tnum))
        return 0;

    return play.CheckScriptTimer(tnum);
}

RoomAreaMask debugLastRoomMask = kRoomAreaNone;
int debugLastMoveChar = -1;

void script_debug(int cmdd, int dataa)
{
    if (play.debug_mode == 0)
        return;

    if (cmdd == 0)
    {
        for (int i = 0; i < game.numinvitems; ++i)
            playerchar->set_item_quantity(i, std::max<int16_t>(1, playerchar->get_item_quantity(i)));
        UpdateInventory();
    }
    else if (cmdd == 1)
    {
        String toDisplay = GetRuntimeInfo();
        DisplayMB(toDisplay.GetCStr());
    }
    else if (cmdd == 2)
    {
        // show room mask
        auto mask = static_cast<RoomAreaMask>(dataa);
        if ((mask < kRoomAreaNone) || (mask > kRoomAreaRegion) ||
            (debugLastRoomMask == mask)) // also act like toggling last mask off
            debugLastRoomMask = kRoomAreaNone;
        else
            debugLastRoomMask = mask;
        debug_draw_room_mask(debugLastRoomMask);
    }
    else if (cmdd == 3)
    {
        int goToRoom = -1;
        if (game.roomNames.size() == 0)
        {
            String inroomtex = String::FromFormat("!Enter new room: (in room %d)", displayed_room);
            setup_for_dialog(play.normal_font);
            goToRoom = enternumberwindow(inroomtex.GetCStr());
            restore_after_dialog();
        }
        else
        {
            setup_for_dialog(play.normal_font);
            goToRoom = roomSelectorWindow(displayed_room, game.roomNames);
            restore_after_dialog();
        }
        if (goToRoom >= 0)
            NewRoom(goToRoom);
    }
    else if (cmdd == 4)
    {
        if (display_fps != kFPS_Forced)
            display_fps = (FPSDisplayMode)dataa;
    }
    else if (cmdd == 5)
    {
        // show the given character's pathfinding; act like a on/off toggle
        debugLastMoveChar = dataa == debugLastMoveChar ? -1 : dataa;
        debug_draw_movelist(debugLastMoveChar);
    }
    else
    {
        debug_script_warn("Debug: unknown command code %d", cmdd);
    }
}

int GetTextWidth(const char *text, int fontnum)
{
    VALIDATE_STRING(text);
    fontnum = ValidateFontNumber("GetTextWidth", fontnum);

    return get_text_width_outlined(text, fontnum);
}

int GetTextHeight(const char *text, int fontnum, int width)
{
    VALIDATE_STRING(text);
    fontnum = ValidateFontNumber("GetTextHeight", fontnum);

    const char *draw_text = skip_voiceover_token(text);
    if (break_up_text_into_lines(draw_text, Lines, width, fontnum) == 0)
        return 0;
    return get_text_lines_height(fontnum, Lines.Count());
}

int GetFontHeight(int fontnum)
{
    fontnum = ValidateFontNumber("GetFontHeight", fontnum);
    return get_font_height_outlined(fontnum);
}

int GetFontLineSpacing(int fontnum)
{
    fontnum = ValidateFontNumber("GetFontLineSpacing", fontnum);
    return get_font_linespacing(fontnum);
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

    if (EngineServices::Current().Platform->GetDiskFreeSpaceMB(get_save_game_directory()) < 2)
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

static bool GetSaveSlotDescription(int slot, String &description)
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
        saves.emplace_back(save_slot, description, ff.GetEntry().Time);
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
        saves.emplace_back(slot, description, File::GetFileTime(path));
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
    EngineServices &services = EngineServices::Current();
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
        services.ResPaths.GamePak.Path = PathFromInstallDir(new_game);
        services.ResPaths.GamePak.Name = new_game;
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
    services.Config.Translation = ""; // reset to default, prevent from trying translation file of game A in game B

    services.Assets->RemoveAllLibraries();

    // TODO: this repeats the engine's startup steps (main/engine.cpp); share them
    if (services.Assets->AddLibrary(services.ResPaths.GamePak.Path) != Common::kAssetNoError)
        quitprintf("!RunAGSGame: unable to load new game file '%s'", services.ResPaths.GamePak.Path.GetCStr());
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
        quit("!StartCutscene: invalid argument, must be 1 to 6.");

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
