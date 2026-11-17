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
#include "game/render/overlay.h"

#include <algorithm>
#include <queue>

#include "debug/debug_log.h"
#include "game/dialog/display.h"
#include "game/dynobj/script_shader.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/render/screen_overlay.h"
#include "game/runtime_defines.h"
#include "game/translation.h"
#include "game/view_def.h"
#include "game/world/character.h"
#include "game/world/object.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "main/game_run.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// All overlays by their IDs; the fixed IDs (OVER_*) are below OVER_FIRSTFREE, custom ones are allocated above
IndexedObjectPool<ScreenOverlay, int32_t> screenover(OVER_FIRSTFREE);
// TODO: reimplement AnimatedOverlays storage using IndexedObjectPool, just like regular overlays
std::vector<AnimatedOverlay> animovers; // animation records; a removed one stays as an empty record
std::vector<size_t> over_to_anim;       // overlay ID to its index in animovers, SIZE_MAX if it has no animation
std::queue<size_t> animover_free_ids;   // indexes of the empty records in animovers, to be reused


ScreenOverlay *GetOverlayValidate(const char *apiname, int over_id)
{
    auto *over = get_overlay(over_id);
    if (!over)
        debug_script_error("%s: invalid overlay specified (%d)", apiname, over_id);
    return over;
}

void Overlay_Remove(ScriptOverlay *script_over)
{
    script_over->Remove();
}

void Overlay_SetText(ScriptOverlay *script_over, int width, int font, int text_color, const char *text)
{
    auto *over = GetOverlayValidate("Overlay.SetText", script_over);
    if (over)
        Overlay_SetText(*over, over->GetX(), over->GetY(), width, font, text_color, text);
}

void Overlay_SetText(ScreenOverlay &over, int x, int y, int width, int font, int text_color, const char *text)
{
    Game &game = Game::Current();
    // TODO: find a nice way to refactor and share these code pieces
    // with Overlay_CreateTextCore
    DisplayTextPosition text_pos = over.IsAutoPosition() ? kDisplayTextPos_Overchar : kDisplayTextPos_Normal;
    const bool auto_width = (width < 0);

    // Allow automatic-width text to shrink to fit.
    DisplayTextShrink allow_shrink =
        (auto_width || over.IsAutoPosition()) ? kDisplayTextShrink_Left : kDisplayTextShrink_None;

    // Calculate automatic width using the same limit as Display().
    if (auto_width)
    {
        width = game.Viewports.GetUIViewport().GetWidth() * 3 / 4;
    }
    else
    {
        if (width < 8)
            width = game.Viewports.GetUIViewport().GetWidth() / 2;
    }

    if (text_color == 0)
        text_color = GUI::GetStandardColor(16);

    const char *draw_text = get_translation(text);
    // Skip a voice-over token, if present
    draw_text = skip_voiceover_token(draw_text);

    // Recreate overlay image
    int adj_x = x, adj_y = y;
    DisplayTextLooks look(kDisplayTextStyle_TextWindow, (DisplayTextPosition)text_pos, allow_shrink);
    look.PreserveTextColor = (game.LoadedDataVersion >= kGameVersion_400_33);

    std::unique_ptr<Bitmap> image =
        create_textual_image(draw_text, look, text_color, x, y, adj_x, adj_y, width, font, nullptr);

    // Update overlay properties
    over.SetFixedPosition(x, y);
    over.SetImage(std::move(image), adj_x - x, adj_y - y);
    over.SetText(text);
}

int Overlay_GetX(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.X", script_over);
    if (over)
    {
        if (over->IsAutoPosition())
            autoposition_overlay(*over); // in case they moved character in script, etc
        return over->GetX();
    }
    return 0;
}

void Overlay_SetX(ScriptOverlay *script_over, int new_x)
{
    auto *over = GetOverlayValidate("Overlay.X", script_over);
    if (over)
        over->SetFixedPosition(new_x, over->GetY());
}

int Overlay_GetY(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Y", script_over);
    if (over)
    {
        if (over->IsAutoPosition())
            autoposition_overlay(*over); // in case they moved character in script, etc
        return over->GetY();
    }
    return 0;
}

void Overlay_SetY(ScriptOverlay *script_over, int new_y)
{
    auto *over = GetOverlayValidate("Overlay.Y", script_over);
    if (over)
        over->SetFixedPosition(over->GetX(), new_y);
}

bool Overlay_GetAutoSize(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.AutoSize", script_over);
    return over ? over->IsAutoSize() : false;
}

