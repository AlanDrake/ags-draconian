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
#include "game/world/walkable_area.h"

#include "debug/debug_log.h"
#include "game/dynobj/cc_walkable_area.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/roomstruct.h"
#include "game/world/character.h"
#include "game/world/object.h"
#include "game/world/properties.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_object.h"
#include "game/world/room_status.h"
#include "gfx/bitmap.h"
#include "script/script_runtime.h"
#include "util/common.h"

using namespace AGS::Common;

void redo_walkable_areas()
{
    Room &room = Room::Current();
    // restore the mask from the backup, then erase the pixels of areas that are switched off
    room.WalkAreaMask->Blit(room.WalkAreaBackup, 0, 0);
    for (int y = 0; y < room.WalkAreaBackup->GetHeight(); ++y)
    {
        uint8_t *walls_scanline = room.WalkAreaMask->GetScanLineForWriting(y);
        for (int x = 0; x < room.WalkAreaBackup->GetWidth(); ++x)
        {
            if ((walls_scanline[x] >= sizeof(room.WalkableAreasOn)) || (room.WalkableAreasOn[walls_scanline[x]] == 0))
                walls_scanline[x] = 0;
        }
    }
}

int get_walkable_area_pixel(Room &room, int x, int y)
{
    return room.WalkAreaMask->GetPixel(room_to_mask_coord(room, x), room_to_mask_coord(room, y));
}

int get_area_scaling(Room &room, int area_id, int y)
{
    int zoom_level = 100;
    y = room_to_mask_coord(room, y); // WalkArea Top and Bottom are mask rows

    if ((area_id >= 0) && (area_id < MAX_WALK_AREAS) && (room.WalkAreas[area_id].ScalingNear != NOT_VECTOR_SCALED))
    {
        // We have vector scaling!
        // In case the character is off the screen, limit the Y co-ordinate
        // to within the area range (otherwise we get silly zoom levels
        // that cause Out Of Memory crashes)
        if (y > room.WalkAreas[area_id].Bottom)
            y = room.WalkAreas[area_id].Bottom;
        if (y < room.WalkAreas[area_id].Top)
            y = room.WalkAreas[area_id].Top;
        // Work it all out without having to use floats
        // Percent = ((y - top) * 100) / (bottom - top)
        // Zoom level = ((near - far) * Percent) / 100 + far, in percent points from 100
        if (room.WalkAreas[area_id].Bottom != room.WalkAreas[area_id].Top)
        {
            int percent = ((y - room.WalkAreas[area_id].Top) * 100) /
                          (room.WalkAreas[area_id].Bottom - room.WalkAreas[area_id].Top);
            zoom_level =
                ((room.WalkAreas[area_id].ScalingNear - room.WalkAreas[area_id].ScalingFar) * (percent)) / 100 +
                room.WalkAreas[area_id].ScalingFar;
        }
        else
        {
            // Special case for 1px tall walkable area: take bottom line scaling
            zoom_level = room.WalkAreas[area_id].ScalingNear;
        }
        zoom_level += 100; // the scaling is stored as the percentage minus 100 (SetAreaScaling)
    }
    else if ((area_id >= 0) && (area_id < MAX_WALK_AREAS))
        zoom_level = room.WalkAreas[area_id].ScalingFar + 100;

    // zero would make the sprite vanish: treat it as normal size
    if (zoom_level == 0)
        zoom_level = 100;

    return zoom_level;
}

void scale_sprite_size(Game &game, int sprite_id, int zoom_level, int *new_width, int *new_height)
{
    new_width[0] = (game.SpriteInfos[sprite_id].Width * zoom_level) / 100;
    new_height[0] = (game.SpriteInfos[sprite_id].Height * zoom_level) / 100;
    if (new_width[0] < 1)
        new_width[0] = 1;
    if (new_height[0] < 1)
        new_height[0] = 1;
}

void remove_walkable_areas_from_temp(Room &room, const Rect &area)
{
    const Rect mask_area = Rect(room_to_mask_coord(room, area.Left), room_to_mask_coord(room, area.Top),
                                room_to_mask_coord(room, area.Right), room_to_mask_coord(room, area.Bottom));

    room.WalkableAreasTemp->FillRect(mask_area, 0);
}

