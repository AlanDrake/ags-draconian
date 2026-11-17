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
#include "game/script.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <stack>
#include <vector>

#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "game/dialog/dialog.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/ui/inv_window.h"
#include "game/ui/mouse.h"
#include "game/world/object.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/script_runtime.h"
#include "util/common.h"
#include "util/memory_compat.h"
#include "util/string_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;

std::stack<std::unique_ptr<ExecutingScript>> executingScripts;
ExecutingScript *curExecScript = nullptr; // non-owning ptr

int inside_script = 0;
// Counts the script functions being run on the non-blocking thread (see DoRunScriptFuncCantBlock), which
// may nest; the commands that block are not allowed while it is above 0
int no_blocking_functions = 0;

std::unique_ptr<ScriptThread> scriptThreadMain;
std::unique_ptr<ScriptThread> scriptThreadNonBlocking;
std::stack<std::unique_ptr<ScriptThread>> scriptThreadCallbacks;

// Run a script function on a non-blocking script thread
static bool DoRunScriptFuncCantBlock(const RuntimeScript *script, NonBlockingScriptFunction *func_to_run,
                                     bool has_func);


// Runs the function in every script module, then in the global script, then in the room script,
// skipping the scripts known not to have it (and noting those that do not); stops as soon as the room changes.
void run_function_on_non_blocking_thread(NonBlockingScriptFunction *func_to_run)
{
    Game &game = Game::Current();
    update_script_mouse_coords();

    int room_changes_was = game.Play.RoomChanges;
    func_to_run->AtLeastOneImplementationExists = false;

    // run modules
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        func_to_run->ModuleHasFunction[i] =
            DoRunScriptFuncCantBlock(game.Scripts.Modules[i].get(), func_to_run, func_to_run->ModuleHasFunction[i]);

        if (room_changes_was != game.Play.RoomChanges)
            return;
    }

    func_to_run->GlobalScriptHasFunction =
        DoRunScriptFuncCantBlock(game.Scripts.Global.get(), func_to_run, func_to_run->GlobalScriptHasFunction);

    if (room_changes_was != game.Play.RoomChanges)
        return;

    func_to_run->RoomHasFunction =
        DoRunScriptFuncCantBlock(Room::Current().Script.get(), func_to_run, func_to_run->RoomHasFunction);
}

// Reports a warning in case a requested event handler function was not run
// TODO: find a way to pass event's name too, but they are not clearly defined as strings inside the engine atm...
static void WarnEventFunctionNotFound(const ScriptFunctionRef &fn_ref, bool in_room)
{
    String message = String::FromFormat("WARNING: event script function '%s' not found", fn_ref.FuncName.GetCStr());
    if (in_room)
        message.AppendFmt(" (Room %d)", Game::Current().DisplayedRoom);
    else
        message.AppendFmt(" (%s)", fn_ref.ModuleName.IsEmpty() ? "Global Script" : fn_ref.ModuleName.GetCStr());
    debug_script_warn("%s", message.GetCStr());
}

// Tests if the interaction handler is referencing an existing function in script.
// Writes down the result of the test for the future use.
static bool test_interaction_handler(ScriptEventsBase *handlers, int event_id, bool in_room)
{
    if (!handlers->HasHandler(event_id))
        return false;
    auto &handler = handlers->GetHandler(static_cast<size_t>(event_id));
    if (!handler.IsChecked())
    {
        if (in_room)
            handler.SetChecked(DoesScriptFunctionExist(Room::Current().Script.get(), handler.FunctionName));
        else
            handler.SetChecked(DoesScriptFunctionExistInModule(handlers->GetScriptModule(), handler.FunctionName));

        if (!handler.IsEnabled())
            WarnEventFunctionNotFound(ScriptFunctionRef(handlers->GetScriptModule(), handler.FunctionName), in_room);
    }
    return handler.IsEnabled();
}

