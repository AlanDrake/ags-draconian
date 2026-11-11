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
#include "ac/dynamicsprite.h"

#include <cmath>
#include <map>

#include "ac/common.h"
#include "ac/draw.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/math.h" // M_PI
#include "ac/path_helper.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/system.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "gfx/graphicsdriver.h"
#include "gui/guibutton.h"
#include "script/runtimescriptvalue.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStruct thisroom;
extern RoomObject *objs;
extern RoomStatus *croom;

extern RGB palette[256];
extern AGS::Engine::IGraphicsDriver *gfxDriver;

char check_dynamic_sprites_at_exit = 1;

int ValidateColorFormat(const char *api_name, ScriptColorFormat color_format)
{
    switch (color_format)
    {
    case kScColorFmt_8bit: return color_format;
    case kScColorFmt_Default: return game.GetColorDepth();
    default:
        debug_script_log("%s: unsupported color depth %d, will fallback to game default %d", api_name, color_format,
                         game.GetColorDepth());
        return game.GetColorDepth();
    }
}

void DynamicSprite_Delete(ScriptDynamicSprite *dynsprite)
{
    if (dynsprite->slot > 0)
    {
        detach_dynsprite_surface(dynsprite->slot);
        free_dynamic_sprite(dynsprite->slot);
        dynsprite->slot = 0; // 0 is a "safety placeholder" sprite id
    }
}

ScriptDrawingSurface *DynamicSprite_GetDrawingSurface(ScriptDynamicSprite *dynsprite)
{
    // If there's already a active DrawingSurface, then return one
    ScriptDrawingSurface *surface = get_dynsprite_surface(dynsprite->slot);
    if (surface)
        return surface;

    surface = new ScriptDrawingSurface();
    surface->dynamicSpriteNumber = dynsprite->slot;
    int handle = ccRegisterManagedObject(surface, surface);
    attach_dynsprite_surface(dynsprite->slot, handle);
    return surface;
}

int DynamicSprite_GetGraphic(ScriptDynamicSprite *dynsprite)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.Graphic: Cannot get graphic, sprite has been deleted");
    return dynsprite->slot;
}

int DynamicSprite_GetWidth(ScriptDynamicSprite *dynsprite)
{
    return game.SpriteInfos[dynsprite->slot].Width;
}

int DynamicSprite_GetHeight(ScriptDynamicSprite *dynsprite)
{
    return game.SpriteInfos[dynsprite->slot].Height;
}

int DynamicSprite_GetColorDepth(ScriptDynamicSprite *dynsprite)
{
    // Dynamic sprite ensures the sprite exists always
    return spriteset[dynsprite->slot]->GetColorDepth();
}

void DynamicSprite_Resize(ScriptDynamicSprite *dynsprite, int width, int height)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.Resize: sprite has been deleted");
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Resize: width and height must be greater than zero");

    if (width * height >= 25000000)
        quitprintf("!DynamicSprite.Resize: new size is too large: %d x %d", width, height);

    // resize the sprite to the requested size
    Bitmap *sprite = spriteset[dynsprite->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth()));
    new_pic->StretchBlt(sprite,
                        RectWH(0, 0, game.SpriteInfos[dynsprite->slot].Width, game.SpriteInfos[dynsprite->slot].Height),
                        RectWH(0, 0, width, height));

    add_dynamic_sprite(dynsprite->slot, std::move(new_pic));
}

void DynamicSprite_Flip(ScriptDynamicSprite *dynsprite, int direction)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.Flip: sprite has been deleted");
    GraphicFlip flip = ValidateFlip("DynamicSprite.Flip", direction);

    Bitmap *sprite = spriteset[dynsprite->slot];
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateTransparentBitmap(sprite->GetWidth(), sprite->GetHeight(), sprite->GetColorDepth()));

    // AGS script FlipDirection corresponds to internal GraphicFlip
    new_pic->FlipBlt(sprite, 0, 0, flip);

    add_dynamic_sprite(dynsprite->slot, std::move(new_pic));
}