void Overlay_SetAutoSize(ScriptOverlay *script_over, bool auto_size)
{
    auto *over = GetOverlayValidate("Overlay.AutoSize", script_over);
    if (over)
        over->SetAutoSize(auto_size);
}

int Overlay_GetGraphic(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Graphic", script_over);
    return over ? over->GetSpriteNum() : 0;
}

void Overlay_SetGraphic(ScriptOverlay *script_over, int slot)
{
    auto *over = GetOverlayValidate("Overlay.Graphic", script_over);
    if (!EngineServices::Current().Sprites.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.SetGraphic: sprite %d is invalid", slot);
        slot = 0;
    }
    if (over)
        over->SetSpriteNum(slot);
}

bool Overlay_InRoom(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.InRoom", script_over);
    return over ? over->IsRoomLayer() : false;
}

int Overlay_GetWidth(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Width", script_over);
    return over ? over->GetDestinationWidth() : 0;
}

int Overlay_GetHeight(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Height", script_over);
    return over ? over->GetDestinationHeight() : 0;
}

int Overlay_GetGraphicWidth(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicWidth", script_over);
    return over ? over->GetGraphicSize().Width : 0;
}

int Overlay_GetGraphicHeight(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicHeight", script_over);
    return over ? over->GetGraphicSize().Height : 0;
}

void Overlay_SetWidth(ScriptOverlay *script_over, int width)
{
    auto *over = GetOverlayValidate("Overlay.Width", script_over);
    if (over)
    {
        if (width > 0)
            over->SetDestinationSize(width, over->GetDestinationHeight());
        else
            debug_script_warn("Overlay.Width: invalid width %d", width);
    }
}

void Overlay_SetHeight(ScriptOverlay *script_over, int height)
{
    auto *over = GetOverlayValidate("Overlay.Height", script_over);
    if (over)
    {
        if (height > 0)
            over->SetDestinationSize(over->GetDestinationWidth(), height);
        else
            debug_script_warn("Overlay.Height: invalid height %d", height);
    }
}

int Overlay_GetFlip(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Flip", script_over);
    return over ? over->GetFlip() : kFlip_None;
}

void Overlay_SetFlip(ScriptOverlay *script_over, int flip)
{
    auto *over = GetOverlayValidate("Overlay.Flip", script_over);
    if (over)
        over->SetFlip(ValidateFlip("Overlay.Flip", flip));
}

float Overlay_GetGraphicAnchorX(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicAnchorX", script_over);
    return over ? over->GetSpriteAnchor().X : 0.f;
}

void Overlay_SetGraphicAnchorX(ScriptOverlay *script_over, float x)
{
    auto *over = GetOverlayValidate("Overlay.GraphicAnchorX", script_over);
    if (over)
        over->SetSpriteAnchor(Pointf(x, over->GetSpriteAnchor().Y));
}

float Overlay_GetGraphicAnchorY(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicAnchorY", script_over);
    return over ? over->GetSpriteAnchor().Y : 0.f;
}

void Overlay_SetGraphicAnchorY(ScriptOverlay *script_over, float y)
{
    auto *over = GetOverlayValidate("Overlay.GraphicAnchorY", script_over);
    if (over)
        over->SetSpriteAnchor(Pointf(over->GetSpriteAnchor().X, y));
}

int Overlay_GetGraphicOffsetX(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicOffsetX", script_over);
    return over ? over->GetSpriteOffset().X : 0;
}

void Overlay_SetGraphicOffsetX(ScriptOverlay *script_over, int x)
{
    auto *over = GetOverlayValidate("Overlay.GraphicOffsetX", script_over);
    if (over)
        over->SetSpriteOffset(Point(x, over->GetSpriteOffset().Y));
}

int Overlay_GetGraphicOffsetY(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicOffsetY", script_over);
    return over ? over->GetSpriteOffset().Y : 0;
}

void Overlay_SetGraphicOffsetY(ScriptOverlay *script_over, int y)
{
    auto *over = GetOverlayValidate("Overlay.GraphicOffsetY", script_over);
    if (over)
        over->SetSpriteOffset(Point(over->GetSpriteOffset().X, y));
}

void *Overlay_GetGraphicPosition(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GetGraphicPosition", script_over);
    if (over)
    {
        over->UpdateGraphicSpace(EngineServices::Current());
        std::vector<Point> points;
        over->GetGraphicSpace().GetTransformedCorners(points, over->GetGraphicSize());
        return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
    }
    return nullptr;
}

void *Overlay_GetGraphicBoundBox(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GetGraphicBoundBox", script_over);
    if (over)
    {
        over->UpdateGraphicSpace(EngineServices::Current());
        std::vector<Point> points;
        over->GetGraphicSpace().GetAABBPoints(points);
        return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
    }
    return nullptr;
}