// Runs new-style interaction event handler in script.
// If there is no enabled handler for 'event_id', runs nothing and returns 0: when 'any_handlers' has an
// enabled one for 'any_event_id' the caller is expected to run it (the "Any Click" case), otherwise
// the "unhandled_event" callback is run if 'do_unhandled_event' is set.
// Returns -1 if the handler was found while game.Play.CheckInteractionOnly is set (it does not run then),
// or if the room changed while it ran; 0 otherwise.
int run_event_script(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int event_id,
                     ScriptEventsBase *any_handlers, int any_event_id, bool do_unhandled_event)
{
    Game &game = Game::Current();
    assert(handlers);
    if (!handlers)
        return 0;

    if (!test_interaction_handler(handlers, event_id, obj_evt.ScType == kScTypeRoom))
    {
        // No response enabled for this event
        // If there is a response for "Any Click", then abort now so as to
        // run that instead
        if ((any_handlers != nullptr) &&
            test_interaction_handler(any_handlers, any_event_id, obj_evt.ScType == kScTypeRoom))
            return 0;

        // Optionally, run unhandled_event
        if (do_unhandled_event)
            run_unhandled_event(obj_evt, event_id);
        return 0;
    }

    // TODO: CheckInteractionOnly is both the request (1, set by the caller) and the answer
    // (2, set here: an enabled handler was found); return the answer from this function instead
    if (game.Play.CheckInteractionOnly)
    {
        game.Play.CheckInteractionOnly = 2;
        return -1;
    }

    const int room_was = game.Play.RoomChanges;

    RunScriptFunctionAuto(obj_evt.ScType,
                          ScriptFunctionRef(handlers->GetScriptModule(), handlers->GetHandler(event_id).FunctionName),
                          obj_evt.ParamCount, obj_evt.Params);

    // if the room changed within the action
    if (room_was != game.Play.RoomChanges)
        return -1;
    return 0;
}

int run_event_script(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int event_id, bool do_unhandled_event)
{
    return run_event_script(obj_evt, handlers, event_id, nullptr, -1, do_unhandled_event);
}

// Runs the handler for 'event_id' if there is an enabled one: unlike run_event_script, it does not look
// at game.Play.CheckInteractionOnly, and has no "Any Click" or "unhandled_event" fallback.
// Returns -1 if the room changed while it ran, 0 otherwise.
int run_event_script_always(const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int event_id)
{
    Game &game = Game::Current();
    assert(handlers);
    if (!handlers)
        return 0;

    if (!test_interaction_handler(handlers, event_id, obj_evt.ScType == kScTypeRoom))
        return 0;

    const int room_was = game.Play.RoomChanges;

    RunScriptFunctionAuto(obj_evt.ScType,
                          ScriptFunctionRef(handlers->GetScriptModule(), handlers->GetHandler(event_id).FunctionName),
                          obj_evt.ParamCount, obj_evt.Params);

    // if the room changed within the action
    if (room_was != game.Play.RoomChanges)
        return -1;
    return 0;
}

static void AllocScriptFuncRefs()
{
    Game &game = Game::Current();
    // Size the per-module arrays to the module count; until a function is known to be missing from a module
    // ("has function" is true), it is tried there
    game.Scripts.ModuleRepExecAddr.resize(game.Scripts.NumModules);
    game.Scripts.RepExecAlways.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.LateRepExecAlways.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.GetDialogOptionsDimensions.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.RenderDialogOptions.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.GetDialogOptionUnderCursor.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.RunDialogOptionMouseClick.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.RunDialogOptionKeyPress.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.RunDialogOptionTextInput.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.RunDialogOptionRepExec.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    game.Scripts.RunDialogOptionClose.ModuleHasFunction.resize(game.Scripts.NumModules, true);
    for (auto &val : game.Scripts.ModuleRepExecAddr)
    {
        val.Invalidate();
    }
}

