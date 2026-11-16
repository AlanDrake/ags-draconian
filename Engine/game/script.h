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

#include <memory>
#include <vector>

#include "game/runtime_defines.h"
#include "game/scripteventtable.h"
#include "script/executingscript.h"
#include "script/runtimescript.h"
#include "script/scriptexecutor.h"
#include "util/string.h"

using AGS::Common::HError;
using AGS::Common::String;
using AGS::Common::ScriptEventsBase;

#define LATE_REP_EXEC_ALWAYS_NAME "late_repeatedly_execute_always"
#define REP_EXEC_ALWAYS_NAME      "repeatedly_execute_always"
#define REP_EXEC_NAME             "repeatedly_execute"

// ObjectEvent - a struct holds data of the object's interaction event,
// such as object's reference and accompanying parameters
struct ObjectEvent
{
    // Script type (i.e. game or room);
    // NOTE: kScTypeGame also may refer to "all modules", not only "globalscript"
    ScriptType ScType = kScTypeNone;
    // Interacted object type, defined as a LOCTYPE_*;
    // this is used for running "unhandled event"
    int ObjectTypeID = LOCTYPE_NOTHING;
    // Interacted object's ID
    int ObjectID = 0;
    // Event parameters
    size_t ParamCount = 0u;
    RuntimeScriptValue Params[MAX_SCRIPT_EVT_PARAMS];

    ObjectEvent() = default;
    // An event without additional parameters
    ObjectEvent(ScriptType sc_type) : ScType(sc_type) {}
    // An interaction event without additional parameters
    ObjectEvent(ScriptType sc_type, int object_type_id, int obj_id)
        : ScType(sc_type),
          ObjectTypeID(object_type_id),
          ObjectID(obj_id)
    {
    }
    // An event with a dynamic object reference
    ObjectEvent(ScriptType sc_type, const RuntimeScriptValue &dyn_obj) : ScType(sc_type)
    {
        ParamCount = 1u;
        Params[0] = dyn_obj;
    }
    // An interaction event with a dynamic object reference
    ObjectEvent(ScriptType sc_type, int object_type_id, int obj_id, const RuntimeScriptValue &dyn_obj)
        : ScType(sc_type),
          ObjectTypeID(object_type_id),
          ObjectID(obj_id)
    {
        ParamCount = 1u;
        Params[0] = dyn_obj;
    }
    // An event with a dynamic object reference and interaction mode
    ObjectEvent(ScriptType sc_type, int object_type_id, int obj_id, const RuntimeScriptValue &dyn_obj, int mode)
        : ScType(sc_type),
          ObjectTypeID(object_type_id),
          ObjectID(obj_id)
    {
        ParamCount = 2u;
        Params[0] = dyn_obj;
        Params[1] = RuntimeScriptValue().SetInt32(mode);
    }

    bool IsValid() const { return ScType != kScTypeNone; }
    operator bool() const { return IsValid(); }
};

// NonBlockingScriptFunction struct contains a script callback that is run on the non-blocking thread,
// and the cached information on which scripts have it: a script is tried until it turns out not to have it.
struct NonBlockingScriptFunction
{
    String FunctionName;
    size_t ParamCount = 0u;
    RuntimeScriptValue Params[MAX_SCRIPT_EVT_PARAMS];
    bool RoomHasFunction;
    bool GlobalScriptHasFunction;
    std::vector<bool> ModuleHasFunction;
    bool AtLeastOneImplementationExists;

    NonBlockingScriptFunction(const String &fn_name, int param_count)
    {
        FunctionName = fn_name;
        ParamCount = param_count;
        AtLeastOneImplementationExists = false;
        RoomHasFunction = true;
        GlobalScriptHasFunction = true;
    }
};