int Overlay_GetValid(ScriptOverlay *script_over)
{
    return get_overlay(script_over->GetOverlayID()) != nullptr;
}

ScreenOverlay *Overlay_CreateGraphicCore(bool room_layer, int x, int y, int slot, bool clone)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    if (!services.Sprites.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.CreateGraphical: sprite %d is invalid", slot);
        return nullptr;
    }

    size_t over_id;
    // We clone only dynamic sprites, because it makes no sense to clone normal ones
    if (clone && ((game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) != 0))
    {
        std::unique_ptr<Bitmap> image(
            new Bitmap(game.SpriteInfos[slot].Width, game.SpriteInfos[slot].Height, game.GetColorDepth()));
        image->Blit(services.Sprites[slot]);
        over_id = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, std::move(image));
    }
    else
    {
        over_id = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, slot);
    }
    return over_id < SIZE_MAX ? &screenover[over_id] : nullptr;
}

ScreenOverlay *Overlay_CreateTextCore(bool room_layer, int x, int y, int width, int font, int text_color,
                                      const char *text, int over_type, DisplayTextStyle style,
                                      DisplayTextShrink allow_shrink, int speech_for_char)
{
    Game &game = Game::Current();
    // NOTE: this was not documented, but apparently passing x < 0 or y < 0
    // to Overlay.CreateTextual actually made it centered on screen
    DisplayTextPosition text_pos = get_textpos_from_scriptcoords(x, y, false);
    if (width < 8)
        width = game.Viewports.GetUIViewport().GetWidth() / 2;
    if (text_color == 0)
        text_color = GUI::GetStandardColor(16);
    // Skip a voice-over token, if present
    const char *draw_text = skip_voiceover_token(text);
    DisplayTextLooks look(style, text_pos, allow_shrink);
    look.PreserveTextColor = (game.LoadedDataVersion >= kGameVersion_400_33);

    return display_main(x, y, width, draw_text, nullptr, kDisplayText_NormalOverlay, over_type, look, font, text_color,
                        speech_for_char, room_layer);
}

ScriptOverlay *Overlay_CreateGraphicalImpl(bool room_layer, int x, int y, int slot, bool clone)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, clone);
    return over ? over->CreateScriptObject() : nullptr;
}

ScriptOverlay *Overlay_CreateTextualImpl(bool room_layer, int x, int y, int width, int font, int text_color,
                                         const char *text)
{
    const bool auto_width = (width < 0);
    if (auto_width)
    {
        // Use the same initial width as Display().
        width = Game::Current().Viewports.GetUIViewport().GetWidth() * 3 / 4;
    }

    auto *over = Overlay_CreateTextCore(room_layer, x, y, width, font, text_color, text, OVER_CUSTOM,
                                        kDisplayTextStyle_TextWindow,
                                        auto_width ? kDisplayTextShrink_Left : kDisplayTextShrink_None);
    return over ? over->CreateScriptObject() : nullptr;
}

ScriptOverlay *Overlay_CreateGraphical(int x, int y, int slot, bool clone)
{
    return Overlay_CreateGraphicalImpl(false, x, y, slot, clone);
}

// This is a fallback for running 3.6.0 games, strictly for regression testing convenience
ScriptOverlay *Overlay_CreateGraphical5(int x, int y, int slot, bool /* dummy */, bool clone)
{
    return Overlay_CreateGraphical(x, y, slot, clone);
}

ScriptOverlay *Overlay_CreateTextual(int x, int y, int width, int font, int text_color, const char *text)
{
    return Overlay_CreateTextualImpl(false, x, y, width, font, text_color, text);
}

ScriptOverlay *Overlay_CreateRoomGraphical(int x, int y, int slot, bool clone)
{
    return Overlay_CreateGraphicalImpl(true, x, y, slot, clone);
}

// This is a fallback for running 3.6.0 games, strictly for regression testing convenience
ScriptOverlay *Overlay_CreateRoomGraphical5(int x, int y, int slot, bool /*dummy*/, bool clone)
{
    return Overlay_CreateRoomGraphical(x, y, slot, clone);
}

ScriptOverlay *Overlay_CreateRoomTextual(int x, int y, int width, int font, int text_color, const char *text)
{
    return Overlay_CreateTextualImpl(true, x, y, width, font, text_color, text);
}

const char *Overlay_GetTextProperty(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Text", script_over);
    return over ? CreateNewScriptString(over->GetText()) : nullptr;
}

