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
// Game-wide functions of the engine: script API of the Game struct, save game files and directories,
// cutscene skipping, input focus switching, movelists.
//
//=============================================================================
#pragma once

#include <memory>
#include <time.h>
#include <vector>

#include "game/dynobj/script_view_frame.h"
#include "game/game_struct_defines.h"
#include "game/game_version.h"
#include "game/runtime_defines.h"
#include "gfx/bitmap.h"
#include "main/game_file.h"
#include "util/string.h"
#include "util/string_types.h"

// Forward declaration
namespace AGS
{
namespace Common
{
class AssetManager;
class Stream;
} // namespace Common
} // namespace AGS

// Flag in the mode of RunAGSGame: unload the running game and load the new one now, instead of scheduling the switch.
#define RAGMODE_LOADNOW 0x8000000

// How a running cutscene may be skipped. The values are the skip_mode of StartCutscene, kept in play.in_cutscene;
// 0 (kSkipSceneUndefined) means that no cutscene is running.
enum CutsceneSkipStyle
{
    kSkipSceneUndefined = 0,
    eSkipSceneEscOnly = 1,
    eSkipSceneAnyKey = 2,
    eSkipSceneMouse = 3,
    eSkipSceneKeyMouse = 4,
    eSkipSceneEscOrRMB = 5,
    eSkipSceneScriptOnly = 6
};

//=============================================================================
// Audio
//=============================================================================
#define VOL_CHANGEEXISTING   1678
#define VOL_SETFUTUREDEFAULT 1679
#define VOL_BOTH             1680

// Script API, Game.StopAudio: stops every playing clip of the audio type, or fades it out if the type has a crossfade
// speed (and the game is not fast-forwarding), and drops the queued clips of that type; SCR_NO_VALUE means all types.
void Game_StopAudio(int audio_type);
// Script API, Game.IsAudioPlaying: returns 1 if a clip of the audio type (any type for SCR_NO_VALUE) is playing;
// always 0 while the game is fast-forwarding.
int Game_IsAudioPlaying(int audio_type);
// Script API: sets how much the clips of the audio type are made quieter while speech plays, and applies it at once if
// a voice-over is playing now.
void Game_SetAudioTypeSpeechVolumeDrop(int audio_type, int volume_drop);
// Script API: sets the volume (0-100; other values are clamped, with a warning) of the audio type. change_type is
// VOL_CHANGEEXISTING (the clips playing now), VOL_SETFUTUREDEFAULT (the default for the clips played next, and the
// queued ones) or VOL_BOTH.
void Game_SetAudioTypeVolume(int audio_type, int volume, int change_type);

//=============================================================================
// Game state, save games
//=============================================================================
// Script API, Game.DialogCount.
int Game_GetDialogCount();

// Sets a default save directory, based on platform driver settings and user config
void SetDefaultSaveDirectory();
// Script API, Game.SetSaveGameDirectory: sets a new save directory within the user data directory (the path may begin
// with the saved games root token; a relative path without it is accepted with a warning); copies the "restart" slot
// if there is one. Returns 1 on success, 0 if the path is absolute or the directory cannot be used.
int Game_SetSaveGameDirectory(const char *new_folder);
// Script API: returns the description of the save in the slot as a new script string, or null if there is no such save
// or it cannot be read.
const char *Game_GetSaveSlotDescription(int slot);

// Various sort parameter validation: each prints a script warning and returns the "none" value if the argument is out
// of range, otherwise the argument as an enum.
ScriptFileSortStyle ValidateFileSort(const char *api_name, int file_sort);
ScriptSaveGameSortStyle ValidateSaveGameSort(const char *api_name, int save_sort);
ScriptSortStyle ValidateSortStyle(const char *api_name, int sort_style);
ScriptSortDirection ValidateSortDirection(const char *api_name, int sort_dir);
ScriptStringComparison ValidateStringComparison(const char *api_name, int compare_style);
// Save slot range validation; fixups min and max slots, returns if the resulting range is non-empty
bool ValidateSaveSlotRange(const char *api_name, int &min_slot, int &max_slot);
// Graphic parameter validation: prints a script warning and returns kFlip_None if the flip is out of range.
AGS::Common::GraphicFlip ValidateFlip(const char *api_name, int flip);

