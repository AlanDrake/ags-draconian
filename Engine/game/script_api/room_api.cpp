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
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/room.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterRoomAPI()
{
    ScFnRegister room_api[] = {
        ScStatic<&Room_Exists>("Room::Exists"),
        ScStatic<&Room_GetDrawingSurfaceForBackground>("Room::GetDrawingSurfaceForBackground^1"),
        ScStatic<&Room_GetProperty>("Room::GetProperty^1"),
        ScStatic<&Room_GetTextProperty>("Room::GetTextProperty^1"),
        ScStatic<&Room_NearestWalkableArea>("Room::NearestWalkableArea^2"),
        ScStatic<&Room_SetProperty>("Room::SetProperty^2"),
        ScStatic<&Room_SetTextProperty>("Room::SetTextProperty^2"),
        ScStatic<&RoomProcessClick>("Room::ProcessClick^3"),
        ScStatic<&Room_GetBackgroundCount>("Room::get_BackgroundCount"),
        ScStatic<&Room_GetBackgroundShader>("Room::get_BackgroundShader"),
        ScStatic<&Room_SetBackgroundShader>("Room::set_BackgroundShader"),
        ScStatic<&Room_GetBottomEdge>("Room::get_BottomEdge"),
        ScStatic<&Room_GetColorDepth>("Room::get_ColorDepth"),
        ScStatic<&Room_GetHeight>("Room::get_Height"),
        ScStatic<&Room_GetLeftEdge>("Room::get_LeftEdge"),
        ScStatic<&Room_GetHotspotCount>("Room::get_HotspotCount"),
        ScStatic<&Room_GetObjectCount>("Room::get_ObjectCount"),
        ScStatic<&Room_GetRegionCount>("Room::get_RegionCount"),
        ScStatic<&Room_GetWalkableAreaCount>("Room::get_WalkableAreaCount"),
        ScStatic<&Room_GetWalkbehindCount>("Room::get_WalkbehindCount"),
        ScStatic<&Room_GetRightEdge>("Room::get_RightEdge"),
        ScStatic<&Room_GetTopEdge>("Room::get_TopEdge"),
        ScStatic<&Room_GetWidth>("Room::get_Width"),
        ScStatic<&Room_GetFaceDirectionRatio>("Room::get_FaceDirectionRatio"),
        ScStatic<&Room_SetFaceDirectionRatio>("Room::set_FaceDirectionRatio"),
        ScStatic<&Room_GetName>("Room::get_Name"),
        ScStatic<&Room_GetNumber>("Room::get_Number"),
        ScStatic<&Room_GetiHotspots>("Room::geti_Hotspots"),
        ScStatic<&Room_GetiObjects>("Room::geti_Objects"),
        ScStatic<&Room_GetiRegions>("Room::geti_Regions"),
        ScStatic<&Room_GetiWalkableAreas>("Room::geti_WalkableAreas"),
        ScStatic<&Room_GetiWalkbehinds>("Room::geti_Walkbehinds"),
        ScStatic<&Room_GetPathFinder>("Room::get_PathFinder"),
    };

    ccAddExternalFunctions(room_api);
}
