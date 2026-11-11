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


// Notify the running game that the engine requested immediate stop
void AbortGame();
void restart_game();
void CopySaveSlot(int old_slot, int new_slot);
void MoveSaveSlot(int old_slot, int new_slot);
void RestoreGameSlot(int slot);
void MoveSaveSlot(int old_slot, int new_slot);
void SaveGameSlot(int slot, const char *description, int sprite_id);
void SaveGameSlot2(int slot, const char *description);
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
void PauseGame();
void UnPauseGame();
int IsGamePaused();
int RunAGSGame(const AGS::Common::String &new_game, unsigned int mode, int data);
void QuitGame(int dialog);
void SetRestartPoint();
void SetGameSpeed(int new_speed);
int GetGameSpeed();
int SetGameOption(int opt, int new_value);
int GetGameOption(int opt);

void SkipUntilCharacterStops(int char_id);
void EndSkippingUntilCharStops();
// skip_mode decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
void StartCutscene(int skip_mode);
int EndCutscene();
// Tell the game to skip current cutscene
void SkipCutscene();

// ShowInputBox assumes a string buffer of MAX_MAXSTRLEN
void ShowInputBox(const char *msg, char *buffer);
AGS::Common::String ShowInputBoxImpl(const char *msg);

int GetLocationType(int x, int y, int hit_options);
int GetLocationType2(int x, int y);
// Returns the name (description) of a location under given coordinates;
// if nothing found, returns an empty string.
const char *GetLocationName(int x, int y, int hit_options);

int IsKeyPressed(int keycode);
bool IsAnyKeyPressed();
void *GetPressedKeys();

int SaveScreenShot1(const char *filename);
int SaveScreenShot4(const char *filename, int width, int height, int layers);
void SetMultitasking(int mode);

void RoomProcessClick(int x, int y, int mood);
int IsInteractionAvailable(int x, int y, int mood);

void _sc_AbortGame(const char *text);

void scrWait(int num_loops);
int WaitKey(int num_loops);
int WaitMouse(int num_loops);
int WaitMouseKey(int num_loops);
int WaitInput(int input_flags, int num_loops);
void SkipWait0();
void SkipWait(int result_value);