// View, loop, frame parameter assertions; each quits the game with an error naming api_name and obj_name if the
// argument is not valid (a view without loops or a loop without frames counts as not valid).
// WARNING: these functions assume that view is already in an internal 0-based range.
void AssertView(const char *api_name, const char *obj_name, int view);
void AssertViewHasLoops(const char *api_name, const char *obj_name, int view);
void AssertLoop(const char *api_name, const char *obj_name, int view, int loop);
void AssertFrame(const char *api_name, const char *obj_name, int view, int loop, int frame);

// Script API: the numbers of inventory items, fonts, mouse cursors, characters, GUIs and views in the game.
int Game_GetInventoryItemCount();
int Game_GetFontCount();
int Game_GetMouseCursorCount();
int Game_GetCharacterCount();
int Game_GetGUICount();
int Game_GetViewCount();
// Script API: returns the width or height of the sprite, or 0 if there is no such sprite.
int Game_GetSpriteWidth(int sprite_num);
int Game_GetSpriteHeight(int sprite_num);
// Script API, for the 1-based view: returns the number of its loops. Quits if the view is invalid.
int Game_GetLoopCountForView(int view);
// Script API, for the 1-based view: returns 1 if the loop is set to run on into the next loop, else 0.
int Game_GetRunNextSettingForLoop(int view, int loop);
// Script API, for the 1-based view: returns the number of frames in the loop.
int Game_GetFrameCountForLoop(int view, int loop);
// Script API, for the 1-based view: returns a new script object for the frame; quits if the frame is invalid.
ScriptViewFrame *Game_GetViewFrame(int view, int loop, int frame);
// Script API: returns 1 the first time it is called with the token and 0 on every later call; the tokens are kept
// in the game state and saved with it (see free_do_once_tokens).
int Game_DoOnceOnly(const char *token);

// Script API: the text reading speed, which sets how long a message stays on screen. The setter quits for a value
// below 1, and is ignored if the player's config sets the speed (EngineServices::Config.Access.TextReadSpeed).
int Game_GetTextReadingSpeed();
void Game_SetTextReadingSpeed(int new_text_speed);
// Script API: the least time a message is shown, in milliseconds. The setter is ignored if the player's config sets
// the text reading speed.
int Game_GetMinimumTextDisplayTimeMs();
void Game_SetMinimumTextDisplayTimeMs(int new_text_min_time);
// Script API: the time in milliseconds after a message is shown during which the user input is ignored.
int Game_GetIgnoreUserInputAfterTextTimeoutMs();
void Game_SetIgnoreUserInputAfterTextTimeoutMs(int new_value_ms);
// Script API: returns the file name of the main game file as a new script string.
const char *Game_GetFileName();
// Script API: the name of the game as a new script string. The setter also sets the window title and updates the
// GUI labels that show the game name.
const char *Game_GetName();
void Game_SetName(const char *new_name);

// Script API: returns 1 while the game is fast-forwarding (a cutscene is being skipped, or SkipUntilCharacterStops
// is running), else 0.
int Game_GetSkippingCutscene();
// Script API: returns 1 while a cutscene is running, else 0.
int Game_GetInSkippableCutscene();

// Script API: returns the game's color for the given RGB values (0-255; other values are clamped, with a warning):
// a palette color in an 8-bit game, an opaque 32-bit color otherwise.
int Game_GetColorFromRGB(int red, int green, int blue);
// Script API: shows an input box with the message and returns the text entered as a new script string.
const char *Game_InputBox(const char *msg);
// Script API: returns the name of what is at the given screen coordinates as a new script string (see GetLocationName).
const char *Game_GetLocationName(int x, int y, int hit_options);

// Script API: the font numbers for the normal text and for speech. A setter takes an invalid number as no font
// (FONT_NULL), with a warning.
int Game_GetSpeechFont();
int Game_GetNormalFont();
void Game_SetNormalFont(int font);
void Game_SetSpeechFont(int font);

// Script API: returns the name of the current translation as a new script string.
const char *Game_GetTranslationFilename();
// Script API: switches to the translation; a null or empty name switches back to the game's own text. Returns 1 on
// success, 0 if the translation could not be loaded (the previous one stays).
int Game_ChangeTranslation(const char *new_filename);
// Script API: returns the name of the speech voice pack in use as a new script string.
const char *Game_GetSpeechVoxFilename();
// Script API: loads another speech voice pack; if that fails and the name is not empty, goes back to the default pack.
// Returns whether the requested pack was loaded.
bool Game_ChangeSpeechVox(const char *new_filename);

//=============================================================================

