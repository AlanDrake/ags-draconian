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
#include <cmath>
#include <map>

#include "gfx/gfx_def.h"
#include "gfx/graphicspace.h"
#include "gtest/gtest.h"

using namespace AGS::Common;

namespace
{

const float kOrigins[] = {0.f, 0.5f, 1.f};
const int kZooms[] = {50, 60, 73, 75, 85, 100, 125, 150, 200, 300}; // percents
const int kPos = 100;                                               // the sprite's position on the axis

// Samples one axis of a sprite as a GPU does with nearest filtering: for each pixel whose centre lies in the
// quad, the texel of the original image (flipped back); counts the pixel centres within 1e-3 of a texel edge
std::map<int, int> SampleSprite(int src_len, int zoom, float origin, bool flipped, int &centres_on_edges)
{
    const float edge = GetSpriteQuadEdge(kPos, src_len, zoom, 100, origin, flipped);
    const double scale = zoom / 100.0;
    std::map<int, int> texels;
    centres_on_edges = 0;
    for (int px = static_cast<int>(std::floor(edge)) - 1; px <= edge + src_len * scale + 1; ++px)
    {
        const double u = (px + 0.5 - edge) / scale;
        if (std::fabs(u - std::round(u)) < 1e-3 && u > -1e-3 && u < src_len + 1e-3)
            ++centres_on_edges;
        if (u < 0.0 || u >= src_len)
            continue;
        const int t = static_cast<int>(std::floor(u));
        texels[px] = flipped ? (src_len - 1 - t) : t;
    }
    return texels;
}

// The origin texel as an index into the original image
int OriginTexel(int src_len, float origin, bool flipped)
{
    const int t = GetSpriteOriginTexel(src_len, origin, flipped);
    return flipped ? (src_len - 1 - t) : t;
}

} // namespace

// Unscaled and unflipped, sprites land where the engine has always put them
TEST(SpritePlacement, UnscaledAsBefore)
{
    for (int len = 1; len <= 64; ++len)
    {
        for (float origin : {0.f, 0.25f, 0.5f, 1.f})
        {
            const float before = kPos - std::roundf((len - 1) * origin);
            EXPECT_EQ(GetSpriteQuadEdge(kPos, len, 1, 1, origin, false), before) << len << " " << origin;
            EXPECT_EQ(GetSpriteQuadEdge(kPos, len, 100, 100, origin, false), before) << len << " " << origin;
            // flipped: the same, except where the origin falls between two texels, which now mirrors
            const bool between = std::fabs((len - 1) * origin - std::floor((len - 1) * origin) - 0.5f) < 1e-6f;
            EXPECT_EQ(GetSpriteQuadEdge(kPos, len, 1, 1, origin, true), between ? before + 1.f : before)
                << len << " " << origin;
        }
    }
}

// Frames of different sizes map the pixels around the position to the same texels, relative to the origin
// texel, at every zoom; this is what keeps scaled animations from jumping between frames
TEST(SpritePlacement, NoDriftBetweenSizes)
{
    for (float origin : kOrigins)
        for (int zoom : kZooms)
            for (bool flipped : {false, true})
            {
                std::map<int, int> first;
                for (int len = 20; len <= 51; ++len)
                {
                    int on_edges = 0;
                    std::map<int, int> rel;
                    const int origin_texel = OriginTexel(len, origin, flipped);
                    for (const auto &pt : SampleSprite(len, zoom, origin, flipped, on_edges))
                        if (std::abs(pt.second - origin_texel) <= 8)
                            rel[pt.first] = pt.second - origin_texel;
                    if (first.empty())
                    {
                        first = rel;
                        continue;
                    }
                    for (const auto &pt : rel)
                    {
                        const auto it = first.find(pt.first);
                        if (it != first.end())
                            EXPECT_EQ(pt.second, it->second) << "origin " << origin << " zoom " << zoom << " flip "
                                                             << flipped << " len " << len << " px " << pt.first;
                    }
                }
            }
}