// Creates the runtime instances of the global script, the modules and the (optional) dialog script
// from the compiled ones; the module names are used in error messages only
HError CreateRuntimeScripts(const ccScript *global_script, const std::vector<ccScript *> &script_modules,
                            const std::vector<String> &script_module_names, const ccScript *dialog_script)
{
    Game &game = Game::Current();
    assert(script_module_names.size() == script_modules.size());

    if (!global_script)
        return new Error("Global script was not loaded");
    game.Scripts.Global = std::move(RuntimeScript::Create(global_script));
    if (!game.Scripts.Global)
        return new Error("Failed to create the global script:\n%s", cc_get_error().ErrorString.GetCStr());
    Debug::Printf("Created global script");

    game.Scripts.NumModules = script_modules.size();
    for (uint32_t i = 0; i < script_modules.size(); ++i)
    {
        if (!script_modules[i])
            return new Error(
                String::FromFormat("Script module (%u) '%s' was not loaded", i, script_module_names[i].GetCStr()));
        std::shared_ptr<RuntimeScript> runtime_module(RuntimeScript::Create(script_modules[i]));
        if (!runtime_module)
            return new Error(String::FromFormat("Failed to create the script module (%u) '%s':\n%s", i,
                                                script_modules[i]->GetScriptName().c_str(),
                                                cc_get_error().ErrorString.GetCStr()));
        game.Scripts.Modules.push_back(runtime_module);
    }

    // Dialog script is optional
    if (dialog_script)
    {
        game.Scripts.Dialog = std::move(RuntimeScript::Create(dialog_script));
        if (!game.Scripts.Dialog)
            return new Error("Failed to create the dialog script:\n%s", cc_get_error().ErrorString.GetCStr());
        Debug::Printf("Created dialog script");
    }

    AllocScriptFuncRefs();
    return HError::None();
}

// Registers aliases for the engine API types and for the types of every registered plugin
void SetupBuiltinTypeAliases()
{
    // Add aliases for the location "ags":
    // this is a reserved name for the engine API types
    RuntimeScript::AddGlobalTypeAliases("ags");
    // Add aliases for the locations corresponding to all the registered plugins
    std::vector<String> pl_names;
    pl_get_plugin_names(pl_names);
    for (const auto &name : pl_names)
        RuntimeScript::AddGlobalTypeAliases(name);
}

HError LinkGlobalScripts()
{
    Game &game = Game::Current();
    ccSetOption(SCOPT_AUTOIMPORT, 1);

    // NOTE: this function assumes that the module lists have their elements preallocated
    // (see CreateRuntimeScripts)

    std::vector<RuntimeScript *> all_insts; // gather all to resolve exports below
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        all_insts.push_back(game.Scripts.Modules[i].get());
    }

    all_insts.push_back(game.Scripts.Global.get());

    if (game.Scripts.Dialog)
    {
        all_insts.push_back(game.Scripts.Dialog.get());
    }

    //
    // Script linking stage
    // Register all the script exports
    for (auto &inst : all_insts)
    {
        inst->RegisterExports(simp);
    }
    // Resolve all the script imports (these include both engine API and script exports)
    for (auto &inst : all_insts)
    {
        if (!inst->ResolveImports(simp))
            return new Error(
                String::FromFormat("Failed to resolve script imports:\n%s", cc_get_error().ErrorString.GetCStr()));
    }

    // Record addresses for 'repeatedly_execute'
    // TODO: nothing reads ScriptState::ModuleRepExecAddr any more, delete it together with this loop
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        game.Scripts.ModuleRepExecAddr[i] = game.Scripts.Modules[i]->GetSymbolAddress(REP_EXEC_NAME);
    }

    ccSetOption(SCOPT_AUTOIMPORT, 0);

    // Register built-in types under simple aliases for dynamic cast feature
    // TODO: maybe find a better place to do this? should be done after JointRTTI is created though
    SetupBuiltinTypeAliases();

    // Optionally dump script's TOC into the log
    if (EngineServices::Current().LogScriptTOC)
    {
        for (const auto &inst : all_insts)
        {
            if (inst->GetTOC())
                Debug::Printf(PrintScriptTOC(*inst->GetTOC(), inst->GetScriptName().GetCStr()));
        }
    }

    return HError::None();
}

void AbortAllScripts()
{
    scriptExecutor->Abort();
    executingScripts = std::stack<std::unique_ptr<ExecutingScript>>();
}

// Returns the room script for the room type; otherwise the module with the given name, or the global
// script if the name is empty or its own. Returns null if there is no such module (or no room script).
RuntimeScript *GetScriptInstance(ScriptType sc_type, const String &script_module)
{
    Game &game = Game::Current();
    if (sc_type == kScTypeRoom)
        return Room::Current().Script.get();

    if (script_module.IsEmpty() || script_module == game.Scripts.Global->GetScriptName())
        return game.Scripts.Global.get();

    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        if (script_module == game.Scripts.Modules[i]->GetScriptName())
            return game.Scripts.Modules[i].get();
    }
    return nullptr;
}

