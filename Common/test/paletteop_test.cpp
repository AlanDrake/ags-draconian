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
#include "gfx/bitmapdata.h"
#include "gtest/gtest.h"

using namespace AGS::Common;

TEST(PixelOp, SetRGB)
{
    Palette pal;
    std::fill(pal.begin(), pal.end(), RGB{0, 0, 0, 0});
    PaletteOp::SetRGB(pal, 4, 255, 128, 64, 32);
    EXPECT_EQ(pal[4].r, 255);
    EXPECT_EQ(pal[4].g, 128);
    EXPECT_EQ(pal[4].b, 64);
    EXPECT_EQ(pal[4].a, 32);
}

TEST(PixelOp, Rotate)
{
    Palette pal;
    std::fill(pal.begin(), pal.end(), RGB{0, 0, 0, 0});
    PaletteOp::SetRGB(pal, 10, 1, 1, 1);
    PaletteOp::SetRGB(pal, 11, 2, 2, 2);
    PaletteOp::SetRGB(pal, 12, 3, 3, 3);
    PaletteOp::SetRGB(pal, 13, 4, 4, 4);
    PaletteOp::SetRGB(pal, 14, 5, 5, 5);

    PaletteOp::Rotate(pal, 10, 14, true /* left */);
    EXPECT_EQ(pal[10].r, 2);
    EXPECT_EQ(pal[11].r, 3);
    EXPECT_EQ(pal[12].r, 4);
    EXPECT_EQ(pal[13].r, 5);
    EXPECT_EQ(pal[14].r, 1);
    PaletteOp::Rotate(pal, 10, 14, true /* left */);
    EXPECT_EQ(pal[10].r, 3);
    EXPECT_EQ(pal[11].r, 4);
    EXPECT_EQ(pal[12].r, 5);
    EXPECT_EQ(pal[13].r, 1);
    EXPECT_EQ(pal[14].r, 2);
    PaletteOp::Rotate(pal, 10, 14, false /* right */);
    EXPECT_EQ(pal[10].r, 2);
    EXPECT_EQ(pal[11].r, 3);
    EXPECT_EQ(pal[12].r, 4);
    EXPECT_EQ(pal[13].r, 5);
    EXPECT_EQ(pal[14].r, 1);
    PaletteOp::Rotate(pal, 10, 14, false /* right */);
    EXPECT_EQ(pal[10].r, 1);
    EXPECT_EQ(pal[11].r, 2);
    EXPECT_EQ(pal[12].r, 3);
    EXPECT_EQ(pal[13].r, 4);
    EXPECT_EQ(pal[14].r, 5);
}

namespace
{

// The distance bestfit_color and create_rgb_table use (Allegro's col_diff weights)
int ColorDistance(const RGB &c, int r, int g, int b)
{
    const int dr = c.r - r, dg = c.g - g, db = c.b - b;
    return dg * dg * 14 * 14 + dr * dr * 8 * 8 + db * db * 3 * 3;
}

// A palette of pseudo-random 8-bit colours; index 0 is black, as in AGS games
void MakeTestPalette(PALETTE pal, unsigned seed)
{
    pal[0] = RGB{0, 0, 0, 0};
    for (int i = 1; i < PAL_SIZE; ++i)
    {
        seed = seed * 1103515245u + 12345u;
        pal[i].r = static_cast<unsigned char>(seed >> 8);
        pal[i].g = static_cast<unsigned char>(seed >> 16);
        pal[i].b = static_cast<unsigned char>(seed >> 24);
        pal[i].a = 0;
    }
}

} // namespace

// create_rgb_table used to work on 6-bit components with a distance table of its own layout,
// kept in the same array as bestfit_color's: after a bestfit_color call it read the wrong
// distances and could fill forever (an 8-bit game hung on its first room with BlackBoxOut)
TEST(PaletteMatch, RgbTableAfterBestfitColor)
{
    for (unsigned seed : {1u, 7u, 1234u})
    {
        PALETTE pal;
        MakeTestPalette(pal, seed);
        bestfit_color(pal, 0, 0, 0); // fills the distance table first, as makecol8 does in the engine
        RGB_MAP map;
        create_rgb_table(&map, pal, nullptr);

        // The fill is an approximation: a cell may get a colour a little farther than the nearest
        // (measured on these palettes: 0.8-1% of cells), never index 0 unless it is pink
        int worse = 0;
        for (int r = 0; r < 32; ++r)
        {
            for (int g = 0; g < 32; ++g)
            {
                for (int b = 0; b < 32; ++b)
                {
                    const int idx = map.data[r][g][b];
                    ASSERT_NE(idx, 0) << "only a pink index 0 may be chosen";
                    const int best = bestfit_color(pal, r * 8, g * 8, b * 8);
                    const int d_map = ColorDistance(pal[idx], r * 8, g * 8, b * 8);
                    const int d_best = ColorDistance(pal[best], r * 8, g * 8, b * 8);
                    if (d_map > d_best)
                        worse++;
                }
            }
        }
        EXPECT_LE(worse, 32 * 32 * 32 / 50) << "seed " << seed;
    }
}

TEST(PaletteMatch, RgbTablePinkIndexZero)
{
    PALETTE pal;
    MakeTestPalette(pal, 99);
    pal[0] = RGB{255, 0, 255, 0};
    RGB_MAP map;
    create_rgb_table(&map, pal, nullptr);
    EXPECT_EQ(map.data[31][0][31], 0);
    pal[0] = RGB{0, 0, 0, 0};
    create_rgb_table(&map, pal, nullptr);
    EXPECT_NE(map.data[31][0][31], 0);
}