void Overlay_SetTextProperty(ScriptOverlay *script_over, const char *text)
{
    auto *over = GetOverlayValidate("Overlay.Text", script_over);
    if (over)
        over->SetText(text);
}

int Overlay_GetBlendMode(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.BlendMode", script_over);
    return over ? over->GetBlendMode() : kBlend_Normal;
}

void Overlay_SetBlendMode(ScriptOverlay *script_over, int blend_mode)
{
    auto *over = GetOverlayValidate("Overlay.BlendMode", script_over);
    if (over)
        over->SetBlendMode(ValidateBlendMode("Overlay.BlendMode", "", blend_mode));
}

ScriptShaderInstance *Overlay_GetShader(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Shader", script_over);
    return over ? static_cast<ScriptShaderInstance *>(ccGetObjectAddressFromHandle(over->GetShaderHandle())) : nullptr;
}

void Overlay_SetShader(ScriptOverlay *script_over, ScriptShaderInstance *shader_inst)
{
    auto *over = GetOverlayValidate("Overlay.Shader", script_over);
    if (over)
        over->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                        ccReplaceObjectHandle(over->GetShaderHandle(), shader_inst));
}

int Overlay_GetOpacity(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Opacity", script_over);
    return over ? over->GetOpacity() : 0;
}

void Overlay_SetOpacity(ScriptOverlay *script_over, int alpha)
{
    auto *over = GetOverlayValidate("Overlay.Opacity", script_over);
    if ((alpha < 0) || (alpha > 255))
        quit("!Overlay.Opacity: alpha value must be between 0 and 255");

    if (over)
        over->SetOpacity(alpha);
}

// [OBSOLETE] the AGS 4.0 Transparency, for compiled 4.0 games
int Overlay_GetTransparency(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Transparency", script_over);
    return over ? GfxDef::OpacityToTrans100(over->GetOpacity()) : 0;
}

void Overlay_SetTransparency(ScriptOverlay *script_over, int trans)
{
    auto *over = GetOverlayValidate("Overlay.Transparency", script_over);
    if ((trans < 0) || (trans > 100))
        quit("!SetTransparency: transparency value must be between 0 and 100");

    if (over)
        over->SetOpacity(GfxDef::Trans100ToOpacity(trans));
}

bool Overlay_GetVisible(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Visible", script_over);
    return over ? over->IsVisible() : false;
}

void Overlay_SetVisible(ScriptOverlay *script_over, bool visible)
{
    auto *over = GetOverlayValidate("Overlay.Visible", script_over);
    if (over)
        over->SetVisible(visible);
}

float Overlay_GetRotation(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.Rotation", script_over);
    return over ? over->GetRotation() : 0.f;
}

void Overlay_SetRotation(ScriptOverlay *script_over, float degrees)
{
    auto *over = GetOverlayValidate("Overlay.Rotation", script_over);
    if (over)
        over->SetRotation(Math::ClampAngle360(degrees));
}

float Overlay_GetGraphicPivotX(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotX", script_over);
    return over ? over->GetPivot().X : 0;
}

void Overlay_SetGraphicPivotX(ScriptOverlay *script_over, float pivot_x)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotX", script_over);
    if (over)
        over->SetPivot(Pointf(pivot_x, over->GetPivot().Y));
}

float Overlay_GetGraphicPivotY(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotY", script_over);
    return over ? over->GetPivot().Y : 0;
}

void Overlay_SetGraphicPivotY(ScriptOverlay *script_over, float pivot_y)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotY", script_over);
    if (over)
        over->SetPivot(Pointf(over->GetPivot().X, pivot_y));
}

int Overlay_GetGraphicPivotOffsetX(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotOffsetX", script_over);
    return over ? over->GetPivotOffset().X : 0;
}

void Overlay_SetGraphicPivotOffsetX(ScriptOverlay *script_over, int pivot_offset_x)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotOffsetX", script_over);
    if (over)
        over->SetPivotOffset(Point(pivot_offset_x, over->GetPivotOffset().Y));
}

int Overlay_GetGraphicPivotOffsetY(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotOffsetY", script_over);
    return over ? over->GetPivotOffset().Y : 0;
}

void Overlay_SetGraphicPivotOffsetY(ScriptOverlay *script_over, int pivot_offset_y)
{
    auto *over = GetOverlayValidate("Overlay.GraphicPivotOffsetY", script_over);
    if (over)
        over->SetPivotOffset(Point(over->GetPivotOffset().X, pivot_offset_y));
}

int Overlay_GetZOrder(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.ZOrder", script_over);
    return over ? over->GetZOrder() : 0;
}

