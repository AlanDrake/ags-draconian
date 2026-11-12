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
// Game loop
//

#include "main/game_run.h"

#include <chrono>
#include <limits>

#include <SDL.h>

#include "ac/button.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/common.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_debug.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/gui.h"
#include "ac/hotspot.h"
#include "ac/joystick.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/overlay.h"
#include "ac/region.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/sys_events.h"
#include "ac/touch.h"
#include "ac/viewframe.h"
#include "ac/walkablearea.h"
#include "ac/walkbehind.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "device/mousew32.h"
#include "gui/animatingguibutton.h"
#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "main/engine.h"
#include "main/update.h"
#include "media/audio/audio_system.h"
#include "media/video/video.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern int mouse_on_iface; // mouse cursor is over this interface
extern int ifacepopped;
extern volatile bool want_exit, abort_engine;
extern int proper_exit;
extern int displayed_room;
extern EnterNewRoomState in_new_room, new_room_was;
extern ScriptSystem scsystem;
extern GameSetupStruct game;
extern RoomStruct thisroom;
extern int game_paused;
extern bool in_enters_screen;
extern bool done_as_error;
extern int in_leaves_screen;
extern CharacterInfo *playerchar;
extern int cur_mode;
extern RoomObject *objs;
extern RoomStatus *croom;
extern SpriteCache spriteset;
extern char check_dynamic_sprites_at_exit;

// Checks if wait mode should continue until condition is met
static bool ShouldStayInWaitMode();

struct GameTimer
{
    uint32_t LoopCounter = 0u;
    // Real FPS (calculated from time between frames)
    float FPS = std::numeric_limits<float>::quiet_NaN();
    // FPS syncing: it's done periodically, not every frame
    const uint32_t FPSSyncPeriodMs = 1000u;
    Clock::time_point LastFPSSyncTime = {};
    uint32_t LastFPSSyncCounter = 0u;
    // For runtime reference
    Clock::time_point StartTime = {};
    bool RuntimeStarted = false;
} static GameTimer;


// Kinds of conditions used when "waiting" for something
#define UNTIL_ANIMEND     1
#define UNTIL_MOVEEND     2
#define UNTIL_CHARIS0     3
#define UNTIL_NOOVERLAY   4
#define UNTIL_NEGATIVE    5
#define UNTIL_INTIS0      6
#define UNTIL_SHORTIS0    7
#define UNTIL_INTISNEG    8
#define UNTIL_ANIMBTNEND  9
#define UNTIL_FLAGUNSET   10
#define UNTIL_VIEWANIM    11
#define UNTIL_ANIMOVEREND 12

static void GameTick();

// Game state instructs the engine to run game loops until
// certain condition is not fullfilled.
// TODO: reimplement the end condition check using a function pointer?
class GameLoopUntilState : public GameState
{
public:
    GameLoopUntilState(int until_what, const void *data_ptr = nullptr, int data1 = 0, int data2 = 0)
        : _untilType(until_what),
          _disabledFor(FOR_EXITLOOP),
          _dataPtr(data_ptr),
          _data1(data1),
          _data2(data2)
    {
    }

    int GetUntilType() const { return _untilType; }
    const void *GetDataPtr() const { return _dataPtr; }
    int GetData1() const { return _data1; }
    int GetData2() const { return _data2; }

    // Begin the state, initialize and prepare any resources
    void Begin() override
    {
        assert(_disabledFor == FOR_EXITLOOP);
        // Only change the mouse cursor if it hasn't been specifically changed first
        // (or if it's speech, always change it)
        bool should_update_cursor = ((cur_cursor == cur_mode) || (_untilType == UNTIL_NOOVERLAY)) &&
                                    (!is_current_cursor_mode(kCursorRole_Wait));
        DisableInterfaceEx(should_update_cursor);
    }
    // End the state, release all resources
    void End() override
    {
        set_our_eip(77);
        EnableInterfaceEx(true /* update cursor */);

        switch (_disabledFor)
        {
        case FOR_EXITLOOP: break;
        // The other FOR_* reasons (FOR_ANIMATION, FOR_SCRIPT) are not supported: obsolete since at least v2.5
        default: quit("Unknown reason to disable user input in the Wait state."); break;
        }
    }
    // Draw the state
    void Draw() override {}
    // Update the state during a game tick
    bool Run() override
    {
        // If skipping cutscene and expecting user input: don't wait at all
        if (play.fast_forward && (play.wait_counter != 0) && ((play.key_skip_wait & ~SKIP_AUTOTIMER) != 0))
        {
            play.SetWaitSkipResult(SKIP_NONE);
            return false;
        }

        GameTick();
        return ShouldStayInWaitMode();
    }

private:
    int _untilType = 0;   // type of condition, UNTIL_* constant
    int _disabledFor = 0; // FOR_* constant
    // pointer to the test variable
    const void *_dataPtr = nullptr;
    // other values used for a test, depend on type
    int _data1 = 0;
    int _data2 = 0;
};

// TODO: this is a global variable, because this state is checked during update;
// find a way to refactor this and not have it here.
std::unique_ptr<GameLoopUntilState> restrict_until;
bool restrict_until_in_script = false;

// Schedules a normal exit from the game
static void ProperExit()
{
    want_exit = false;
    proper_exit = 1;
    quit("||exit!");
}

// Checks for potential general problems at the start of the game update,
// that may lead to warnings or game aborts
static void GameUpdateCheckProblems()
{
    if (displayed_room < 0)
    {
        quit("!A blocking function was called before the first room has been loaded");
    }

    if ((in_enters_screen) && (!done_as_error))
    {
        debug_script_warn("Wait() was used in \"Before Fadein\" event; use \"After Fadein\" instead");
        done_as_error = true;
    }

    if (!get_can_run_delayed_command())
    {
        quit("!A blocking function was called from within a non-blocking event such as " REP_EXEC_ALWAYS_NAME);
    }

    // If we're not fading in, don't count the fadeouts
    // TODO: this fixup does not belong to the per-frame check; it should be done once, after restoring
    // a save, and after 'no_hicolor_fadein' or OPT_FADETYPE is changed
    if ((play.no_hicolor_fadein) && (game.options[OPT_FADETYPE] == kScrTran_Fade))
        play.screen_is_faded_out = false;
}

// Runs rep-exec-always immediately,
// schedules global and room's rep-exec events to be run during script event processing.
static void GameUpdateEarlyRepExec()
{
    if (in_new_room == kEnterRoom_None)
    {
        // Run the room and game script repeatedly_execute
        run_function_on_non_blocking_thread(&repExecAlways);
        setevent(AGSEvent_Script(kTS_RepeatedlyExecute));
        setevent(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_Repexec));
    }
}

// Runs late-rep-exec-always
static void GameUpdateLateRepExec()
{
    if (in_new_room == kEnterRoom_None)
    {
        // sync drawable object states before running event
        // in case they access these properties in a script callback
        SyncDrawablesState();
        // Run the room and game script late_repeatedly_execute
        run_function_on_non_blocking_thread(&lateRepExecAlways);
    }
}

