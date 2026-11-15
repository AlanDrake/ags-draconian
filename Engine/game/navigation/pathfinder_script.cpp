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
// Pathfinder script API.
//
//=============================================================================
#include "game/navigation/pathfinder_script.h"

#include "debug/debug_log.h"
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_pathfinder.h"
#include "game/game.h"
#include "game/navigation/route_finder.h"
#include "game/world/object.h"
#include "gfx/sprite_cache.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"

using namespace AGS::Common;
using namespace AGS::Engine;

//=============================================================================
//
// Pathfinder API.
//
//=============================================================================

void *Pathfinder_FindPath(ScriptPathfinder *pathfind, int srcx, int srcy, int dstx, int dsty)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source

    std::vector<Point> path;
    pathfind->GetRouteFinder()->FindRoute(path, srcx, srcy, dstx, dsty);
    if (path.empty())
        return nullptr;

    return ScriptStructHelpers::CreateArrayOfPoints(path).Obj();
}

ScriptUserObject *Pathfinder_Trace(ScriptPathfinder *pathfind, int srcx, int srcy, int dstx, int dsty)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source

    int lastx = srcx, lasty = srcy;
    pathfind->GetRouteFinder()->CanSeeFrom(srcx, srcy, dstx, dsty, &lastx, &lasty);
    return ScriptStructHelpers::CreatePoint(lastx, lasty);
}

bool Pathfinder_IsWalkableAt(ScriptPathfinder *pathfind, int x, int y)
{
    if (!pathfind->GetRouteFinder())
        return false;

    pathfind->SyncPathfinder(); // sync with the source
    return pathfind->GetRouteFinder()->IsWalkableAt(x, y);
}

ScriptUserObject *Pathfinder_NearestWalkablePoint(ScriptPathfinder *pathfind, int x, int y)
{
    if (!pathfind->GetRouteFinder())
        return nullptr;

    pathfind->SyncPathfinder(); // sync with the source
    Point result;
    if (!pathfind->GetRouteFinder()->FindNearestWalkablePoint(Point(x, y), result))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(result.X, result.Y);
}

ScriptMaskPathfinder *MaskPathfinder_Create(int mask_sprite)
{
    return ScriptMaskPathfinder::CreateFromMaskSprite(mask_sprite);
}

void MaskPathfinder_SetMask(ScriptMaskPathfinder *pathfind, int mask_sprite)
{
    pathfind->SetMaskSprite(mask_sprite);
}

//=============================================================================
//
// MotionPath API.
//
//=============================================================================

ScriptMotionPath *MotionPath_Create(void *path_arr, float speedx, float speedy, int repeat, int direction)
{
    std::vector<Point> path;
    if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
    {
        debug_script_warn("MotionPath.Create: path is null, or failed to resolve array of points");
        return nullptr;
    }

    ValidateAnimParams("MotionPath.Create", "", repeat, direction);

    MoveList mlist;
    MoveListOp::CalculateMoveList(
        mlist, path, speedx, speedy, 0u,
        RunPathParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction)));
    return static_cast<ScriptMotionPath *>(ScriptMotionPath::Create(add_movelist(std::move(mlist))).Obj());
}

ScriptMotionPath *MotionPath_Create2(void *path_arr, void *speedx_arr, void *speedy_arr, int repeat, int direction)
{
    std::vector<Point> path;
    std::vector<float> speedxs, speedys;
    if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
    {
        debug_script_warn("MotionPath.Create: path is null, or failed to resolve array of points");
        return nullptr;
    }
    if (!speedx_arr || !DynamicArrayHelpers::ResolveFloatArray(speedx_arr, speedxs))
    {
        debug_script_warn("MotionPath.Create: speedx array is null, or failed to resolve array of points");
        return nullptr;
    }
    if (!speedy_arr || !DynamicArrayHelpers::ResolveFloatArray(speedy_arr, speedys))
    {
        debug_script_warn("MotionPath.Create: speedy array is null, or failed to resolve array of points");
        return nullptr;
    }
    if (speedxs.size() < path.size() - 1 || speedys.size() < path.size() - 1)
    {
        debug_script_warn("MotionPath.Create: speeds arrays are smaller than the number of path's stages");
        return nullptr;
    }

    ValidateAnimParams("MotionPath.Create", "", repeat, direction);

    std::vector<Pointf> speeds;
    for (size_t i = 0; i < speedxs.size() && i < speedys.size(); ++i)
        speeds.emplace_back(speedxs[i], speedys[i]);
    MoveList mlist;
    MoveListOp::CalculateMoveList(
        mlist, path, speeds, 0u,
        RunPathParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction)));
    return static_cast<ScriptMotionPath *>(ScriptMotionPath::Create(add_movelist(std::move(mlist))).Obj());
}

