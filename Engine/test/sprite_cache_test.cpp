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
// The engine's sprite cache (the global spriteset, with the engine's sprite
// callbacks) as the engine uses it: game sprites load from the sprite file on
// demand, the least recently used ones are evicted when the cache is full and
// load again when needed, sprite 0 stays locked, precaching only loads, and
// dynamic sprites are kept out of eviction and their slots reused.
// Common/test/spritecache_test.cpp covers the sprite file and the slots; the
// texture cache on top (draw.cpp) needs an initialized graphics driver and is
// not tested.
//
//=============================================================================
#include <memory>
#include <vector>

#include "ac/dynamicsprite.h"
#include "ac/gamesetupstruct.h"
#include "ac/spritecache.h"
#include "gfx/ali3dsw.h"
#include "gfx/bitmap.h"
#include "gtest/gtest.h"
#include "util/memorystream.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern IGraphicsDriver *gfxDriver;

namespace
{

const int kSpriteCount = 10; // sprites 0..9 in the sprite file
const int kSpriteSize = 16;  // 16x16, 32-bit

color_t SpriteColor(int index)
{
    return 0xFF000000 | (index * 0x00102030);
}

} // namespace

// Fixture for the engine's sprite cache. The engine globals it touches, all reset in TearDown:
// - spriteset: opens a sprite file made in memory, with sprites 0..9, each filled with its own colour;
//   the cache limit is changed by the tests
// - game.SpriteInfos: filled from the sprite file
// - game.color_depth: 32-bit, so the engine's sprite initialization keeps the sprites as they are
// - gfxDriver: a software driver, not initialized: sprite initialization only asks it for the
//   bitmap format it can use
class EngineSpriteCache : public ::testing::Test
{
protected:
    void SetUp() override
    {
        game.color_depth = 4;
        _driver = std::make_unique<ALSW::SDLRendererGraphicsDriver>();
        gfxDriver = _driver.get();
        {
            std::vector<SpriteInfo> infos;
            SpriteCache source(infos, SpriteCache::Callbacks());
            for (int i = 0; i < kSpriteCount; ++i)
            {
                auto bmp = std::make_unique<Bitmap>(kSpriteSize, kSpriteSize, 32);
                bmp->Clear(SpriteColor(i));
                source.SetSprite(i, std::move(bmp));
            }
            SpriteFileIndex index;
            HError err =
                source.SaveToFile(std::make_unique<Stream>(std::make_unique<VectorStream>(_spriteFile, kStream_Write)),
                                  0, kSprCompress_None, index);
            ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        }
        HError err = spriteset.InitFile(std::make_unique<Stream>(std::make_unique<VectorStream>(_spriteFile)), nullptr);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        // the memory one sprite takes in the cache
        spriteset.PrecacheSprite(1);
        _oneSprite = spriteset.GetCacheSize();
        spriteset.DisposeAllFreeCached();
    }

    void TearDown() override
    {
        spriteset.Reset();
        spriteset.SetMaxCacheSize(DEFAULTCACHESIZE_KB * 1024u);
        game.SpriteInfos.clear();
        game.color_depth = 0;
        gfxDriver = nullptr;
        _driver.reset();
    }

    // Limits the cache to this many sprites (and a half, so that one more does not fit)
    void LimitTo(size_t sprites) { spriteset.SetMaxCacheSize(_oneSprite * sprites + _oneSprite / 2); }

    static void Use(int index) { ASSERT_NE(nullptr, spriteset[index]); }

    std::unique_ptr<ALSW::SDLRendererGraphicsDriver> _driver;
    std::vector<uint8_t> _spriteFile;
    size_t _oneSprite = 0u;
};

TEST_F(EngineSpriteCache, GameSpritesLoadOnDemand)
{
    ASSERT_GT(_oneSprite, 0u);
    EXPECT_EQ(static_cast<size_t>(kSpriteCount), game.SpriteInfos.size());
    EXPECT_EQ(kSpriteSize, game.SpriteInfos[3].Width);
    for (int i = 0; i < kSpriteCount; ++i)
    {
        EXPECT_TRUE(spriteset.IsAssetSprite(i));
        EXPECT_FALSE(spriteset.IsSpriteLoaded(i));
    }
    Bitmap *sprite = spriteset[3];
    ASSERT_NE(nullptr, sprite);
    EXPECT_TRUE(spriteset.IsSpriteLoaded(3));
    EXPECT_EQ(32, sprite->GetColorDepth());
    EXPECT_EQ(SpriteColor(3), static_cast<color_t>(sprite->GetPixel(5, 5)));
    EXPECT_EQ(_oneSprite, spriteset.GetCacheSize());
}