bool DoesScriptFunctionExist(const RuntimeScript *script, const String &fn_name)
{
    return script->GetSymbolAddress(fn_name).Type == kScValCodePtr;
}

bool DoesScriptFunctionExistInModules(const String &fn_name)
{
    Game &game = Game::Current();
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        if (DoesScriptFunctionExist(game.Scripts.Modules[i].get(), fn_name))
            return true;
    }
    return DoesScriptFunctionExist(game.Scripts.Global.get(), fn_name);
}

bool DoesScriptFunctionExistInModule(const String &script_module, const String &fn_name)
{
    Game &game = Game::Current();
    if (script_module.IsEmpty() || script_module == game.Scripts.Global->GetScriptName())
        return DoesScriptFunctionExist(game.Scripts.Global.get(), fn_name);

    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        if (script_module == game.Scripts.Modules[i]->GetScriptName())
            return DoesScriptFunctionExist(game.Scripts.Modules[i].get(), fn_name);
    }
    return false;
}

void TryRunScriptHandler(ScriptType sc_type, const String &script_module, const ScriptEventHandler &handler,
                         size_t param_count, const RuntimeScriptValue *params)
{
    if (handler.IsEnabled())
        RunScriptFunctionAuto(sc_type, ScriptFunctionRef(script_module, handler.FunctionName), param_count, params);
}

// Runs the function in one script on the non-blocking thread, unless 'has_func' says the script is known
// not to have it. Returns the new "has function" state, which is false once the function turned out
// to be missing. Any other script error quits the game.
static bool DoRunScriptFuncCantBlock(const RuntimeScript *script, NonBlockingScriptFunction *func_to_run, bool has_func)
{
    if (!has_func)
        return (false);

    no_blocking_functions++;
    const ScriptExecError result = scriptExecutor->Run(scriptThreadNonBlocking.get(), script, func_to_run->FunctionName,
                                                       func_to_run->Params, func_to_run->ParamCount);

    if (result == kScExecErr_FuncNotFound)
    {
        // the function doesn't exist, so don't try and run it again
        has_func = false;
    }
    else if ((result != kScExecErr_None) && (result != kScExecErr_Aborted))
    {
        quit_with_script_error(func_to_run->FunctionName);
    }
    else
    {
        func_to_run->AtLeastOneImplementationExists = true;
    }
    // this might be nested, so don't disrupt blocked scripts
    cc_clear_error();
    no_blocking_functions--;
    return (has_func);
}

// Checks that the function can be run now (the executor is free, the script has the function), and makes
// a new ExecutingScript the current one for it
static RunScFuncResult PrepareTextScript(const RuntimeScript *script, const String &fn_name)
{
    assert(script);
    cc_clear_error();
    if (scriptExecutor->IsBusy())
    {
        cc_error("script is currently in execution");
        return kScFnRes_ScriptBusy;
    }
    if (!DoesScriptFunctionExist(script, fn_name))
    {
        cc_error("no such function in script");
        return kScFnRes_NotFound;
    }
    executingScripts.push(std::make_unique<ExecutingScript>(script));
    curExecScript = executingScripts.top().get();
    update_script_mouse_coords();
    inside_script++;
    return kScFnRes_Done;
}

void PostScriptProcessing();

// Pushes a new thread on the stack of callback threads, for a function run while the main thread is busy
ScriptThread *AllocateDynamicScriptThread()
{
    scriptThreadCallbacks.push(
        std::make_unique<ScriptThread>(String::FromFormat("Callback%04zu", scriptThreadCallbacks.size() + 1u)));
    return scriptThreadCallbacks.top().get();
}

// Pops the thread from the stack of callback threads; it must be the topmost one
void FreeDynamicScriptThread(ScriptThread *script_thread)
{
    assert(script_thread == scriptThreadCallbacks.top().get());
    if (script_thread == scriptThreadCallbacks.top().get())
        scriptThreadCallbacks.pop();
}