static bool ValidateMoveList(const char *api_name, ScriptMotionPath *mpath)
{
    if (!mpath->GetMoveList())
    {
        debug_script_warn("%s: motion path is invalid, underlying data was disposed.", api_name);
        return false;
    }
    return true;
}

static bool ValidateMoveListAndStageIndex(const char *api_name, ScriptMotionPath *mpath, int stage)
{
    MoveList *mlist = mpath->GetMoveList();
    if (!mlist)
    {
        debug_script_warn("%s: motion path is invalid, underlying data was disposed.", api_name);
        return false;
    }
    if (stage < 0 || static_cast<uint32_t>(stage) >= mlist->GetNumStages())
    {
        debug_script_warn("%s: stage out of range for a motion path, requested %d, valid range 0..%u.", api_name, stage,
                          mlist->GetNumStages());
        return false;
    }
    return true;
}

void *MotionPath_GetPath(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.GetPath", mpath))
        return nullptr;
    return ScriptStructHelpers::CreateArrayOfPoints(mpath->GetMoveList()->pos).Obj();
}

void MotionPath_StepBack(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.StepBack", mpath))
        return;
    mpath->GetMoveList()->Backward();
}

void MotionPath_StepForward(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.StepForward", mpath))
        return;
    mpath->GetMoveList()->Forward();
}

void MotionPath_Reset(ScriptMotionPath *mpath, int stage, float progress)
{
    if (!ValidateMoveListAndStageIndex("MotionPath.Reset", mpath, stage))
        return;
    mpath->GetMoveList()->ResetToStage(stage, progress);
}

bool MotionPath_GetValid(ScriptMotionPath *mpath)
{
    return mpath->IsValid();
}

bool MotionPath_GetIsCompleted(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.IsCompleted", mpath))
        return false;
    return mpath->GetMoveList()->IsDone();
}

int MotionPath_GetStageCount(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.StageCount", mpath))
        return 0;
    return mpath->GetMoveList()->GetNumStages();
}

int MotionPath_GetDirection(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.Direction", mpath))
        return 0;
    return mpath->GetMoveList()->GetRunParams().IsForward() ? FORWARDS : BACKWARDS;
}

int MotionPath_GetRepeatStyle(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.RepeatStyle", mpath))
        return 0;
    return mpath->GetMoveList()->GetRunParams().Flow;
}

int MotionPath_GetWalkWhere(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.WalkWhere", mpath))
        return 0;
    return mpath->GetMoveList()->IsStageDirect() ? ANYWHERE : WALKABLE_AREAS;
}

int MotionPath_GetStageX(ScriptMotionPath *mpath, int index)
{
    if (!ValidateMoveListAndStageIndex("MotionPath.StageX", mpath, index))
        return 0;
    return mpath->GetMoveList()->GetStagePos(index).X;
}

int MotionPath_GetStageY(ScriptMotionPath *mpath, int index)
{
    if (!ValidateMoveListAndStageIndex("MotionPath.StageY", mpath, index))
        return 0;
    return mpath->GetMoveList()->GetStagePos(index).Y;
}

float MotionPath_GetStageSpeedX(ScriptMotionPath *mpath, int index)
{
    if (!ValidateMoveListAndStageIndex("MotionPath.SpeedX", mpath, index))
        return 0;
    return mpath->GetMoveList()->GetStageSpeed(index).X;
}

float MotionPath_GetStageSpeedY(ScriptMotionPath *mpath, int index)
{
    if (!ValidateMoveListAndStageIndex("MotionPath.SpeedY", mpath, index))
        return 0;
    return mpath->GetMoveList()->GetStageSpeed(index).Y;
}

int MotionPath_GetStage(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.Stage", mpath))
        return 0;
    return mpath->GetMoveList()->GetStage();
}

float MotionPath_GetProgress(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.Progress", mpath))
        return 0.f;
    return mpath->GetMoveList()->GetStageProgress();
}

int MotionPath_GetPositionX(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.PositionX", mpath))
        return 0;
    return mpath->GetMoveList()->GetCurrentPos().X;
}

int MotionPath_GetPositionY(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.PositionY", mpath))
        return 0;
    return mpath->GetMoveList()->GetCurrentPos().Y;
}

float MotionPath_GetVelocityX(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.VelocityX", mpath))
        return 0;
    return mpath->GetMoveList()->GetCurrentSpeed().X;
}

float MotionPath_GetVelocityY(ScriptMotionPath *mpath)
{
    if (!ValidateMoveList("MotionPath.VelocityY", mpath))
        return 0;
    return mpath->GetMoveList()->GetCurrentSpeed().Y;
}