// Sets play.debug_mode.
void set_debug_mode(bool on);
// Sets logical game FPS, telling how often the game should update
void set_game_speed(int new_fps);
// Gets strictly logical game FPS, regardless of whether this is real FPS right now or not.
float get_game_speed();
// Prepares the game for a built-in dialog (save, restore, input box): sets the dialog font to use_font, remembers the
// current cursor and changes it to the pointer cursor.
void setup_for_dialog(int use_font);
// Ends what setup_for_dialog did: restores the cursor and redraws the screen.
void restore_after_dialog();
// Returns the directory the game saves are read from and written to.
AGS::Common::String get_save_game_directory();
// Returns the suffix added to the names of save files ("agssave.NNN" + suffix).
AGS::Common::String get_save_game_suffix();
// Sets the suffix added to the names of save files.
void set_save_game_suffix(const AGS::Common::String &suffix);
// Returns full path to the save for the given slot number
AGS::Common::String get_save_game_path(int slot);
// Parses an input filepath and retrieves final path and respective save slot number, if possible.
// * if input contains a number, then creates a path to the save file with that number;
// * else the file name must be a save file name ("agssave.NNN"), otherwise fails;
// * if input is only a file name and there is no such file in the current directory, then looks for it in the game's
//   save directory;
// * else uses input directly, whether relative to the current directory or absolute.
// Returns true on success.
bool get_save_filepath_and_slot(const AGS::Common::String &input_savepath, AGS::Common::String &filepath, int &slot);
// Try calling built-in restore game dialog;
// NOTE: this is a script command; may be aborted according to the game & room settings
void restore_game_dialog();
// The same for a range of slots: max_slot is replaced by the config's override (EngineServices::Config.Override.MaxSaveSlot) if set.
// When called from a script, queues the dialog to run after the script ends.
void restore_game_dialog2(int min_slot, int max_slot);
// Unconditionally display a built-in restore game dialog; restores the save the player chose. Returns true if the
// player chose one.
bool do_restore_game_dialog(int min_slot, int max_slot, int use_font);
// Try calling built-in save game dialog;
// NOTE: this is a script command; may be aborted according to the game & room settings
void save_game_dialog();
// The same for a range of slots: max_slot is replaced by the config's override (EngineServices::Config.Override.MaxSaveSlot) if set.
// When called from a script, queues the dialog to run after the script ends.
void save_game_dialog2(int min_slot, int max_slot);
// Unconditionally display a built-in save game dialog; saves the game into the slot the player chose. Returns true if
// the player chose one.
bool do_save_game_dialog(int min_slot, int max_slot, int use_font);
// Forgets all the tokens given to Game.DoOnceOnly.
void free_do_once_tokens();
// Shuts down game's running state objects
void shutdown_game_state();
// Free all the memory associated with the game
void unload_game();
// Saves the game into the slot. If image is null and the game's options ask for a screenshot, makes one. Shows an
// error message if saving fails, otherwise runs the GameSaved script event.
void save_game(int slot, const AGS::Common::String &description,
               std::unique_ptr<AGS::Common::Bitmap> &&image = nullptr);
// Saves the game into the slot, using a copy of the sprite as the save's image if sprite_num >= 0. When called from a
// script, queues the save to run after the script ends.
void schedule_save_game(int slot, const AGS::Common::String &description, int sprite_num = -1);
// Returns a new bitmap with the picture of the main viewport; a width or height of 0 or less takes the viewport's.
// layers are the render layer flags to include; 0 gives an empty image.
std::unique_ptr<AGS::Common::Bitmap> create_game_screenshot(int width, int height, int layers);
// Reads the description text of a save file. Returns false, and logs why, if there is no such file or it cannot be read.
bool read_savedgame_description(const AGS::Common::String &filename, AGS::Common::String &description);
// Reads the screenshot stored in a save file, converted to the game's color depth and made opaque. Returns null if
// there is none or the file cannot be read.
std::unique_ptr<AGS::Common::Bitmap> read_savedgame_screenshot(const AGS::Common::String &filename);
// Tries to restore saved game and displays an error on failure; if the error occurred
// too late, when the game data was already overwritten, shuts engine down.
// Returns false on failure.
bool try_restore_save(int slot, bool startup = false);
bool try_restore_save(const AGS::Common::String &path, int slot, bool startup = false);
// Does the scan queued by Game.ScanSaveSlots: fills the script array behind the managed handle with the slots of the
// saves in the range that can be restored (sorted as asked, the rest -1), releases the handle, then runs the
// SavesScanComplete script event with user_param.
void prescan_save_slots(int dest_arr_handle, int min_slot, int max_slot, int save_sort, int sort_dir, int user_param);

