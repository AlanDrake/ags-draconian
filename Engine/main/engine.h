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

#include "util/error.h"
#include "util/geometry.h"
#include "util/ini_util.h"

// Returns the engine's descriptive name
const char *get_engine_name();
// Returns the engine's full version string
const char *get_engine_version();
// Returns the full version string with the build info (if any), the pointer size and the byte order
AGS::Common::String get_engine_version_and_build();
// Shows the "preload" image from the game assets, if there is one, for half a second
void show_preload();
// Sets the runtime game state defaults and applies the game options to it
void engine_init_game_settings();
// Opens the sprite file and sets up the sprite cache
AGS::Common::HError engine_init_sprites();
// Starts the engine: finds the game data, reads the configuration, sets up the systems,
// loads the game and runs it; returns when the game ends, with an EXIT_* code
int initialize_engine(const AGS::Common::ConfigTree &startup_opts);

struct DisplayModeSetup;
// Try to set new graphics mode deduced from given configuration;
// if requested mode fails, tries to find any compatible mode close to the
// requested one.
bool engine_try_set_gfxmode_any(const DisplayModeSetup &setup);
// Tries to switch between fullscreen and windowed mode; uses previously saved
// setup if it is available, or default settings for the new mode
bool engine_try_switch_windowed_gfxmode();
// Update graphic renderer and render frame when window size changes
void engine_on_window_changed(const Size &size);
// Shutdown graphics mode (used before shutting down the application)
void engine_shutdown_gfxmode();

// (Re-)Assign all known asset search paths to the AssetManager
void engine_assign_assetpaths();

// Register a callback that will be called before engine is initialised.
// Used for apps to register their own plugins and other configuration
typedef void (*t_engine_pre_init_callback)(void);
void engine_set_pre_init_callback(t_engine_pre_init_callback callback);