// Runs a script function on a non-blocking script thread, in every script that has it
// (script modules, the global script, the room script); stops as soon as the room changes
void run_function_on_non_blocking_thread(NonBlockingScriptFunction *func_to_run);
// Runs new-style interaction event handler in script.
// If there is no enabled handler for 'event_id', runs nothing and returns 0: when 'any_handlers' has an
// enabled one for 'any_event_id' the caller is expected to run it (the "Any Click" case), otherwise
// the "unhandled_event" callback is run if 'do_unhandled_event' is set.
// Returns -1 if the handler was found while game.Play.check_interaction_only is set (it does not run then),
// or if the room changed while it ran; 0 otherwise.
int run_event_script(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int event_id,
                     ScriptEventsBase *any_handlers, int any_event_id, bool do_unhandled_event);
// The same, with no "Any Click" handlers to fall back to
int run_event_script(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int event_id,
                     bool do_unhandled_event = false);
// Runs the handler for 'event_id' if there is an enabled one: unlike run_event_script, it does not look
// at game.Play.check_interaction_only, and has no "Any Click" or "unhandled_event" fallback.
// Returns -1 if the room changed while it ran, 0 otherwise.
int run_event_script_always(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int event_id);
// Runs the game's "unhandled_event" callback for an interaction that had no handler, giving it the location
// type (hotspot 0 counts as "no location") and the interaction mode. Does nothing if interactions are only
// being checked, or for the walk mode.
void run_unhandled_event(const ObjectEvent &obj_evt, int mode);

enum RunScFuncResult
{
    kScFnRes_Done = 0,
    kScFnRes_GenericInstError = -1, // running instance generic failure
    kScFnRes_NotFound = -2,         // function not found
    kScFnRes_ScriptBusy = -3,       // script is already being executed
};

// Returns the room script for the room type; otherwise the module with the given name, or the global
// script if the name is empty or its own. Returns null if there is no such module (or no room script).
AGS::Engine::RuntimeScript *GetScriptInstance(ScriptType sc_type, const String &script_module = {});
// Tests if a function exists in the given script module
bool DoesScriptFunctionExist(const AGS::Engine::RuntimeScript *script, const String &fn_name);
// Tests if a function exists in any of the regular script modules or the global script, *except* room script
bool DoesScriptFunctionExistInModules(const String &fn_name);
// Tests if a function exists in a particular script module (except room script);
// an empty module name means the global script
bool DoesScriptFunctionExistInModule(const String &script_module, const String &fn_name);
// Tries to execute a script handler, which is a script function associated with some event.
// The function is identified by its name and script module, and will be run by RunScriptFunctionAuto().
// If the handler was disabled for any reason, then skips execution and returns immediately.
void TryRunScriptHandler(ScriptType sc_type, const AGS::Common::String &script_module,
                         const AGS::Common::ScriptEventHandler &handler, size_t param_count = 0,
                         const RuntimeScriptValue *params = nullptr);
// Runs a function in the script on the main script thread, or on a new callback thread if the main one is
// busy, then performs the post-script actions. Returns kScFnRes_NotFound if there is no such function,
// kScFnRes_Done if it ran (even if aborted); other results quit the game.
RunScFuncResult RunScriptFunction(const AGS::Engine::RuntimeScript *script, const String &fn_name,
                                  size_t param_count = 0, const RuntimeScriptValue *params = nullptr);
// Run a script function in all the regular script modules, in order, where available;
// includes globalscript (last), but not the current room script.
// Returns if at least one instance of a function was run successfully.
bool RunScriptFunctionInModules(const String &fn_name, size_t param_count = 0,
                                const RuntimeScriptValue *params = nullptr);
// Run a script function in the current room script; returns if a function was run successfully
// (false if no room is loaded).
bool RunScriptFunctionInRoom(const String &fn_name, size_t param_count = 0, const RuntimeScriptValue *params = nullptr);
// Try to run a script function, guessing the behavior by its name and script instance type;
// depending on the type may run a claimable callback chain (room script, modules, global script, stopping
// at the first one that claims the event), or run it in all the modules (repeatedly_execute).
// Returns if the function was run successfully in the last script it was tried in (for a claimable
// event, true if some script claimed it). For the room type runs it in the room script only.
bool RunScriptFunctionAuto(ScriptType sc_type, const ScriptFunctionRef &fn_ref, size_t param_count = 0,
                           const RuntimeScriptValue *params = nullptr);
