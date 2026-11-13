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
#pragma once

class ScriptMaskPathfinder;
class ScriptMotionPath;
class ScriptPathfinder;
struct ScriptUserObject;

// Script API: Pathfinder::FindPath
void *Pathfinder_FindPath(ScriptPathfinder *pathfind, int srcx, int srcy, int dstx, int dsty);

// Script API: Pathfinder::Trace
ScriptUserObject *Pathfinder_Trace(ScriptPathfinder *pathfind, int srcx, int srcy, int dstx, int dsty);

// Script API: Pathfinder::IsWalkableAt
bool Pathfinder_IsWalkableAt(ScriptPathfinder *pathfind, int x, int y);

// Script API: Pathfinder::NearestWalkablePoint
ScriptUserObject *Pathfinder_NearestWalkablePoint(ScriptPathfinder *pathfind, int x, int y);

// Script API: MaskPathfinder::Create
ScriptMaskPathfinder *MaskPathfinder_Create(int mask_sprite);

// Script API: MaskPathfinder::SetMask
void MaskPathfinder_SetMask(ScriptMaskPathfinder *pathfind, int mask_sprite);

// Script API: MotionPath::Create
ScriptMotionPath *MotionPath_Create(void *path_arr, float speedx, float speedy, int repeat, int direction);

// Script API: MotionPath::Create2
ScriptMotionPath *MotionPath_Create2(void *path_arr, void *speedx_arr, void *speedy_arr, int repeat, int direction);

// Script API: MotionPath::GetPath
void *MotionPath_GetPath(ScriptMotionPath *mpath);

// Script API: MotionPath::StepBack
void MotionPath_StepBack(ScriptMotionPath *mpath);

// Script API: MotionPath::StepForward
void MotionPath_StepForward(ScriptMotionPath *mpath);

// Script API: MotionPath::Reset
void MotionPath_Reset(ScriptMotionPath *mpath, int stage, float progress);

// Script API: MotionPath::get_Valid
bool MotionPath_GetValid(ScriptMotionPath *mpath);

// Script API: MotionPath::get_IsCompleted
bool MotionPath_GetIsCompleted(ScriptMotionPath *mpath);

// Script API: MotionPath::get_StageCount
int MotionPath_GetStageCount(ScriptMotionPath *mpath);

// Script API: MotionPath::get_Direction
int MotionPath_GetDirection(ScriptMotionPath *mpath);

// Script API: MotionPath::get_RepeatStyle
int MotionPath_GetRepeatStyle(ScriptMotionPath *mpath);

// Script API: MotionPath::get_WalkWhere
int MotionPath_GetWalkWhere(ScriptMotionPath *mpath);

// Script API: MotionPath::geti_StageX
int MotionPath_GetStageX(ScriptMotionPath *mpath, int index);

// Script API: MotionPath::geti_StageY
int MotionPath_GetStageY(ScriptMotionPath *mpath, int index);

// Script API: MotionPath::geti_StageSpeedX
float MotionPath_GetStageSpeedX(ScriptMotionPath *mpath, int index);

// Script API: MotionPath::geti_StageSpeedY
float MotionPath_GetStageSpeedY(ScriptMotionPath *mpath, int index);

// Script API: MotionPath::get_Stage
int MotionPath_GetStage(ScriptMotionPath *mpath);

// Script API: MotionPath::get_Progress
float MotionPath_GetProgress(ScriptMotionPath *mpath);

// Script API: MotionPath::get_PositionX
int MotionPath_GetPositionX(ScriptMotionPath *mpath);

// Script API: MotionPath::get_PositionY
int MotionPath_GetPositionY(ScriptMotionPath *mpath);

// Script API: MotionPath::get_VelocityX
float MotionPath_GetVelocityX(ScriptMotionPath *mpath);

// Script API: MotionPath::get_VelocityY
float MotionPath_GetVelocityY(ScriptMotionPath *mpath);