// TODO: possibly optimize this by caching and/or knowing if walkable area changed since the last call?
// Copies the walkable mask to room.WalkableAreasTemp and cuts out the blocking rectangles of the enabled,
// blocking characters (other than source_char_id) and of the solid objects in the current room, except the
// rectangles source_char_id stands in.
Bitmap *prepare_walkable_areas(int source_char_id)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    // copy the walkable areas to the temp bitmap
    room.WalkableAreasTemp->Blit(room.WalkAreaMask.get(), 0, 0, 0, 0, room.WalkAreaMask->GetWidth(),
                                 room.WalkAreaMask->GetHeight());
    // if the character who's moving doesn't block, don't bother checking: it passes through everybody
    if ((source_char_id >= 0) && (game.Characters[source_char_id].flags & CHF_NOBLOCKING))
        return room.WalkableAreasTemp;

    // for each character in the current room, make the area under them unwalkable
    for (int char_id = 0; char_id < game.numcharacters; char_id++)
    {
        if (!game.Characters[char_id].is_enabled())
            continue;
        if (game.Characters[char_id].room != game.DisplayedRoom)
            continue;
        if (char_id == source_char_id)
            continue;
        if (game.Characters[char_id].flags & CHF_NOBLOCKING)
            continue;
        if (room_to_mask_coord(room, game.Characters[char_id].y) >= room.WalkableAreasTemp->GetHeight())
            continue;
        if (room_to_mask_coord(room, game.Characters[char_id].x) >= room.WalkableAreasTemp->GetWidth())
            continue;
        if ((game.Characters[char_id].y < 0) || (game.Characters[char_id].x < 0))
            continue;

        // If walking character is already inside that other character's blocking rect,
        // then ignore that blocking rect (otherwise character may get stuck forever)
        const Rect blocking_rect = get_char_blocking_rect(game, char_id);
        if ((source_char_id >= 0) &&
            blocking_rect.IsInside(game.Characters[source_char_id].x, game.Characters[source_char_id].y))
            continue;

        remove_walkable_areas_from_temp(room, blocking_rect);
    }

    // check for any blocking objects in the room, and deal with them as well
    for (uint32_t obj_id = 0; obj_id < room.State->numobj; obj_id++)
    {
        if (!room.Objs[obj_id].is_enabled())
            continue;
        if ((room.Objs[obj_id].flags & OBJF_SOLID) == 0)
            continue;
        if (room_to_mask_coord(room, room.Objs[obj_id].y) >= room.WalkableAreasTemp->GetHeight())
            continue;
        if (room_to_mask_coord(room, room.Objs[obj_id].x) >= room.WalkableAreasTemp->GetWidth())
            continue;
        if ((room.Objs[obj_id].y < 0) || (room.Objs[obj_id].x < 0))
            continue;

        Rect blocking_rect = get_object_blocking_rect(room, obj_id);

        // if the character is currently standing on the object, ignore
        // it so as to allow him to escape
        if ((source_char_id >= 0) &&
            blocking_rect.IsInside(game.Characters[source_char_id].x, game.Characters[source_char_id].y))
            continue;

        remove_walkable_areas_from_temp(room, blocking_rect);
    }

    return room.WalkableAreasTemp;
}

int get_walkable_area_at_location(Room &room, int x, int y)
{
    int area_id = get_walkable_area_pixel(room, x, y);

    if (area_id < 0)
    {
        // the point is outside the mask, e.g. the character has walked off the edge of the room: take the
        // area at the nearest edge, so that the scaling does not jump to full size when leaving
        if (x >= room.Width)
            area_id = get_walkable_area_pixel(room, room.Width - 1, y);
        else if (x < 0)
            area_id = get_walkable_area_pixel(room, 0, y);
        else if (y >= room.Height)
            area_id = get_walkable_area_pixel(room, x, room.Height - 1);
        else if (y < 0)
            area_id = get_walkable_area_pixel(room, x, 0);
    }
    if (area_id == 0)
    {
        // the path finder sometimes slightly goes into non-walkable areas;
        // so check for scaling in adjacent pixels (TRYGAP room pixels away, not mask pixels)
        const int TRYGAP = 2;
        area_id = get_walkable_area_pixel(room, x + TRYGAP, y);
        if (area_id <= 0)
            area_id = get_walkable_area_pixel(room, x - TRYGAP, y);
        if (area_id <= 0)
            area_id = get_walkable_area_pixel(room, x, y + TRYGAP);
        if (area_id <= 0)
            area_id = get_walkable_area_pixel(room, x, y - TRYGAP);
        if (area_id < 0)
            area_id = 0;
    }

    return (area_id >= 0 && area_id < MAX_WALK_AREAS) ? area_id : 0;
}

int get_walkable_area_at_character(int char_id)
{
    Character *ch = &Game::Current().Characters[char_id];
    return get_walkable_area_at_location(Room::Current(), ch->x, ch->y);
}

ScriptWalkableArea *Walkarea_GetAtRoomXY(int x, int y)
{
    return &Game::Current().ScWalkableAreas[GetWalkableAreaAtRoom(x, y)]; // always a valid index (0 for no area)
}

ScriptWalkableArea *Walkarea_GetAtScreenXY(int x, int y)
{
    int area = GetWalkableAreaAtScreen(x, y);
    if (area < 0 || area >= MAX_WALK_AREAS)
        return nullptr;
    return &Game::Current().ScWalkableAreas[area];
}

ScriptWalkableArea *Walkarea_GetByName(const char *name)
{
    return static_cast<ScriptWalkableArea *>(ccGetScriptObjectAddress(name, ccDynamicWalkarea.GetType()));
}