void Overlay_SetZOrder(ScriptOverlay *script_over, int zorder)
{
    auto *over = GetOverlayValidate("Overlay.ZOrder", script_over);
    if (over)
        over->SetZOrder(zorder);
}

void Overlay_SetPosition(ScriptOverlay *script_over, int x, int y, int width, int height)
{
    auto *over = GetOverlayValidate("Overlay.SetPosition", script_over);
    if (over)
    {
        over->SetFixedPosition(x, y);
        // width and height are optional here
        if (width > 0 || height > 0)
        {
            width = width > 0 ? width : over->GetDestinationWidth();
            height = height > 0 ? height : over->GetDestinationHeight();
            over->SetDestinationSize(width, height);
        }
    }
}

void Overlay_SetSize(ScriptOverlay *script_over, int width, int height)
{
    auto *over = GetOverlayValidate("Overlay.SetSize", script_over);
    if (over)
    {
        if (width > 0 && height > 0)
            over->SetDestinationSize(width, height);
        else
            debug_script_warn("Overlay.SetSize: invalid dimensions: %d x %d", width, height);
    }
}

void Overlay_SetScale(ScriptOverlay *script_over, float scale_x, float scale_y)
{
    auto *over = GetOverlayValidate("Overlay.SetScale", script_over);
    if (over)
        over->SetScale(scale_x, scale_y);
}

float Overlay_GetScaleX(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.ScaleX", script_over);
    return over ? over->GetScale().X : 0;
}

void Overlay_SetScaleX(ScriptOverlay *script_over, float scale_x)
{
    auto *over = GetOverlayValidate("Overlay.ScaleX", script_over);
    if (over)
        over->SetScale(scale_x, over->GetScale().Y);
}

float Overlay_GetScaleY(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.ScaleY", script_over);
    return over ? over->GetScale().Y : 0;
}

void Overlay_SetScaleY(ScriptOverlay *script_over, float scale_y)
{
    auto *over = GetOverlayValidate("Overlay.ScaleY", script_over);
    if (over)
        over->SetScale(over->GetScale().X, scale_y);
}

void Overlay_Tint(ScriptOverlay *script_over, int red, int green, int blue, int opacity, int luminance)
{
    auto *over = GetOverlayValidate("Overlay.Tint", script_over);
    if (!over)
        return;

    if ((red < 0) || (green < 0) || (blue < 0) || (red > 255) || (green > 255) || (blue > 255) || (opacity < 0) ||
        (opacity > 255) || (luminance < 0) || (luminance > 255))
    {
        debug_script_warn("Overlay.Tint: invalid parameter(s). R,G,B must be 0-255 (passed: %d,%d,%d), opacity & "
                          "luminance 0-255 (passed: %d,%d)",
                          red, green, blue, opacity, luminance);
        return;
    }

    over->SetTint(red, green, blue, opacity, luminance);
}

void Overlay_SetLightLevel(ScriptOverlay *script_over, int light_level)
{
    auto *over = GetOverlayValidate("Overlay.SetLightLevel", script_over);
    if (over)
        over->SetLightLevel(Math::Clamp(light_level, -255, 255));
}

void Overlay_RemoveTint(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.RemoveTint", script_over);
    if (over)
        over->RemoveTint();
}

bool Overlay_GetHasLight(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.HasLight", script_over);
    return over ? over->HasLightLevel() : false;
}

bool Overlay_GetHasTint(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.HasTint", script_over);
    return over ? over->HasTint() : false;
}

int Overlay_GetLightLevel(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.LightLevel", script_over);
    return over ? (over->HasLightLevel() ? over->GetTintLight() : 0) : 0;
}

int Overlay_GetTintRed(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.TintRed", script_over);
    return over ? (over->HasTint() ? over->GetTintR() : 0) : 0;
}

int Overlay_GetTintGreen(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.TintGreen", script_over);
    return over ? (over->HasTint() ? over->GetTintG() : 0) : 0;
}

int Overlay_GetTintBlue(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.TintBlue", script_over);
    return over ? (over->HasTint() ? over->GetTintB() : 0) : 0;
}

int Overlay_GetTintSaturation(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.TintSaturation", script_over);
    return over ? (over->HasTint() ? over->GetTintLevel() : 0) : 0;
}

int Overlay_GetTintLuminance(ScriptOverlay *script_over)
{
    auto *over = GetOverlayValidate("Overlay.TintLuminance", script_over);
    return over ? (over->HasTint() ? over->GetTintLight() : 0) : 0;
}

//=============================================================================