// Runs a function in the script on the main thread, or on a new callback thread if the main one is busy,
// then performs the post-script actions. Returns kScFnRes_NotFound if there is no such function,
// kScFnRes_Done if it ran (even if aborted); other results quit the game.
RunScFuncResult RunScriptFunction(const RuntimeScript *script, const String &fn_name, size_t param_count,
                                  const RuntimeScriptValue *params)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    assert(script);
    int old_restore_count = services.GameRestoredCount;
    // TODO: research why this is really necessary, and refactor to avoid such hacks!
    // First, save the current ccError state
    // This is necessary because we might be attempting
    // to run Script B, while Script A is still running in the
    // background.
    // If CallInstance here has an error, it would otherwise
    // also abort Script A because ccError is a global variable.
    // A copy, not a reference: cc_get_error() returns that global, which the calls below change
    ScriptError cached_error = cc_get_error(); // NOLINT(performance-unnecessary-copy-initialization)

    const RunScFuncResult res = PrepareTextScript(script, fn_name);
    if (res != kScFnRes_Done)
    {
        if (res != kScFnRes_NotFound)
            quit_with_script_error(fn_name);
        cc_error(cached_error); // restore cached error state
        return res;
    }

    // Choose a script thread. Use main thread by default if it's free
    // (does not have any script loaded up). Otherwise, create a new dynamic
    // callback thread and push to stack of threads.
    ScriptThread *script_thread = scriptThreadMain.get();
    if (scriptThreadMain->IsBusy())
    {
        script_thread = AllocateDynamicScriptThread();
    }

    const ScriptExecError inst_ret =
        scriptExecutor->Run(script_thread, curExecScript->Script, fn_name, params, param_count);
    if ((inst_ret != kScExecErr_None) && (inst_ret != kScExecErr_FuncNotFound) && (inst_ret != kScExecErr_Aborted))
    {
        quit_with_script_error(fn_name);
    }

    if (script_thread != scriptThreadMain.get())
    {
        FreeDynamicScriptThread(script_thread);
    }

    // FIXME: here we save the return value of the current called function,
    // because anything scheduled for the post-script processing will be run by the same executor,
    // and may replace "return value" field saved in a executor.
    // That's ugly. Should revise and redesign this later. BTW, ScriptExecutor keeping
    // last return value is also a hack, used currently only by dialogs. This should be done differently.
    const int ret_value = scriptExecutor->GetReturnValue();
    PostScriptProcessing();
    scriptExecutor->SetReturnValue(ret_value); // restore saved ret value

    // restore cached error state
    cc_error(cached_error);

    // if the game has been restored, ensure that any further scripts are not run
    if ((old_restore_count != services.GameRestoredCount) && (game.Events.Claimed == EVENT_INPROGRESS))
        game.Events.Claimed = EVENT_CLAIMED;

    // Convert any instance exec error into RunScriptFunction result;
    // NOTE: only kScExecErr_FuncNotFound and kScExecErr_Aborted can reach here
    if (inst_ret == kScExecErr_FuncNotFound)
        return kScFnRes_NotFound;
    return kScFnRes_Done;
}

bool RunScriptFunctionInModules(const String &fn_name, size_t param_count, const RuntimeScriptValue *params)
{
    Game &game = Game::Current();
    bool result = false;
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
        result |= RunScriptFunction(game.Scripts.Modules[i].get(), fn_name, param_count, params) == kScFnRes_Done;
    result |= RunScriptFunction(game.Scripts.Global.get(), fn_name, param_count, params) == kScFnRes_Done;
    return result;
}

bool RunScriptFunctionInRoom(const String &fn_name, size_t param_count, const RuntimeScriptValue *params)
{
    Room &room = Room::Current();
    if (!room.Script)
        return false; // room is not loaded yet

    return RunScriptFunction(room.Script.get(), fn_name, param_count, params) == kScFnRes_Done;
}

// Run an event in all script modules in order, and then in the global script, *excluding* room;
// stop before the global script if a module ran it and the room changed or a save was restored since the start,
// or if it failed to run, or if 'break_after_first' is set and a module had the function.
// Returns whether the function ran successfully in the last script it was tried in.
static bool RunEventInModules(const String &fn_name, size_t param_count, const RuntimeScriptValue *params,
                              bool break_after_first)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    const int room_changes_was = game.Play.RoomChanges;
    const int restore_game_count_was = services.GameRestoredCount;
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        const RunScFuncResult ret = RunScriptFunction(game.Scripts.Modules[i].get(), fn_name, param_count, params);
        if (ret != kScFnRes_NotFound)
        {
            // Break on room change or save restoration,
            // or if was told to break after the first found callback,
            // or if script execution error occured
            if ((ret != kScFnRes_Done) || break_after_first || (room_changes_was != game.Play.RoomChanges) ||
                (restore_game_count_was != services.GameRestoredCount))
                return ret == kScFnRes_Done; // return if succeeded to run
        }
    }
    // Try global script last
    return RunScriptFunction(game.Scripts.Global.get(), fn_name, param_count, params) == kScFnRes_Done;
}

