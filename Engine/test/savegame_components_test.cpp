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
// Savegame components that depend on the game content.
//
//=============================================================================
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "ac/gamesetupstruct.h"
#include "ac/view.h"
#include "game/savegame.h"
#include "game/savegame_components.h"
#include "game/savegame_internal.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;

class SavegameViews : public ::testing::Test
{
protected:
    void TearDown() override
    {
        views.clear();
        game.numviews = 0;
    }

    // Makes the game's views, each with the same number of loops and frames;
    // every frame has a sprite and an event name, so that a save has frame data
    // and strings to step over
    static void SetViews(int view_count, int loop_count, int frame_count)
    {
        views.clear();
        views.resize(view_count);
        game.numviews = view_count;
        for (auto &view : views)
        {
            view.Initialize(loop_count);
            for (auto &loop : view.loops)
            {
                loop.Initialize(frame_count);
                for (int frame = 0; frame < frame_count; ++frame)
                {
                    loop.frames[frame].pic = 10 + frame;
                    loop.frames[frame].event_name = String::FromFormat("step%d", frame);
                }
            }
        }
    }
};

TEST_F(SavegameViews, PrescanStepsOverViewFrames)
{
    // the prescan skips each loop's frames and their event names,
    // and counts every loop of every view
    SetViews(2, 3, 4);
    std::vector<uint8_t> saved;
    {
        Stream out(std::unique_ptr<IStreamBase>(new VectorStream(saved, kStream_Write)));
        HSaveError err = SavegameComponents::WriteAllCommon(&out, kSaveCmp_Views, false);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    }

    Stream in(std::unique_ptr<IStreamBase>(new VectorStream(saved)));
    PreservedParams pp;
    RestoredData r_data;
    HSaveError err = SavegameComponents::PrescanAll(&in, kSvgVersion_Current, kSaveCmp_Views, pp, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(2u, r_data.DataCounts.Views);
    EXPECT_EQ((std::vector<uint32_t>{3u, 3u}), r_data.DataCounts.ViewLoops);
    EXPECT_EQ((std::vector<uint32_t>{12u, 12u}), r_data.DataCounts.ViewFrames);
}