// Check room's "ground" triggers, such as regions and hotspots with
// "Stand On" event callbacks.
static void GameUpdateCheckGroundInteractions()
{
    // Do not check for ground interactions if we are in the process of
    // entering the room (between "load room" and "after fade-in" events).
    // (For backwards compatibility, skip this condition for older games)
    if (in_new_room != kEnterRoom_None)
        return;

    // If ground interactions are disabled completely, then bail out
    if ((play.ground_level_areas_disabled & GLED_INTERACTION) != 0)
        return;

    // Do not check for ground interactions while in the waiting
    // (a blocking action, or a Wait call from the user script).
    // This is done because interaction event handlers are scheduled and are
    // only run after blocking action / wait is over.
    // Which may cause all kinds of unexpected and untimely effects.
    // NOTE: this condition was not present in the older versions of the
    // engine, but result was more or less same by accident, as the number
    // of scheduled callbacks was limited to a very small number.
    // (That was pretty unreliable though.)
    if (IsInBlockingAction())
    {
        // NOTE: play.player_on_region is not updated here; the change is detected by the first update
        // after the blocking action ends. Updating it here would instead let the player trigger
        // "walk on/off region" after finishing a blocking walk, if it was walking back and forth
        // the region and the last step has crossed the region's border.
        return;
    }
    else
    {
        // check if he's standing on a hotspot
        int hotspot_there = GetHotspotIDAtRoom(playerchar->x, playerchar->y, kHit_Interactable);
        // run Stands on Hotspot event;
        // NOTE: this runs even for hotspot 0 (no hotspot), in case one has a script function attached
        setevent(AGSEvent_Object(kObjEventType_Hotspot, hotspot_there, kHotspotEvent_StandOn));

        // check current region
        int on_region = GetRegionIDAtRoom(playerchar->x, playerchar->y, kHit_Interactable);
        if (on_region != play.player_on_region)
        {
            // we need to save this and set play.player_on_region
            // now, so it's correct going into RunRegionInteraction
            int old_region = play.player_on_region;

            play.player_on_region = on_region;
            // Walks Off last region
            if (old_region > 0)
                setevent(AGSEvent_Object(kObjEventType_Region, old_region, kRegionEvent_WalkOff));
            // Walks Onto new region
            if (on_region > 0)
                setevent(AGSEvent_Object(kObjEventType_Region, on_region, kRegionEvent_WalkOn));
        }

        if (play.player_on_region > 0) // player stands on region
        {
            setevent(AGSEvent_Object(kObjEventType_Region, play.player_on_region, kRegionEvent_Standing));
        }
    }
}

static void lock_mouse_on_click()
{
    // Only update when in windowed mode, as always locked in fullscreen
    if (usetup.MouseAutoLock && scsystem.windowed != 0)
        Mouse::TryLockToWindow();
}

static void toggle_mouse_lock()
{
    // Only update when in windowed mode, as always locked in fullscreen
    if (scsystem.windowed)
    {
        if (Mouse::IsLockedToWindow())
            Mouse::UnlockFromWindow();
        else
            Mouse::TryLockToWindow();
    }
}

bool run_service_mb_controls(eAGSMouseButton &out_mbut, Point *out_mpos)
{
    out_mbut = kMouseNone; // clear the output
    if (out_mpos)
        *out_mpos = {};
    if (ags_inputevent_ready() != kInputMouse)
        return false; // there was no mouse event

    const SDL_Event mb_evt = ags_get_next_inputevent();
    if (mb_evt.type == SDL_MOUSEBUTTONDOWN)
    {
        out_mbut = sdl_mbut_to_ags_but(mb_evt.button.button);
        if (out_mpos)
            *out_mpos = Mouse::SysToGamePos(mb_evt.button.x, mb_evt.button.y);
        lock_mouse_on_click();
    }
    return out_mbut != kMouseNone;
}

static eAGSMouseButton wasbutdown = kMouseNone;
static int wasongui = 0;

// Runs default handling of mouse movement, button state, and wheel
static void check_mouse_state(int &was_mouse_on_iface)
{
    if (!play.mouse_input_enabled)
        return;

    mouse_on_iface = gui_on_mouse_move(mousex, mousey);
    was_mouse_on_iface = mouse_on_iface;

    if ((ifacepopped >= 0) && (mousey >= guis[ifacepopped].GetY() + guis[ifacepopped].GetHeight()))
        remove_popup_interface(ifacepopped);

    // check mouse clicks on GUIs
    if ((wasbutdown > kMouseNone) && (ags_misbuttondown(wasbutdown)))
    {
        gui_on_mouse_hold(wasongui, wasbutdown);
    }
    else if ((wasbutdown > kMouseNone) && (!ags_misbuttondown(wasbutdown)))
    {
        eAGSMouseButton mouse_btn_up = wasbutdown;
        wasbutdown = kMouseNone; // reset before event, avoid recursive call of "mouse up"
        gui_on_mouse_up(wasongui, mouse_btn_up, mousex, mousey);
    }

    int mwheelz = ags_check_mouse_wheel();
    if (mwheelz < 0)
        setevent(AGSEvent_Script(kTS_MouseClick, 9, mousex, mousey));
    else if (mwheelz > 0)
        setevent(AGSEvent_Script(kTS_MouseClick, 8, mousex, mousey));
}

// Runs default mouse button handling
static void check_mouse_controls(const int was_mouse_on_iface)
{
    eAGSMouseButton mbut;
    Point mpos;
    if (run_service_mb_controls(mbut, &mpos) && mbut > kMouseNone)
    {
        // NOTE: this option disables mouse *in game*, the "service" controls are always run.
        if (!play.mouse_input_enabled)
        {
            // Still run the plugin hooks though, as it may be overriding game logic
            // TODO: we might consider to expand plugin api, having this issue in mind
            // (i.e. have separate events for any mouse click and in-game mouse click)
            if (pl_run_plugin_hooks(kPluginEvt_MouseClick, mbut))
                debug_script_log("Plugin handled mouse button %d", mbut);
            return;
        }

        check_skip_cutscene_mclick(mbut);

        if (play.fast_forward || play.IsIgnoringInput())
        { /* do nothing if skipping cutscene or input disabled */
        }
        else if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_MOUSECLICK) != 0)
        {
            play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
        }
        else if (play.text_overlay_on > 0)
        {
            if (play.speech_skip_style & SKIP_MOUSECLICK)
            {
                remove_screen_overlay(play.text_overlay_on);
                play.SetWaitSkipResult(SKIP_MOUSECLICK, mbut);
            }
        }
        else if (!IsInterfaceEnabled())
            ; // blocking cutscene, ignore mouse
        else if (pl_run_plugin_hooks(kPluginEvt_MouseClick, mbut))
        {
            // plugin took the click
            debug_script_log("Plugin handled mouse button %d", mbut);
        }
        else if (was_mouse_on_iface >= 0)
        {
            if (wasbutdown == kMouseNone)
            {
                // FIXME: passes the current mousex, mousey, which differ from mpos (the position recorded
                // with the click event) when the cursor moved after the click was queued; passing mpos needs
                // a check of how it affects other GUI processing (mouse up and motion)
                gui_on_mouse_down(was_mouse_on_iface, mbut, mousex, mousey);
            }
            wasongui = was_mouse_on_iface;
            wasbutdown = mbut;
        }
        else
            setevent(AGSEvent_Script(kTS_MouseClick, mbut, mpos.X, mpos.Y));
    }
}

