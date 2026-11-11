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
#pragma once

#include <locale>
#include <stdexcept>
#include <time.h>

#include "ac/runtime_defines.h"
#include "util/string.h"
#include "util/string_types.h"

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


// Stops all running scripts at their next step.
void AbortGame();
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

// Returns nonzero if the key with the given script key code is held down.
int IsKeyPressed(int keycode);
// Returns true if any key is held down.
bool IsAnyKeyPressed();
// Returns a new script array (int, script key codes) of the keys held down now.
void *GetPressedKeys();

// Saves a screenshot of the whole game screen (all layers) to a file; see SaveScreenShot4.
int SaveScreenShot1(const char *filename);
// Saves a screenshot of the given size (0 = the main viewport's size) and layers (RENDER_BATCH_* flags; 0 gives an
// empty image) into the saved games directory, unless filename starts with a "$" path token; adds ".bmp" if filename
// has no extension, the extension picks the image format. Returns 1 on success, 0 on failure.
int SaveScreenShot4(const char *filename, int width, int height, int layers);
// Sets whether the game runs when its window is in the background (mode 1) or not (0); the user's config, a connected
// editor debugger and exclusive fullscreen can override the request. Quits on another mode value.
void SetMultitasking(int mode);

// Processes a click at the given screen coordinates with the given cursor mode, as if the player clicked in the room.
void RoomProcessClick(int x, int y, int mood);
// Returns 1 if a click at the given screen coordinates with the given cursor mode would do something (a walk
// mode always does), else 0.
int IsInteractionAvailable(int x, int y, int mood);

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
