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
// The engine's sprite cache (EngineServices::Sprites, with the engine's sprite
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

#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/render/dynamic_sprite.h"
#include "gfx/ali3dsw.h"
#include "gfx/bitmap.h"
#include "gfx/sprite_cache.h"
#include "gtest/gtest.h"
#include "platform/base/engine_services.h"
#include "util/memorystream.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace
{

const int kSpriteCount = 10; // sprites 0..9 in the sprite file
const int kSpriteSize = 16;  // 16x16, 32-bit

color_t SpriteColor(int index)
{
    return 0xFF000000 | (index * 0x00102030);
}

} // namespace

// Fixture for the engine's sprite cache. The engine state it touches, all reset in TearDown:
// - EngineServices::Sprites: given the game's sprite table and the engine's callbacks (init_sprite_cache; they stay
//   attached, as in the engine), opens a sprite file made in memory, with sprites 0..9, each filled with its own
//   colour; the cache limit is changed by the tests
// - game.SpriteInfos: filled from the sprite file
// - game.color_depth: 32-bit, so the engine's sprite initialization keeps the sprites as they are
// - EngineServices::GfxDriver: a software driver, not initialized: sprite initialization only asks it for the
//   bitmap format it can use
class EngineSpriteCache : public ::testing::Test
{
protected:
    void SetUp() override
    {
        EngineServices &services = EngineServices::Current();
        init_sprite_cache();
        Game::Current().color_depth = 4;
        _driver = std::make_unique<ALSW::SDLRendererGraphicsDriver>();
        services.GfxDriver = _driver.get();
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
        HError err =
            services.Sprites.InitFile(std::make_unique<Stream>(std::make_unique<VectorStream>(_spriteFile)), nullptr);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        // the memory one sprite takes in the cache
        services.Sprites.PrecacheSprite(1);
        _oneSprite = services.Sprites.GetCacheSize();
        services.Sprites.DisposeAllFreeCached();
    }

    void TearDown() override
    {
        EngineServices &services = EngineServices::Current();
        Game &game = Game::Current();
        services.Sprites.Reset();
        services.Sprites.SetMaxCacheSize(DEFAULTCACHESIZE_KB * 1024u);
        game.SpriteInfos.clear();
        game.color_depth = 0;
        services.GfxDriver = nullptr;
        _driver.reset();
    }

    // Limits the cache to this many sprites (and a half, so that one more does not fit)
    void LimitTo(size_t sprites)
    {
        EngineServices::Current().Sprites.SetMaxCacheSize(_oneSprite * sprites + _oneSprite / 2);
    }

    static void Use(int index) { ASSERT_NE(nullptr, EngineServices::Current().Sprites[index]); }

    std::unique_ptr<ALSW::SDLRendererGraphicsDriver> _driver;
    std::vector<uint8_t> _spriteFile;
    size_t _oneSprite = 0u;
};

TEST_F(EngineSpriteCache, GameSpritesLoadOnDemand)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    ASSERT_GT(_oneSprite, 0u);
    EXPECT_EQ(static_cast<size_t>(kSpriteCount), game.SpriteInfos.size());
    EXPECT_EQ(kSpriteSize, game.SpriteInfos[3].Width);
    for (int i = 0; i < kSpriteCount; ++i)
    {
        EXPECT_TRUE(services.Sprites.IsAssetSprite(i));
        EXPECT_FALSE(services.Sprites.IsSpriteLoaded(i));
    }
    Bitmap *sprite = services.Sprites[3];
    ASSERT_NE(nullptr, sprite);
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(3));
    EXPECT_EQ(32, sprite->GetColorDepth());
    EXPECT_EQ(SpriteColor(3), static_cast<color_t>(sprite->GetPixel(5, 5)));
    EXPECT_EQ(_oneSprite, services.Sprites.GetCacheSize());
}

TEST_F(EngineSpriteCache, LeastRecentlyUsedSpriteIsEvicted)
{
    EngineServices &services = EngineServices::Current();
    LimitTo(3);
    Use(1);
    Use(2);
    Use(3);
    Use(1); // 2 is now the least recently used
    Use(4);
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(1));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(2));
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(3));
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(4));
    EXPECT_LE(services.Sprites.GetCacheSize(), services.Sprites.GetMaxCacheSize());
    // an evicted sprite loads again, with its pixels
    Bitmap *sprite = services.Sprites[2];
    ASSERT_NE(nullptr, sprite);
    EXPECT_EQ(SpriteColor(2), static_cast<color_t>(sprite->GetPixel(0, 0)));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(3));
}

