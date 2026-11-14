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
// Script bindings generated from the signature of the function they call.
//
// A row of a registration table names the engine function and the script name:
//
//     ScMethod<&Character_AddInventory>("Character::AddInventory^2"),
//     ScStatic<&Character_GetAtScreenXY>("Character::GetAtScreenXY^2"),
//
// ScMethod binds a function whose first parameter is the object the script calls the method on, ScStatic one
// that has no such object. The row registers the generated wrapper (ScMethodWrapper<Fn>() or ScStaticWrapper<Fn>(),
// for a registration that is not a row) for scripts and the function itself for plugins, which call it directly
// (contract C4). An optional second argument gives another function for plugins.
//
// The wrapper reads each script parameter as the function's parameter type asks: a pointer from Ptr, a float or
// double from FValue, a bool as "not null" (GetAsBool), another integer or an enum from IValue, a const String &
// from the script string. It returns the result the same way: void as the integer 0 (old compiled scripts may
// read the value of a void call), a bool as 0 or 1, a float, another integer, or a pointer as a script object.
// A pointer result is managed by ScriptManagerOf<T> when a specialisation names the manager of its type T
// (game/script_api/bindings.h names the game's), else the object is its own manager (an IScriptObject).
//
// Where the script's contract differs from the signature, the row says so with tags, extra template arguments
// after the function (ScVoidResult, ScUndefinedResult, ScBoolResult, ScResultAs, ScManagedBy, ScBoolArg,
// ScIntArg, below), e.g. ScMethod<&Button_RunFrameEvent, ScVoidResult>("Button::RunFrameEvent^1").
//
// Each binding is a function of its own (StaticCall<Fn, Tags, 0, 1, ...>) that calls the bound function directly
// and reads the parameters in its own body, so that the compiler inlines the conversions into it: the generated
// code is the one a hand-written wrapper had, with MSVC at /O1 too (an inner helper per call was not inlined there
// for some functions). The script VM calls these per call instruction, a hot path.
//
//=============================================================================
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include "script/runtimescriptvalue.h"
#include "script/script_runtime.h"
#include "util/string.h"

// The manager of the script objects of type T that bound functions return: a specialisation defines
// static IScriptObject *Get(). A type without one must be an IScriptObject, its own manager.
template <typename T>
struct ScriptManagerOf
{
};

// The base of the tags that say how a result goes to the script; a row takes at most one of them.
struct ScResultTag
{
};

namespace ScriptBinderDetail
{

template <typename T>
struct AlwaysFalse : std::false_type
{
};

// True when ScriptManagerOf<T> names a manager
template <typename T, typename = void>
struct HasFixedManager : std::false_type
{
};
template <typename T>
struct HasFixedManager<T, std::void_t<decltype(ScriptManagerOf<T>::Get())>> : std::true_type
{
};

template <typename T>
inline void *ObjectPointer(T *object)
{
    return const_cast<void *>(static_cast<const void *>(object));
}

// A pointer result as a script object, managed as its type says
template <typename T>
inline RuntimeScriptValue ObjectToScript(T *object)
{
    using Plain = std::remove_cv_t<T>;
    if constexpr (HasFixedManager<Plain>::value)
    {
        return RuntimeScriptValue().SetScriptObject(ObjectPointer(object), ScriptManagerOf<Plain>::Get());
    }
    else
    {
        static_assert(!std::is_void_v<Plain>, "a void * result needs ScResultAs<T> or ScManagedBy<manager>");
        return RuntimeScriptValue().SetScriptObject(object, object); // the object is its own manager
    }
}

} // namespace ScriptBinderDetail

// The function returns a value that the script does not get: the script sees a void function (0)
struct ScVoidResult : ScResultTag
{
    template <typename R>
    static RuntimeScriptValue ToScript(R /*result*/)
    {
        return RuntimeScriptValue(int32_t{0});
    }
};

// A void function whose script call returns an undefined value rather than 0
struct ScUndefinedResult : ScResultTag
{
};

// The integer result is a script bool: 1 for any value other than 0
struct ScBoolResult : ScResultTag
{
    template <typename R>
    static RuntimeScriptValue ToScript(R result)
    {
        return RuntimeScriptValue().SetInt32AsBool(result);
    }
};

// The pointer result (a void *, which says nothing of its object) points to a T, managed as T is
template <typename T>
struct ScResultAs : ScResultTag
{
    template <typename R>
    static RuntimeScriptValue ToScript(R *result)
    {
        return ScriptBinderDetail::ObjectToScript(static_cast<T *>(result));
    }
};