ScriptAnimatedOverlay *create_scriptanimoverlay(ScreenOverlay &over)
{
    ScriptAnimatedOverlay *script_over = new ScriptAnimatedOverlay(over.GetID());
    over.AssignScriptObject(script_over);
    return script_over;
}

void remove_screen_overlay(int type)
{
    Game &game = Game::Current();
    if (type < 0 || static_cast<uint32_t>(type) >= screenover.size() || screenover.IsFree(type))
        return; // requested non-existing overlay

    // Remove animation instance if one exists for this overlay
    if (IsOverlayAnimating(type))
        RemoveAnimatedOverlay(type);

    ScreenOverlay &over = screenover[type];
    // Dispose overlay's resources and try dispose a script object (if one exists)
    over.OnRemove();
    // Do necessary changes to the game state after overlay's removal
    // TODO: move these custom settings outside of this function
    // TODO: consider using a callback function ptr, run when overlay is removed
    if (over.GetID() == game.Play.complete_overlay_on)
    {
        game.Play.complete_overlay_on = 0;
    }
    else if (over.GetID() == game.Play.text_overlay_on)
    {
        // reset ref for speech text
        game.Play.speech_text_schandle = ccRemoveObjectHandle(game.Play.speech_text_schandle);
        game.Play.text_overlay_on = 0;
    }
    else if (over.GetID() == OVER_PICTURE)
    {
        // reset ref for speech face
        game.Play.speech_face_schandle = ccRemoveObjectHandle(game.Play.speech_face_schandle);
        game.Speech.FaceTalking = -1;
    }
    else if (over.GetCharacterRef() >= 0)
    {
        // release internal ref for bg speech
        ccReleaseObjectReference(over.GetScriptHandle());
    }

    screenover.Free(type);

    reset_drawobj_for_overlay(type);

    game.Play.overlay_count--;
    // If all overlays have been removed, reset dynamic draw index (helps vs overflows);
    // we do this here, because overlays are the only dynamic objects atm
    if (game.Play.overlay_count == 0)
        reset_drawobj_dynamic_index();
}

void remove_all_overlays()
{
    RemoveAllAnimatedOverlays();
    for (auto &over : screenover)
        remove_screen_overlay(over.GetID());
}

ScreenOverlay *get_overlay(int type)
{
    return (type >= 0 && static_cast<uint32_t>(type) < screenover.size() && screenover.IsInUse(type))
               ? &screenover[type]
               : nullptr;
}

static size_t add_screen_overlay_impl(bool room_layer, int x, int y, int type, int sprite_num,
                                      std::unique_ptr<Bitmap> image, int offset_x, int offset_y)
{
    Game &game = Game::Current();
    // If a custom overlay is requested, then allocate a free slot
    if (type == OVER_CUSTOM)
    {
        type = screenover.Add();
    }
    else
    {
        screenover.Set(type);
    }

    ScreenOverlay over(type);
    if (image)
    {
        over.SetImage(std::move(image), offset_x, offset_y);
    }
    else
    {
        over.SetSpriteNum(sprite_num, offset_x, offset_y);
    }
    over.SetFixedPosition(x, y);
    // By default draw speech and portraits are over GUI, and the rest under GUI;
    // room overlays are created above all by default as well
    if (room_layer || type == OVER_TEXTMSG)
        over.SetZOrder(INT32_MAX);
    else if (type == OVER_PICTURE || type == OVER_TEXTSPEECH)
        over.SetZOrder(game.Play.speech_zorder);
    else
        over.SetZOrder(INT_MIN);
    over.SetRoomLayer(room_layer);
    // TODO: move these custom settings outside of this function
    if (type == OVER_COMPLETE)
    {
        game.Play.complete_overlay_on = type;
    }
    else if (type == OVER_TEXTMSG || type == OVER_TEXTSPEECH)
    {
        game.Play.text_overlay_on = type;
        // only make script object for blocking speech now, because messagebox blocks all script
        // and therefore cannot be accessed, so no practical reason for that atm
        if (type == OVER_TEXTSPEECH)
        {
            game.Play.speech_text_schandle = ccAssignObjectHandle(over.CreateScriptObject());
        }
    }
    else if (type == OVER_PICTURE)
    {
        game.Play.speech_face_schandle = ccAssignObjectHandle(over.CreateScriptObject());
    }
    over.MarkChanged();
    screenover[type] = std::move(over);
    game.Play.overlay_count++;
    add_drawobj_for_overlay(type);
    return type;
}

size_t add_screen_overlay(bool room_layer, int x, int y, int type, int sprite_num)
{
    return add_screen_overlay_impl(room_layer, x, y, type, sprite_num, nullptr, 0, 0);
}

