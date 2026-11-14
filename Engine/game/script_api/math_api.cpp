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
// Script bindings registered by RegisterMathAPI().
//
//=============================================================================
#include "game/lib/math.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// float (float value)
RuntimeScriptValue Sc_Math_ArcCos(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_ArcCos);
}

// float (float value)
RuntimeScriptValue Sc_Math_ArcSin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_ArcSin);
}

// float (float value)
RuntimeScriptValue Sc_Math_ArcTan(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_ArcTan);
}

// float (SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval))
RuntimeScriptValue Sc_Math_ArcTan2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT2(Math_ArcTan2);
}

// float (float value)
RuntimeScriptValue Sc_Math_Cos(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Cos);
}

// float (float value)
RuntimeScriptValue Sc_Math_Cosh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Cosh);
}

// float (float value)
RuntimeScriptValue Sc_Math_DegreesToRadians(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_DegreesToRadians);
}

// float (float value)
RuntimeScriptValue Sc_Math_Exp(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Exp);
}

// float (float value)
RuntimeScriptValue Sc_Math_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Log);
}

// float (float value)
RuntimeScriptValue Sc_Math_Log10(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Log10);
}

// float (float value)
RuntimeScriptValue Sc_Math_RadiansToDegrees(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_RadiansToDegrees);
}

// float (SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp))
RuntimeScriptValue Sc_Math_RaiseToPower(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT2(Math_RaiseToPower);
}

RuntimeScriptValue Sc_Math_Random(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Random);
}

RuntimeScriptValue Sc_Math_RandomFloat(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Math_RandomFloat);
}

RuntimeScriptValue Sc_Math_Round(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT_PINT(Math_Round);
}

// float (float value)
RuntimeScriptValue Sc_Math_Sin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Sin);
}

// float (float value)
RuntimeScriptValue Sc_Math_Sinh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Sinh);
}

// float (float value)
RuntimeScriptValue Sc_Math_Sqrt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Sqrt);
}

// float (float value)
RuntimeScriptValue Sc_Math_Tan(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Tan);
}

// float (float value)
RuntimeScriptValue Sc_Math_Tanh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Tanh);
}

// float ()
RuntimeScriptValue Sc_Math_GetPi(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Math_GetPi);
}


void RegisterMathAPI()
{
    ScFnRegister math_api[] = {
        {"Maths::ArcCos^1", API_FN_PAIR(Math_ArcCos)},
        {"Maths::ArcSin^1", API_FN_PAIR(Math_ArcSin)},
        {"Maths::ArcTan^1", API_FN_PAIR(Math_ArcTan)},
        {"Maths::ArcTan2^2", API_FN_PAIR(Math_ArcTan2)},
        {"Maths::Cos^1", API_FN_PAIR(Math_Cos)},
        {"Maths::Cosh^1", API_FN_PAIR(Math_Cosh)},
        {"Maths::DegreesToRadians^1", API_FN_PAIR(Math_DegreesToRadians)},
        {"Maths::Exp^1", API_FN_PAIR(Math_Exp)},
        {"Maths::Log^1", API_FN_PAIR(Math_Log)},
        {"Maths::Log10^1", API_FN_PAIR(Math_Log10)},
        {"Maths::RadiansToDegrees^1", API_FN_PAIR(Math_RadiansToDegrees)},
        {"Maths::RaiseToPower^2", API_FN_PAIR(Math_RaiseToPower)},
        {"Maths::Random^1", API_FN_PAIR(Math_Random)},
        {"Maths::RandomFloat^0", API_FN_PAIR(Math_RandomFloat)},
        {"Maths::Round^2", API_FN_PAIR(Math_Round)},
        {"Maths::Sin^1", API_FN_PAIR(Math_Sin)},
        {"Maths::Sinh^1", API_FN_PAIR(Math_Sinh)},
        {"Maths::Sqrt^1", API_FN_PAIR(Math_Sqrt)},
        {"Maths::Tan^1", API_FN_PAIR(Math_Tan)},
        {"Maths::Tanh^1", API_FN_PAIR(Math_Tanh)},
        {"Maths::get_Pi", API_FN_PAIR(Math_GetPi)},
    };

    ccAddExternalFunctions(math_api);
}