void DynamicSprite_CopyTransparencyMask(ScriptDynamicSprite *dynsprite, int source_sprite)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.CopyTransparencyMask: sprite has been deleted");

    if ((game.SpriteInfos[dynsprite->slot].Width != game.SpriteInfos[source_sprite].Width) ||
        (game.SpriteInfos[dynsprite->slot].Height != game.SpriteInfos[source_sprite].Height))
    {
        quit("!DynamicSprite.CopyTransparencyMask: sprites are not the same size");
    }

    Bitmap *dst = spriteset[dynsprite->slot];
    Bitmap *src = spriteset[source_sprite];

    if (dst->GetColorDepth() != src->GetColorDepth())
    {
        quit("!DynamicSprite.CopyTransparencyMask: sprites are not the same colour depth");
    }

    // merge the source's transparency (alpha channel or mask) into the dynamic sprite
    BitmapHelper::CopyTransparency(dst, src, dst->GetColorDepth() == 32, src->GetColorDepth() == 32);
    game_sprite_updated(dynsprite->slot);
}

void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *dynsprite, int width, int height, int x, int y)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.ChangeCanvasSize: sprite has been deleted");
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.ChangeCanvasSize: width and height must be greater than zero");

    Bitmap *sprite = spriteset[dynsprite->slot];
    if (sprite->GetWidth() == width && sprite->GetHeight() == height && x == 0 && y == 0)
        return; // same canvas size and no offset: no need to do anything

    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth()));
    new_pic->Blit(sprite, 0, 0, x, y, sprite->GetWidth(), sprite->GetHeight());

    // replace the bitmap in the sprite set
    add_dynamic_sprite(dynsprite->slot, std::move(new_pic));
}

void DynamicSprite_Crop(ScriptDynamicSprite *dynsprite, int x, int y, int width, int height)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.Crop: sprite has been deleted");
    if ((width < 1) || (height < 1))
        quit("!DynamicSprite.Crop: width and height must be greater than zero");

    // Warn if the crop area is outside the original image, then clamp it to the image
    if ((x < 0) || (y < 0) || (width > game.SpriteInfos[dynsprite->slot].Width - x) ||
        (height > game.SpriteInfos[dynsprite->slot].Height - y))
    {
        debug_script_warn("DynamicSprite.Crop: requested to crop to an area outside of the source image: image is "
                          "%dx%d, crop area %d,%d %dx%d",
                          game.SpriteInfos[dynsprite->slot].Width, game.SpriteInfos[dynsprite->slot].Height, x, y,
                          width, height);
    }
    Math::ClampLength(x, width, 0, game.SpriteInfos[dynsprite->slot].Width);
    Math::ClampLength(y, height, 0, game.SpriteInfos[dynsprite->slot].Height);

    if (width <= 0 || height <= 0)
        return; // cannot crop to zero size

    Bitmap *sprite = spriteset[dynsprite->slot];
    if (sprite->GetWidth() == width && sprite->GetHeight() == height && x == 0 && y == 0)
        return; // same canvas size and no offset: no need to do anything

    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, sprite->GetColorDepth()));
    new_pic->Blit(sprite, x, y, 0, 0, new_pic->GetWidth(), new_pic->GetHeight());

    // replace the bitmap in the sprite set
    add_dynamic_sprite(dynsprite->slot, std::move(new_pic));
}

void DynamicSprite_Rotate(ScriptDynamicSprite *dynsprite, int angle, int width, int height)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.Rotate: sprite has been deleted");
    if ((angle < 1) || (angle > 359))
        quit("!DynamicSprite.Rotate: invalid angle (must be 1-359)");
    if ((width != SCR_NO_VALUE) && (width < 1) || (height != SCR_NO_VALUE) && (height < 1))
        quit("!DynamicSprite.Rotate: width and height must be greater than zero");

    const int src_width = game.SpriteInfos[dynsprite->slot].Width;
    const int src_height = game.SpriteInfos[dynsprite->slot].Height;
    if ((width == SCR_NO_VALUE) || (height == SCR_NO_VALUE))
    {
        Size rot_sz = RotateSize(Size(src_width, src_height), angle);
        width = rot_sz.Width;
        height = rot_sz.Height;
    }

    // draw the sprite onto a new transparent bitmap of the requested size
    Bitmap *sprite = spriteset[dynsprite->slot];
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateTransparentBitmap(width, height, sprite->GetColorDepth()));

    // rotate the sprite about its centre
    // (+ width%2 fixes one pixel offset problem)
    new_pic->RotateBlt(sprite, width / 2 + width % 2, height / 2, src_width / 2, src_height / 2, angle);

    // replace the bitmap in the sprite set
    add_dynamic_sprite(dynsprite->slot, std::move(new_pic));
}

