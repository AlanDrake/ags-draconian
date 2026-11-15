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
// This unit provides game initialization routine, which takes place after
// main game file was successfully loaded.
//
//=============================================================================

#pragma once

#include <vector>

#include "game/main_game_file.h"
#include "util/error.h"
#include "util/string.h"

struct GameSetupStruct;
struct GamePlayState;
struct GameSetup;

// Static arrays of managed handles for individual game objects, by object index; for the GUI controls, by GUI
// and then by control index.
extern std::vector<int> StaticCharacterArray;
extern std::vector<int> StaticObjectArray;
extern std::vector<int> StaticGUIArray;
extern std::vector<int> StaticHotspotArray;
extern std::vector<int> StaticRegionArray;
extern std::vector<int> StaticWalkareaArray;
extern std::vector<int> StaticWalkbehindArray;
extern std::vector<int> StaticInventoryArray;
extern std::vector<int> StaticDialogArray;
extern std::vector<std::vector<int>> StaticGUIControlsHandles;

namespace AGS
{
namespace Engine
{


// Error codes for initializing the game
enum GameInitErrorType
{
    kGameInitErr_NoError,
    kGameInitErr_UnknownDataVersion,
    // currently AGS requires at least one font to be present in game
    kGameInitErr_NoFonts,
    kGameInitErr_EntityInitFail,
    kGameInitErr_PluginNameInvalid,
    kGameInitErr_ScriptInitFailed
};

// Returns the message for a game init error code
Common::String GetGameInitErrorText(GameInitErrorType err);

typedef Common::TypedCodeError<GameInitErrorType, GetGameInitErrorText> GameInitError;
typedef Common::ErrorHandle<GameInitError> HGameInitError;

// Sets up game state for play using preloaded data: puts the GUIs, views and dialogs into the engine's
// globals, registers the game entities and the engine API in the script system, loads fonts and lip sync data,
// starts up the plugins, and creates and links the scripts
HGameInitError InitGameState(const Common::LoadedGameEntities &ents, GameDataVersion data_ver);
// Applies accessibility options, some of them may override game settings
void ApplyAccessibilityOptions(GamePlayState &play, const GameSetup &setup);
// Applies configurable behavior options for this game, which are the runtime behavior switches
// (the engine's defaults, plus the overrides from the config)
void ApplyBehaviorOptions(GameSetupStruct &game, GamePlayState &play, const GameSetup &setup);
// Prints behavior options to the log
void PrintBehaviorOptions(const GameSetupStruct &game, const GamePlayState &play);

// Registers any persistent script objects that are missing from the managed pool,
// e.g. if restoring a save made before they were added to the game.
void RegisterMissingManagedObjects(GameSetupStruct &game);

} // namespace Engine
} // namespace AGS