// Returns true while a cutscene is running.
bool is_in_cutscene();
// Returns how the running cutscene may be skipped.
CutsceneSkipStyle get_cutscene_skipstyle();
// Starts fast-forwarding the cutscene: removes a popped up GUI and the text message shown now.
void start_skipping_cutscene();
// The check_skip_* functions are called on user input while a cutscene runs: if the input skips it according to the
// cutscene's style, they start skipping and return true. A key skips with any key but a service key (modifiers and
// the like), or with Escape only in the Escape styles.
bool check_skip_cutscene_keypress(int key);
// A mouse click skips in the mouse styles with any button, in the "Esc or right button" style with the right one only.
bool check_skip_cutscene_mclick(int mouse_button);
// A gamepad button skips only in the "any key" style.
bool check_skip_cutscene_gamepad(int gamepad_button);
// Called by StartCutscene and SkipUntilCharacterStops before they start fast-forwarding; does nothing at present.
void initialize_skippable_cutscene();
// Ends the fast-forwarding of a cutscene or of SkipUntilCharacterStops: clears play.fast_forward, applies the game
// palette again and unmutes the sounds.
void stop_fast_forwarding();

// Returns location type under the given screen coordinates.
// click_through_gui defines whether the hit test should be intercepted by a GUI.
// allow_hotspot0 defines whether Hotspot 0 returns LOCTYPE_HOTSPOT or LOCTYPE_NOTHING.
// Returns LOCTYPE_NOTHING if either:
//  - intercepted by GUI, unless it is over an inventory window item, which returns LOCTYPE_INVITEM;
//  - outside a room viewport;
//  - on a hotspot 0 and allow_hotspot0 == false
// If location type != LOCTYPE_NOTHING, then fills locobj_index with object's or hotspot's index.
int GetLocationTypeImpl(int *locobj_index, int x, int y, int hit_options, bool click_through_gui = false,
                        bool allow_hotspot0 = false);

// Called whenever game loses input focus
void display_switch_out();
// Called whenever game gets input focus
void display_switch_in();
// Called when the game loses input focus and must suspend
void display_switch_out_suspend();
// Called when the game gets input focus and should resume
void display_switch_in_resume();

// Notifies the game objects that certain sprite was updated.
// This make them update their render states, caches, and so on.
void game_sprite_updated(int sprite_num, bool deleted = false);
// Precaches sprites for a view, within a selected range of loops; the loop numbers are clamped to the view's loops,
// and a negative view does nothing. The view is a 0-based index. With_sounds also precaches the frames' sounds.
// Logs the time taken and the cache sizes.
void precache_view(int view, int first_loop = 0, int last_loop = INT32_MAX, bool with_sounds = false);

// Loads and initializes font at certain index using given properties; if the font file cannot be loaded, tries to
// load the font from font 0's file instead (except for font 0). Returns false if no font could be loaded.
bool load_game_font(int at_index, const FontInfo &font_info, GameDataVersion data_ver);
// Do additional fixups to the game fonts. This requires *all fonts* to be loaded,
// as some fixups depend on relation between the main font and its outline font.
void fixup_game_fonts();

class MoveList;
// Allocates a movelist slot and moves data over; returns new movelist's index,
// or 0 if not possible to allocate one.
uint32_t add_movelist(MoveList &&move_list);
// Allocates a movelist slot at the given index and assigns new data.
uint32_t add_movelist(MoveList &&move_list, uint32_t at_index);
// Removes a movelist, frees the occupied slot
void remove_movelist(uint32_t index);
// Returns movelist by its index, or null if no such valid movelist exists
MoveList *get_movelist(uint32_t index);
// Restores movelists, e.g. after restoring a game save: movelists are put into the slots the save had, so this
// rebuilds the list of free slots.
void restore_movelists();
// Returns a ref to movelists vector, useful for iterating over them;
// TODO: only the save writer uses this, change it to return a const reference
std::vector<MoveList> &get_movelists();
// Releases a script movelist's reference and invalidates the script object.
void release_script_movelist(int handle);


struct GameDef;
struct ViewStruct;
namespace AGS
{
namespace Common
{
class SpriteCache;
} // namespace Common
} // namespace AGS

// The game's data, as loaded from the game file.
extern GameDef game;
// The game's views.
extern std::vector<ViewStruct> views;
// The game's sprites.
extern AGS::Common::SpriteCache spriteset;

// A request to run another game (the mode and flags given to RunAGSGame), served by the game loop; 0 when none.
extern unsigned int load_new_game;
// Counts the restore attempts, so that a script run that was in progress can tell that one happened (script.cpp)
extern int gameHasBeenRestored;