// The pointer result is managed by Manager, whatever its type
template <auto &Manager>
struct ScManagedBy : ScResultTag
{
    template <typename R>
    static RuntimeScriptValue ToScript(R *result)
    {
        return RuntimeScriptValue().SetScriptObject(ScriptBinderDetail::ObjectPointer(result), &Manager);
    }
};

// Script parameter I (0-based, the object of a method not counted) is read as a bool, whatever type the function
// takes
template <size_t I>
struct ScBoolArg
{
};

// Script parameter I is read as an integer, whatever type the function takes
template <size_t I>
struct ScIntArg
{
};

namespace ScriptBinderDetail
{

template <typename... Tags>
struct TagList
{
};

template <typename Tag, typename List>
struct HasTag;
template <typename Tag, typename... Tags>
struct HasTag<Tag, TagList<Tags...>> : std::bool_constant<(std::is_same_v<Tag, Tags> || ...)>
{
};

struct NoResultTag
{
};

// The result tag of a list, or NoResultTag
template <typename List>
struct ResultTagOf
{
    using Type = NoResultTag;
};
template <typename First, typename... Rest>
struct ResultTagOf<TagList<First, Rest...>>
{
    using Type =
        std::conditional_t<std::is_base_of_v<ScResultTag, First>, First, typename ResultTagOf<TagList<Rest...>>::Type>;
};

// Script parameter I, for a function parameter of type T; the call converts it to T where the types differ (an
// int32_t to a bool parameter for ScIntArg, a const char * to a String)
template <typename T, size_t I, typename Tags>
inline auto FromScript(const RuntimeScriptValue &value)
{
    using Plain = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (HasTag<ScIntArg<I>, Tags>::value)
    {
        return value.IValue;
    }
    else if constexpr (HasTag<ScBoolArg<I>, Tags>::value || std::is_same_v<Plain, bool>)
    {
        return value.GetAsBool();
    }
    else if constexpr (std::is_pointer_v<Plain>)
    {
        return static_cast<Plain>(value.Ptr);
    }
    else if constexpr (std::is_floating_point_v<Plain>)
    {
        return value.FValue;
    }
    else if constexpr (std::is_integral_v<Plain> || std::is_enum_v<Plain>)
    {
        return static_cast<Plain>(value.IValue);
    }
    else
    {
        static_assert(std::is_same_v<Plain, AGS::Common::String>, "a parameter type the binder cannot read");
        return static_cast<const char *>(value.Ptr);
    }
}

// The script value of a function's result
template <typename R, typename Tags>
inline RuntimeScriptValue ResultToScript(R result)
{
    using Tag = typename ResultTagOf<Tags>::Type;
    if constexpr (!std::is_same_v<Tag, NoResultTag>)
    {
        return Tag::ToScript(result);
    }
    else if constexpr (std::is_same_v<R, bool>)
    {
        return RuntimeScriptValue().SetInt32AsBool(result);
    }
    else if constexpr (std::is_floating_point_v<R>)
    {
        return RuntimeScriptValue().SetFloat(result);
    }
    else if constexpr (std::is_pointer_v<R>)
    {
        return ObjectToScript(result);
    }
    else
    {
        return RuntimeScriptValue().SetInt32(static_cast<int32_t>(result));
    }
}

// The script value of a void function's call
template <typename Tags>
inline RuntimeScriptValue VoidToScript()
{
    using Tag = typename ResultTagOf<Tags>::Type;
    static_assert(std::is_same_v<Tag, NoResultTag> || std::is_same_v<Tag, ScUndefinedResult>,
                  "a void function takes no result tag but ScUndefinedResult");
    if constexpr (std::is_same_v<Tag, ScUndefinedResult>)
    {
        return RuntimeScriptValue();
    }
    else
    {
        return RuntimeScriptValue(int32_t{0});
    }
}

// The bound function's signature, for a function without an object parameter
template <typename F>
struct StaticFunction
{
    static_assert(AlwaysFalse<F>::value, "the binder takes a pointer to a plain function");
};
template <typename R, typename... Args>
struct StaticFunction<R (*)(Args...)>
{
    using Result = R;
    static constexpr size_t kParamCount = sizeof...(Args);
    template <size_t I>
    using Arg = std::tuple_element_t<I, std::tuple<Args...>>;
};

// The bound function's signature, for a function whose first parameter is the object of a script method
template <typename F>
struct MethodFunction
{
    static_assert(AlwaysFalse<F>::value, "a method binds a plain function whose first parameter is a pointer");
};
template <typename R, typename Self, typename... Args>
struct MethodFunction<R (*)(Self *, Args...)>
{
    using Result = R;
    using Object = Self;
    static constexpr size_t kParamCount = sizeof...(Args);
    template <size_t I>
    using Arg = std::tuple_element_t<I, std::tuple<Args...>>;
};

// The assert of a wrapper: the VM passes at least the parameters the function takes
inline void AssertParams(const RuntimeScriptValue *params, int32_t param_count, size_t needed)
{
    (void)params;
    (void)param_count;
    (void)needed;
    assert((needed == 0 || (params != nullptr && param_count >= static_cast<int32_t>(needed))) &&
           "Not enough parameters in call to API function");
}

// The wrapper of a function without an object parameter: script parameters I... are Fn's parameters. One function
// per binding, so that the call of Fn and the conversions are inlined into it as a hand-written wrapper's are.
template <auto Fn, typename Tags, size_t... I>
RuntimeScriptValue StaticCall(const RuntimeScriptValue *params, int32_t param_count)
{
    using Function = StaticFunction<decltype(Fn)>;
    using R = typename Function::Result;
    AssertParams(params, param_count, Function::kParamCount);
    if constexpr (std::is_void_v<R>)
    {
        Fn(FromScript<typename Function::template Arg<I>, I, Tags>(params[I])...);
        return VoidToScript<Tags>();
    }
    else
    {
        return ResultToScript<R, Tags>(Fn(FromScript<typename Function::template Arg<I>, I, Tags>(params[I])...));
    }
}

// The wrapper of a method: self is the object, Fn's first parameter
template <auto Fn, typename Tags, size_t... I>
RuntimeScriptValue MethodCall(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    using Function = MethodFunction<decltype(Fn)>;
    using R = typename Function::Result;
    using Self = typename Function::Object;
    assert((self != nullptr) && "Object pointer is null in call to API function");
    AssertParams(params, param_count, Function::kParamCount);
    if constexpr (std::is_void_v<R>)
    {
        Fn(static_cast<Self *>(self), FromScript<typename Function::template Arg<I>, I, Tags>(params[I])...);
        return VoidToScript<Tags>();
    }
    else
    {
        return ResultToScript<R, Tags>(
            Fn(static_cast<Self *>(self), FromScript<typename Function::template Arg<I>, I, Tags>(params[I])...));
    }
}

// The wrapper instantiated for the script parameters 0..N-1, N the function's parameter count
template <auto Fn, typename Tags, size_t... I>
constexpr ScriptAPIFunction *StaticCallOf(std::index_sequence<I...> /*params*/)
{
    return &StaticCall<Fn, Tags, I...>;
}

template <auto Fn, typename Tags, size_t... I>
constexpr ScriptAPIObjectFunction *MethodCallOf(std::index_sequence<I...> /*params*/)
{
    return &MethodCall<Fn, Tags, I...>;
}

} // namespace ScriptBinderDetail