// Runs service key controls, returns false if service key combinations were handled
// and no more processing required, otherwise returns true and provides current keycode and key shifts.
//
// * The special controls compare the backward-compatible combined key (KeyInput::CompatKey: key + mod),
//   because game variables may store old-style key + mod codes.
bool run_service_key_controls(KeyInput &out_key)
{
    out_key = KeyInput(); // clear the output
    if (ags_inputevent_ready() != kInputKeyboard)
        return false; // there was no key event

    const SDL_Event key_evt = ags_get_next_inputevent();
    bool handled = false;

    // Following section is for testing for pushed and released mod-keys.
    // A bit of explanation: some service actions may require combination of
    // mod-keys, for example [Ctrl + Alt] toggles mouse lock in window.
    // Here comes a problem: other actions may also use [Ctrl + Alt] mods in
    // combination with a third key: e.g. [Ctrl + Alt + V] displays engine info.
    // For this reason we cannot simply test for pressed Ctrl and Alt here,
    // but we must wait until player *releases at least one mod key* of this combo,
    // while no third key was pressed.
    // In other words, such action should only trigger if:
    // * if combination of held down mod-keys was gathered,
    // * if no other key was pressed meanwhile,
    // * if at least one of those gathered mod-keys was released.
    //
    // TODO: maybe split this mod handling into sep procedure and make it easier to use (not that it's used alot)?
    int cur_mod = sys_modkeys;
    bool is_only_mod_key = false;
    switch (key_evt.type)
    {
    case SDL_KEYDOWN:
        is_only_mod_key = is_sdl_mod_key(key_evt.key.keysym);
        cur_mod |= make_sdl_merged_mod(make_sdl_mod_flag(key_evt.key.keysym));
        break;
    case SDL_KEYUP:
        is_only_mod_key = is_sdl_mod_key(key_evt.key.keysym);
        cur_mod &= ~make_sdl_merged_mod(make_sdl_mod_flag(key_evt.key.keysym));
        break;
    }

    // If mods combination have already triggered an action,
    // then do nothing until all the current mods are released
    if (!sys_modkeys_fired)
    {
        // If any non-mod key is pressed, add "fired" flag to indicate that
        // this is no longer a pure mod keys combination
        if ((sys_modkeys != 0) && !is_only_mod_key)
        {
            sys_modkeys_fired = true;
        }
        // If some of the previously pressed mods were released, then run key combo action
        // and set "fired" flag to prevent multiple execution
        else if ((sys_modkeys != 0) && ((sys_modkeys & cur_mod) != sys_modkeys))
        {
            // Toggle mouse lock on Ctrl + Alt
            if (sys_modkeys == (KMOD_CTRL | KMOD_ALT))
            {
                toggle_mouse_lock();
                handled = true;
            }
            sys_modkeys_fired = true;
        }
    }
    // Save new mod flags, keep or erase the "fired" flag,
    // depending on whether there are any mod keys still pressed
    sys_modkeys = cur_mod;
    sys_modkeys_fired = sys_modkeys_fired && (cur_mod != 0);

    // If a service key combination was handled, then stop here
    if (handled)
        return false;

    KeyInput ki = sdl_keyevt_to_ags_key(key_evt);
    if ((ki.Key == eAGSKeyCodeNone) && (ki.UChar == 0))
        return false; // should skip this key event

    // Use backward-compatible combined key for special controls,
    // because game variables may store old-style key + mod codes
    const eAGSKeyCode ags_combo_key = AGSKeyToScriptKey(ki.CompatKey);

    // LAlt or RAlt + Enter/Return
    if ((ki.Mod & eAGSModAlt) && (ags_combo_key == eAGSKeyCodeReturn))
    {
        engine_try_switch_windowed_gfxmode();
        return false;
    }

    // Alt+X, abort (but only once game is loaded)
    if ((displayed_room >= 0) && (play.abort_key > 0) && (ags_combo_key == play.abort_key))
    {
        Debug::Printf("Abort key pressed");
        check_dynamic_sprites_at_exit = 0;
        quit("!|");
    }

    if ((ags_combo_key == eAGSKeyCodeCtrlE) && (display_fps == kFPS_Forced))
    {
        // if --fps parameter is used, Ctrl+E will toggle maxed out frame rate
        setTimerFps(frames_per_second, !isTimerFpsMaxed());
        return false;
    }

    // FIXME: the info is incomplete in a game with many objects or characters (at most 8 objects are listed;
    // the characters list is split into message boxes by a fixed text length), which makes it inconvenient
    if ((ags_combo_key == eAGSKeyCodeCtrlD) && (play.debug_mode > 0))
    {
        // ctrl+D - show info
        String buffer = String::FromFormat("In room %d %s\nPlayer at %d, %d (view %d, loop %d, frame %d)%s%s%s",
                                           displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""),
                                           playerchar->x, playerchar->y, playerchar->view + 1, playerchar->loop,
                                           playerchar->frame, (IsGamePaused() == 0) ? "" : "\nGame paused.",
                                           (play.ground_level_areas_disabled == 0) ? "" : "\nGround areas disabled.",
                                           (IsInterfaceEnabled() == 0) ? "\nGame in Wait state" : "");
        for (uint32_t obj_id = 0; obj_id < croom->numobj; obj_id++)
        {
            if (obj_id >= 8)
                break; // TODO: measure the graphical size of the text instead of counting objects
            buffer.AppendFmt(
                "\nObject %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%s slot:%d opacity:%d clkble:%d", obj_id,
                objs[obj_id].x, objs[obj_id].y,
                (spriteset.DoesSpriteExist(objs[obj_id].num) ? game.SpriteInfos[objs[obj_id].num].Width : 0),
                (spriteset.DoesSpriteExist(objs[obj_id].num) ? game.SpriteInfos[objs[obj_id].num].Height : 0),
                objs[obj_id].is_enabled(), objs[obj_id].is_moving() ? "yes" : "no",
                objs[obj_id].is_animating() ? "yes" : "no", objs[obj_id].num, objs[obj_id].opacity,
                ((objs[obj_id].flags & OBJF_NOINTERACT) != 0) ? 0 : 1);
        }
        DisplayMB(buffer.GetCStr());
        int listed_char_id = game.playercharacter;
        buffer = "CHARACTERS IN THIS ROOM:\n";
        for (int char_id = 0; char_id < game.numcharacters; char_id++)
        {
            if (game.chars[char_id].room != displayed_room)
                continue;
            if (buffer.GetLength() > 430)
            { // TODO: the limit is a text length in characters; measure the graphical size instead
                buffer.Append("and more...");
                DisplayMB(buffer.GetCStr());
                buffer = "CHARACTERS IN THIS ROOM (cont'd):\n";
            }
            listed_char_id = char_id;
            buffer.AppendFmt("%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%s anim:%s "
                             "follow:%d flags:%X wait:%d zoom:%d)\n",
                             game.chars[listed_char_id].scrname.GetCStr(), game.chars[listed_char_id].view + 1,
                             game.chars[listed_char_id].loop, game.chars[listed_char_id].frame,
                             game.chars[listed_char_id].x, game.chars[listed_char_id].y, game.chars[listed_char_id].z,
                             game.chars[listed_char_id].idleview, game.chars[listed_char_id].idledelay,
                             game.chars[listed_char_id].idleleft, game.chars[listed_char_id].is_moving() ? "yes" : "no",
                             charextra[listed_char_id].IsAnimating() ? "yes" : "no",
                             charextra[listed_char_id].following, game.chars[listed_char_id].flags,
                             game.chars[listed_char_id].wait, charextra[listed_char_id].zoom);
        }
        DisplayMB(buffer.GetCStr());
        return false;
    }

    if (((ags_combo_key == eAGSKeyCodeCtrlV) && (ki.Mod & eAGSModAlt) != 0) && (play.wait_counter < 1) &&
        (play.text_overlay_on == 0) && (!restrict_until))
    {
        // make sure we can't interrupt a Wait()
        // and desync the music to cutscene
        play.debug_mode++;
        script_debug(1, 0);
        play.debug_mode--;
        return false;
    }

    // No service operation triggered? return active keypress and mods to caller
    out_key = ki;
    return true;
}

