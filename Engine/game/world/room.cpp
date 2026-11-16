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
#include "game/world/room.h"

#include <cctype> // for toupper

#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "device/sys_events.h"
#include "game/dialog/dialog.h"
#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_pathfinder.h"
#include "game/dynobj/script_shader.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_init.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/render/overlay.h"
#include "game/render/screen.h"
#include "game/room_file.h"
#include "game/room_version.h"
#include "game/script.h"
#include "game/system.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/world/character.h"
#include "game/world/hotspot.h"
#include "game/world/move_list.h"
#include "game/world/object.h"
#include "game/world/properties.h"
#include "game/world/region.h"
#include "game/world/room_context.h"
#include "game/world/room_object.h"
#include "game/world/room_status.h"
#include "game/world/walk_behind.h"
#include "game/world/walkable_area.h"
#include "gfx/bitmap.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "platform/platform.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/managed_object_pool.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"
#include "util/common.h"
#include "util/stream.h"
#include "util/string_utils.h" //strlwr()

using namespace AGS::Common;
using namespace AGS::Engine;

//=============================================================================
namespace
{
// The engine's own instance
Room engineRoom;
} // namespace

// The address of a static object is a constant initializer: Current() is valid before any dynamic
// initialization, as the globals thisroom, croom and objs that the instance replaces were
Room *Room::_current = &engineRoom;

Room::Room() : _tempState(std::make_unique<RoomStatus>()), TempState(*_tempState) {}

Room::~Room() = default;

Room *Room::SetCurrent(Room *room)
{
    Room *was = _current;
    _current = room ? room : &engineRoom;
    return was;
}

void Room::ResetPlayState()
{
    LoadedWidth = 0;
    LoadedHeight = 0;
    ScriptFinished = 0;
    AnimBackgroundSpeed = 0;
    BgFrame = 0;
    BgFrameLocked = 0;
    std::fill(std::begin(WalkableAreasOn), std::end(WalkableAreasOn), 0);
    EnteredAtX = 0;
    EnteredAtY = 0;
    EnteredEdge = 0;
    BgModified = std::vector<bool>(); // frees the storage, as the assignment of a new game state did
    std::fill(std::begin(MaskModified), std::end(MaskModified), false);
}

std::unique_ptr<MaskRouteFinder> room_pathfinder;
RGB_MAP rgb_table; // for 256-col antialiasing
int new_room_flags = 0;
// TODO: find a way to merge these with DynamicSpriteDSRef
// Drawing Surface handles, for backgrounds
std::vector<int> RoomBgDS;
// Drawing Surface handles, for room masks
int RoomMaskDS[kNumRoomAreaTypes];

ScriptDrawingSurface *Room_GetDrawingSurfaceForBackground(int bg_index)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (game.DisplayedRoom < 0)
        quit("!Room.GetDrawingSurfaceForBackground: no room is currently loaded");

    if (bg_index == SCR_NO_VALUE)
    {
        bg_index = room.BgFrame;
    }

    if ((bg_index < 0) || (static_cast<uint32_t>(bg_index) >= room.BgFrameCount))
    {
        debug_script_warn("Room.GetDrawingSurfaceForBackground: invalid background number specified: %d, valid range "
                          "in this room is 0..%u",
                          bg_index, room.BgFrameCount - 1);
        return nullptr;
    }

    ScriptDrawingSurface *surface = get_room_bg_surface(bg_index);
    if (surface)
        return surface;

    surface = new ScriptDrawingSurface();
    surface->roomBackgroundNumber = bg_index;
    ccRegisterManagedObject(surface, surface);
    return surface;
}

ScriptDrawingSurface *Room_GetDrawingSurfaceForMask(RoomAreaMask mask)
{
    if (Game::Current().DisplayedRoom < 0)
        quit("!Room_GetDrawingSurfaceForMask: no room is currently loaded");

    if (mask <= kRoomAreaNone || mask >= kNumRoomAreaTypes)
    {
        debug_script_warn("Room.GetDrawingSurfaceForMask: invalid mask index specified: %d, valid range is %d..%d",
                          mask, kRoomAreaHotspot, kRoomAreaRegion);
        return nullptr;
    }

    ScriptDrawingSurface *surface = get_room_mask_surface(mask);
    if (surface)
        return surface;

    surface = new ScriptDrawingSurface();
    surface->roomMaskType = mask;
    ccRegisterManagedObject(surface, surface);
    return surface;
}

int Room_GetHotspotCount()
{
    return MAX_ROOM_HOTSPOTS;
}

int Room_GetObjectCount()
{
    return Room::Current().State->numobj;
}

int Room_GetRegionCount()
{
    return MAX_ROOM_REGIONS;
}

int Room_GetWalkableAreaCount()
{
    return MAX_WALK_AREAS;
}

int Room_GetWalkbehindCount()
{
    return MAX_WALK_BEHINDS;
}

int Room_GetWidth()
{
    return Room::Current().Width;
}

int Room_GetHeight()
{
    return Room::Current().Height;
}

int Room_GetColorDepth()
{
    return Room::Current().BgImages[0]->GetColorDepth();
}

int Room_GetBackgroundCount()
{
    return Room::Current().BgFrameCount;
}

ScriptShaderInstance *Room_GetBackgroundShader()
{
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(Room::Current().State->GetBgShaderHandle()));
}

void Room_SetBackgroundShader(ScriptShaderInstance *shader_inst)
{
    Room &room = Room::Current();
    room.State->SetBgShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                            ccReplaceObjectHandle(room.State->GetBgShaderHandle(), shader_inst));
}

int Room_GetLeftEdge()
{
    return Room::Current().Edges.Left;
}

int Room_GetRightEdge()
{
    return Room::Current().Edges.Right;
}

int Room_GetTopEdge()
{
    return Room::Current().Edges.Top;
}

int Room_GetBottomEdge()
{
    return Room::Current().Edges.Bottom;
}

int Room_GetProperty(const char *property)
{
    Room &room = Room::Current();
    return get_int_property(room.Properties, room.State->roomProps, property);
}

const char *Room_GetTextProperty(const char *property)
{
    Room &room = Room::Current();
    return get_text_property_dynamic_string(room.Properties, room.State->roomProps, property);
}

bool Room_SetProperty(const char *property, int value)
{
    return set_int_property(Room::Current().State->roomProps, property, value);
}