TEST_F(EngineSpriteCache, LeastRecentlyUsedSpriteIsEvicted)
{
    LimitTo(3);
    Use(1);
    Use(2);
    Use(3);
    Use(1); // 2 is now the least recently used
    Use(4);
    EXPECT_TRUE(spriteset.IsSpriteLoaded(1));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(2));
    EXPECT_TRUE(spriteset.IsSpriteLoaded(3));
    EXPECT_TRUE(spriteset.IsSpriteLoaded(4));
    EXPECT_LE(spriteset.GetCacheSize(), spriteset.GetMaxCacheSize());
    // an evicted sprite loads again, with its pixels
    Bitmap *sprite = spriteset[2];
    ASSERT_NE(nullptr, sprite);
    EXPECT_EQ(SpriteColor(2), static_cast<color_t>(sprite->GetPixel(0, 0)));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(3));
}

TEST_F(EngineSpriteCache, LoweringTheLimitEvictsAtOnce)
{
    for (int i = 1; i <= 6; ++i)
        Use(i);
    EXPECT_EQ(6 * _oneSprite, spriteset.GetCacheSize());
    LimitTo(2);
    EXPECT_LE(spriteset.GetCacheSize(), spriteset.GetMaxCacheSize());
    // the two most recently used remain
    EXPECT_TRUE(spriteset.IsSpriteLoaded(5));
    EXPECT_TRUE(spriteset.IsSpriteLoaded(6));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(4));
}

TEST_F(EngineSpriteCache, SpriteZeroStaysLocked)
{
    LimitTo(3);
    Use(0);
    EXPECT_EQ(_oneSprite, spriteset.GetLockedSize());
    for (int i = 1; i < kSpriteCount; ++i)
        Use(i);
    EXPECT_TRUE(spriteset.IsSpriteLoaded(0));
    // the locked sprite counts against the limit: two others fit beside it
    EXPECT_TRUE(spriteset.IsSpriteLoaded(9));
    EXPECT_TRUE(spriteset.IsSpriteLoaded(8));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(7));
}

TEST_F(EngineSpriteCache, PrecachedSpriteCanBeEvicted)
{
    // Game.PrecacheSprite loads a sprite ahead, but does not lock it
    LimitTo(2);
    spriteset.PrecacheSprite(7);
    EXPECT_TRUE(spriteset.IsSpriteLoaded(7));
    EXPECT_EQ(0u, spriteset.GetLockedSize());
    Use(1);
    Use(2);
    EXPECT_FALSE(spriteset.IsSpriteLoaded(7));
}

TEST_F(EngineSpriteCache, DynamicSpritesAreNotEvicted)
{
    LimitTo(2);
    auto image = std::make_unique<Bitmap>(64, 64, 32);
    image->Clear(0xFF123456);
    const int slot = add_dynamic_sprite(std::move(image));
    ASSERT_GE(slot, kSpriteCount);
    EXPECT_FALSE(spriteset.IsAssetSprite(slot));
    EXPECT_EQ(64, game.SpriteInfos[slot].Width);
    // dynamic sprites do not count against the limit
    EXPECT_EQ(0u, spriteset.GetCacheSize());
    EXPECT_GT(spriteset.GetExternalSize(), 0u);
    for (int i = 1; i < kSpriteCount; ++i)
        Use(i);
    ASSERT_TRUE(spriteset.DoesSpriteExist(slot));
    EXPECT_EQ(0xFF123456u, static_cast<color_t>(spriteset[slot]->GetPixel(10, 10)));

    // a freed slot is taken by the next dynamic sprite
    free_dynamic_sprite(slot);
    EXPECT_FALSE(spriteset.DoesSpriteExist(slot));
    EXPECT_EQ(0u, spriteset.GetExternalSize());
    EXPECT_EQ(slot, add_dynamic_sprite(std::make_unique<Bitmap>(8, 8, 32)));
    free_dynamic_sprite(slot);
}

TEST_F(EngineSpriteCache, DynamicSpriteReplacingAGameSpriteIsRefused)
{
#ifdef NDEBUG
    // a dynamic sprite may not take a slot of the game's own sprites
    EXPECT_EQ(0, add_dynamic_sprite(3, std::make_unique<Bitmap>(8, 8, 32)));
    EXPECT_TRUE(spriteset.IsAssetSprite(3));
#else
    GTEST_SKIP() << "debug builds assert on this input";
#endif
}

TEST_F(EngineSpriteCache, ClearingTheCacheKeepsLockedSprites)
{
    // what the engine does on a room change, with clear_cache_on_room_change in the config
    Use(0); // sprite 0 is always locked
    Use(1);
    Use(2);
    Use(3);
    spriteset.DisposeAllFreeCached();
    EXPECT_TRUE(spriteset.IsSpriteLoaded(0));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(1));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(2));
    EXPECT_FALSE(spriteset.IsSpriteLoaded(3));
    EXPECT_EQ(_oneSprite, spriteset.GetCacheSize());
    EXPECT_EQ(_oneSprite, spriteset.GetLockedSize());
    // the cache goes on working: sprites load and are freed again
    Use(1);
    EXPECT_EQ(2 * _oneSprite, spriteset.GetCacheSize());
    EXPECT_EQ(_oneSprite, spriteset.GetLockedSize());
    spriteset.DisposeAllFreeCached();
    EXPECT_EQ(_oneSprite, spriteset.GetCacheSize());
    EXPECT_TRUE(spriteset.IsSpriteLoaded(0));
}