void DynamicSprite_Tint(ScriptDynamicSprite *dynsprite, int red, int green, int blue, int saturation, int luminance)
{
    Bitmap *src = spriteset[dynsprite->slot];
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateBitmap(src->GetWidth(), src->GetHeight(), src->GetColorDepth()));

    tint_image(new_pic.get(), src, red, green, blue, saturation, luminance);

    add_dynamic_sprite(dynsprite->slot, std::move(new_pic));
}

int DynamicSprite_SaveToFile(ScriptDynamicSprite *dynsprite, const char *name)
{
    if (dynsprite->slot == 0)
        quit("!DynamicSprite.SaveToFile: sprite has been deleted");

    auto filename = String(name);
    if (filename.FindChar('.') == String::NoIndex)
        filename.Append(".bmp");

    ResolvedPath rp = ResolveWritePathAndCreateDirs(filename);
    if (!rp)
        return 0;
    return spriteset[dynsprite->slot]->SaveToFile(rp.FullPath, palette) ? 1 : 0;
}

ScriptDynamicSprite *DynamicSprite_CreateFromSaveGame(int save_slot, int width, int height)
{
    auto screenshot = read_savedgame_screenshot(get_save_game_path(save_slot));
    if (!screenshot)
        return nullptr;

    // resize the sprite to the requested size
    if ((screenshot->GetWidth() != width) || (screenshot->GetHeight() != height))
    {
        std::unique_ptr<Bitmap> temp(BitmapHelper::CreateBitmap(width, height, screenshot->GetColorDepth()));
        temp->StretchBlt(screenshot.get(), RectWH(0, 0, screenshot->GetWidth(), screenshot->GetHeight()),
                         RectWH(0, 0, width, height));
        screenshot = std::move(temp);
    }

    int new_slot = add_dynamic_sprite(std::move(screenshot));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite *DynamicSprite_CreateFromFile(const char *filename, int color_fmt)
{
    std::unique_ptr<Stream> in(ResolveScriptPathAndOpen(filename, FileOpenMode::kFile_Open, StreamMode::kStream_Read));
    if (!in)
        return nullptr;

    String ext = Path::GetFileExtension(filename);
    int dst_color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));
    std::unique_ptr<Bitmap> image(BitmapHelper::LoadBitmap(in.get(), ext, dst_color_depth));
    if (!image)
        return nullptr;

    image.reset(PrepareSpriteForUse(image.release(), false /* no depth conv */, false /* not force opaque */));

    int new_slot = add_dynamic_sprite(std::move(image));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite *DynamicSprite_CreateFromScreenShot(int width, int height, int layers)
{
    // TODO: refactor and merge with create_savegame_screenshot()
    auto new_pic = create_game_screenshot(width, height, layers);
    if (!new_pic)
        return nullptr; // out of mem or invalid parameters
    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite *DynamicSprite_CreateFromScreenShot2(int width, int height)
{
    return DynamicSprite_CreateFromScreenShot(width, height, RENDER_BATCH_ALL);
}

ScriptDynamicSprite *DynamicSprite_CreateFromExistingSprite(int slot, int color_fmt)
{
    if (!spriteset.DoesSpriteExist(slot))
        quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

    int dst_color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));

    // create a new sprite as a copy of the existing one
    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmapCopy(spriteset[slot], dst_color_depth));
    if (!new_pic)
        return nullptr;

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite *DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *surface, int x, int y, int width,
                                                            int height, int color_fmt)
{
    if (!surface->IsValid())
    {
        debug_script_warn("DynamicSprite.CreateFromDrawingSurface: attempted to use surface after its source image was "
                          "disposed or DrawingSurface.Release() was called.");
        return nullptr;
    }

    int dst_color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));

    if (width <= 0 || height <= 0)
    {
        debug_script_warn("DynamicSprite.CreateFromDrawingSurface: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    Bitmap *src = surface->GetBitmapSurface();
    if ((x < 0) || (y < 0) || (x + width > src->GetWidth()) || (y + height > src->GetHeight()))
    {
        debug_script_warn("DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");
        Math::ClampLength(x, width, 0, src->GetWidth());
        Math::ClampLength(y, height, 0, src->GetHeight());
    }

    std::unique_ptr<Bitmap> new_pic(BitmapHelper::CreateBitmap(width, height, dst_color_depth));
    new_pic->Blit(src, x, y, 0, 0, width, height);

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite *DynamicSprite_Create(int width, int height, int color_fmt)
{
    int color_depth = ValidateColorFormat("DynamicSprite.Create", static_cast<ScriptColorFormat>(color_fmt));

    if (width <= 0 || height <= 0)
    {
        debug_script_warn("DynamicSprite.Create: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    std::unique_ptr<Bitmap> new_pic(CreateCompatBitmap(width, height, color_depth));
    if (!new_pic)
        return nullptr;

    new_pic->ClearTransparent();

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

ScriptDynamicSprite *DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height)
{
    if (frame == SCR_NO_VALUE)
    {
        frame = play.bg_frame;
    }
    else if ((frame < 0) || ((size_t)frame >= thisroom.BgFrameCount))
        quit("!DynamicSprite.CreateFromBackground: invalid frame specified");

    if (x1 == SCR_NO_VALUE)
        x1 = 0;
    if (y1 == SCR_NO_VALUE)
        y1 = 0;
    if (width == SCR_NO_VALUE)
        width = play.room_width;
    if (height == SCR_NO_VALUE)
        height = play.room_height;

    if (width <= 0 || height <= 0)
    {
        debug_script_warn("DynamicSprite.CreateFromBackground: invalid size %d x %d, will adjust", width, height);
        width = std::max(1, width);
        height = std::max(1, height);
    }

    if ((x1 < 0) || (y1 < 0) || (x1 + width > play.room_width) || (y1 + height > play.room_height))
        quit("!DynamicSprite.CreateFromBackground: invalid co-ordinates specified");

    // create a new sprite as a copy of the background area
    std::unique_ptr<Bitmap> new_pic(
        BitmapHelper::CreateBitmap(width, height, thisroom.BgImages[frame]->GetColorDepth()));
    if (!new_pic)
        return nullptr;

    new_pic->Blit(thisroom.BgImages[frame].get(), x1, y1, 0, 0, width, height);

    int new_slot = add_dynamic_sprite(std::move(new_pic));
    if (new_slot <= 0)
        return nullptr; // something went wrong
    return new ScriptDynamicSprite(new_slot);
}

//=============================================================================

int add_dynamic_sprite(std::unique_ptr<Bitmap> image, uint32_t extra_flags)
{
    uint32_t flags = SPF_DYNAMICALLOC | extra_flags;
    return spriteset.AddSprite(std::move(image), flags);
}

int add_dynamic_sprite(int slot, std::unique_ptr<Bitmap> image, uint32_t extra_flags)
{
    assert(slot > 0 && !spriteset.IsAssetSprite(slot));
    if (slot <= 0 || spriteset.IsAssetSprite(slot))
        return 0; // invalid slot, or reserved for the static sprite

    uint32_t flags = SPF_DYNAMICALLOC | extra_flags;
    if (!spriteset.SetSprite(slot, std::move(image), flags))
        return 0; // failed to add the sprite, bad image or realloc failed

    // Notify a new (normal) dynamic sprite in case some objects
    // have this number assigned to their Graphic property
    if ((extra_flags & SPF_OBJECTOWNED) == 0)
        game_sprite_updated(slot);
    return slot;
}

void free_dynamic_sprite(int slot, bool notify_all)
{
    assert((slot > 0) && (static_cast<size_t>(slot) < game.SpriteInfos.size()) &&
           (game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC));
    if ((slot <= 0) || (static_cast<size_t>(slot) >= game.SpriteInfos.size()) ||
        (game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) == 0)
        return;

    spriteset.DeleteSprite(slot);
    if (notify_all)
        game_sprite_updated(slot, true);
    else
        notify_sprite_changed(slot, true);
}

// Dynamic sprite (slot index) map to DrawingSurface handle.
// NOTE: I decided to put this separate from the ScriptDynamicSprite, because
// statistically a reference to drawing surface is a temporary data present
// in very small numbers (1-2) at one time; while there could be many hundreds
// and thousands of dynamic sprites in game. Thus keeping a DrawingSurface ref
// field in ScriptDynamicSprite will increase memory consumption in rates
// which are highly unproportional to the actual needs.
// TODO: of course this should not be a standalone global variable,
// but put into some kind of a manager.
// NOTE: this is multimap, because old versions of the engine allowed multiple
// script drawing surface objects per a sprite, and we may still restore older
// saves.
std::multimap<int, int> DynamicSpriteDSRef;

ScriptDrawingSurface *get_dynsprite_surface(int slot)
{
    auto ref_it = DynamicSpriteDSRef.find(slot);
    if ((ref_it != DynamicSpriteDSRef.end()) && (ref_it->second > 0))
    {
        return static_cast<ScriptDrawingSurface *>(ccGetObjectAddressFromHandle(ref_it->second));
    }
    return nullptr;
}

void attach_dynsprite_surface(int slot, int surface_handle)
{
    DynamicSpriteDSRef.insert(std::make_pair(slot, surface_handle));
    // Mark the sprite info as having sprite's surface acquired (for notification purposes)
    game.SpriteInfos[slot].Flags |= SPF_SURFACEACQUIRED;
}

void detach_dynsprite_surface(int slot)
{
    // Invalidate DrawingSurface (if one is currently acquired)
    auto range = DynamicSpriteDSRef.equal_range(slot);
    for (auto ref_it = range.first; ref_it != range.second; ++ref_it)
    {
        ScriptDrawingSurface *surface =
            static_cast<ScriptDrawingSurface *>(ccGetObjectAddressFromHandle(ref_it->second));
        if (surface)
            surface->Invalidate();
    }
    DynamicSpriteDSRef.erase(slot);
}

void on_dynsprite_surface_release(int slot, bool modified)
{
    if (modified)
    {
        game_sprite_updated(slot);
    }

    detach_dynsprite_surface(slot);
    // Unmark the sprite info as having sprite's surface acquired (for notification purposes)
    game.SpriteInfos[slot].Flags &= ~SPF_SURFACEACQUIRED;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// void (ScriptDynamicSprite *dynsprite, int width, int height, int x, int y)
RuntimeScriptValue Sc_DynamicSprite_ChangeCanvasSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDynamicSprite, DynamicSprite_ChangeCanvasSize);
}

// void (ScriptDynamicSprite *dynsprite, int source_sprite)
RuntimeScriptValue Sc_DynamicSprite_CopyTransparencyMask(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDynamicSprite, DynamicSprite_CopyTransparencyMask);
}

// void (ScriptDynamicSprite *dynsprite, int x, int y, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Crop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDynamicSprite, DynamicSprite_Crop);
}

// void (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDynamicSprite, DynamicSprite_Delete);
}