// Same as above, but the function is identified by the function name only,
// uses default script module for the given "script type".
bool RunScriptFunctionAuto(ScriptType sc_type, const String &fn_name, size_t param_count = 0,
                           const RuntimeScriptValue *params = nullptr);

// Allocates script executor and standard threads (main and non-blocking), and sets the script alive timer
void InitScriptExec();
// Frees script executor and all threads
void ShutdownScriptExec();
// Creates the runtime instances of the global script, the modules and the (optional) dialog script
// from the compiled ones; the module names are used in error messages only
// TODO: pick out a struct for holding only game scripts
HError CreateRuntimeScripts(const ccScript *global_script, const std::vector<ccScript *> &script_modules,
                            const std::vector<String> &script_module_names, const ccScript *dialog_script);
// Link all script modules into a single program,
// registers script exports in a global symbol table
HError LinkGlobalScripts();
// Cancel any running scripts
void AbortAllScripts();
// Unlinks scripts, unregister script exports
void UnlinkAllScripts();
// Delete only the current room script instance
void FreeRoomScript();
// Deletes all the global scripts and modules;
// this frees all of the bytecode and runtime script memory.
void FreeAllScripts();

//=============================================================================

// Quits the game reporting the current script error and the name of the function that was running;
// the call stack is added unless it is a user error (the "!" kind)
void quit_with_script_error(const String &fn_name);
// Quits the game with an error if a script function is currently run on the non-blocking thread
// (such as repeatedly_execute_always), where the commands that block are not allowed
void can_run_delayed_command();
// Tells whether no script function is currently run on the non-blocking thread (see can_run_delayed_command)
bool get_can_run_delayed_command();

// Gets current running script position; returns false if there is no script executor
bool get_script_position(AGS::Engine::ScriptPosition &script_pos);
// Formats the call stack of the running script, at most 'max_lines' lines; empty if there is no executor
String cc_get_callstack(int max_lines = INT_MAX);

// Tells if there's a script running right now
bool is_inside_script();
// Gets current ExecutingScript object (null if no script function is running)
ExecutingScript *get_executingscript();

// The game's scripts and the script callbacks that run on the non-blocking thread (Game::Scripts)
struct ScriptState
{
    AGS::Engine::PRuntimeScript Global;
    AGS::Engine::PRuntimeScript Dialog;
    std::vector<AGS::Engine::PRuntimeScript> Modules;
    std::vector<RuntimeScriptValue> ModuleRepExecAddr;
    size_t NumModules = 0;

    // The script callbacks that run on the non-blocking thread
    NonBlockingScriptFunction RepExecAlways{REP_EXEC_ALWAYS_NAME, 0};
    NonBlockingScriptFunction LateRepExecAlways{LATE_REP_EXEC_ALWAYS_NAME, 0};
    NonBlockingScriptFunction GetDialogOptionsDimensions{"dialog_options_get_dimensions", 1};
    NonBlockingScriptFunction RenderDialogOptions{"dialog_options_render", 1};
    NonBlockingScriptFunction GetDialogOptionUnderCursor{"dialog_options_get_active", 1};
    NonBlockingScriptFunction RunDialogOptionMouseClick{"dialog_options_mouse_click", 4};
    NonBlockingScriptFunction RunDialogOptionKeyPress{"dialog_options_key_press", 3};
    NonBlockingScriptFunction RunDialogOptionTextInput{"dialog_options_text_input", 2};
    NonBlockingScriptFunction RunDialogOptionRepExec{"dialog_options_repexec", 1};
    NonBlockingScriptFunction RunDialogOptionClose{"dialog_options_close", 1};
};

// Stops all running scripts at their next step.
void AbortGame();