// An internal state id, telling how the new room was entered.
// This defines which events to run after room was loaded.
enum EnterNewRoomState
{
    kEnterRoom_None = 0,
    kEnterRoom_Normal = 1,       // changed to room and finished fade-in
    kEnterRoom_FirstTime = 2,    // same as normal, but this is first time this room is visited
    kEnterRoom_RestoredSave = 3, // loaded a room after restoring a save
};

extern EnterNewRoomState in_new_room;
// The value in_new_room had when the room was entered, kept for the events that run after the fade-in.
extern EnterNewRoomState new_room_was;
// Where to put the player character in the room being entered; 0 means no instruction. A value of 1000 or more is an
// entrance at an edge of the room, set by Character.ChangeRoomAutoPosition: the thousands give the edge (1000 left,
// 2000 right, 3000 bottom, 4000 top), the rest the coordinate along it (y for the sides, x for bottom and top; 0 is
// the middle). Reset once applied.
extern int new_room_pos;
// Position and loop (facing) to give the player character in the room being entered, by Character.ChangeRoom;
// SCR_NO_VALUE when not set. Reset once applied.
extern int new_room_x, new_room_y, new_room_loop;
// Whether to move the player character onto the walkable area after placing it at new_room_x, new_room_y.
extern bool new_room_placeonwalkable;
// The number of the room that is loaded now.
extern int displayed_room;
extern int frames_per_second; // fixed game fps, set by script
// Pause nesting level, above 0 while the game is paused.
extern int game_paused;

struct CCAudioClip;

// Used by the script bindings in game/script_api/audio_channel_api.cpp
extern CCAudioClip ccDynamicAudioClip;

struct CCAudioChannel;

// Used by the script bindings in game/script_api/audio_clip_api.cpp
extern CCAudioChannel ccDynamicAudio;

struct CCCharacter;
struct CCInventory;

// Used by the script bindings in game/script_api/character_api.cpp
extern CCCharacter ccDynamicCharacter;

// Used by the script bindings in game/script_api/character_api.cpp
extern CCInventory ccDynamicInv;

struct CCDialog;
struct CCGUI;

// Used by the script bindings in game/script_api/dialog_api.cpp
extern CCGUI ccDynamicGUI;

// Used by the script bindings in game/script_api/dialog_api.cpp
extern CCDialog ccDynamicDialog;

struct CharacterInfo;
struct ScriptAudioChannel;
struct ScriptAudioClip;
struct ScriptCamera;
struct ScriptDateTime;

// Script API: Game::GetAudioTypeCrossfadeSpeed^1
int Game_GetAudioTypeCrossfadeSpeed(int audio_type);

// Script API: Game::SetAudioTypeCrossfadeSpeed^2
void Game_SetAudioTypeCrossfadeSpeed(int audio_type, int crossfade_speed);

// Script API: Game::GetAudioTypeSpeechVolumeDrop^1
int Game_GetAudioTypeSpeechVolumeDrop(int audio_type);

// Script API: Game::GetAudioTypeVolume^1
int Game_GetAudioTypeVolume(int audio_type);

// Script API: Game::GetSaveSlotTime^1
ScriptDateTime *Game_GetSaveSlotTime(int slot);

// Script API: Game::get_InventoryCursorHotspotGraphic
int Game_GetInventoryCursorHotspotGraphic();

// Script API: Game::set_InventoryCursorHotspotGraphic
void Game_SetInventoryCursorHotspotGraphic(int graphic);

// Script API: Game::get_InventoryCursorHotspotColor
int Game_GetInventoryCursorHotspotColor();

// Script API: Game::set_InventoryCursorHotspotColor
void Game_SetInventoryCursorHotspotColor(int color);

// Script API: Game::get_InventoryCursorHotspotCrossColor
int Game_GetInventoryCursorHotspotCrossColor();

// Script API: Game::set_InventoryCursorHotspotCrossColor
void Game_SetInventoryCursorHotspotCrossColor(int color);

// Script API: Game::get_IsPaused
bool Game_GetIsPaused();

// Script API: Game::get_UseActiveInventoryGraphicForCursor
bool Game_GetUseActiveInventoryGraphicForCursor();

// Script API: Game::set_UseActiveInventoryGraphicForCursor
void Game_SetUseActiveInventoryGraphicForCursor(bool use_inv_graphic);

// Script API: Game::get_TopSpriteNumber
int Game_GetTopSpriteNumber();