// Run non-claimable event (without parameters) in all script modules, *excluding* room;
// it is not stopped by any script, only by a room change or a save restoration (see RunEventInModules)
static bool RunUnclaimableEvent(const String &fn_name)
{
    return RunEventInModules(fn_name, 0, nullptr, false);
}

// Run a single event callback in the specified script module;
// if the name is not provided, then tries to run it in global script.
static bool RunEventInModule(const ScriptFunctionRef &fn_ref, size_t param_count, const RuntimeScriptValue *params)
{
    Game &game = Game::Current();
    if (!fn_ref.ModuleName.IsEmpty())
    {
        for (size_t i = 0; i < game.Scripts.NumModules; ++i)
        {
            if (fn_ref.ModuleName.Compare(game.Scripts.Modules[i]->GetScriptName()) == 0)
            {
                return RunScriptFunction(game.Scripts.Modules[i].get(), fn_ref.FuncName, param_count, params) ==
                       kScFnRes_Done;
            }
        }
    }
    // Try global script last, for backwards compatibility
    return RunScriptFunction(game.Scripts.Global.get(), fn_ref.FuncName, param_count, params) == kScFnRes_Done;
}

// Run claimable event in the room script, all script modules, and then the global script;
// break if event was claimed by any of the run callbacks.
// TODO: unlike RunEventInModules, a room change does not stop the chain; a save restoration does, because
// RunScriptFunction then marks the event as claimed. Decide if a room change should stop it too.
static bool RunClaimableEvent(const String &fn_name, size_t param_count, const RuntimeScriptValue *params)
{
    // Run claimable event chain in script modules and room script
    bool was_claimed;
    run_claimable_event(fn_name, true, param_count, params, &was_claimed);
    // Break on event claim
    if (was_claimed)
        return true; // suppose if claimed then some function ran successfully
    return RunScriptFunction(Game::Current().Scripts.Global.get(), fn_name, param_count, params) == kScFnRes_Done;
}

bool RunScriptFunctionAuto(ScriptType sc_type, const String &fn_name, size_t param_count,
                           const RuntimeScriptValue *params)
{
    return RunScriptFunctionAuto(sc_type, ScriptFunctionRef(fn_name), param_count, params);
}

bool RunScriptFunctionAuto(ScriptType sc_type, const ScriptFunctionRef &fn_ref, size_t param_count,
                           const RuntimeScriptValue *params)
{
    // If told to use a room instance, then run only there
    if (sc_type == kScTypeRoom)
    {
        return RunScriptFunctionInRoom(fn_ref.FuncName, param_count, params);
    }

    // TODO: optimize this function name mapping to propagation type
    const String &fn_name = fn_ref.FuncName;
    auto evt_cb = std::find_if(ScriptEventCb.begin(), ScriptEventCb.end(),
                               [fn_name](const ScriptEventCallback &cb) { return cb.FnName == fn_name; });
    ScriptCallbackPropagation propagate = evt_cb != ScriptEventCb.end() ? evt_cb->CbPropagate : kScCallback_Direct;

    // Unclaimable event means it is run in all modules except room,
    // and cannot be stopped from propagating
    // NOTE: only 'repeatedly_execute' is one (see ScriptEventCb), which is why its name is used here
    if (propagate == kScCallback_CommonUnClaimable)
    {
        return RunUnclaimableEvent(REP_EXEC_NAME);
    }
    // Claimable event is run in all the script modules and room script,
    // before running in the globalscript instance
    if (propagate == kScCallback_CommonClaimable)
    {
        return RunClaimableEvent(fn_name, param_count, params);
    }

    // Else run this event in script modules (except room) according to the function ref
    return RunEventInModule(fn_ref, param_count, params);
}

