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
#pragma once

#include "game/character_info.h"
#include "game/dynobj/script_drawing_surface.h"
#include "game/navigation/route_finder.h"
#include "game/roomstruct.h"
#include "script/runtimescriptvalue.h"

// Script function: returns a DrawingSurface for the given room background (SCR_NO_VALUE: the current one);
// reuses the surface already attached to it, if any. Returns null for an invalid index.
ScriptDrawingSurface *Room_GetDrawingSurfaceForBackground(int bg_index);
// Script function: returns a DrawingSurface for the given room mask; null for an invalid mask
ScriptDrawingSurface *Room_GetDrawingSurfaceForMask(RoomAreaMask mask);
// Script function: the number of objects in the current room
int Room_GetObjectCount();
// Script function: the room's width, in room coordinates
int Room_GetWidth();
// Script function: the room's height, in room coordinates
int Room_GetHeight();
// Script function: the color depth of the room's first background
int Room_GetColorDepth();
// Script function: the room's left edge, as set in the room's edge settings
int Room_GetLeftEdge();
// Script function: the room's right edge
int Room_GetRightEdge();
// Script function: the room's top edge
int Room_GetTopEdge();
// Script function: the room's bottom edge
int Room_GetBottomEdge();
// Script function: the text value of a custom room property, as a new script string
const char *Room_GetTextProperty(const char *property);
// Script function: the integer value of a custom room property
int Room_GetProperty(const char *property);
// Script function: a DrawingSurface for the room's walkable area mask
ScriptDrawingSurface *GetDrawingSurfaceForWalkableArea();
// Script function: a DrawingSurface for the room's walk-behind mask
ScriptDrawingSurface *GetDrawingSurfaceForWalkbehind();
// Script function: a DrawingSurface for the room's hotspot mask
ScriptDrawingSurface *Hotspot_GetDrawingSurface();
// Script function: a DrawingSurface for the room's region mask
ScriptDrawingSurface *Region_GetDrawingSurface();

//=============================================================================

// Copies the room script's global data into the current room status, so that it is kept with the room
void save_room_data_segment();
// Runs the room-leaving events and disposes the current room's state; does nothing when no room is
// displayed (e.g. when switching games on restore)
void unload_old_room();
// Loads room room_id from its file and makes it the current room. ch is the character entering the room
// (the player on NewRoom), who gets placed at the entry position; nullptr when restoring a saved game
void load_new_room(int room_id, CharacterInfo *ch);
// Changes the current room: runs the room-leaving events (their scripts may change the destination),
// unloads the old room and loads the new one with load_new_room()
void new_room(int room_id, CharacterInfo *ch);
// Sets up a placeholder room object; this is used to avoid occasional crashes
// in case an API function was called that needs to access a room, while no real room is loaded
void set_room_placeholder();
// Prepares the state for the first room of a game: starts the room transition, forgets the player's
// previous room and resets the background frame state
void first_room_initialization();
// If a new room was just entered (and not restored from a save), runs its "before fade-in" event
void check_new_room();
// Creates the runtime room script (Room::Script) from the room's compiled script, registers its exports and resolves
// its imports; quits on error
void compile_room_script();
// Updates the screen and the palette after the current background frame was changed
void on_background_frame_change();
// Clear the current room pointer if room status is no longer valid
void croom_ptr_clear();

// Retrieves a DrawingSurface object, connected to the given room background.
ScriptDrawingSurface *get_room_bg_surface(uint32_t bg_index);
// Retrieves a DrawingSurface object, connected to the given room mask.
ScriptDrawingSurface *get_room_mask_surface(RoomAreaMask mask);
// Attaches drawing surface handle to the room background.
// This method is used when unserializing drawing surfaces from the game save.
void attach_room_bg_surface(uint32_t bg_index, int surface_handle);
// Attaches drawing surface handle to the room mask.
// This method is used when unserializing drawing surfaces from the game save.
void attach_room_mask_surface(RoomAreaMask mask, int surface_handle);
// Detaches all drawing surfaces related to the room background and masks.
// This invalidates DrawingSurface script object(s), and erases a bg-surface reference.
void detach_room_bg_surfaces();
// Notifies room that its background's drawing surface was released
void on_room_bg_surface_release(uint32_t bg_index, bool modified);
// Notifies room that its mask's drawing surface was released
void on_room_mask_surface_release(RoomAreaMask mask, bool modified);
// Creates the room pathfinder object, if it does not exist yet
void init_room_pathfinder();
// Destroys the room pathfinder object
void dispose_room_pathfinder();
// Gets current room's pathfinder object
AGS::Engine::MaskRouteFinder *get_room_pathfinder();