// Runs default keyboard handling
static void check_keyboard_controls()
{
    // First check for service engine's combinations (mouse lock, display mode switch, and so forth)
    KeyInput ki;
    if (!run_service_key_controls(ki))
    {
        return;
    }

    // AGS script key will depend on the current "key handling mode";
    // in case of mod+key combinations it will be either individual key (new mode)
    // or combo-key code (old mode).
    const eAGSKeyCode ags_key = AGSKeyToScriptKey(ki.Key);
    // Use backward-compatible combined key for special controls,
    // because game variables may store old-style key + mod codes
    const eAGSKeyCode ags_combo_key = AGSKeyToScriptKey(ki.CompatKey);

    // Suggest a key to the plugins
    if (pl_run_plugin_hooks(kPluginEvt_KeyPress, ags_key))
    {
        debug_script_log("Keypress code %d claimed by plugin", ags_key);
        return;
    }

    // Cutscene skip
    check_skip_cutscene_keypress(ags_combo_key);
    if (play.fast_forward)
    {
        return;
    }
    // Cancel if temporarily ignoring input (cutscene skipping was already checked above)
    if (play.IsIgnoringInput())
    {
        return;
    }

    // Skip speech if instructed by Speech.SkipStyle
    if ((play.text_overlay_on > 0) && (play.speech_skip_style & SKIP_KEYPRESS) && !IsAGSServiceKey(ki.Key))
    {
        // only allow a key to remove the overlay if the icon bar isn't up
        if (IsGamePaused() == 0)
        {
            // check if it requires a specific keypress
            if ((play.skip_speech_specific_key == 0) || (ags_combo_key == play.skip_speech_specific_key))
            {
                remove_screen_overlay(play.text_overlay_on);
                play.SetWaitKeySkip(ki);
            }
        }
        return;
    }

    // Skip Wait state
    if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_KEYPRESS) && !IsAGSServiceKey(ki.Key))
    {
        play.SetWaitKeySkip(ki);
        return;
    }

    // Don't queue up another keypress if it can't be run instantly
    if (is_inside_script())
    {
        debug_script_log("Keypress %d ignored (game blocked)", ags_key);
        return;
    }

    bool key_was_processed = false;
    // First pass the key/text events to any active Text Box control
    if (GUI::IsEnabledState())
    {
        GUITextBox *textbox = gui_get_active_textbox();
        if (textbox)
        {
            const bool handled = textbox->OnKeyPress(ki);
            switch (game.options[OPT_TEXTBOXCLAIMSKEYS])
            {
            case kScTextBoxClaim_Handled: key_was_processed |= handled; break;
            case kScTextBoxClaim_TextOnly: key_was_processed |= (ki.UChar > 0); break;
            case kScTextBoxClaim_Classic:
            default:
                key_was_processed = (ki.UChar > 0) || ((ags_key >= 32) && (ags_key <= 255)) ||
                                    (ags_key == eAGSKeyCodeReturn) || (ags_key == eAGSKeyCodeBackspace);
                break;
            }

            if (textbox->IsActivated())
            {
                textbox->SetActivated(false);
                // TODO: the "mouse button" argument is abused here: 1 (kMouseLeft) is passed, though no mouse
                // button was used
                setevent(AGSEvent_GUI(textbox->GetParentID(), textbox->GetID(), static_cast<eAGSMouseButton>(1)));
            }
        }
    }

    if (key_was_processed)
        return;

    // Built-in key-presses
    if ((usetup.Override.KeySaveGame > 0) && (ags_combo_key == usetup.Override.KeySaveGame))
    {
        do_save_game_dialog(0, TOP_SAVESLOT - 1, play.normal_font); // ignore special slot 999
        return;
    }
    else if ((usetup.Override.KeyRestoreGame > 0) && (ags_combo_key == usetup.Override.KeyRestoreGame))
    {
        do_restore_game_dialog(0, TOP_SAVESLOT - 1, play.normal_font); // ignore special slot 999
        return;
    }
    else if ((usetup.Override.KeyDebugWalkable > 0) && (ags_combo_key == usetup.Override.KeyDebugWalkable))
    {
        script_debug(2, kRoomAreaWalkable);
        return;
    }
    else if ((usetup.Override.KeyDebugPathfind > 0) && (ags_combo_key == usetup.Override.KeyDebugPathfind))
    {
        script_debug(5, playerchar->index_id);
        return;
    }

    // Pass the key event to the script.
    if (ki.UChar == 0)
    {
        const int ags_key_mod = ki.Mod;
        debug_script_log("Running on_key_press keycode %d, mod %d", ags_key, ags_key_mod);
        setevent(AGSEvent_Script(kTS_KeyPress, ags_key, ags_key_mod));
    }
    if (ki.UChar > 0)
    {
        debug_script_log("Running on_text_input char %s (%d)", ki.Text, ki.UChar);
        setevent(AGSEvent_Script(kTS_TextInput, ki.UChar));
    }
}

bool run_service_gamepad_controls(GamepadInput &out_key)
{
    out_key.JoystickID = -1;
    out_key.Type = eAGSGamepad_InputTypeNone;
    out_key.Button = eAGSGamepad_ButtonInvalid;
    out_key.Axis = eAGSGamepad_AxisInvalid;

    if (ags_inputevent_ready() != kInputGamepad)
        return false; // there was no gamepad event

    const auto gp_evt = ags_get_next_inputevent();
    switch (gp_evt.type)
    {
    case SDL_CONTROLLERBUTTONDOWN:
        out_key.JoystickID = gp_evt.cbutton.which;
        out_key.Type = eAGSGamepad_InputTypeButton;
        out_key.Button = Gamepad_Button_SDLtoAGS(static_cast<SDL_GameControllerButton>(gp_evt.cbutton.button));
        break;
    }
    return out_key.Button != eAGSGamepad_ButtonInvalid;
}