TEST_F(EngineSpriteCache, LoweringTheLimitEvictsAtOnce)
{
    EngineServices &services = EngineServices::Current();
    for (int i = 1; i <= 6; ++i)
        Use(i);
    EXPECT_EQ(6 * _oneSprite, services.Sprites.GetCacheSize());
    LimitTo(2);
    EXPECT_LE(services.Sprites.GetCacheSize(), services.Sprites.GetMaxCacheSize());
    // the two most recently used remain
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(5));
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(6));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(4));
}

TEST_F(EngineSpriteCache, SpriteZeroStaysLocked)
{
    EngineServices &services = EngineServices::Current();
    LimitTo(3);
    Use(0);
    EXPECT_EQ(_oneSprite, services.Sprites.GetLockedSize());
    for (int i = 1; i < kSpriteCount; ++i)
        Use(i);
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(0));
    // the locked sprite counts against the limit: two others fit beside it
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(9));
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(8));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(7));
}

TEST_F(EngineSpriteCache, PrecachedSpriteCanBeEvicted)
{
    EngineServices &services = EngineServices::Current();
    // Game.PrecacheSprite loads a sprite ahead, but does not lock it
    LimitTo(2);
    services.Sprites.PrecacheSprite(7);
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(7));
    EXPECT_EQ(0u, services.Sprites.GetLockedSize());
    Use(1);
    Use(2);
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(7));
}

TEST_F(EngineSpriteCache, DynamicSpritesAreNotEvicted)
{
    EngineServices &services = EngineServices::Current();
    LimitTo(2);
    auto image = std::make_unique<Bitmap>(64, 64, 32);
    image->Clear(0xFF123456);
    const int slot = add_dynamic_sprite(std::move(image));
    ASSERT_GE(slot, kSpriteCount);
    EXPECT_FALSE(services.Sprites.IsAssetSprite(slot));
    EXPECT_EQ(64, Game::Current().SpriteInfos[slot].Width);
    // dynamic sprites do not count against the limit
    EXPECT_EQ(0u, services.Sprites.GetCacheSize());
    EXPECT_GT(services.Sprites.GetExternalSize(), 0u);
    for (int i = 1; i < kSpriteCount; ++i)
        Use(i);
    ASSERT_TRUE(services.Sprites.DoesSpriteExist(slot));
    EXPECT_EQ(0xFF123456u, static_cast<color_t>(services.Sprites[slot]->GetPixel(10, 10)));

    // a freed slot is taken by the next dynamic sprite
    free_dynamic_sprite(slot);
    EXPECT_FALSE(services.Sprites.DoesSpriteExist(slot));
    EXPECT_EQ(0u, services.Sprites.GetExternalSize());
    EXPECT_EQ(slot, add_dynamic_sprite(std::make_unique<Bitmap>(8, 8, 32)));
    free_dynamic_sprite(slot);
}

TEST_F(EngineSpriteCache, DynamicSpriteReplacingAGameSpriteIsRefused)
{
#ifdef NDEBUG
    // a dynamic sprite may not take a slot of the game's own sprites
    EXPECT_EQ(0, add_dynamic_sprite(3, std::make_unique<Bitmap>(8, 8, 32)));
    EXPECT_TRUE(EngineServices::Current().Sprites.IsAssetSprite(3));
#else
    GTEST_SKIP() << "debug builds assert on this input";
#endif
}

TEST_F(EngineSpriteCache, ClearingTheCacheKeepsLockedSprites)
{
    EngineServices &services = EngineServices::Current();
    // what the engine does on a room change, with clear_cache_on_room_change in the config
    Use(0); // sprite 0 is always locked
    Use(1);
    Use(2);
    Use(3);
    services.Sprites.DisposeAllFreeCached();
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(0));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(1));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(2));
    EXPECT_FALSE(services.Sprites.IsSpriteLoaded(3));
    EXPECT_EQ(_oneSprite, services.Sprites.GetCacheSize());
    EXPECT_EQ(_oneSprite, services.Sprites.GetLockedSize());
    // the cache goes on working: sprites load and are freed again
    Use(1);
    EXPECT_EQ(2 * _oneSprite, services.Sprites.GetCacheSize());
    EXPECT_EQ(_oneSprite, services.Sprites.GetLockedSize());
    services.Sprites.DisposeAllFreeCached();
    EXPECT_EQ(_oneSprite, services.Sprites.GetCacheSize());
    EXPECT_TRUE(services.Sprites.IsSpriteLoaded(0));
}
