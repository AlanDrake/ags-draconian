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

#include "game/game_def.h"
#include "game/game_state.h"

// One play-through: what the game file says (the base, GameDef) and everything that changes while the game
// runs, which is what a save holds. The definitions and the live state are one object until each entity
// gets a live copy (PLAN.md 5.1a). The game logic reaches it through Game::Current(); a function that uses
// it more than once takes a reference at its top:
//     Game &game = Game::Current();
class Game : public GameDef
{
public:
    Game();
    ~Game(); // out of line: later members are vectors of types that are incomplete here
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    // The instance in use
    static Game &Current() { return *_current; }
    // Makes another instance the current one, for a test, and returns the one that was;
    // nullptr brings the engine's own instance back
    static Game *SetCurrent(Game *game);

    // The game's own state: settings the script changes, play progress, blocking and transient state
    GamePlayState Play;

private:
    static Game *_current;
};