// Runs default gamepad handling
static void check_gamepad_controls()
{
    // Read the gamepad button press
    GamepadInput gi;
    if (!run_service_gamepad_controls(gi))
    {
        return;
    }
    eAGSGamepad_Button gamepad_button = gi.Button;
    // Then, check cutscene skip
    check_skip_cutscene_gamepad(gamepad_button);
    if (play.fast_forward)
    {
        return;
    }
    if (play.IsIgnoringInput())
    {
        return;
    }

    // skip speech if desired by Speech.SkipStyle
    if ((play.text_overlay_on > 0) && (play.speech_skip_style & SKIP_KEYPRESS))
    {
        // only allow a key to remove the overlay if the icon bar isn't up
        if (IsGamePaused() == 0)
        {
            remove_screen_overlay(play.text_overlay_on);
            play.SetWaitSkipResult(SKIP_GAMEPAD, gamepad_button);
        }

        return;
    }

    if ((play.wait_counter != 0) && (play.key_skip_wait & SKIP_KEYPRESS) != 0)
    {
        play.SetWaitSkipResult(SKIP_GAMEPAD, gamepad_button);
        return;
    }

    if (is_inside_script())
    {
        // Don't queue up another button press if it can't be run instantly
        debug_script_log("Gamepad button %d ignored (game blocked)", gamepad_button);
        return;
    }
}

bool run_service_touch_controls(TouchInput &out_touch)
{
    out_touch = {}; // clear the output
    if (ags_inputevent_ready() != kInputTouch)
        return false; // there was no touch event

    const SDL_Event t_evt = ags_get_next_inputevent();
    if (t_evt.type == AGS_SDL_EVT_TOUCHDOWN || t_evt.type == AGS_SDL_EVT_TOUCHUP ||
        t_evt.type == AGS_SDL_EVT_TOUCHMOTION)
    {
        const AGS_TouchPointerEventData *data = static_cast<AGS_TouchPointerEventData *>(t_evt.user.data1);
        out_touch.PointerID = data->PointerID;
        out_touch.Position = Mouse::SysToGamePos(data->Position);
        switch (t_evt.type)
        {
        case AGS_SDL_EVT_TOUCHDOWN: out_touch.Phase = TouchPhase::Down; break;
        case AGS_SDL_EVT_TOUCHUP: out_touch.Phase = TouchPhase::Up; break;
        case AGS_SDL_EVT_TOUCHMOTION: out_touch.Phase = TouchPhase::Motion; break;
        default: assert(false); break; // must not happen
        }
        ags_dispose_userevent(t_evt);
    }
    return out_touch.PointerID >= 0;
}

static void check_touch_controls()
{
    TouchInput ti;
    if (!run_service_touch_controls(ti))
        return;

    // TODO: check cutscene skip
    if (play.fast_forward)
        return;
    if (play.IsIgnoringInput())
        return;

    // Run script callbacks
    switch (ti.Phase)
    {
    case TouchPhase::Down:
        debug_script_log("Running on_pointer_down pointer id %d, pos %d,%d", ti.PointerID, ti.Position.X,
                         ti.Position.Y);
        setevent(AGSEvent_Script(kTS_PointerDown, ti.PointerID, ti.Position.X, ti.Position.Y));
        break;
    case TouchPhase::Up:
        debug_script_log("Running on_pointer_up pointer id %d, pos %d,%d", ti.PointerID, ti.Position.X, ti.Position.Y);
        setevent(AGSEvent_Script(kTS_PointerUp, ti.PointerID, ti.Position.X, ti.Position.Y));
        break;
    case TouchPhase::Motion:
        debug_script_log("Running on_pointer_move pointer id %d, pos %d,%d", ti.PointerID, ti.Position.X,
                         ti.Position.Y);
        setevent(AGSEvent_Script(kTS_PointerMove, ti.PointerID, ti.Position.X, ti.Position.Y));
        break;
    default: break;
    }
}

// Process player input
static void GameUpdateCheckControls()
{
    // don't let the player do anything before the screen fades in
    if (in_new_room != kEnterRoom_None)
        return;

    set_our_eip(1007);

    sys_evt_process_pending();

    // First handle mouse state, which does not depend on down/up events
    // (motion, wheel axis, etc)
    // FIXME: atm we must save the last mouse_on_iface value *locally* for use
    // further in check_mouse_controls, because there may be 1+ nested
    // check_controls() calls as a result of some triggered script callbacks,
    // during which some global vars like mouse_on_iface may change...
    // need to rewrite all this interface interaction ugliness!
    int was_mouse_on_iface;
    check_mouse_state(was_mouse_on_iface); // NOTE: this also polls mousewheel

    // Handle all the buffered input events
    for (InputType type = kInputNone; ags_hasinputevent_ready(type);)
    {
        switch (type)
        {
        case kInputKeyboard: check_keyboard_controls(); break;
        case kInputMouse: check_mouse_controls(was_mouse_on_iface); break;
        case kInputGamepad: check_gamepad_controls(); break;
        case kInputTouch: check_touch_controls(); break;
        default: ags_drop_next_inputevent(); break;
        }
    }
}

// Check room edges triggers
static void GameUpdateCheckRoomEdges()
{
    if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) &&
        ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0) && (in_new_room == kEnterRoom_None) &&
        (new_room_was == kEnterRoom_None))
    {
        // Only allow walking off edges if not in wait mode, and
        // if not in Player Enters Screen (allow walking in from off-screen)
        int edges_activated[4] = {0, 0, 0, 0};
        if (playerchar->x <= thisroom.Edges.Left)
            edges_activated[0] = 1;
        else if (playerchar->x >= thisroom.Edges.Right)
            edges_activated[1] = 1;
        if (playerchar->y >= thisroom.Edges.Bottom)
            edges_activated[2] = 1;
        else if (playerchar->y <= thisroom.Edges.Top)
            edges_activated[3] = 1;

        if ((play.entered_edge >= 0) && (play.entered_edge <= 3))
        {
            // once the player is no longer outside the edge, forget the stored edge
            if (edges_activated[play.entered_edge] == 0)
                play.entered_edge = -10;
            // if we are walking in from off-screen, don't activate edges
            else
                edges_activated[play.entered_edge] = 0;
        }

        for (int edge = 0; edge < 4; edge++)
        {
            if (edges_activated[edge])
                setevent(AGSEvent_Object(kObjEventType_Room, 0, edge));
        }
    }
}

static void game_loop_update_background_animation()
{
    if (play.bg_anim_delay > 0)
        play.bg_anim_delay--;
    else if (play.bg_frame_locked)
        ;
    else
    {
        play.bg_anim_delay = play.anim_background_speed;
        play.bg_frame++;
        if ((size_t)play.bg_frame >= thisroom.BgFrameCount)
            play.bg_frame = 0;
        if (thisroom.BgFrameCount >= 2)
        {
            // get the new frame's palette
            on_background_frame_change();
        }
    }
}

static void game_loop_update_game_counters()
{
    if (play.wait_counter > 0)
        play.wait_counter--;
    if (play.shakesc_length > 0)
        play.shakesc_length--;
}