// The wrapper the script VM calls for a function without an object parameter
template <auto Fn, typename... Tags>
constexpr ScriptAPIFunction *ScStaticWrapper()
{
    using Function = ScriptBinderDetail::StaticFunction<decltype(Fn)>;
    return ScriptBinderDetail::StaticCallOf<Fn, ScriptBinderDetail::TagList<Tags...>>(
        std::make_index_sequence<Function::kParamCount>());
}

// The wrapper the script VM calls for a method
template <auto Fn, typename... Tags>
constexpr ScriptAPIObjectFunction *ScMethodWrapper()
{
    using Function = ScriptBinderDetail::MethodFunction<decltype(Fn)>;
    return ScriptBinderDetail::MethodCallOf<Fn, ScriptBinderDetail::TagList<Tags...>>(
        std::make_index_sequence<Function::kParamCount>());
}

// A registration row: the generated wrapper for scripts, Fn for plugins
template <auto Fn, typename... Tags>
inline ScFnRegister ScStatic(const char *name)
{
    return ScFnRegister(name, ScStaticWrapper<Fn, Tags...>(), reinterpret_cast<void *>(Fn));
}

// A registration row: the generated wrapper for scripts, plugin_fn for plugins
template <auto Fn, typename... Tags, typename PluginFn>
inline ScFnRegister ScStatic(const char *name, PluginFn plugin_fn)
{
    return ScFnRegister(name, ScStaticWrapper<Fn, Tags...>(), plugin_fn);
}

// A registration row of a method: the generated wrapper for scripts, Fn for plugins
template <auto Fn, typename... Tags>
inline ScFnRegister ScMethod(const char *name)
{
    return ScFnRegister(name, ScMethodWrapper<Fn, Tags...>(), reinterpret_cast<void *>(Fn));
}

// A registration row of a method: the generated wrapper for scripts, plugin_fn for plugins
template <auto Fn, typename... Tags, typename PluginFn>
inline ScFnRegister ScMethod(const char *name, PluginFn plugin_fn)
{
    return ScFnRegister(name, ScMethodWrapper<Fn, Tags...>(), plugin_fn);
}