// Following functions convert coordinates between room resolution and region mask.
// Region masks can be 1:N of the room size: 1:1, 1:2 etc.
// In contemporary games this is simply multiplying or dividing on mask resolution.
// In legacy upscale mode (and generally pre-3.* high-res games) things are more
// complicated, as first we need to make an additional conversion between data coords
// and upscale game coordinates.
//
// coordinate conversion (room) ---> (room mask)
int room_to_mask_coord(int coord);
// coordinate conversion (room mask) ---> (room)
int mask_to_room_coord(int coord);

class ScriptPathfinder;
class ScriptShaderInstance;
struct ScriptHotspot;
struct ScriptObject;
struct ScriptRegion;
struct ScriptUserObject;
struct ScriptWalkableArea;
struct ScriptWalkbehind;

// Script API: Room::get_HotspotCount
int Room_GetHotspotCount();

// Script API: Room::get_RegionCount
int Room_GetRegionCount();

// Script API: Room::get_WalkableAreaCount
int Room_GetWalkableAreaCount();

// Script API: Room::get_WalkbehindCount
int Room_GetWalkbehindCount();

// Script API: Room::get_BackgroundCount
int Room_GetBackgroundCount();

// Script API: Room::get_BackgroundShader
ScriptShaderInstance *Room_GetBackgroundShader();

// Script API: Room::set_BackgroundShader
void Room_SetBackgroundShader(ScriptShaderInstance *shader_inst);

// Script API: Room::SetProperty^2
bool Room_SetProperty(const char *property, int value);

// Script API: Room::SetTextProperty^2
bool Room_SetTextProperty(const char *property, const char *value);

// Script API: Room::get_FaceDirectionRatio
float Room_GetFaceDirectionRatio();

// Script API: Room::set_FaceDirectionRatio
void Room_SetFaceDirectionRatio(float ratio);

// Script API: Room::get_Name
const char *Room_GetName();

// Script API: Room::get_Number
int Room_GetNumber();

// Script API: Room::Exists
bool Room_Exists(int room);

// Script API: Room::NearestWalkableArea^2
ScriptUserObject *Room_NearestWalkableArea(int x, int y);

// Script API: Room::geti_Hotspots
ScriptHotspot *Room_GetiHotspots(int idx);

// Script API: Room::geti_Objects
ScriptObject *Room_GetiObjects(int idx);

// Script API: Room::geti_Regions
ScriptRegion *Room_GetiRegions(int idx);

// Script API: Room::geti_WalkableAreas
ScriptWalkableArea *Room_GetiWalkableAreas(int idx);

// Script API: Room::geti_Walkbehinds
ScriptWalkbehind *Room_GetiWalkbehinds(int idx);

// Script API: Room::get_PathFinder
ScriptPathfinder *Room_GetPathFinder();

void SetAmbientTint(int red, int green, int blue, int opacity, int luminance);
void SetAmbientLightLevel(int light_level);
void NewRoom(int nrnum); // still widely used
void ResetRoom(int nrnum);
int HasPlayerBeenInRoom(int roomnum);
void CallRoomScript(int value);

void SetBackgroundFrame(int frnum);
int GetBackgroundFrame();

// Processes a click at the given screen coordinates with the given cursor mode, as if the player clicked in the room.
void RoomProcessClick(int x, int y, int mood);
// Returns 1 if a click at the given screen coordinates with the given cursor mode would do something (a walk
// mode always does), else 0.
int IsInteractionAvailable(int x, int y, int mood);