size_t add_screen_overlay(bool room_layer, int x, int y, int type, std::unique_ptr<Bitmap> image)
{
    return add_screen_overlay_impl(room_layer, x, y, type, -1, std::move(image), 0, 0);
}

size_t add_screen_overlay(bool room_layer, int x, int y, int type, std::unique_ptr<Bitmap> image, int offset_x,
                          int offset_y)
{
    return add_screen_overlay_impl(room_layer, x, y, type, -1, std::move(image), offset_x, offset_y);
}

// Calculates overlay position above linked character: centred on the character's x, above its head and kept
// inside the UI viewport; if the character is in another room, the overlay is centred on the screen
// TODO: this code assumes that overlay is in the screen layer;
// perhaps add a branch for room layer as well?
void autoposition_overlay(ScreenOverlay &over)
{
    Game &game = Game::Current();
    assert(over.IsAutoPosition());
    if (!over.IsAutoPosition() || (over.GetCharacterRef() < 0))
        return;

    const Rect &ui_view = game.Viewports.GetUIViewport();
    const int char_id = over.GetCharacterRef();

    auto view = FindNearestViewport(char_id);
    const int char_sprite =
        game.Views[game.Characters[char_id].View].Loops[game.Characters[char_id].Loop].Frames[0].Sprite;
    const int height =
        (game.Characters[char_id].Height < 1) ? game.SpriteInfos[char_sprite].Height : game.Characters[char_id].Height;
    const Point screen_pt =
        view->RoomToScreen(game.Characters[char_id].X, game.Characters[char_id].GetEffectiveY() - height).first;
    const Size pic_size = over.GetGraphicSize();
    int over_x = std::max(0, screen_pt.X - pic_size.Width / 2);
    int over_y = screen_pt.Y - 5;
    over_y -= pic_size.Height;
    over_y = std::max(5, over_y);

    if ((over_x + pic_size.Width) >= ui_view.GetWidth())
        over_x = (ui_view.GetWidth() - pic_size.Width) - 1;
    if (game.Characters[char_id].Room != game.DisplayedRoom)
    {
        over_x = ui_view.GetWidth() / 2 - pic_size.Width / 2;
        over_y = ui_view.GetHeight() / 2 - pic_size.Height / 2;
    }

    over.SetAutoPosition(char_id, over_x, over_y);
}

void restore_overlays()
{
    // TODO: an iterator that goes over only valid elements
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        if (screenover.IsFree(i))
            continue; // empty slot
        auto &over = screenover[i];
        if (over.GetID() >= 0)
        {
            if (over.IsAutoPosition())
                autoposition_overlay(over);
            over.MarkChanged(); // force recreate texture on next draw
        }
    }
}

IndexedObjectPool<ScreenOverlay, int32_t> &get_overlays()
{
    return screenover;
}

void CreateAnimatedOverlay(int over_id, bool pause_with_game)
{
    AddAnimatedOverlay(AnimatedOverlay(over_id, pause_with_game));
}

static void UpdateOverlayState(Game &game, const AnimatedOverlay &aover)
{
    assert(aover.GetOverID() >= 0 && aover.GetOverID() < screenover.size() && screenover.IsInUse(aover.GetOverID()));
    if (aover.GetOverID() < 0 || aover.GetOverID() >= screenover.size() || !screenover.IsInUse(aover.GetOverID()))
        return;

    auto &over = screenover[aover.GetOverID()];
    const ViewFrame *view_frame = aover.GetViewFrame(game);
    if (view_frame)
    {
        over.SetSpriteNum(view_frame->Sprite, view_frame->OffsetX, view_frame->OffsetY);
        over.SetFlip(GfxDef::GetFlipFromFlags(view_frame->Flags));
    }
}

void BeginAnimateOverlay(int over_id, int view, int loop, int frame, const ViewAnimateParams &params)
{
    auto *aover = GetOverlayAnimation(over_id);
    if (!aover)
        return;

    aover->Begin(view, loop, frame, params);
    UpdateOverlayState(Game::Current(), *aover);
}