int Walkarea_GetID(ScriptWalkableArea *wa)
{
    return wa->id;
}

int Walkarea_GetEnabled(ScriptWalkableArea *wa)
{
    return Room::Current().WalkableAreasOn[wa->id];
}

void Walkarea_SetEnabled(ScriptWalkableArea *wa, int enable)
{
    if (enable == Room::Current().WalkableAreasOn[wa->id])
        return; // no change necessary
    if (enable)
        RestoreWalkableArea(wa->id);
    else
        RemoveWalkableArea(wa->id);
}

void Walkarea_SetScaling(ScriptWalkableArea *wa, int scale_min, int scale_max)
{
    SetAreaScaling(wa->id, scale_min, scale_max);
}

int Walkarea_GetScalingMin(ScriptWalkableArea *wa)
{
    return Room::Current().WalkAreas[wa->id].ScalingFar;
}

int Walkarea_GetScalingMax(ScriptWalkableArea *wa)
{
    return Room::Current().WalkAreas[wa->id].ScalingNear;
}

const char *Walkarea_GetScriptName(ScriptWalkableArea *wa)
{
    return CreateNewScriptString(Room::Current().WalkAreas[wa->id].ScriptName);
}

float Walkarea_GetFaceDirectionRatio(ScriptWalkableArea *wa)
{
    return Room::Current().WalkAreas[wa->id].FaceDirectionRatio;
}

void Walkarea_SetFaceDirectionRatio(ScriptWalkableArea *wa, float ratio)
{
    Room::Current().WalkAreas[wa->id].FaceDirectionRatio = ratio;
}

int Walkarea_GetProperty(ScriptWalkableArea *wa, const char *property)
{
    Room &room = Room::Current();
    return get_int_property(room.WalkAreas[wa->id].Properties, room.State->waProps[wa->id], property);
}

const char *Walkarea_GetTextProperty(ScriptWalkableArea *wa, const char *property)
{
    Room &room = Room::Current();
    return get_text_property_dynamic_string(room.WalkAreas[wa->id].Properties, room.State->waProps[wa->id], property);
}

bool Walkarea_SetProperty(ScriptWalkableArea *wa, const char *property, int value)
{
    return set_int_property(Room::Current().State->waProps[wa->id], property, value);
}

bool Walkarea_SetTextProperty(ScriptWalkableArea *wa, const char *property, const char *value)
{
    return set_text_property(Room::Current().State->waProps[wa->id], property, value);
}

#include "script/script_runtime.h"

int GetScalingAt(int x, int y)
{
    Room &room = Room::Current();
    int onarea = get_walkable_area_pixel(room, x, y);
    if (onarea < 0)
        return 100;

    return get_area_scaling(room, onarea, y);
}

void SetAreaScaling(int area, int min, int max)
{
    Room &room = Room::Current();
    if ((area < 0) || (area >= MAX_WALK_AREAS))
        quit("!SetAreaScaling: invalid walkalbe area");

    if (min > max)
        quit("!SetAreaScaling: min > max");

    if ((min < 5) || (max < 5) || (min > 200) || (max > 200))
        quit("!SetAreaScaling: min and max must be in range 5-200");

    // the values are stored differently
    min -= 100;
    max -= 100;

    if (min == max)
    {
        room.WalkAreas[area].ScalingFar = min;
        room.WalkAreas[area].ScalingNear = NOT_VECTOR_SCALED;
    }
    else
    {
        room.WalkAreas[area].ScalingFar = min;
        room.WalkAreas[area].ScalingNear = max;
    }
}

void RemoveWalkableArea(int areanum)
{
    if ((areanum < 1) || (areanum >= MAX_WALK_AREAS))
        quitprintf("!RemoveWalkableArea: invalid area number specified (1-%d).", MAX_WALK_AREAS - 1);
    Room::Current().WalkableAreasOn[areanum] = 0;
    redo_walkable_areas();
    debug_script_log("Walkable area %d removed", areanum);
}

void RestoreWalkableArea(int areanum)
{
    if ((areanum < 1) || (areanum >= MAX_WALK_AREAS))
        quitprintf("!RestoreWalkableArea: invalid area number specified (1-%d).", MAX_WALK_AREAS - 1);
    Room::Current().WalkableAreasOn[areanum] = 1;
    redo_walkable_areas();
    debug_script_log("Walkable area %d restored", areanum);
}

int GetWalkableAreaAtScreen(int x, int y)
{
    VpPoint vpt = Game::Current().Viewports.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return -1;
    return GetWalkableAreaAtRoom(vpt.first.X, vpt.first.Y);
}

int GetWalkableAreaAtRoom(int x, int y)
{
    int area = get_walkable_area_pixel(Room::Current(), x, y);
    // IMPORTANT: disabled walkable areas are actually erased completely from the mask;
    // see: RemoveWalkableArea() and RestoreWalkableArea().
    return (area >= 0 && area < MAX_WALK_AREAS) ? area : 0;
}