static void GameUpdateGameState()
{
    if ((debug_flags & DBG_NOUPDATE) == 0)
    {
        if (game_paused == 0)
        {
            update_game_objects();
            game_loop_update_background_animation();
        }
    }

    // Following global counters are updated during game pause too
    game_loop_update_game_counters();
}

// Updates objects that do not auto-pause when the game is paused
static void GameUpdatePersistentAnimations()
{
    // Update animating GUI buttons and overlays
    // The buttons are animated even if the game is paused
    for (size_t i = 0; i < GetAnimatingButtonCount(); ++i)
    {
        if (!UpdateAnimatingButton(i))
        {
            StopButtonAnimation(i);
            i--;
        }
    }

    UpdateOverlayAnimations();
}

extern std::vector<ViewStruct> views;

static void update_objects_scale()
{
    if (displayed_room < 0)
        return;

    for (uint32_t obj_id = 0; obj_id < croom->numobj; ++obj_id)
    {
        update_object_scale(obj_id);
    }

    for (int char_id = 0; char_id < game.numcharacters; ++char_id)
    {
        update_character_scale(char_id);
    }
}

static void update_overlay_positions()
{
    auto &overs = get_overlays();
    for (auto &over : overs)
    {
        if (over.IsAutoPosition())
        {
            autoposition_overlay(over);
        }
    }
}

// Updates GUI reaction to the cursor position change
// TODO: possibly may be merged with gui_on_mouse_move()
static void update_cursor_over_gui()
{
    if (((debug_flags & DBG_NOIFACE) != 0) || (displayed_room < 0))
        return; // GUI is disabled (debug flag) or room is not loaded
    if (!IsInterfaceEnabled() || !play.mouse_input_enabled)
        return; // interface is disabled (by script or blocking action)
    // Poll guis
    int focused_gui = GetGUIAt(mousex, mousey, kHit_Interactable);
    for (auto &gui : guis)
    {
        if (!gui.IsDisplayed())
            continue; // not on screen
        if (!gui.IsClickable())
            continue; // don't update non-clickable
        gui.Poll(mousex, mousey, focused_gui == gui.GetID());
    }
}

extern int lastmx, lastmy;
extern int mouse_frame, mouse_delay;

static void update_cursor_view()
{
    // update animating mouse cursor
    // NOTE: we are still doing this even if the in-game mouse input is disabled,
    // or mouse cursor is hidden, the animation logic keeps running in the background.
    const auto &mcur = game.mcurs[cur_cursor];
    if (mcur.view >= 0 && mcur.view < game.numviews)
    {
        // only on mousemove, and it's not moving
        if (((mcur.flags & MCF_ANIMMOVE) != 0) && (mousex == lastmx) && (mousey == lastmy))
            ;
        // only on hotspot, and it's not on one
        else if (((mcur.flags & MCF_HOTSPOT) != 0) && (GetLocationType(mousex, mousey, kHit_Interactable) == 0))
            set_new_cursor_graphic(mcur.pic);
        else if (mouse_delay > 0)
            mouse_delay--;
        // only animate if the loop 0 exists and has frames
        else if (views[mcur.view].numLoops > 0 && views[mcur.view].loops[0].numFrames > 0)
        {
            const int viewnum = mcur.view;
            const int loopnum = 0;
            mouse_frame++;
            if (mouse_frame >= views[viewnum].loops[loopnum].numFrames)
                mouse_frame = 0;
            set_new_cursor_graphic(views[viewnum].loops[loopnum].frames[mouse_frame].pic);
            mouse_delay = views[viewnum].loops[loopnum].frames[mouse_frame].speed + mcur.animdelay;
            CheckViewFrame(viewnum, loopnum, mouse_frame);
        }
        lastmx = mousex;
        lastmy = mousey;
    }
}

// Update the "saved cursor until it leaves location" state
static void UpdateSavedCursorOverLocation()
{
    if (!play.mouse_input_enabled)
        return;

    // Call GetLocationName - it will internally force a GUI refresh
    // if the result it returns has changed from last time
    // The comparison below needs this: GetLocationName() is what updates get_loc_name_last_time. UpdateGUIContext()
    // calls it again, but at the end of the tick, after the input and the script events.
    GetLocationName(mousex, mousey, kHit_Interactable);

    if ((play.get_loc_name_save_cursor.IsDefined()) && (play.get_loc_name_save_cursor != play.get_loc_name_last_time) &&
        (mouse_on_iface < 0) && (ifacepopped < 0))
    {
        // we have saved the cursor, but the mouse location has changed
        // and it's time to restore it
        play.get_loc_name_save_cursor = {};
        set_cursor_mode(play.restore_cursor_mode_to);

        if (cur_mode == play.restore_cursor_mode_to)
        {
            // make sure it changed -- the new mode might have been disabled
            // in which case don't change the image
            set_cursor_look(play.restore_cursor_image_to);
        }
        debug_script_log("Restore mouse to mode %d cursor %d", play.restore_cursor_mode_to,
                         play.restore_cursor_image_to);
    }
}

// Assign GUI context parameters, read from the game state.
// This will be required for GUI labels render.
static void UpdateGUIContext()
{
    if (play.fast_forward)
        return;
    if (displayed_room < 0)
        return;

    // It's easier to update these every game tick, this is simple data and copy is fast,
    // OTOH it may be inconvenient to have this in every place where this data changes.
    GUI::Context.GameTitle = play.game_name;

    if (playerchar->activeinv < 0 || playerchar->activeinv >= game.numinvitems)
        GUI::Context.InventoryPic = -1;
    else
        GUI::Context.InventoryPic = game.invinfo[playerchar->activeinv].pic;

    // Update a location name for the GUI labels. We do this every game
    // tick, because this does not depend only on cursor position, but also
    // on game object positions, and game state changes, and we cannot track all of that here.
    //
    // While game is in Wait mode, or in room transition: set empty overhotspot text.
    if (!IsInterfaceEnabled() || in_room_transition || !play.mouse_input_enabled)
        GUI::Context.Overhotspot = "";
    else
        GUI::Context.Overhotspot = GetLocationName(mousex, mousey, kHit_Interactable);
}

// Detect mouse move over hotspot, and run respective event if necessary
static void update_cursor_over_location(int mouse_was_x, int mouse_was_y)
{
    if (play.fast_forward)
        return;
    if (displayed_room < 0 || !play.mouse_input_enabled)
        return;

    auto view = play.GetRoomViewportAt(mousex, mousey);
    auto cam = view ? view->GetCamera() : nullptr;
    if (!cam)
        return;

    // NOTE: all cameras are in same room right now, so their positions are in same coordinate system;
    // therefore we may use this as an indication that mouse is over different camera too.
    // TODO: do not use static variables!
    // TODO: if we support rotation then we also need to compare full transform!
    static int offsetxWas = -1000, offsetyWas = -1000;
    int offset_x = cam->GetRect().Left;
    int offset_y = cam->GetRect().Top;

    if (((mouse_was_x != mousex) || (mouse_was_y != mousey) || (offsetxWas != offset_x) || (offsetyWas != offset_y)))
    {
        // mouse moves over hotspot
        int getloctype_index = -1;
        if (GetLocationTypeImpl(&getloctype_index, mousex, mousey, kHit_Interactable,
                                false /* dont click-through gui */, true /* allow hotspot0 */) == LOCTYPE_HOTSPOT)
        {
            setevent(AGSEvent_Object(kObjEventType_Hotspot, getloctype_index, kHotspotEvent_MouseOver));
        }
    }

    offsetxWas = offset_x;
    offsetyWas = offset_y;
}