// Creates the script executor and the main and non-blocking threads, and sets the script alive timer
void InitScriptExec()
{
    scriptExecutor = std::make_unique<ScriptExecutor>();
    scriptThreadMain = std::make_unique<ScriptThread>("Main");
    scriptThreadNonBlocking = std::make_unique<ScriptThread>("Non-Blocking");

    ccSetScriptAliveTimer(1000 / 60u, 1000u, 150000u);
}

void ShutdownScriptExec()
{
    scriptExecutor = {};
    scriptThreadMain = {};
    scriptThreadNonBlocking = {};
}

void UnlinkAllScripts()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (room.Script)
        room.Script->UnRegisterExports(simp);
    if (game.Scripts.Global)
        game.Scripts.Global->UnRegisterExports(simp);
    if (game.Scripts.Dialog)
        game.Scripts.Dialog->UnRegisterExports(simp);
    for (auto &module : game.Scripts.Modules)
        module->UnRegisterExports(simp);
}

void FreeRoomScript()
{
    Room &room = Room::Current();
    if (room.Script)
        room.Script->UnRegisterExports(simp);
    room.Script = nullptr;
}

void FreeAllScripts()
{
    Game &game = Game::Current();
    UnlinkAllScripts();

    Room::Current().Script = nullptr;
    game.Scripts.Global = nullptr;
    game.Scripts.Dialog = nullptr;
    game.Scripts.Modules.clear();
    game.Scripts.NumModules = 0;

    game.Scripts.RepExecAlways.ModuleHasFunction.clear();
    game.Scripts.LateRepExecAlways.ModuleHasFunction.clear();
    game.Scripts.GetDialogOptionsDimensions.ModuleHasFunction.clear();
    game.Scripts.RenderDialogOptions.ModuleHasFunction.clear();
    game.Scripts.GetDialogOptionUnderCursor.ModuleHasFunction.clear();
    game.Scripts.RunDialogOptionMouseClick.ModuleHasFunction.clear();
    game.Scripts.RunDialogOptionKeyPress.ModuleHasFunction.clear();
    game.Scripts.RunDialogOptionTextInput.ModuleHasFunction.clear();
    game.Scripts.RunDialogOptionRepExec.ModuleHasFunction.clear();
    game.Scripts.RunDialogOptionClose.ModuleHasFunction.clear();
}

//=============================================================================

// Called when a script function has returned to the engine: quits if the script left an error, pops its
// ExecutingScript, and then either hands its queued post-script actions (new room, restoring or saving
// a game, dialogs and so on) to the script below it in the stack, or, if none is left, runs them.
void PostScriptProcessing()
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    if (cc_has_error())
        quit(cc_get_error().ErrorString);

    std::unique_ptr<ExecutingScript> finished_script;
    // save current ExecutingScript until the end of function
    if (!executingScripts.empty())
    {
        finished_script = std::move(executingScripts.top());
        executingScripts.pop();
    }
    inside_script--;

    curExecScript = !executingScripts.empty() ? executingScripts.top().get() : nullptr;

    // If there was no current executing script, then bail out now
    // (this might happen if script got stopped and disposed in the midst of run).
    if (!finished_script)
    {
        return;
    }

    // If there's still a pending script in the callstack (or rather thread stack),
    // then reschedule post-script actions to the previous exec script in stack, and break
    if (curExecScript)
    {
        for (auto &action : finished_script->PostScriptActions)
            curExecScript->QueueAction(std::move(action));
        return;
    }

    // TODO: audio is synced whenever any action is scheduled (and again after running them), while only
    // screen changing or time-consuming ones need it
    if (!finished_script->PostScriptActions.empty())
    {
        sync_audio_playback();
    }

    const int old_room_number = game.DisplayedRoom;

    // run the queued post-script actions
    for (auto &action : finished_script->PostScriptActions)
    {
        const int data1 = action.Data[0];
        const int data2 = action.Data[1];

        switch (action.Type)
        {
        case ePSANewRoom: new_room(data1, game.PlayerChar); return; // skip any remaining scheduled actions
        case ePSARestoreGame:
            AbortAllScripts();
            try_restore_save(data1);
            return; // skip any remaining scheduled actions
        case ePSARestoreGameDialog:
            do_restore_game_dialog(data1 & 0xFFFF, (data1 >> 16), data2);
            return; // skip any remaining scheduled actions
        case ePSARunAGSGame:
            AbortAllScripts();
            services.LoadNewGame = data1;
            return; // skip any remaining scheduled actions
        case ePSARunDialog:
            if (is_in_dialog())
            {
                set_dialog_result_goto(data1, data2);
            }
            else
            {
                do_conversation(data1, data2);
            }
            break;
        case ePSAStopDialog:
            if (is_dialog_executing_script())
            {
                set_dialog_result_stop();
            }
            else if (is_in_dialogoptions())
            {
                schedule_dialog_stop();
            }
            break;
        case ePSARestartGame:
            AbortAllScripts();
            restart_game();
            return; // skip any remaining scheduled actions
        case ePSASaveGame: save_game(data1, action.Text, std::move(action.Image)); break;
        case ePSASaveGameDialog:
            do_save_game_dialog(data1 & 0xFFFF, (data1 >> 16), data2);
            return; // skip any remaining scheduled actions
        case ePSAScanSaves:
            prescan_save_slots(action.Data[0], action.Data[1], action.Data[2], action.Data[3], action.Data[4],
                               action.Data[5]);
            break;
        default: quitprintf("undefined post script action found: %d", action.Type);
        }

        // if the room changed in a conversation, for example, abort
        if (old_room_number != game.DisplayedRoom)
        {
            return;
        }
    }

    if (!finished_script->PostScriptActions.empty())
    {
        sync_audio_playback();
    }
}

