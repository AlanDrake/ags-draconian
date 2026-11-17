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
// What the script binder (script/script_binder.h) needs from the game: the managers of the objects that the
// script API returns, and the rows of the functions that take a formatted text. Every file of
// game/script_api/ that binds functions includes this header, so that each binding sees the same managers.
//
//=============================================================================
#pragma once

#include <vector>

#include "game/dynobj/cc_audio_channel.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/dynobj/cc_character.h"
#include "game/dynobj/cc_dialog.h"
#include "game/dynobj/cc_gui.h"
#include "game/dynobj/cc_gui_control.h"
#include "game/dynobj/cc_hotspot.h"
#include "game/dynobj/cc_inventory.h"
#include "game/dynobj/cc_object.h"
#include "game/dynobj/cc_region.h"
#include "game/dynobj/cc_walk_behind.h"
#include "game/dynobj/cc_walkable_area.h"
#include "game/dynobj/script_string.h"
#include "game/game.h"
#include "game/script_api/exports.h"
#include "game/translation.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_api.h"
#include "script/script_binder.h"

class Character;
struct ScriptAudioChannel;
class AudioClip;
struct ScriptDialog;
struct ScriptGUI;
struct ScriptHotspot;
struct ScriptInvItem;
struct ScriptObject;
struct ScriptRegion;
struct ScriptWalkableArea;
struct ScriptWalkbehind;

namespace AGS
{
namespace Common
{
class GUIButton;
class GUIControl;
class GUIInvWindow;
class GUILabel;
class GUIListBox;
class GUIObject;
class GUISlider;
class GUITextBox;
} // namespace Common
} // namespace AGS

// A const char * (or char *) result is a new script string
template <>
struct ScriptManagerOf<char>
{
    static IScriptObject *Get() { return &myScriptStringImpl; }
};

// The handles of the game's entities, one manager each
template <>
struct ScriptManagerOf<Character>
{
    static IScriptObject *Get() { return &ccDynamicCharacter; }
};
template <>
struct ScriptManagerOf<ScriptAudioChannel>
{
    static IScriptObject *Get() { return &ccDynamicAudio; }
};
template <>
struct ScriptManagerOf<AudioClip>
{
    static IScriptObject *Get() { return &ccDynamicAudioClip; }
};
template <>
struct ScriptManagerOf<ScriptDialog>
{
    static IScriptObject *Get() { return &ccDynamicDialog; }
};
template <>
struct ScriptManagerOf<ScriptGUI>
{
    static IScriptObject *Get() { return &ccDynamicGUI; }
};
template <>
struct ScriptManagerOf<ScriptHotspot>
{
    static IScriptObject *Get() { return &ccDynamicHotspot; }
};
template <>
struct ScriptManagerOf<ScriptInvItem>
{
    static IScriptObject *Get() { return &ccDynamicInv; }
};
template <>
struct ScriptManagerOf<ScriptObject>
{
    static IScriptObject *Get() { return &ccDynamicObject; }
};
template <>
struct ScriptManagerOf<ScriptRegion>
{
    static IScriptObject *Get() { return &ccDynamicRegion; }
};
template <>
struct ScriptManagerOf<ScriptWalkableArea>
{
    static IScriptObject *Get() { return &ccDynamicWalkarea; }
};
template <>
struct ScriptManagerOf<ScriptWalkbehind>
{
    static IScriptObject *Get() { return &ccDynamicWalkbehind; }
};

// The GUI controls, of every class, share one manager
template <>
struct ScriptManagerOf<AGS::Common::GUIObject>
{
    static IScriptObject *Get() { return &ccDynamicGUIControl; }
};
template <>
struct ScriptManagerOf<AGS::Common::GUIControl> : ScriptManagerOf<AGS::Common::GUIObject>
{
};
template <>
struct ScriptManagerOf<AGS::Common::GUIButton> : ScriptManagerOf<AGS::Common::GUIObject>
{
};
template <>
struct ScriptManagerOf<AGS::Common::GUIInvWindow> : ScriptManagerOf<AGS::Common::GUIObject>
{
};
template <>
struct ScriptManagerOf<AGS::Common::GUILabel> : ScriptManagerOf<AGS::Common::GUIObject>
{
};
template <>
struct ScriptManagerOf<AGS::Common::GUIListBox> : ScriptManagerOf<AGS::Common::GUIObject>
{
};
template <>
struct ScriptManagerOf<AGS::Common::GUISlider> : ScriptManagerOf<AGS::Common::GUIObject>
{
};
template <>
struct ScriptManagerOf<AGS::Common::GUITextBox> : ScriptManagerOf<AGS::Common::GUIObject>
{
};

