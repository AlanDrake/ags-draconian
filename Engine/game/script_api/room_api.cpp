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
// Script bindings registered by RegisterRoomAPI().
//
//=============================================================================
#include "game/dynobj/all_dynamic_classes.h"
#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_pathfinder.h"
#include "game/dynobj/script_shader.h"
#include "game/dynobj/script_string.h"
#include "game/game.h"
#include "game/script.h"
#include "game/world/room.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// ScriptDrawingSurface* (int backgroundNumber)
RuntimeScriptValue Sc_Room_GetDrawingSurfaceForBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDrawingSurface, Room_GetDrawingSurfaceForBackground);
}

// int (const char *property)
RuntimeScriptValue Sc_Room_GetProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Room_GetProperty, const char);
}

// const char* (const char *property)
RuntimeScriptValue Sc_Room_GetTextProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, Room_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Room_SetProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ_PINT(Room_SetProperty, const char);
}

// const char* (const char *property)
RuntimeScriptValue Sc_Room_SetTextProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ2(Room_SetTextProperty, const char, const char);
}

// int ()
RuntimeScriptValue Sc_Room_GetBottomEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetBottomEdge);
}

// int ()
RuntimeScriptValue Sc_Room_GetColorDepth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetColorDepth);
}

// int ()
RuntimeScriptValue Sc_Room_GetHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetHeight);
}

RuntimeScriptValue Sc_Room_GetBackgroundCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetBackgroundCount);
}

RuntimeScriptValue Sc_Room_GetBackgroundShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptShaderInstance, Room_GetBackgroundShader);
}

RuntimeScriptValue Sc_Room_SetBackgroundShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Room_SetBackgroundShader, ScriptShaderInstance);
}

// int ()
RuntimeScriptValue Sc_Room_GetLeftEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetLeftEdge);
}

RuntimeScriptValue Sc_Room_GetHotspotCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetHotspotCount);
}

// int ()
RuntimeScriptValue Sc_Room_GetObjectCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetObjectCount);
}

RuntimeScriptValue Sc_Room_GetRegionCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetRegionCount);
}

RuntimeScriptValue Sc_Room_GetWalkableAreaCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetWalkableAreaCount);
}

RuntimeScriptValue Sc_Room_GetWalkbehindCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetWalkbehindCount);
}

// int ()
RuntimeScriptValue Sc_Room_GetRightEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetRightEdge);
}

// int ()
RuntimeScriptValue Sc_Room_GetTopEdge(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetTopEdge);
}

// int ()
RuntimeScriptValue Sc_Room_GetWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetWidth);
}

RuntimeScriptValue Sc_Room_GetFaceDirectionRatio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Room_GetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Room_SetFaceDirectionRatio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PFLOAT(Room_SetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Room_GetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char *, myScriptStringImpl, Room_GetName);
}

RuntimeScriptValue Sc_Room_GetNumber(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Room_GetNumber);
}

// void (int xx,int yy,int mood)
RuntimeScriptValue Sc_RoomProcessClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(RoomProcessClick);
}

RuntimeScriptValue Sc_Room_Exists(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_PINT(Room_Exists);
}

RuntimeScriptValue Sc_Room_NearestWalkableArea(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptUserObject, Room_NearestWalkableArea);
}

RuntimeScriptValue Sc_Room_GetiHotspots(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptHotspot, ccDynamicHotspot, Room_GetiHotspots);
}

RuntimeScriptValue Sc_Room_GetiObjects(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptObject, ccDynamicObject, Room_GetiObjects);
}

RuntimeScriptValue Sc_Room_GetiRegions(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptRegion, ccDynamicRegion, Room_GetiRegions);
}

RuntimeScriptValue Sc_Room_GetiWalkableAreas(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptWalkableArea, ccDynamicWalkarea, Room_GetiWalkableAreas);
}

RuntimeScriptValue Sc_Room_GetiWalkbehinds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptWalkbehind, ccDynamicWalkbehind, Room_GetiWalkbehinds);
}

RuntimeScriptValue Sc_Room_GetPathFinder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptPathfinder, Room_GetPathFinder);
}

void RegisterRoomAPI()
{
    ScFnRegister room_api[] = {
        {"Room::Exists", API_FN_PAIR(Room_Exists)},
        {"Room::GetDrawingSurfaceForBackground^1", API_FN_PAIR(Room_GetDrawingSurfaceForBackground)},
        {"Room::GetProperty^1", API_FN_PAIR(Room_GetProperty)},
        {"Room::GetTextProperty^1", API_FN_PAIR(Room_GetTextProperty)},
        {"Room::NearestWalkableArea^2", API_FN_PAIR(Room_NearestWalkableArea)},
        {"Room::SetProperty^2", API_FN_PAIR(Room_SetProperty)},
        {"Room::SetTextProperty^2", API_FN_PAIR(Room_SetTextProperty)},
        {"Room::ProcessClick^3", API_FN_PAIR(RoomProcessClick)},
        {"Room::get_BackgroundCount", API_FN_PAIR(Room_GetBackgroundCount)},
        {"Room::get_BackgroundShader", API_FN_PAIR(Room_GetBackgroundShader)},
        {"Room::set_BackgroundShader", API_FN_PAIR(Room_SetBackgroundShader)},
        {"Room::get_BottomEdge", API_FN_PAIR(Room_GetBottomEdge)},
        {"Room::get_ColorDepth", API_FN_PAIR(Room_GetColorDepth)},
        {"Room::get_Height", API_FN_PAIR(Room_GetHeight)},
        {"Room::get_LeftEdge", API_FN_PAIR(Room_GetLeftEdge)},
        {"Room::get_HotspotCount", API_FN_PAIR(Room_GetHotspotCount)},
        {"Room::get_ObjectCount", API_FN_PAIR(Room_GetObjectCount)},
        {"Room::get_RegionCount", API_FN_PAIR(Room_GetRegionCount)},
        {"Room::get_WalkableAreaCount", API_FN_PAIR(Room_GetWalkableAreaCount)},
        {"Room::get_WalkbehindCount", API_FN_PAIR(Room_GetWalkbehindCount)},
        {"Room::get_RightEdge", API_FN_PAIR(Room_GetRightEdge)},
        {"Room::get_TopEdge", API_FN_PAIR(Room_GetTopEdge)},
        {"Room::get_Width", API_FN_PAIR(Room_GetWidth)},
        {"Room::get_FaceDirectionRatio", API_FN_PAIR(Room_GetFaceDirectionRatio)},
        {"Room::set_FaceDirectionRatio", API_FN_PAIR(Room_SetFaceDirectionRatio)},
        {"Room::get_Name", API_FN_PAIR(Room_GetName)},
        {"Room::get_Number", API_FN_PAIR(Room_GetNumber)},
        {"Room::geti_Hotspots", API_FN_PAIR(Room_GetiHotspots)},
        {"Room::geti_Objects", API_FN_PAIR(Room_GetiObjects)},
        {"Room::geti_Regions", API_FN_PAIR(Room_GetiRegions)},
        {"Room::geti_WalkableAreas", API_FN_PAIR(Room_GetiWalkableAreas)},
        {"Room::geti_Walkbehinds", API_FN_PAIR(Room_GetiWalkbehinds)},
        {"Room::get_PathFinder", API_FN_PAIR(Room_GetPathFinder)},
    };

    ccAddExternalFunctions(room_api);
}