// void (ScriptDynamicSprite *dynsprite, int direction)
RuntimeScriptValue Sc_DynamicSprite_Flip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDynamicSprite, DynamicSprite_Flip);
}

// ScriptDrawingSurface* (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetDrawingSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDynamicSprite, ScriptDrawingSurface, DynamicSprite_GetDrawingSurface);
}

// void (ScriptDynamicSprite *dynsprite, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Resize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDynamicSprite, DynamicSprite_Resize);
}

// void (ScriptDynamicSprite *dynsprite, int angle, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Rotate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDynamicSprite, DynamicSprite_Rotate);
}

// int (ScriptDynamicSprite *dynsprite, const char* name)
RuntimeScriptValue Sc_DynamicSprite_SaveToFile(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptDynamicSprite, DynamicSprite_SaveToFile, const char);
}

// void (ScriptDynamicSprite *dynsprite, int red, int green, int blue, int saturation, int luminance)
RuntimeScriptValue Sc_DynamicSprite_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDynamicSprite, DynamicSprite_Tint);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetColorDepth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetColorDepth);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetGraphic);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetHeight);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetWidth);
}

RuntimeScriptValue Sc_DynamicSprite_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_Create);
}

// ScriptDynamicSprite* (int frame, int x1, int y1, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT5(ScriptDynamicSprite, DynamicSprite_CreateFromBackground);
}