bool Room_SetTextProperty(const char *property, const char *value)
{
    return set_text_property(Room::Current().State->roomProps, property, value);
}

float Room_GetFaceDirectionRatio()
{
    return Room::Current().State->face_dir_ratio;
}

void Room_SetFaceDirectionRatio(float ratio)
{
    Room::Current().State->face_dir_ratio = ratio;
}

const char *Room_GetName()
{
    return CreateNewScriptString(Room::Current().GetName());
}

int Room_GetNumber()
{
    return Game::Current().DisplayedRoom;
}

bool Room_Exists(int room)
{
    String room_filename;
    room_filename.Format("room%d.crm", room);
    return EngineServices::Current().Assets->DoesAssetExist(room_filename);
}

ScriptUserObject *Room_NearestWalkableArea(int x, int y)
{
    Room &room = Room::Current();
    if (Game::Current().DisplayedRoom < 0)
        quit("!Room.NearestWalkableArea: no room is currently loaded");
    Point found_pt;
    if (Pathfinding::FindNearestWalkablePoint(
            room.WalkAreaMask.get(), Point(room_to_mask_coord(room, x), room_to_mask_coord(room, y)), found_pt))
    {
        return ScriptStructHelpers::CreatePoint(mask_to_room_coord(room, found_pt.X),
                                                mask_to_room_coord(room, found_pt.Y));
    }
    return nullptr;
}

ScriptDrawingSurface *GetDrawingSurfaceForWalkableArea()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaWalkable);
}

ScriptDrawingSurface *GetDrawingSurfaceForWalkbehind()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaWalkBehind);
}

ScriptDrawingSurface *Hotspot_GetDrawingSurface()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaHotspot);
}

ScriptDrawingSurface *Region_GetDrawingSurface()
{
    return Room_GetDrawingSurfaceForMask(kRoomAreaRegion);
}

ScriptHotspot *Room_GetiHotspots(int idx)
{
    if ((idx < 0) || (idx >= MAX_ROOM_HOTSPOTS))
        return nullptr;
    return &Game::Current().ScHotspots[idx];
}

ScriptObject *Room_GetiObjects(int idx)
{
    if ((idx < 0) || (idx >= MAX_ROOM_OBJECTS))
        return nullptr;
    return &Game::Current().ScObjects[idx];
}

ScriptRegion *Room_GetiRegions(int idx)
{
    if ((idx < 0) || (idx >= MAX_ROOM_REGIONS))
        return nullptr;
    return &Game::Current().ScRegions[idx];
}

ScriptWalkableArea *Room_GetiWalkableAreas(int idx)
{
    if ((idx < 0) || (idx >= MAX_WALK_AREAS))
        return nullptr;
    return &Game::Current().ScWalkableAreas[idx];
}

ScriptWalkbehind *Room_GetiWalkbehinds(int idx)
{
    if ((idx < 0) || (idx >= MAX_WALK_BEHINDS))
        return nullptr;
    return &Game::Current().ScWalkBehinds[idx];
}

ScriptPathfinder *Room_GetPathFinder()
{
    return RoomPathfinder::Create();
}

//=============================================================================

void save_room_data_segment()
{
    Room &room = Room::Current();
    const auto &global_data = room.Script->GetGlobalData();
    room.State->tsdatasize = global_data.size();
    room.State->tsdata.resize(global_data.size());
    if (room.State->tsdatasize > 0)
    {
        std::copy(global_data.begin(), global_data.end(), room.State->tsdata.begin());
    }
}

void unload_old_room()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    // if switching games on restore, don't do this
    if (game.DisplayedRoom < 0)
        return;

    // Set Game::Events.InRoomTransition right prior to the transition effect,
    // this will prevent a cursor and @overhotspot@ texts from displaying.
    // The flag will be unset right prior to "after fade-in" event, if it's a
    // normal room change, or immediately on room load if it's a save game
    // getting restored.
    game.Events.InRoomTransition = true;
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);

    current_fade_out_effect();

    // room unloaded callback
    run_room_event(kRoomEvent_AfterFadeout);
    // global room unloaded event
    run_on_event(kScriptEvent_RoomAfterFadeout, game.DisplayedRoom);

    debug_script_log("Unloading room %d", game.DisplayedRoom);

    dispose_room_drawdata();

    for (uint32_t i = 0; i < room.State->numobj; i++)
        room.Objs[i].moving = 0;

    AbortAllScripts();
    // Invalidate and detach any active DrawingSurfaces connected to this room
    detach_room_bg_surfaces();

    game.Events.Queue.clear(); // cancel any pending room events

    dispose_room_background_ddb();

    // FIXME: room.State was already dereferenced above (room.State->numobj), so a null one would have crashed there
    if (room.State == nullptr)
        ;
    else if (room.Script)
    {
        save_room_data_segment();
        FreeRoomScript();
    }
    else
        room.State->tsdatasize = 0;
    memset(&room.WalkableAreasOn[0], 1, MAX_WALK_AREAS);
    room.BgFrame = 0;
    room.BgFrameLocked = 0;
    remove_all_overlays();
    std::fill(room.BgModified.begin(), room.BgModified.end(), false);
    for (int i = 0; i < kNumRoomAreaTypes; ++i)
        room.MaskModified[i] = false;

    // ensure that any half-moves (eg. with scaled movement) are stopped
    for (int i = 0; i < game.numcharacters; i++)
    {
        game.Characters[i].xwas = INVALID_X;
    }

    game.Play.swap_portrait_lastchar = -1;
    game.Play.swap_portrait_lastlastchar = -1;

    // Remove named room elements from script
    for (uint32_t i = 0; i < room.Objects.size(); ++i)
    {
        if (!room.Objects[i].ScriptName.IsEmpty())
            ccRemoveExternalSymbol(room.Objects[i].ScriptName);
    }

    for (uint32_t i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        if (!room.Hotspots[i].ScriptName.IsEmpty())
            ccRemoveExternalSymbol(room.Hotspots[i].ScriptName);
    }

    for (uint32_t i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        if (!room.Regions[i].ScriptName.IsEmpty())
            ccRemoveExternalSymbol(room.Regions[i].ScriptName);
    }

    for (uint32_t i = 0; i < MAX_WALK_AREAS; ++i)
    {
        if (!room.WalkAreas[i].ScriptName.IsEmpty())
            ccRemoveExternalSymbol(room.WalkAreas[i].ScriptName);
    }

    for (uint32_t i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        if (!room.WalkBehinds[i].ScriptName.IsEmpty())
            ccRemoveExternalSymbol(room.WalkBehinds[i].ScriptName);
    }

    croom_ptr_clear();

    // clear the draw caches to save memory, since many of the the involved
    // objects probably aren't on the new screen
    clear_drawobj_cache(true /* keep object callbacks */);

    // if Hide Player Character was ticked, restore it to visible
    if (game.Play.temporarily_turned_off_character >= 0)
    {
        game.Characters[game.Play.temporarily_turned_off_character].set_enabled(true);
        game.Play.temporarily_turned_off_character = -1;
    }

    pool.PrintStats();

    game.DisplayedRoom = -10;
}