// Script API: Game::geti_SpriteColorDepth
int Game_GetSpriteDepth(int sprite_num);

// Script API: Game::geti_IsSpriteDynamic
bool Game_GetIsSpriteDynamic(int sprite_num);

// Script API: Game::ResetDoOnceOnly
void Game_ResetDoOnceOnly();

// Script API: Game::get_TickCounter
int Game_GetTickCounter();

// Script API: Game::GetColorFromRGBA^4
int Game_GetColorFromRGBA(int red, int green, int blue, int alpha);

// Script API: Game::GetLocationName^2
const char *Game_GetLocationName2(int x, int y);

// Script API: Game::get_Speed
int Game_GetSpeed();

// Script API: Game::set_Speed
void Game_SetSpeed(int speed);

// Script API: Game::get_AudioClipCount
int Game_GetAudioClipCount();

// Script API: Game::geti_AudioClips
ScriptAudioClip *Game_GetAudioClip(int index);

// Script API: Game::get_Camera
ScriptCamera *Game_GetCamera();

// Script API: Game::get_CameraCount
int Game_GetCameraCount();

// Script API: Game::geti_Cameras
ScriptCamera *Game_GetAnyCamera(int index);

// Script API: Game::SimulateKeyPress^2
void Game_SimulateKeyPress(int key, int mod);

// Script API: Game::SimulateKeyPress^1
void Game_SimulateKeyPressOld(int key);

// Script API: Game::get_BlockingWaitCounter
int Game_BlockingWaitCounter();

// Script API: Game::get_BlockingWaitSkipped
int Game_BlockingWaitSkipped();

// Script API: Game::get_BlockingWaitSkipType
int Game_BlockingWaitSkipType();

// Script API: Game::get_InBlockingAction
int Game_InBlockingAction();

// Script API: Game::get_InBlockingWait
int Game_InBlockingWait();

// Script API: Game::Pause
void Game_Pause();

// Script API: Game::Resume
void Game_Resume();

// Script API: Game::PlayVoiceClip
ScriptAudioChannel *Game_PlayVoiceClip(CharacterInfo *ch, int sound_id, bool as_speech);

// Script API: Game::PlayVoiceClipAsType
ScriptAudioChannel *Game_PlayVoiceClipAsType(CharacterInfo *ch, int sound_id, int audio_type, int chan_id, int priority,
                                             int repeat);

// Script API: Game::PrecacheSprite
void Game_PrecacheSprite(int sprite_num);

// Script API: Game::PrecacheView
void Game_PrecacheView(int view, int first_loop, int last_loop);

// Script API: Game::get_FaceDirectionRatio
float Game_GetFaceDirectionRatio();

// Script API: Game::set_FaceDirectionRatio
void Game_SetFaceDirectionRatio(float ratio);

// Script API: Game::get_RoomCount
int Game_GetRoomCount();

// Script API: Game::geti_RoomNumbers
int Game_GetRoomNumber(int index);

// Script API: Game::geti_RoomNames
const char *Game_GetRoomName(int room_number);

// Script API: Game::GetSaveSlots^4
void *Game_GetSaveSlots(int min_slot, int max_slot, int save_sort, int sort_dir);

// Script API: Game::ScanSaveSlots^6
void Game_ScanSaveSlots(void *dest_arr, int min_slot, int max_slot, int save_sort, int sort_dir, int user_param);

// Script API: GameInfo::get_Title
const char *GameInfo_GetTitle();

// Script API: GameInfo::get_Description
const char *GameInfo_GetDescription();

// Script API: GameInfo::get_DeveloperName
const char *GameInfo_GetDeveloperName();

// Script API: GameInfo::get_DeveloperURL
const char *GameInfo_GetDeveloperURL();

// Script API: GameInfo::get_Genre
const char *GameInfo_GetGenre();

// Script API: GameInfo::get_ReleaseDate
ScriptDateTime *GameInfo_GetReleaseDate();

// Script API: GameInfo::get_Version
const char *GameInfo_GetVersion();

struct CCGUIControl;

// Used by the script bindings in game/script_api/gui_api.cpp
extern CCGUIControl ccDynamicGUIControl;

struct CCGUIButton;
struct CCGUIInvWindow;
struct CCGUILabel;
struct CCGUIListBox;
struct CCGUISlider;
struct CCGUITextBox;
struct CCTextWindowGUI;