size_t AddAnimatedOverlay(AnimatedOverlay &&aover)
{
    const int over_id = aover.GetOverID();
    if (over_id < 0)
        return SIZE_MAX;

    if (static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
    {
        animovers[over_to_anim[over_id]] = std::move(aover);
        return over_to_anim[over_id];
    }
    else
    {
        // Find free id
        size_t aover_id;
        if (animover_free_ids.size() > 0)
        {
            aover_id = animover_free_ids.front();
            animover_free_ids.pop();
        }
        else
        {
            aover_id = animovers.size();
            animovers.resize(aover_id + 1);
        }

        animovers[aover_id] = std::move(aover);
        if (static_cast<uint32_t>(over_id) >= over_to_anim.size())
            over_to_anim.resize(over_id + 1, SIZE_MAX);
        over_to_anim[over_id] = aover_id;
        return aover_id;
    }
}

const std::vector<AnimatedOverlay> &GetAnimatedOverlays()
{
    return animovers;
}

bool IsOverlayAnimating(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
        return animovers[over_to_anim[over_id]].IsAnimating();
    else
        return false;
}

AnimatedOverlay *GetOverlayAnimation(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
        return &animovers[over_to_anim[over_id]];
    return nullptr;
}

static void StopOverlayAnimImpl(size_t aover_id)
{
    animovers[aover_id].Reset();
}

void UpdateOverlayAnimations()
{
    Game &game = Game::Current();
    for (size_t i = 0; i < animovers.size(); ++i)
    {
        auto &aover = animovers[i];
        if (!aover.IsAnimating())
            continue;
        if (aover.GetPauseWithGame() && IsGamePaused())
            continue;

        if (aover.UpdateOnce(game))
        {
            UpdateOverlayState(game, aover);
        }
        else
        {
            StopOverlayAnimImpl(i);
        }
    }
}

void RemoveAnimatedOverlay(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
    {
        size_t aover_id = over_to_anim[over_id];
        animovers[aover_id] = {};
        animover_free_ids.push(aover_id);
        over_to_anim[over_id] = SIZE_MAX;
    }
}

void RemoveAllAnimatedOverlays()
{
    animovers.clear();
    over_to_anim.clear();
    animover_free_ids = std::queue<size_t>();
}

//=============================================================================

ScriptAnimatedOverlay *AnimatedOverlay_CreateAnimatedImpl(bool room_layer, int x, int y, int slot, bool pause_with_game)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, false);
    if (!over)
        return nullptr;
    CreateAnimatedOverlay(over->GetID(), pause_with_game);
    return create_scriptanimoverlay(*over);
}

ScriptAnimatedOverlay *AnimatedOverlay_CreateAnimated(int x, int y, int slot, bool pause_with_game)
{
    return AnimatedOverlay_CreateAnimatedImpl(false, x, y, slot, pause_with_game);
}

ScriptAnimatedOverlay *AnimatedOverlay_CreateRoomAnimated(int x, int y, int slot, bool pause_with_game)
{
    return AnimatedOverlay_CreateAnimatedImpl(true, x, y, slot, pause_with_game);
}

void AnimatedOverlay_Animate(ScriptAnimatedOverlay *script_over, int view, int loop, int speed, int repeat,
                             int blocking, int direction, int start_frame, int volume)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Animate", script_over);
    if (!over)
        return;

    view--; // convert to internal 0-based view ID
    ValidateViewAnimVLF("AnimatedOverlay.Animate", "", view, loop, start_frame);
    ValidateViewAnimParams("AnimatedOverlay.Animate", "", blocking, repeat, direction);

    volume = Math::Clamp(volume, 0, 100);

    BeginAnimateOverlay(over->GetID(), view, loop, start_frame,
                        ViewAnimateParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction),
                                          speed, volume));

    // Blocking animate
    if (blocking)
        GameLoopUntilOverlayAnimEnd(over->GetID());
}

bool AnimatedOverlay_GetAnimating(ScriptAnimatedOverlay *script_over)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Animating", script_over);
    return over ? IsOverlayAnimating(over->GetID()) : false;
}

int AnimatedOverlay_GetFrame(ScriptAnimatedOverlay *script_over)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Frame", script_over);
    const auto *aover = over ? GetOverlayAnimation(over->GetID()) : nullptr;
    return aover ? aover->GetFrame() : -1;
}

int AnimatedOverlay_GetLoop(ScriptAnimatedOverlay *script_over)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Loop", script_over);
    const auto *aover = over ? GetOverlayAnimation(over->GetID()) : nullptr;
    return aover ? aover->GetLoop() : -1;
}

bool AnimatedOverlay_GetPauseWithGame(ScriptAnimatedOverlay *script_over)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.PauseWithGame", script_over);
    const auto *aover = over ? GetOverlayAnimation(over->GetID()) : nullptr;
    return aover ? aover->GetPauseWithGame() : false;
}

int AnimatedOverlay_GetView(ScriptAnimatedOverlay *script_over)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.View", script_over);
    const auto *aover = over ? GetOverlayAnimation(over->GetID()) : nullptr;
    return aover ? aover->GetView() + 1 : 0; // convert view index from internal to script
}
