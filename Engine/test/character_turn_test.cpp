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
// Character turning (start_character_turning): the direction and the length of
// a turn, including the tie-break of a 180 degrees turn by the turn order
// priority game option.
//
//=============================================================================
#include <algorithm>
#include <vector>

#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/view_def.h"
#include "game/world/character.h"
#include "gtest/gtest.h"

// Fixture: one character with an 8-loop view of one frame per loop
class CharacterTurning : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Game &game = Game::Current();
        game.Views.resize(1);
        game.Views[0].Initialize(MAX_FACE_DIRECTIONS);
        for (int i = 0; i < MAX_FACE_DIRECTIONS; ++i)
            game.Views[0].loops[i].Initialize(1);
        game.Characters.resize(1);
        game.numcharacters = 1;
        game.Characters[0].View = 0;
        game.Characters[0].ID = 0;
        _savedPriority = game.Options[OPT_TURNORDERPRIORITY];
    }

    void TearDown() override
    {
        Game &game = Game::Current();
        game.Options[OPT_TURNORDERPRIORITY] = _savedPriority;
        game.Characters.clear();
        game.numcharacters = 0;
        game.Views.clear();
    }

    // Starts turning from the loop at index from_idx of turnlooporder to the one at to_idx;
    // returns the indices of turnlooporder that the character passes between the two,
    // empty if it does not turn
    std::vector<int> Turn(int from_idx, int to_idx)
    {
        Game &game = Game::Current();
        game.Characters[0].Loop = turnlooporder[from_idx];
        game.Characters[0].SetTurning(false, false, 0);
        start_character_turning(&game.Characters[0], turnlooporder[to_idx], true);
        std::vector<int> passed;
        if (!game.Characters[0].IsTurning())
            return passed;
        const int step = game.Characters[0].IsTurningCounterClockwise() ? -1 : 1;
        // turns counts the loops left behind, the start included; the last of them leads to the target
        for (int i = 1; i < game.Characters[0].Turns; ++i)
            passed.push_back(((from_idx + step * i) % MAX_FACE_DIRECTIONS + MAX_FACE_DIRECTIONS) % MAX_FACE_DIRECTIONS);
        return passed;
    }

    int _savedPriority = 0;
};

TEST_F(CharacterTurning, ShorterWayIsTaken)
{
    Game &game = Game::Current();
    game.Options[OPT_TURNORDERPRIORITY] = kScTurnOrder_Clockwise;
    // two steps clockwise: one loop between
    EXPECT_EQ(std::vector<int>({1}), Turn(0, 2));
    EXPECT_TRUE(game.Characters[0].IsTurningClockwise());
    // three steps the other way: anti-clockwise through two loops
    EXPECT_EQ(std::vector<int>({1, 0}), Turn(2, 7));
    EXPECT_TRUE(game.Characters[0].IsTurningCounterClockwise());
}

TEST_F(CharacterTurning, FaceDownPassesThroughDownFromEveryStart)
{
    Game::Current().Options[OPT_TURNORDERPRIORITY] = kScTurnOrder_FaceDown;
    const int down_idx = 0;
    const int up_idx = MAX_FACE_DIRECTIONS / 2;
    for (int from_idx = 0; from_idx < MAX_FACE_DIRECTIONS; ++from_idx)
    {
        const int to_idx = (from_idx + MAX_FACE_DIRECTIONS / 2) % MAX_FACE_DIRECTIONS;
        const std::vector<int> passed = Turn(from_idx, to_idx);
        ASSERT_EQ(MAX_FACE_DIRECTIONS / 2 - 1, static_cast<int>(passed.size())) << "from " << from_idx;
        const bool through_down = std::find(passed.begin(), passed.end(), down_idx) != passed.end();
        if (from_idx == down_idx || from_idx == up_idx)
        {
            // Down and Up themselves go by the right side
            EXPECT_FALSE(through_down) << "from " << from_idx;
        }
        else
        {
            EXPECT_TRUE(through_down) << "from " << from_idx;
        }
    }
}