// Automatically reset primary room viewport and camera to match the new room size
static void adjust_viewport_to_room()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    const Size real_room_sz = Size(room.Width, room.Height);
    const Rect main_view = game.Viewports.GetMainViewport();
    Rect new_room_view = RectWH(Size::Clamp(real_room_sz, Size(1, 1), main_view.GetSize()));

    auto view = game.Viewports.GetRoomViewport(0);
    view->SetRect(new_room_view);
    auto cam = view->GetCamera();
    if (cam)
    {
        cam->SetSize(new_room_view.GetSize());
        cam->SetAt(0, 0);
        cam->Release();
    }
}

// Run through all viewports and cameras to make sure they can work in new room's bounds
static void update_all_viewcams_with_newroom()
{
    Game &game = Game::Current();
    for (int i = 0; i < game.Viewports.GetRoomCameraCount(); ++i)
    {
        auto cam = game.Viewports.GetRoomCamera(i);
        const Rect old_pos = cam->GetRect();
        cam->SetSize(old_pos.GetSize());
        cam->SetAt(old_pos.Left, old_pos.Top);
    }
}

// Looks for the room script available as a separate asset (room<N>.o).
// This is optional, so no error is raised if one is not found.
// If found however, it will replace room script if one had been loaded
// from the room file itself.
HError LoadRoomScript(RoomStruct *room, int room_id)
{
    String filename = String::FromFormat("room%d.o", room_id);
    auto in = EngineServices::Current().Assets->OpenAsset(filename);
    if (in)
    {
        auto script = UScript(ccScript::CreateFromStream(in.get()));
        if (!script)
            return new Error("Failed to load a script module: %s\n%s", filename.GetCStr(),
                             cc_get_error().ErrorString.GetCStr());
        room->CompiledScript = std::move(script);
    }
    return HError::None();
}

static void reset_temp_room()
{
    Room::Current().TempState = RoomStatus();
}

// Initializes object states in the current RoomStatus object (room.State),
// copying default properties from the room's objects (room.Objects), in the element range [start, end).
static void init_object_states(size_t start, size_t end)
{
    Room &room = Room::Current();
    for (uint32_t i = start; i < end; ++i)
    {
        const auto &obj_info = room.Objects[i];
        auto &room_obj = room.State->obj[i];
        room_obj.id = i;
        room_obj.x = obj_info.X;
        room_obj.y = obj_info.Y;
        room_obj.num = Math::InRangeOrDef<uint16_t>(obj_info.Sprite, 0);
        room_obj.flags = obj_info.Flags;
        room_obj.view = RoomObject::NoView;
        room_obj.loop = 0u;
        room_obj.frame = 0u;
        room_obj.wait = 0;
        room_obj.opacity = obj_info.Opacity;
        room_obj.moving = -1;
        room_obj.flags = obj_info.Flags;
        room_obj.baseline = obj_info.Baseline;
        room_obj.zoom = 100;
        room_obj.blocking_x = obj_info.BlockingRect.Left;
        room_obj.blocking_y = obj_info.BlockingRect.Top;
        room_obj.blocking_width = obj_info.BlockingRect.GetWidth();
        room_obj.blocking_height = obj_info.BlockingRect.GetHeight();
        room_obj.name = obj_info.Name;
        if (obj_info.Baseline >= 0)
            room_obj.baseline = obj_info.Baseline;
        room_obj.blend_mode = obj_info.BlendMode;
        room_obj.spr_anchor = obj_info.GraphicAnchor;
        room_obj.spr_offset = obj_info.GraphicOffset;
        room_obj.UpdateGraphicSpace();
        if (obj_info.Sprite > UINT16_MAX)
            debug_script_warn("Object's (id %d) sprite %d outside of internal range (%d), reset to 0", i,
                              obj_info.Sprite, UINT16_MAX);
    }
}

HError LoadRoom(const String &filename, RoomStruct *room, AssetManager *mgr,
                const std::vector<SpriteInfo> &sprite_infos)
{
    auto in = mgr->OpenAsset(filename);
    if (in == nullptr)
        return new RoomFileError(kRoomFileErr_FileOpenFailed, "Filename: %s.", filename.GetCStr());
    RoomData room_data;
    HRoomFileError err = LoadRoom(&room_data, std::move(in), &sprite_infos);
    if (!err)
        return new Error(*err);
    *room = RoomStruct(room_data);
    return HError::None();
}