// The managers of the GUI control types and of the text window GUIs.
extern CCGUIButton ccDynamicGUIButton;
extern CCGUIInvWindow ccDynamicGUIInvWindow;
extern CCGUILabel ccDynamicGUILabel;
extern CCGUIListBox ccDynamicGUIListBox;
extern CCGUISlider ccDynamicGUISlider;
extern CCGUITextBox ccDynamicGUITextBox;
extern CCTextWindowGUI ccDynamicTextWindowGUI;

struct CCHotspot;

// Used by the script bindings in game/script_api/hotspot_api.cpp
extern CCHotspot ccDynamicHotspot;

struct CCObject;

// Used by the script bindings in game/script_api/object_api.cpp
extern CCObject ccDynamicObject;

struct CCRegion;

// Used by the script bindings in game/script_api/region_api.cpp
extern CCRegion ccDynamicRegion;

struct CCWalkableArea;
struct CCWalkbehind;

// Used by the script bindings in game/script_api/room_api.cpp
extern CCWalkableArea ccDynamicWalkarea;

// Used by the script bindings in game/script_api/room_api.cpp
extern CCWalkbehind ccDynamicWalkbehind;

void SetTimer(int tnum, int timeout);
int GetTimerPos(int tnum);
int IsTimerExpired(int tnum);

void script_debug(int cmdd, int dataa);

int GetTextWidth(const char *text, int fontnum);
int GetTextHeight(const char *text, int fontnum, int width);
int GetFontHeight(int fontnum);
int GetFontLineSpacing(int fontnum);

struct SaveListItem
{
    int Slot;
    AGS::Common::String Description;
    time_t FileTime = 0;

    SaveListItem(int slot, const AGS::Common::String &desc, time_t ft) : Slot(slot), Description(desc), FileTime(ft) {}
};

//
// SaveListItem comparers, for sorting
//
struct SaveItemCmpByNumber
{
    bool operator()(const SaveListItem &item1, const SaveListItem &item2) const { return item1.Slot < item2.Slot; }
};

struct SaveItemCmpByTime
{
    bool operator()(const SaveListItem &item1, const SaveListItem &item2) const
    {
        return item1.FileTime < item2.FileTime;
    }
};

struct SaveItemCmpByDesc
{
    SaveItemCmpByDesc(std::unique_ptr<AGS::Common::IStrCmp> &&cmp_impl) : _cmpImpl(std::move(cmp_impl)) {}

    bool operator()(const SaveListItem &item1, const SaveListItem &item2) const
    {
        return _cmpImpl->operator()(item1.Description, item2.Description) < 0;
    }

private:
    // It's shared ptr, because STL requires a copy constructor for predicates
    std::shared_ptr<AGS::Common::IStrCmp> _cmpImpl;
};

// Restores the restart point save (RESTART_POINT_SAVE_GAME_NUMBER); when called from a script, queues the action
// to run after the script ends.
void restart_game();
// Copies the save file of old_slot over new_slot's; does nothing if the slots are the same.
void CopySaveSlot(int old_slot, int new_slot);
// Renames the save file of old_slot to new_slot's; does nothing if the slots are the same.
void MoveSaveSlot(int old_slot, int new_slot);
// Restores the game from the given save slot; quits if no room is loaded yet (the game is in game_start).
// When called from a script, queues the action to run after the script ends.
void RestoreGameSlot(int slot);
// Schedules saving the game into the given slot, with a description and, if sprite_id >= 0, a copy of that sprite
// as the save's image. Does nothing but show a message if the disk has less than 2 MB free.
void SaveGameSlot(int slot, const char *description, int sprite_id);
// SaveGameSlot without a sprite (the save gets the usual screenshot, if the game's options ask for one).
void SaveGameSlot2(int slot, const char *description);
// Deletes the save file of the given slot, if there is one.
void DeleteSaveSlot(int slot);
// Fills a list of SaveListItems by any save files found within the given range
void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, bool get_description);
// Fills a list of SaveListItems by any save files found within the given range; sorts the resulting list
void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, bool get_description,
                  ScriptSaveGameSortStyle save_sort, ScriptSortDirection sort_dir);
// Fills a list of SaveListItems by any save files found in the source list of slots;
// slots may be listed in any order, and post-sorting is only optional
void FillSaveList(const std::vector<int> &slots, std::vector<SaveListItem> &saves, bool get_description,
                  ScriptSaveGameSortStyle save_sort = kScSaveGameSort_None, ScriptSortDirection sort_dir = kScSortNone);