static void UpdateDrawableObjectStates(bool do_cursor, int mouse_was_x, int mouse_was_y)
{
    if (displayed_room < 0)
        return;

    // camera positions may be linked to a player character
    play.UpdateRoomCameras();

    update_objects_scale();
    // overlay positions may be linked to a certain character
    update_overlay_positions();
    if (do_cursor)
    {
        update_cursor_over_location(mouse_was_x, mouse_was_y);
        update_cursor_view();
    }
}

// Process all events scheduled during the last game update
static void GameUpdateProcessEvents()
{
    new_room_was = in_new_room;
    // If we're in the new room (after "room load" event), then queue "fade in" event,
    // it will be processed right away
    if (in_new_room != kEnterRoom_None)
    {
        setevent({kAGSEvent_FadeIn});
        in_new_room = kEnterRoom_None;
    }

    processallevents();

    // If in a new room, and the room wasn't just changed again in update_events,
    // then queue the Enters Screen scripts run after the room is faded in.
    if ((new_room_was != kEnterRoom_None) && (in_new_room == kEnterRoom_None))
    {
        // After-Fade-in event is run immediately at the same
        // game update right after the Fade-in (transition) event was run (transition).
        // This ensures that there's no extra updates or redraws in between.
        switch (new_room_was)
        {
        case kEnterRoom_FirstTime: // first time enters screen
            setevent(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_FirstEnter));
            /* fall-through */
        case kEnterRoom_Normal: // enters screen after fadein
            setevent(AGSEvent_Object(kObjEventType_Room, 0, kRoomEvent_AfterFadein));
            break;
        case kEnterRoom_RestoredSave:
            in_room_transition = false; // room transition ends here
            break;
        }

        processallevents();
    }
}

static void game_loop_update_loop_counter()
{
    GameTimer.LoopCounter++;
}

static void game_loop_update_fps()
{
    const auto now = Clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - GameTimer.LastFPSSyncTime);
    const auto frames = GameTimer.LoopCounter - GameTimer.LastFPSSyncCounter;

    if (duration >= std::chrono::milliseconds(GameTimer.FPSSyncPeriodMs) && frames > 0)
    {
        GameTimer.FPS = 1000.0f * frames / duration.count();
        GameTimer.LastFPSSyncTime = now;
        GameTimer.LastFPSSyncCounter = GameTimer.LoopCounter;
    }
}

float get_game_fps()
{
    // if we have maxed out framerate then return the frame rate we're seeing instead
    // fps must be greater that 0 or some timings will take forever.
    // TODO: review this, consider always returning fixed property here,
    // and use get_real_fps() whenever we need real time;
    // but must double check all situations where this function is used!
    if (isTimerFpsMaxed() && GameTimer.FPS > 0.0f)
    {
        return GameTimer.FPS;
    }
    return frames_per_second;
}

float get_real_fps()
{
    return GameTimer.FPS;
}

void set_loop_counter(uint32_t new_counter)
{
    GameTimer.LoopCounter = new_counter;
    GameTimer.LastFPSSyncCounter = GameTimer.LoopCounter;
    GameTimer.LastFPSSyncTime = Clock::now();
    GameTimer.FPS = std::numeric_limits<float>::quiet_NaN();
}

void increment_loop_counter()
{
    GameTimer.LoopCounter++;
    GameTimer.LastFPSSyncCounter = GameTimer.LoopCounter;
}

uint32_t get_loop_counter()
{
    return GameTimer.LoopCounter;
}

void set_runtime_start()
{
    GameTimer.StartTime = Clock::now();
    GameTimer.RuntimeStarted = true;
}

bool is_runtime_set()
{
    return GameTimer.RuntimeStarted;
}

uint32_t get_runtime_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - GameTimer.StartTime).count();
}

//
// UpdateGameOnce is a primary game update routine.
//
// Plan:
//  * System events (parse and record for further use down the routine).
//  * Check for exit request.
//  * Early bad state check
//  * New room check
//  * repeatedly_execute_always
//  * Game state general update
//  * Player input handling
//  * late_repeatedly_execute_always
//  * Process scheduled game events
//  * Sync video & audio, update drawable objects for render
//  * Rendering
//  * Loop counter increment
//  * Wait for next frame
//
void UpdateGameOnce(bool do_controls, AGS::Engine::IDriverDependantBitmap *extra_ddb, int extra_x, int extra_y)
{
    set_our_eip(1000);

    sys_evt_process_pending();

    if (want_exit)
    {
        ProperExit();
        return;
    }

    set_our_eip(1001);

    GameUpdateCheckProblems();

    set_our_eip(1002);

    // Check "breakpoint" key prior to running any script callbacks
    check_debug_keys();
    // Tell scripts that the game is alive
    ccNotifyScriptStillAlive();
    // Check if we just entered a new room, and run "before fade-in" script event
    check_new_room();

    set_our_eip(1003);

    // Run early rep-exec-always, and schedule rep-execs for later
    GameUpdateEarlyRepExec();

    set_our_eip(1004);

    // Do the overall game state update
    GameUpdateGameState();
    GameUpdatePersistentAnimations();
    // Check edges and ground interactions, after the game objects have updated their positions
    GameUpdateCheckRoomEdges();
    GameUpdateCheckGroundInteractions();

    set_our_eip(1005);

    // Handle player's input
    // remember old mouse pos, needed for update_cursor_over_location() later
    const int mouse_was_x = mousex, mouse_was_y = mousey;
    // TODO: move this reset to a related update function; check_mouse_state() sets it, unless mouse input is disabled
    mouse_on_iface = -1;
    // update mouse position (mousex, mousey)
    ags_domouse();
    // update gui under mouse; this also updates gui control focus;
    // atm we must call this before "check_controls", because GUI interaction
    // relies on remembering which control was focused by the cursor prior
    update_gui_disabled_status();
    update_cursor_over_gui();
    UpdateSavedCursorOverLocation();
    // handle actual input (keys, mouse, and so forth)
    if (do_controls)
    {
        GameUpdateCheckControls();
    }

    set_our_eip(1006);

    // Run late rep-exec-always, after all the game was updated
    GameUpdateLateRepExec();
    // Then process all the accumulated events for this game tick
    GameUpdateProcessEvents();

    set_our_eip(1007);

    // Sync video and audio with the game logic
    update_video_system_on_game_loop();
    update_audio_system_on_game_loop();
    // Update drawable object states, which depend on their positions,
    // room region properties, or other object positions
    UpdateDrawableObjectStates(true /* cursor-related update */, mouse_was_x, mouse_was_y);

    set_our_eip(1008);

    // Record necessary values for the GUI rendering
    UpdateGUIContext();
    // Only render if we are not skipping a cutscene
    if (!play.fast_forward)
    {
        update_gui_disabled_status(); // in case they changed it in the late script update
        render_graphics(extra_ddb, extra_x, extra_y);
    }

    set_our_eip(1009);

    game_loop_update_loop_counter();
    update_polled_stuff();
    game_loop_update_fps();

    set_our_eip(1010);

    // Immediately start the next frame if we are skipping a cutscene
    if (play.fast_forward)
        return;

    set_our_eip(1011);

    WaitForNextFrame();

    set_our_eip(1012);
}