namespace ScriptBinderDetail
{

// The wrapper of a function whose last parameter is a text that the script passes as a format and its arguments
// (variadic, as Display(const string format, ...)): the format is translated, then formatted with the script
// parameters after it; script parameters I... are Fn's parameters before the text, read as StaticCall reads them
template <auto Fn, size_t... I>
RuntimeScriptValue StaticFormatCall(const RuntimeScriptValue *params, int32_t param_count)
{
    using Function = StaticFunction<decltype(Fn)>;
    using R = typename Function::Result;
    constexpr int32_t kFixedCount = static_cast<int32_t>(Function::kParamCount); // the format included
    AssertParams(params, param_count, Function::kParamCount);
    std::vector<char> buffer;
    const char *text = ScriptSprintf(buffer, get_translation(params[kFixedCount - 1].CStr), params + kFixedCount,
                                     param_count - kFixedCount);
    if constexpr (std::is_void_v<R>)
    {
        Fn(FromScript<typename Function::template Arg<I>, I, TagList<>>(params[I])..., text);
        return VoidToScript<TagList<>>();
    }
    else
    {
        return ResultToScript<R, TagList<>>(
            Fn(FromScript<typename Function::template Arg<I>, I, TagList<>>(params[I])..., text));
    }
}

// StaticFormatCall for a method: self is the object, Fn's first parameter
template <auto Fn, size_t... I>
RuntimeScriptValue MethodFormatCall(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    using Function = MethodFunction<decltype(Fn)>;
    using R = typename Function::Result;
    using Self = typename Function::Object;
    constexpr int32_t kFixedCount = static_cast<int32_t>(Function::kParamCount);
    assert((self != nullptr) && "Object pointer is null in call to API function");
    AssertParams(params, param_count, Function::kParamCount);
    std::vector<char> buffer;
    const char *text = ScriptSprintf(buffer, get_translation(params[kFixedCount - 1].CStr), params + kFixedCount,
                                     param_count - kFixedCount);
    if constexpr (std::is_void_v<R>)
    {
        Fn(static_cast<Self *>(self), FromScript<typename Function::template Arg<I>, I, TagList<>>(params[I])..., text);
        return VoidToScript<TagList<>>();
    }
    else
    {
        return ResultToScript<R, TagList<>>(
            Fn(static_cast<Self *>(self), FromScript<typename Function::template Arg<I>, I, TagList<>>(params[I])...,
               text));
    }
}

template <auto Fn, size_t... I>
constexpr ScriptAPIFunction *StaticFormatCallOf(std::index_sequence<I...> /*params*/)
{
    return &StaticFormatCall<Fn, I...>;
}

template <auto Fn, size_t... I>
constexpr ScriptAPIObjectFunction *MethodFormatCallOf(std::index_sequence<I...> /*params*/)
{
    return &MethodFormatCall<Fn, I...>;
}

} // namespace ScriptBinderDetail

// The wrapper the script VM calls for a function that takes a formatted text, its last parameter
template <auto Fn>
constexpr ScriptAPIFunction *ScStaticFormatWrapper()
{
    using Function = ScriptBinderDetail::StaticFunction<decltype(Fn)>;
    static_assert(Function::kParamCount > 0, "the text is the function's last parameter");
    return ScriptBinderDetail::StaticFormatCallOf<Fn>(std::make_index_sequence<Function::kParamCount - 1>());
}

// The same, for a method
template <auto Fn>
constexpr ScriptAPIObjectFunction *ScMethodFormatWrapper()
{
    using Function = ScriptBinderDetail::MethodFunction<decltype(Fn)>;
    static_assert(Function::kParamCount > 0, "the text is the function's last parameter");
    return ScriptBinderDetail::MethodFormatCallOf<Fn>(std::make_index_sequence<Function::kParamCount - 1>());
}

// A registration row of a function that takes a formatted text: the generated wrapper for scripts, plugin_fn
// (a C variadic function that formats the text itself) for plugins
template <auto Fn, typename PluginFn>
inline ScFnRegister ScStaticFormat(const char *name, PluginFn plugin_fn)
{
    return ScFnRegister(name, ScStaticFormatWrapper<Fn>(), plugin_fn);
}

// The same, for a method
template <auto Fn, typename PluginFn>
inline ScFnRegister ScMethodFormat(const char *name, PluginFn plugin_fn)
{
    return ScFnRegister(name, ScMethodFormatWrapper<Fn>(), plugin_fn);
}