// Find the latest save slot, returns the slot index or -1 at failure
int GetLastSaveSlot();
// Pauses the game; pauses nest, each PauseGame needs its own UnPauseGame.
void PauseGame();
// Undoes one PauseGame; does nothing if the game is not paused.
void UnPauseGame();
// Returns 1 if the game is paused, else 0.
int IsGamePaused();
// Runs another game file. Without RAGMODE_LOADNOW in mode, only stores the new game and data (play.takeover_data)
// and schedules the switch: after the script ends if called from a script, else through load_new_game. With it,
// unloads the current game, loads new_game, and starts it or, if load_new_game_restore is set, restores that save.
// Quits on an unknown mode or when run from the editor's debugger. Always returns 0.
int RunAGSGame(const AGS::Common::String &new_game, unsigned int mode, int data);
// Quits the game; with a nonzero dialog first shows the quit dialog, and returns if the player chooses to keep playing.
void QuitGame(int dialog);
// Saves the game into the restart point slot (RESTART_POINT_SAVE_GAME_NUMBER).
void SetRestartPoint();
// Sets the game speed in frames per second; the speed actually used is new_speed plus play.game_speed_modifier,
// limited to 10..1000, while GetGameSpeed returns new_speed.
void SetGameSpeed(int new_speed);
// Returns the speed last given to SetGameSpeed, initially the game's setting.
int GetGameSpeed();
// Sets a game option (OPT_*) and applies its effect at once. Returns the previous value; for an invalid option
// returns 0, and for an option that cannot be changed at runtime returns its current value.
int SetGameOption(int opt, int new_value);
// Returns the value of a game option (OPT_*), or 0 for an invalid option.
int GetGameOption(int opt);

// Fast-forwards the game until the character stops walking. Quits if the character is not in the current room;
// does nothing if it is not walking; then quits if a cutscene is running.
void SkipUntilCharacterStops(int char_id);
// Stops the fast-forward started by SkipUntilCharacterStops, if there is one.
void EndSkippingUntilCharStops();
// skip_mode decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
// 6 = only by the script (SkipCutscene)
// Quits if a cutscene is already running.
void StartCutscene(int skip_mode);
// Ends the cutscene and stops its fast-forwarding; quits if there is no cutscene. Returns nonzero if the player
// skipped it.
int EndCutscene();
// Tell the game to skip current cutscene
void SkipCutscene();

// ShowInputBox assumes a string buffer of MAX_MAXSTRLEN
void ShowInputBox(const char *msg, char *buffer);
// Shows the (translated) message in an input dialog and returns the text the player entered.
AGS::Common::String ShowInputBoxImpl(const char *msg);

// Returns the type (LOCTYPE_*) of what is at the given screen coordinates; hit_options are the kHit_* flags.
// Returns LOCTYPE_NOTHING if a GUI is there, the point is outside the room viewport, or it is on hotspot 0.
int GetLocationType(int x, int y, int hit_options);
// GetLocationType for only the interactable things (kHit_Interactable).
int GetLocationType2(int x, int y);
// Returns the name (description) of a location under given coordinates;
// if nothing found, returns an empty string.
const char *GetLocationName(int x, int y, int hit_options);

// Saves a screenshot of the whole game screen (all layers) to a file; see SaveScreenShot4.
int SaveScreenShot1(const char *filename);
// Saves a screenshot of the given size (0 = the main viewport's size) and layers (RENDER_BATCH_* flags; 0 gives an
// empty image) into the saved games directory, unless filename starts with a "$" path token; adds ".bmp" if filename
// has no extension, the extension picks the image format. Returns 1 on success, 0 on failure.
int SaveScreenShot4(const char *filename, int width, int height, int layers);

// Quits the game with a script-abort error ('!?' message prefix) that carries the given text; script API AbortGame.
void _sc_AbortGame(const char *text);

// The Wait functions wait for num_loops game loops (0 returns at once), or until the input they accept skips the
// wait; they return what ended it, see WaitImpl in the .cpp.
// Waits for the loops only.
void scrWait(int num_loops);
// Waits for a key press too.
int WaitKey(int num_loops);
// Waits for a mouse click too.
int WaitMouse(int num_loops);
// Waits for a key press or a mouse click too.
int WaitMouseKey(int num_loops);
// Waits for the kinds of input in input_flags (the skip type flags shifted by SKIP_RESULT_TYPE_SHIFT) too.
int WaitInput(int input_flags, int num_loops);
// Skips the current wait as if by the script, with result 0.
void SkipWait0();
// Skips the current wait as if by the script, with the given result data.
void SkipWait(int result_value);