// ScriptDynamicSprite* (ScriptDrawingSurface *surface, int x, int y, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT5(ScriptDynamicSprite, DynamicSprite_CreateFromDrawingSurface, ScriptDrawingSurface);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromExistingSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromExistingSprite);
}

// ScriptDynamicSprite* (const char *filename)
RuntimeScriptValue Sc_DynamicSprite_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT(ScriptDynamicSprite, DynamicSprite_CreateFromFile, const char);
}

// ScriptDynamicSprite* (int save_slot, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromSaveGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromSaveGame);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot2);
}


void RegisterDynamicSpriteAPI()
{
    // WARNING: DynamicSprite.Create and CreateFromExistingSprite have 1 param REPLACED since ags4
    ScFnRegister dynsprite_api[] = {
        {"DynamicSprite::Create^3", API_FN_PAIR(DynamicSprite_Create)},
        {"DynamicSprite::CreateFromBackground", API_FN_PAIR(DynamicSprite_CreateFromBackground)},
        {"DynamicSprite::CreateFromDrawingSurface^6", API_FN_PAIR(DynamicSprite_CreateFromDrawingSurface)},
        {"DynamicSprite::CreateFromExistingSprite^2", API_FN_PAIR(DynamicSprite_CreateFromExistingSprite)},
        {"DynamicSprite::CreateFromFile^2", API_FN_PAIR(DynamicSprite_CreateFromFile)},
        {"DynamicSprite::CreateFromSaveGame", API_FN_PAIR(DynamicSprite_CreateFromSaveGame)},
        {"DynamicSprite::CreateFromScreenShot^2", API_FN_PAIR(DynamicSprite_CreateFromScreenShot2)},
        {"DynamicSprite::CreateFromScreenShot^3", API_FN_PAIR(DynamicSprite_CreateFromScreenShot)},
        {"DynamicSprite::ChangeCanvasSize^4", API_FN_PAIR(DynamicSprite_ChangeCanvasSize)},
        {"DynamicSprite::CopyTransparencyMask^1", API_FN_PAIR(DynamicSprite_CopyTransparencyMask)},
        {"DynamicSprite::Crop^4", API_FN_PAIR(DynamicSprite_Crop)},
        {"DynamicSprite::Delete", API_FN_PAIR(DynamicSprite_Delete)},
        {"DynamicSprite::Flip^1", API_FN_PAIR(DynamicSprite_Flip)},
        {"DynamicSprite::GetDrawingSurface^0", API_FN_PAIR(DynamicSprite_GetDrawingSurface)},
        {"DynamicSprite::Resize^2", API_FN_PAIR(DynamicSprite_Resize)},
        {"DynamicSprite::Rotate^3", API_FN_PAIR(DynamicSprite_Rotate)},
        {"DynamicSprite::SaveToFile^1", API_FN_PAIR(DynamicSprite_SaveToFile)},
        {"DynamicSprite::Tint^5", API_FN_PAIR(DynamicSprite_Tint)},
        {"DynamicSprite::get_ColorDepth", API_FN_PAIR(DynamicSprite_GetColorDepth)},
        {"DynamicSprite::get_Graphic", API_FN_PAIR(DynamicSprite_GetGraphic)},
        {"DynamicSprite::get_Height", API_FN_PAIR(DynamicSprite_GetHeight)},
        {"DynamicSprite::get_Width", API_FN_PAIR(DynamicSprite_GetWidth)},
    };

    ccAddExternalFunctions(dynsprite_api);
}