// Quits the game reporting the current script error and the name of the function that was running;
// the call stack is added unless it is a user error (the "!" kind)
void quit_with_script_error(const String &fn_name)
{
    // TODO: clean up the error reporting logic. Now engine will append call
    // stack info in quit_check_for_error_state() but only in case of explicit
    // script error ("!" type), and not in other case.
    const auto &error = cc_get_error();
    if (error.IsUserError)
        quitprintf("!Error running function '%s':\n%s", fn_name.GetCStr(), error.ErrorString.GetCStr());
    else
        quitprintf("Error running function '%s':\n%s\n\n%s", fn_name.GetCStr(), error.ErrorString.GetCStr(),
                   error.CallStack.GetCStr());
}

void can_run_delayed_command()
{
    if (no_blocking_functions)
        quit("!This command cannot be used within non-blocking events such as " REP_EXEC_ALWAYS_NAME);
}

bool get_can_run_delayed_command()
{
    return no_blocking_functions == 0;
}

// Runs the game's "unhandled_event" callback for an interaction that had no handler, giving it the location
// type (hotspot 0 counts as "no location") and the interaction mode. Does nothing if interactions are only
// being checked, or for the walk mode.
void run_unhandled_event(const ObjectEvent &obj_evt, int mode)
{
    Game &game = Game::Current();
    // FIXME: do not use a global variable here, pass as a function argument!
    if (game.Play.CheckInteractionOnly)
        return;

    // No "unhandled event" for walk mode
    if (game.Def().HasCursorRole(mode, kCursorRole_Walk))
        return;

    int loc_type = obj_evt.ObjectTypeID;
    int obj_id = obj_evt.ObjectID;

    // Special case for Hotspot 0: change to "no location"
    if (loc_type == LOCTYPE_HOTSPOT && obj_id == 0)
        loc_type = LOCTYPE_NOTHING;

    can_run_delayed_command();
    RuntimeScriptValue params[] = {loc_type, mode};
    RunScriptFunctionAuto(kScTypeGame, "unhandled_event", 2, params);
}

bool is_inside_script()
{
    return inside_script > 0;
}

ExecutingScript *get_executingscript()
{
    return curExecScript;
}

bool get_script_position(ScriptPosition &script_pos)
{
    if (scriptExecutor)
    {
        scriptExecutor->GetScriptPosition(script_pos);
        return true;
    }
    return false;
}

String cc_get_callstack(int max_lines)
{
    if (scriptExecutor && max_lines > 0)
        return scriptExecutor->FormatCallStack(max_lines);
    return {};
}

String cc_format_error(const String &message)
{
    if (currentline > 0)
        return String::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    else
        return String::FromFormat("Error (line unknown): %s", message.GetCStr());
}

void AbortGame()
{
    // make sure scripts stop at the next step
    AbortAllScripts();
}