// A flipped image is the exact mirror of the unflipped one around the origin texel (centred origin)
TEST(SpritePlacement, FlipMirrorsAroundOrigin)
{
    for (int zoom : kZooms)
        for (int len = 20; len <= 51; ++len)
        {
            int on_edges = 0;
            const auto plain = SampleSprite(len, zoom, 0.5f, false, on_edges);
            const auto flipped = SampleSprite(len, zoom, 0.5f, true, on_edges);
            bool mirrored = false;
            for (int axis2 = 2 * kPos - 1; axis2 <= 2 * kPos + 1 && !mirrored; ++axis2)
            {
                mirrored = true;
                for (const auto &pt : flipped)
                {
                    const auto it = plain.find(axis2 - pt.first);
                    if (it != plain.end() && it->second != pt.second)
                        mirrored = false;
                }
            }
            EXPECT_TRUE(mirrored) << "zoom " << zoom << " len " << len;
        }
}

// No pixel centre falls on a texel edge, where the GPU's rounding would pick the texel
TEST(SpritePlacement, NoPixelCentreOnTexelEdge)
{
    for (float origin : kOrigins)
        for (int zoom : kZooms)
            for (bool flipped : {false, true})
                for (int len = 20; len <= 51; ++len)
                {
                    int on_edges = 0;
                    SampleSprite(len, zoom, origin, flipped, on_edges);
                    EXPECT_EQ(on_edges, 0)
                        << "origin " << origin << " zoom " << zoom << " flip " << flipped << " len " << len;
                }
}

// The origin texel stays over the position whenever the zoom leaves it a pixel centre to cover:
// always for a centred origin, and at zooms of 50% and up for the edges
TEST(SpritePlacement, OriginTexelCoversPosition)
{
    for (float origin : kOrigins)
        for (int zoom : kZooms)
            for (bool flipped : {false, true})
                for (int len = 20; len <= 51; ++len)
                {
                    int on_edges = 0;
                    const auto texels = SampleSprite(len, zoom, origin, flipped, on_edges);
                    const auto it = texels.find(kPos);
                    ASSERT_TRUE(it != texels.end()) << origin << " " << zoom << " " << flipped << " " << len;
                    EXPECT_EQ(it->second, OriginTexel(len, origin, flipped))
                        << "origin " << origin << " zoom " << zoom << " flip " << flipped << " len " << len;
                }
}

TEST(GfxDef, LegacyTransparency)
{
    // Test that every transparency which is a multiple of 10 is converted
    // forth and back without loosing precision
    const size_t arr_sz = 11;
    const int trans100[arr_sz] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    int trans255[arr_sz] = {0};
    int trans100_back[arr_sz] = {0};

    for (int i = 0; i < arr_sz; ++i)
    {
        trans255[i] = GfxDef::Trans100ToLegacyTrans255(trans100[i]);
        trans100_back[i] = GfxDef::LegacyTrans255ToTrans100(trans255[i]);
        ASSERT_TRUE(trans100[i] == trans100_back[i]);
    }
}

// The 4.0 Transparency script property over opacity reads back what AGS 4.0 read back, which kept
// the legacy 255-ranged transparency
TEST(GfxDef, Trans100OverOpacity)
{
    EXPECT_EQ(GfxDef::Trans100ToOpacity(0), 255);
    EXPECT_EQ(GfxDef::Trans100ToOpacity(100), 0);
    EXPECT_EQ(GfxDef::OpacityToTrans100(255), 0);
    EXPECT_EQ(GfxDef::OpacityToTrans100(0), 100);
    for (int t = 0; t <= 100; ++t)
    {
        const int legacy = GfxDef::Trans100ToLegacyTrans255(t);
        EXPECT_EQ(GfxDef::Trans100ToOpacity(t), GfxDef::LegacyTrans255ToAlpha255(legacy)) << t;
        EXPECT_EQ(GfxDef::OpacityToTrans100(GfxDef::Trans100ToOpacity(t)), GfxDef::LegacyTrans255ToTrans100(legacy))
            << t;
    }
    EXPECT_EQ(GfxDef::OpacityToTrans100(GfxDef::Trans100ToOpacity(37)), 38); // as in 4.0
    EXPECT_EQ(GfxDef::OpacityToTrans100(GfxDef::Trans100ToOpacity(50)), 50);
    for (int a = 0; a <= 255; ++a)
    {
        const int t = GfxDef::OpacityToTrans100(a);
        EXPECT_TRUE(t >= 0 && t <= 100) << a;
    }
}