void load_new_room(int room_id, Character *ch)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    Room &room = Room::Current();
    debug_script_log("Loading room %d", room_id);

    String room_filename;
    game.Events.DoneAsError = false;
    game.Play.room_changes++;
    game.DisplayedRoom = room_id;

    room_filename.Format("room%d.crm", room_id);

    // load the room from disk
    set_our_eip(200);
    room.GameID = NO_GAME_ID_IN_ROOM_FILE;
    HError err = LoadRoom(room_filename, &room, services.Assets.get(), game.SpriteInfos);
    if (!err)
    {
        quitprintf("Unable to load the room file '%s'. Error: %s", room_filename.GetCStr(),
                   err->FullMessage().GetCStr());
    }

    if ((room.GameID != NO_GAME_ID_IN_ROOM_FILE) && (room.GameID != game.uniqueid))
    {
        quitprintf("!Unable to load '%s'. This room file is assigned to a different game.", room_filename.GetCStr());
    }

    err = LoadRoomScript(&room, room_id);
    if (!err)
    {
        quitprintf("!Unable to load script from '%s'. Error: %s", room_filename.GetCStr(),
                   err->FullMessage().GetCStr());
    }

    // Optionally dump joint RTTI into the log
    if (services.LogScriptRTTI && RuntimeScript::GetJointRTTI())
    {
        Debug::Printf(PrintRTTI(RuntimeScript::GetJointRTTI()->AsConstRTTI()));
    }
    // Optionally dump room script's TOC into the log
    if (services.LogScriptTOC && room.CompiledScript && room.CompiledScript->sctoc)
    {
        Debug::Printf(PrintScriptTOC(*room.CompiledScript->sctoc, room.CompiledScript->sectionNames[0].c_str()));
    }

    set_our_eip(201);

    room.LoadedWidth = room.Width;
    room.LoadedHeight = room.Height;
    room.AnimBackgroundSpeed = room.BgAnimSpeed;
    game.Play.bg_anim_delay = room.AnimBackgroundSpeed;
    room.BgModified.resize(room.BgFrameCount);

    // Fixup the frame index, in case the new room does not have enough background frames
    if (room.BgFrame < 0 || static_cast<size_t>(room.BgFrame) >= room.BgFrameCount)
        room.BgFrame = 0;

    // Set up the 8-bit palettes: a color the game marks as PAL_BACKGROUND takes the room's palette;
    // for every other color the game-wide one is copied into each background frame's palette
    for (size_t pal_index = 0; pal_index < 256; pal_index++)
    {
        if (game.paluses[pal_index] == PAL_BACKGROUND)
            game.Palette[pal_index] = room.Palette[pal_index];
        else
        {
            // copy the game-wide colours into the room palette
            for (size_t i = 0; i < room.BgFrameCount; ++i)
                room.BgFrames[i].Palette[pal_index] = game.Palette[pal_index];
        }
    }

    for (size_t i = 0; i < room.BgFrameCount; ++i)
    {
        room.BgImages[i] = PrepareSpriteForUse(room.BgImages[i], false /* no alpha */, false /* no keep mask */,
                                               false /* dont care about alpha */, room.BgFrames[i].Palette);
    }

    // If the room script data did not have any name, then assign room filename, primarily for test/debug purposes
    if (room.CompiledScript && room.CompiledScript->GetScriptName().empty())
        room.CompiledScript->SetScriptName(room_filename.GetCStr());

    set_our_eip(202);
    Mouse_SetBounds(0, 0, 0, 0);

    set_our_eip(203);
    game.RoomChange.InNewRoom = kEnterRoom_Normal;


    // room.WalkableAreasTemp is used by the pathfinder to generate a
    // copy of the walkable areas - allocate it here to save time later
    delete room.WalkableAreasTemp;
    room.WalkableAreasTemp =
        BitmapHelper::CreateBitmap(room.WalkAreaMask->GetWidth(), room.WalkAreaMask->GetHeight(), 8);

    // Make a backup copy of the walkable areas prior to
    // any RemoveWalkableArea commands
    delete room.WalkAreaBackup;
    // copy of the walkable area mask
    room.WalkAreaBackup = BitmapHelper::CreateBitmapCopy(room.WalkAreaMask.get());

    set_our_eip(204);
    redo_walkable_areas();
    walkbehinds_recalc();

    set_our_eip(205);
    if (ch != nullptr)
    {
        // if not restoring a game, always reset temp room
        reset_temp_room();
    }

    // A room with a valid number has its own saveable RoomStatus, any other one (e.g. the intro) uses room.TempState
    if ((room_id >= 0) & (room_id < MAX_ROOMS))
        room.State = GetRoomState(room_id);
    else
        room.State = &room.TempState;

    // Decide what to do if we have been or not in this room before
    const bool been_here = room.State->beenhere > 0;
    if (room.State->beenhere == 0)
    {
        // If we have not been in this room before, then copy necessary fields from the room's data
        room.State->numobj = room.Objects.size();
        room.State->tsdatasize = 0u;
        room.State->obj.resize(room.State->numobj);
        room.State->objProps.resize(room.State->numobj);
        room.State->face_dir_ratio = room.Options.FaceDirectionRatio;
        init_object_states(0u, room.Objects.size());

        for (size_t i = 0; i < (size_t)MAX_WALK_BEHINDS; ++i)
            room.State->walkbehind_base[i] = room.WalkBehinds[i].Baseline;

        for (int i = 0; i < MAX_ROOM_HOTSPOTS; i++)
        {
            room.State->hotspot[i].Enabled = true;
            room.State->hotspot[i].Name = room.Hotspots[i].Name;
        }
        for (int i = 0; i < MAX_ROOM_REGIONS; i++)
        {
            room.State->region_enabled[i] = 1;
        }
        for (int i = 0; i < MAX_WALK_AREAS; ++i)
        {
            room.State->walkareas[i].FaceDirectionRatio = room.WalkAreas[i].FaceDirectionRatio;
        }

        room.State->beenhere = 1;
        game.RoomChange.InNewRoom = kEnterRoom_FirstTime;
    }
    else
    {
        // We HAD been in this room, so normally we don't have to do anything... but
        // If the restored number of object states does not match real room,
        // then adjust the *room state* by either expanding or cutting objects array
        if (room.State->obj.size() != room.Objects.size())
        {
            Debug::Printf(kDbgMsg_Warn,
                          "WARNING: the restored room state has different number of object states than the current "
                          "room has objects (%u vs %u).",
                          room.State->numobj, (uint32_t)room.Objects.size());
            const size_t was_obj_states = room.State->obj.size();
            room.State->numobj = room.Objects.size();
            room.State->obj.resize(room.Objects.size());
            room.State->objProps.resize(room.Objects.size());
            if (was_obj_states < room.Objects.size())
                init_object_states(was_obj_states, room.Objects.size());
        }
    }

    // Reset contentFormat hint to avoid doing fixups later
    room.State->contentFormat = kRoomStatSvgVersion_Current;

    room.Objs = !room.State->obj.empty() ? &room.State->obj[0] : nullptr;

    // Export named room elements to script
    for (uint32_t i = 0; i < room.Objects.size(); ++i)
    {
        if (!room.Objects[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(room.Objects[i].ScriptName, &game.Handles.Objects[i]);
    }

    for (uint32_t i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        if (!room.Hotspots[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(room.Hotspots[i].ScriptName, &game.Handles.Hotspots[i]);
    }

    for (uint32_t i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        if (!room.Regions[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(room.Regions[i].ScriptName, &game.Handles.Regions[i]);
    }

    for (uint32_t i = 0; i < MAX_WALK_AREAS; ++i)
    {
        if (!room.WalkAreas[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(room.WalkAreas[i].ScriptName, &game.Handles.WalkableAreas[i]);
    }

    for (uint32_t i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        if (!room.WalkBehinds[i].ScriptName.IsEmpty())
            ccAddExternalScriptObjectHandle(room.WalkBehinds[i].ScriptName, &game.Handles.WalkBehinds[i]);
    }

    set_our_eip(210);
    if (game.Play.ShouldAASprites())
    {
        create_rgb_table(&rgb_table, game.Palette, nullptr);
        rgb_map = &rgb_table;
    }
    set_our_eip(211);
    if (ch != nullptr)
    {
        // if it's not a Restore Game

        // if a following character is still waiting to come into the
        // previous room, force it out so that the timer resets
        for (int i = 0; i < game.numcharacters; i++)
        {
            if ((game.Characters[i].following >= 0) && (game.Characters[i].room < 0))
            {
                if ((game.Characters[i].following == game.playercharacter) && (ch->prevroom == room_id))
                    // the player went back to the previous room, so make sure
                    // the following character is still there
                    game.Characters[i].room = room_id;
                else
                    game.Characters[i].room = game.Characters[game.Characters[i].following].room;
            }
        }

        ch->prevroom = ch->room;
        ch->room = room_id;
        // only stop moving if it's a new room, not a restore game
        for (int i = 0; i < game.numcharacters; i++)
            Character_StopMoving(&game.Characters[i]);
    }

    room.Script = nullptr;
    if (services.Debug.Flags & DBG_NOSCRIPT)
        ;
    else if (room.CompiledScript)
    {
        compile_room_script();
        if (been_here)
        {
            const auto &global_data = room.Script->GetGlobalData();
            if (room.State->tsdatasize > global_data.size())
            {
                quitprintf("Restored Room %d script data size exceeds current script data (%u vs %zu bytes).", room_id,
                           room.State->tsdatasize, global_data.size());
            }
            else if (room.State->tsdatasize < global_data.size())
            {
                Debug::Printf(
                    kDbgMsg_Warn,
                    "WARNING: Restored Room %d script data size is less than the current script data (%u vs %zu bytes)",
                    room_id, room.State->tsdatasize, global_data.size());
            }
            room.Script->CopyGlobalData(room.State->tsdata);
        }
    }
    set_our_eip(207);
    room.EnteredEdge = -1;

    if ((game.RoomChange.X != SCR_NO_VALUE) && (ch != nullptr))
    {
        ch->x = game.RoomChange.X;
        ch->y = game.RoomChange.Y;
        if (game.RoomChange.PlaceOnWalkable)
            Character_PlaceOnWalkableArea(ch);

        if (game.RoomChange.Loop != SCR_NO_VALUE)
            ch->loop = game.RoomChange.Loop;
    }
    // reset new_room instructions
    game.RoomChange.X = game.RoomChange.Y = SCR_NO_VALUE;
    game.RoomChange.Loop = SCR_NO_VALUE;
    game.RoomChange.PlaceOnWalkable = false;

    if ((game.RoomChange.Pos > 0) & (ch != nullptr))
    {
        if (game.RoomChange.Pos >= 4000)
        {
            room.EnteredEdge = 3;
            ch->y = room.Edges.Top + 1;
            ch->x = game.RoomChange.Pos % 1000;
            if (ch->x == 0)
                ch->x = room.Width / 2;
            if (ch->x <= room.Edges.Left)
                ch->x = room.Edges.Left + 3;
            if (ch->x >= room.Edges.Right)
                ch->x = room.Edges.Right - 3;
            ch->loop = 0;
        }
        else if (game.RoomChange.Pos >= 3000)
        {
            room.EnteredEdge = 2;
            ch->y = room.Edges.Bottom - 1;
            ch->x = game.RoomChange.Pos % 1000;
            if (ch->x == 0)
                ch->x = room.Width / 2;
            if (ch->x <= room.Edges.Left)
                ch->x = room.Edges.Left + 3;
            if (ch->x >= room.Edges.Right)
                ch->x = room.Edges.Right - 3;
            ch->loop = 3;
        }
        else if (game.RoomChange.Pos >= 2000)
        {
            room.EnteredEdge = 1;
            ch->x = room.Edges.Right - 1;
            ch->y = game.RoomChange.Pos % 1000;
            if (ch->y == 0)
                ch->y = room.Height / 2;
            if (ch->y <= room.Edges.Top)
                ch->y = room.Edges.Top + 3;
            if (ch->y >= room.Edges.Bottom)
                ch->y = room.Edges.Bottom - 3;
            ch->loop = 1;
        }
        else if (game.RoomChange.Pos >= 1000)
        {
            room.EnteredEdge = 0;
            ch->x = room.Edges.Left + 1;
            ch->y = game.RoomChange.Pos % 1000;
            if (ch->y == 0)
                ch->y = room.Height / 2;
            if (ch->y <= room.Edges.Top)
                ch->y = room.Edges.Top + 3;
            if (ch->y >= room.Edges.Bottom)
                ch->y = room.Edges.Bottom - 3;
            ch->loop = 2;
        }
        // if starts on un-walkable area
        if (get_walkable_area_pixel(room, ch->x, ch->y) == 0)
        {
            if (game.RoomChange.Pos >= 3000)
            { // bottom or top of screen
                int try_left = ch->x - 1, try_right = ch->x + 1;
                while (1)
                {
                    if (get_walkable_area_pixel(room, try_left, ch->y) > 0)
                    {
                        ch->x = try_left;
                        break;
                    }
                    if (get_walkable_area_pixel(room, try_right, ch->y) > 0)
                    {
                        ch->x = try_right;
                        break;
                    }
                    int nowhere = 0;
                    if (try_left > room.Edges.Left)
                    {
                        try_left--;
                        nowhere++;
                    }
                    if (try_right < room.Edges.Right)
                    {
                        try_right++;
                        nowhere++;
                    }
                    if (nowhere == 0)
                        break; // no place to go, so leave him
                }
            }
            else if (game.RoomChange.Pos >= 1000)
            { // left or right
                int try_left = ch->y - 1, try_right = ch->y + 1;
                while (1)
                {
                    if (get_walkable_area_pixel(room, ch->x, try_left) > 0)
                    {
                        ch->y = try_left;
                        break;
                    }
                    if (get_walkable_area_pixel(room, ch->x, try_right) > 0)
                    {
                        ch->y = try_right;
                        break;
                    }
                    int nowhere = 0;
                    if (try_left > room.Edges.Top)
                    {
                        try_left--;
                        nowhere++;
                    }
                    if (try_right < room.Edges.Bottom)
                    {
                        try_right++;
                        nowhere++;
                    }
                    if (nowhere == 0)
                        break; // no place to go, so leave him
                }
            }
        }
        game.RoomChange.Pos = 0;
    }
    if (ch != nullptr)
    {
        room.EnteredAtX = ch->x;
        room.EnteredAtY = ch->y;
        if (ch->x >= room.Edges.Right)
            room.EnteredEdge = 1;
        else if (ch->x <= room.Edges.Left)
            room.EnteredEdge = 0;
        else if (ch->y >= room.Edges.Bottom)
            room.EnteredEdge = 2;
        else if (ch->y <= room.Edges.Top)
            room.EnteredEdge = 3;
    }

    set_our_eip(208);
    if (ch != nullptr)
    {
        if (room.Options.PlayerCharOff == 0)
        {
            ch->set_enabled(true);
            enable_cursor_mode(game.GetModeWalk());
        }
        else
        {
            ch->set_enabled(false);
            disable_cursor_mode(game.GetModeWalk());
            // remember which character we turned off, in case they
            // use SetPlayerCharacter within this room (so we re-enable
            // the correct character when leaving the room)
            game.Play.temporarily_turned_off_character = game.playercharacter;
        }
        if (ch->flags & CHF_FIXVIEW)
            ;
        else if (room.Options.PlayerView == 0)
            Character_SetViewDirect(ch, ch->defview, true);
        else
            Character_SetViewDirect(ch, room.Options.PlayerView - 1, true);
    }
    color_map = nullptr;

    set_our_eip(209);
    generate_light_table();

    // If we are not restoring a save, update cameras to accomodate for this
    // new room; otherwise this is done later when cameras are recreated.
    if (ch != nullptr)
    {
        if (game.Viewports.IsAutoRoomViewport())
            adjust_viewport_to_room();
        update_all_viewcams_with_newroom();
        game.Viewports.UpdateRoomCameras(); // update auto tracking
    }

    init_room_pathfinder();
    init_room_drawdata();
    RoomBgDS.resize(room.BgFrameCount);

    set_our_eip(212);
    invalidate_screen();
    new_room_flags = 0;
    game.Play.gscript_timer = -1; // avoid screw-ups with changing screens
    game.Play.player_on_region = 0;

    // drop any input which they might have done while it was loading
    ags_clear_input_queue();
    // no fade in, so set the palette immediately in case of 256-col sprites
    if (game.color_depth > 1)
        setpal();

    set_our_eip(220);
    update_polled_stuff();
    debug_script_log("Now in room %d", game.DisplayedRoom);
    game.Cursor.GState.MarkChanged();
    GUIE::MarkAllGUIForUpdate(true, true);
    pl_run_plugin_hooks(kPluginEvt_EnterRoom, game.DisplayedRoom);
}

void new_room(int room_id, Character *ch)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    Room &room = Room::Current();
    EndSkippingUntilCharStops();

    debug_script_log("Room change requested to room %d", room_id);

    // we are currently running Leaves Screen scripts
    game.Events.InLeavesScreen = room_id;

    // player leaves screen event
    run_room_event(kRoomEvent_BeforeFadeout);
    // Run the global OnRoomLeave event
    run_on_event(kScriptEvent_RoomLeave, game.DisplayedRoom);

    pl_run_plugin_hooks(kPluginEvt_LeaveRoom, game.DisplayedRoom);

    // update the new room number if it has been altered by OnLeave scripts
    room_id = game.Events.InLeavesScreen;
    game.Events.InLeavesScreen = -1;

    if ((game.PlayerChar->following >= 0) && (game.Characters[game.PlayerChar->following].room != room_id))
    {
        // the player character is following another character,
        // who is not in the new room. therefore, abort the follow
        game.PlayerChar->SetFollowing(-1);
    }

    // change rooms
    unload_old_room();

    if (services.Config.ClearCacheOnRoomChange)
    {
        // Delete all cached resources
        services.Sprites.DisposeAllFreeCached();
        soundcache_clear();
        texturecache_clear();
    }

    load_new_room(room_id, ch);

    // Update background frame state (it's not a part of the RoomStatus currently)
    room.BgFrame = 0;
    room.BgFrameLocked = (room.Options.Flags & kRoomFlag_BkgFrameLocked) != 0;
}

void set_room_placeholder()
{
    Room &room = Room::Current();
    room.InitDefaults();
    std::shared_ptr<Bitmap> dummy_bg(new Bitmap(1, 1, 8));
    room.BgImages.resize(1, dummy_bg);
    room.HotspotMask = dummy_bg;
    room.RegionMask = dummy_bg;
    room.WalkAreaMask = dummy_bg;
    room.WalkBehindMask = dummy_bg;

    reset_temp_room();
    room.State = &room.TempState;
}

void first_room_initialization()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    game.Events.InRoomTransition = true;
    game.PlayerChar->prevroom = -1;
    set_loop_counter(0);
    // Reset background frame state
    room.BgFrame = 0;
    room.BgFrameLocked = (room.Options.Flags & kRoomFlag_BkgFrameLocked) != 0;
}

void check_new_room()
{
    Game &game = Game::Current();
    // if they're in a new room, run Player Enters Screen and on_event(ENTER_ROOM)
    if ((game.RoomChange.InNewRoom != kEnterRoom_None) && (game.RoomChange.InNewRoom != kEnterRoom_RestoredSave))
    {
        AGSEvent event(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_BeforeFadein, game.playercharacter));
        // make sure that any script calls don't re-call enters screen
        const EnterNewRoomState saved_in_new_room = game.RoomChange.InNewRoom;
        game.RoomChange.InNewRoom = kEnterRoom_None;
        DisableInterfaceEx(true /* update cursor */);
        // sync drawable object states before running event
        // in case they access these properties in a script callback
        SyncDrawablesState();
        process_event(&event);
        EnableInterfaceEx(true /* update cursor */);
        game.RoomChange.InNewRoom = saved_in_new_room;
    }
}

void compile_room_script()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    cc_clear_error();

    room.Script = RuntimeScript::Create(room.CompiledScript.get());
    if ((cc_has_error()) || (room.Script == nullptr))
    {
        quitprintf("Unable to create local script:\n%s", cc_get_error().ErrorString.GetCStr());
    }

    room.Script->RegisterExports(simp);
    if (!room.Script->ResolveImports(simp))
        quitprintf("Unable to resolve imports in room script:\n%s", cc_get_error().ErrorString.GetCStr());

    game.Scripts.RepExecAlways.RoomHasFunction = true;
    game.Scripts.LateRepExecAlways.RoomHasFunction = true;
    game.Scripts.GetDialogOptionsDimensions.RoomHasFunction = true;
}

void on_background_frame_change()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    invalidate_screen();
    mark_current_background_dirty();

    // get the new frame's palette
    memcpy(game.Palette, room.BgFrames[room.BgFrame].Palette, sizeof(RGB) * 256);

    // hi-colour, update the palette. It won't have an immediate effect
    // but will be drawn properly when the screen fades in
    if (game.color_depth > 1)
        setpal();

    if (game.Events.InEntersScreen)
        return;

    // Don't update the palette if it hasn't changed
    if (room.BgFrames[room.BgFrame].IsPaletteShared)
        return;

    // 256-colours, tell it to update the palette (will actually be done as
    // close as possible to the screen update to prevent flicker problem)
    if (game.color_depth == 1)
        room.BgJustChanged = 1;
}

void croom_ptr_clear()
{
    Room &room = Room::Current();
    room.State = nullptr;
    room.Objs = nullptr;
}

ScriptDrawingSurface *get_room_bg_surface(uint32_t bg_index)
{
    assert(bg_index >= 0 && bg_index < RoomBgDS.size());
    // FIXME: the bare "bg_index &&" makes this return null for background 0 even if it has a surface attached
    if (bg_index >= 0 && bg_index && RoomBgDS[bg_index] > 0)
    {
        ScriptDrawingSurface *surface =
            static_cast<ScriptDrawingSurface *>(ccGetObjectAddressFromHandle(RoomBgDS[bg_index]));
        if (surface)
            return surface;
    }
    return nullptr;
}

ScriptDrawingSurface *get_room_mask_surface(RoomAreaMask mask)
{
    assert(mask > kRoomAreaNone && mask < kNumRoomAreaTypes);
    if (mask > kRoomAreaNone && mask < kNumRoomAreaTypes && RoomMaskDS[mask] > 0)
    {
        ScriptDrawingSurface *surface =
            static_cast<ScriptDrawingSurface *>(ccGetObjectAddressFromHandle(RoomMaskDS[mask]));
        if (surface)
            return surface;
    }
    return nullptr;
}

void attach_room_bg_surface(uint32_t bg_index, int surface_handle)
{
    assert(bg_index >= 0 && bg_index < RoomBgDS.size());
    if (bg_index >= 0 && bg_index < RoomBgDS.size())
        RoomBgDS[bg_index] = surface_handle;
}

void attach_room_mask_surface(RoomAreaMask mask, int surface_handle)
{
    assert(mask > kRoomAreaNone && mask < kNumRoomAreaTypes);
    if (mask > kRoomAreaNone && mask < kNumRoomAreaTypes)
        RoomMaskDS[mask] = surface_handle;
}

void detach_room_bg_surfaces()
{
    // Invalidate any linked DrawingSurfaces
    for (auto &ds_handle : RoomBgDS)
    {
        if (ds_handle > 0)
        {
            ScriptDrawingSurface *surface =
                static_cast<ScriptDrawingSurface *>(ccGetObjectAddressFromHandle(ds_handle));
            if (surface)
                surface->Invalidate();
            ds_handle = 0;
        }
    }
    for (auto &ds_handle : RoomMaskDS)
    {
        if (ds_handle > 0)
        {
            ScriptDrawingSurface *surface =
                static_cast<ScriptDrawingSurface *>(ccGetObjectAddressFromHandle(ds_handle));
            if (surface)
                surface->Invalidate();
            ds_handle = 0;
        }
    }
}

void on_room_bg_surface_release(uint32_t bg_index, bool modified)
{
    Room &room = Room::Current();
    if (modified)
    {
        if (bg_index == room.BgFrame)
        {
            invalidate_screen();
            mark_current_background_dirty();
        }
        room.BgModified[bg_index] = true;
    }
    RoomBgDS[bg_index] = 0;
}

void on_room_mask_surface_release(RoomAreaMask mask, bool modified)
{
    if (modified)
    {
        if (mask == kRoomAreaWalkBehind)
        {
            walkbehinds_recalc();
        }
        if (get_room_mask_debugmode() == mask)
        {
            debug_draw_room_mask(mask);
        }
        Room::Current().MaskModified[mask] = true;
    }
    RoomMaskDS[mask] = 0;
}

void init_room_pathfinder()
{
    if (!room_pathfinder)
        room_pathfinder = Pathfinding::CreateDefaultMaskPathfinder();
}

void dispose_room_pathfinder()
{
    room_pathfinder.reset();
}

MaskRouteFinder *get_room_pathfinder()
{
    return room_pathfinder.get();
}

// coordinate conversion (room) ---> (room mask)
int room_to_mask_coord(Room &room, int coord)
{
    return coord / room.MaskResolution;
}
// coordinate conversion (room mask) ---> (room)
int mask_to_room_coord(Room &room, int coord)
{
    return coord * room.MaskResolution;
}

#include "debug/out.h"
#include "game/dynobj/script_string.h"
#include "script/script_runtime.h"

void SetAmbientTint(int red, int green, int blue, int opacity, int luminance)
{
    Game &game = Game::Current();
    if ((red < 0) || (green < 0) || (blue < 0) || (red > 255) || (green > 255) || (blue > 255) || (opacity < 0) ||
        (opacity > 255) || (luminance < 0) || (luminance > 255))
        quit("!SetTint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-255");

    debug_script_log("Set ambient tint RGB(%d,%d,%d) %d", red, green, blue, opacity);

    game.Play.rtint_enabled = opacity > 0;
    game.Play.rtint_red = red;
    game.Play.rtint_green = green;
    game.Play.rtint_blue = blue;
    game.Play.rtint_level = opacity;
    game.Play.rtint_light = luminance;
}

void SetAmbientLightLevel(int light_level)
{
    Game &game = Game::Current();
    light_level = Math::Clamp(light_level, -255, 255);

    game.Play.rtint_enabled = light_level != 0;
    game.Play.rtint_level = 0;
    game.Play.rtint_light = light_level;
}

void NewRoom(int nrnum)
{
    Game &game = Game::Current();
    if (nrnum < 0)
        quitprintf("!NewRoom: room change requested to invalid room number %d.", nrnum);

    if (game.DisplayedRoom < 0)
    {
        // called from game_start; change the room where the game will start
        game.PlayerChar->room = nrnum;
        return;
    }


    debug_script_log("Room change requested to room %d", nrnum);
    EndSkippingUntilCharStops();

    can_run_delayed_command();

    if (handle_state_change_in_dialog_request("NewRoom", DIALOG_NEWROOM + nrnum))
        return; // handled

    if (game.Events.InLeavesScreen >= 0)
    {
        // NewRoom called from the Player Leaves Screen event -- just
        // change which room it will go to
        game.Events.InLeavesScreen = nrnum;
    }
    else if (game.Events.InEntersScreen)
    {
        setevent(AGSEvent_NewRoom(nrnum));
        return;
    }
    else if (!is_inside_script())
    {
        new_room(nrnum, game.PlayerChar);
        return;
    }
    else if (is_inside_script())
    {
        get_executingscript()->QueueAction(PostScriptAction(ePSANewRoom, nrnum, "NewRoom"));
        // we might be within a MoveCharacterBlocking -- the room
        // change should abort it
        if (is_char_walking_ndirect(game.PlayerChar))
        {
            // stop the character, but doesn't fix the character to a walkable area
            Character_StopMovingEx(game.PlayerChar, false);
        }
    }
}

void ResetRoom(int nrnum)
{
    if (nrnum == Game::Current().DisplayedRoom)
        quitprintf("!ResetRoom: cannot reset current room  %d", nrnum);
    if ((nrnum < 0) || (nrnum >= MAX_ROOMS))
        quitprintf("!ResetRoom: invalid room number %d", nrnum);

    ResetRoomState(nrnum);

    debug_script_log("Room %d reset to original state", nrnum);
}

int HasPlayerBeenInRoom(int roomnum)
{
    if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
        return 0;
    RoomStatus *roomstat = GetRoomStateIfExists(roomnum);
    return (roomstat != nullptr) && (roomstat->beenhere != 0);
}

void CallRoomScript(int value)
{
    Room &room = Room::Current();
    can_run_delayed_command();

    if (!is_inside_script())
        quit("!CallRoomScript: not inside a script???");

    room.ScriptFinished = 0;
    RuntimeScriptValue params[]{value, RuntimeScriptValue()};
    RunScriptFunctionAuto(kScTypeRoom, "on_call", 1, params);
    room.ScriptFinished = 1;
}

void SetBackgroundFrame(int frnum)
{
    Room &room = Room::Current();
    if ((frnum < -1) || (frnum != -1 && (size_t)frnum >= room.BgFrameCount))
    {
        debug_script_error(
            "SetBackgrondFrame: invalid background number specified: %d, valid range in this room is 0..%u", frnum,
            room.BgFrameCount - 1);
        return;
    }

    if (frnum < 0)
    {
        room.BgFrameLocked = 0;
        return;
    }

    room.BgFrameLocked = 1;

    if (frnum == room.BgFrame)
    {
        // already on this frame, do nothing
        return;
    }

    room.BgFrame = frnum;
    on_background_frame_change();
}

int GetBackgroundFrame()
{
    return Room::Current().BgFrame;
}

void RoomProcessClick(int x, int y, int mood)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    int loc_id = -1;
    int loc_type = GetLocationTypeImpl(&loc_id, x, y, kHit_Interactable, true /* click-through gui */,
                                       false /* not allow hotspot0 */);
    VpPoint vpt = game.Viewports.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return;
    x = vpt.first.X;
    y = vpt.first.Y;

    if (game.HasCursorRole(mood, kCursorRole_Walk) && (game.options[OPT_NOWALKMODE] == 0))
    {
        int hotspot_id = GetHotspotIDAtRoom(x, y, kHit_Interactable);
        if (hotspot_id < 1)
            ;
        else if (room.Hotspots[hotspot_id].WalkTo.X < 1)
            ;
        else if (game.Play.auto_use_walkto_points == 0)
            ;
        else
        {
            x = room.Hotspots[hotspot_id].WalkTo.X;
            y = room.Hotspots[hotspot_id].WalkTo.Y;
            debug_script_log("Move to walk-to point hotspot %d", hotspot_id);
        }
        walk_character(game.PlayerChar, x, y, false /* walk areas */);
        return;
    }
    game.Play.usedmode = mood;

    if (loc_type == 0)
    {
        // click on nothing -> hotspot 0
        loc_id = 0;
        loc_type = LOCTYPE_HOTSPOT;
    }

    if (loc_type == LOCTYPE_CHAR)
    {
        if (check_click_on_character(x, y, mood))
            return;
    }
    else if (loc_type == LOCTYPE_OBJ)
    {
        if (check_click_on_object(x, y, mood))
            return;
    }
    else if (loc_type == LOCTYPE_HOTSPOT)
        RunHotspotInteraction(loc_id, mood);
}

int IsInteractionAvailable(int x, int y, int mood)
{
    Game &game = Game::Current();
    int loc_id = -1;
    int loc_type = GetLocationTypeImpl(&loc_id, x, y, kHit_Interactable, true /* click-through gui */,
                                       false /* not allow hotspot0 */);
    VpPoint vpt = game.Viewports.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return 0;
    x = vpt.first.X;
    y = vpt.first.Y;

    // You can always walk places
    if (game.HasCursorRole(mood, kCursorRole_Walk) && (game.options[OPT_NOWALKMODE] == 0))
        return 1;

    game.Play.check_interaction_only = 1;

    if (loc_type == 0)
    {
        // click on nothing -> hotspot 0
        loc_id = 0;
        loc_type = LOCTYPE_HOTSPOT;
    }

    if (loc_type == LOCTYPE_CHAR)
    {
        check_click_on_character(x, y, mood);
    }
    else if (loc_type == LOCTYPE_OBJ)
    {
        check_click_on_object(x, y, mood);
    }
    else if (loc_type == LOCTYPE_HOTSPOT)
        RunHotspotInteraction(loc_id, mood);

    int check_result = game.Play.check_interaction_only;
    game.Play.check_interaction_only = 0;

    if (check_result == 2)
        return 1;

    return 0;
}