void UpdateGameAudioOnly()
{
    update_audio_system_on_game_loop();
    game_loop_update_loop_counter();
    game_loop_update_fps();
    WaitForNextFrame();
}

bool IsInBlockingAction()
{
    return restrict_until != nullptr;
}

// Checks if wait mode should continue until condition is met
// TODO: should be a private method of GameLoopUntilState; it is only called by GameLoopUntilState::Run()
// (hence the declaration at the top of the file, the class being defined before this function)
static bool ShouldStayInWaitMode()
{
    assert(restrict_until);
    if (!restrict_until)
        return false;

    switch (restrict_until->GetUntilType())
    {
    case UNTIL_MOVEEND:
    {
        short *wkptr = (short *)restrict_until->GetDataPtr();
        return !(wkptr[0] < 1);
    }
    case UNTIL_CHARIS0:
    {
        char *chptr = (char *)restrict_until->GetDataPtr();
        return !(chptr[0] == 0);
    }
    case UNTIL_NEGATIVE:
    {
        short *wkptr = (short *)restrict_until->GetDataPtr();
        return !(wkptr[0] < 0);
    }
    case UNTIL_INTISNEG:
    {
        int *wkptr = (int *)restrict_until->GetDataPtr();
        return !(wkptr[0] < 0);
    }
    case UNTIL_NOOVERLAY:
    {
        return !(play.text_overlay_on == 0);
    }
    case UNTIL_INTIS0:
    {
        int *wkptr = (int *)restrict_until->GetDataPtr();
        return !(wkptr[0] == 0);
    }
    case UNTIL_SHORTIS0:
    {
        short *wkptr = (short *)restrict_until->GetDataPtr();
        return !(wkptr[0] == 0);
    }
    case UNTIL_ANIMBTNEND:
    { // still animating?
        return FindButtonAnimation(restrict_until->GetData1(), restrict_until->GetData2()) >= 0;
    }
    case UNTIL_FLAGUNSET:
    {
        const int *bitset = static_cast<const int *>(restrict_until->GetDataPtr());
        return ((*bitset) & restrict_until->GetData1()) != 0;
    }
    case UNTIL_VIEWANIM:
    {
        const ViewAnimateParams *anim = static_cast<const ViewAnimateParams *>(restrict_until->GetDataPtr());
        return anim->IsValid();
    }
    case UNTIL_ANIMOVEREND:
    {
        return IsOverlayAnimating(restrict_until->GetData1());
    }
    default: debug_script_warn("loop_until: unknown until event, aborting"); return false;
    }
}

// Run single game iteration; calls UpdateGameOnce() internally
static void GameTick()
{
    UpdateGameOnce(true);
}

// Runs game loops until the condition of the given UNTIL_* type is met;
// called by the GameLoopUntil*() functions, which are called from the game core, character, room object etc
static void GameLoopUntilEvent(int until_what, const void *data_ptr = nullptr, int data1 = 0, int data2 = 0)
{
    // blocking cutscene - end skipping
    EndSkippingUntilCharStops();

    // this function can get called in a nested context, so
    // remember the state of these vars in case a higher level
    // call needs them
    std::unique_ptr<GameLoopUntilState> cached_restrict_until = std::move(restrict_until);
    bool cached_restrict_until_in_script = restrict_until_in_script;

    restrict_until_in_script = is_inside_script();
    restrict_until = std::make_unique<GameLoopUntilState>(until_what, data_ptr, data1, data2);
    restrict_until->Begin();
    while (restrict_until->Run())
        ;
    restrict_until->End();

    set_our_eip(78);

    restrict_until = std::move(cached_restrict_until);
    restrict_until_in_script = cached_restrict_until_in_script;
}

void GameLoopUntilValueIsZero(const char *value)
{
    GameLoopUntilEvent(UNTIL_CHARIS0, value);
}

void GameLoopUntilValueIsZero(const short *value)
{
    GameLoopUntilEvent(UNTIL_SHORTIS0, value);
}

void GameLoopUntilValueIsZero(const int *value)
{
    GameLoopUntilEvent(UNTIL_INTIS0, value);
}

void GameLoopUntilValueIsNegative(const short *value)
{
    GameLoopUntilEvent(UNTIL_NEGATIVE, value);
}

void GameLoopUntilValueIsNegative(const int *value)
{
    GameLoopUntilEvent(UNTIL_INTISNEG, value);
}

void GameLoopUntilNotMoving(const short *move)
{
    GameLoopUntilEvent(UNTIL_MOVEEND, move);
}

void GameLoopUntilNoOverlay()
{
    GameLoopUntilEvent(UNTIL_NOOVERLAY);
}

void GameLoopUntilButAnimEnd(int gui_id, int btn_id)
{
    GameLoopUntilEvent(UNTIL_ANIMBTNEND, nullptr, gui_id, btn_id);
}

void GameLoopUntilOverlayAnimEnd(int over_id)
{
    GameLoopUntilEvent(UNTIL_ANIMOVEREND, nullptr, over_id);
}

void GameLoopUntilFlagUnset(const int *flagset, int flagbit)
{
    GameLoopUntilEvent(UNTIL_FLAGUNSET, flagset, flagbit);
}

void GameLoopUntilViewAnimEnd(const ViewAnimateParams *anim)
{
    GameLoopUntilEvent(UNTIL_VIEWANIM, anim);
}

extern unsigned int load_new_game;
void RunGameUntilAborted()
{
    // skip ticks to account for time spent starting game.
    skipMissedTicks();

    while (!abort_engine)
    {
        GameTick();

        if (load_new_game)
        {
            RunAGSGame(nullptr, load_new_game, 0);
            load_new_game = 0;
        }
    }
}

void UpdateCursorAndDrawables()
{
    const int mouse_was_x = mousex, mouse_was_y = mousey;
    ags_domouse();
    update_cursor_over_gui();
    // TODO: following does not have to be called every frame while in a
    // fully blocking state (like Display() func), refactor to only call it
    // once the blocking state begins.
    UpdateDrawableObjectStates(true /* cursor-related update */, mouse_was_x, mouse_was_y);
}

void SyncDrawablesState()
{
    UpdateDrawableObjectStates(false /* NO cursor-related update */, -1, -1);
}

void ShutGameWaitState()
{
    restrict_until = {};
    restrict_until_in_script = false;
}

void update_polled_stuff()
{
    if (want_exit)
    {
        want_exit = false;
        quit("||exit!");
    }

    if (editor_debugging_initialized)
        check_for_messages_from_debugger();
}
