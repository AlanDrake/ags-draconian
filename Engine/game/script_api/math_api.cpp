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
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterMathAPI()
{
    ScFnRegister math_api[] = {
        ScStatic<&Math_ArcCos>("Maths::ArcCos^1"),
        ScStatic<&Math_ArcSin>("Maths::ArcSin^1"),
        ScStatic<&Math_ArcTan>("Maths::ArcTan^1"),
        ScStatic<&Math_ArcTan2>("Maths::ArcTan2^2"),
        ScStatic<&Math_Cos>("Maths::Cos^1"),
        ScStatic<&Math_Cosh>("Maths::Cosh^1"),
        ScStatic<&Math_DegreesToRadians>("Maths::DegreesToRadians^1"),
        ScStatic<&Math_Exp>("Maths::Exp^1"),
        ScStatic<&Math_Log>("Maths::Log^1"),
        ScStatic<&Math_Log10>("Maths::Log10^1"),
        ScStatic<&Math_RadiansToDegrees>("Maths::RadiansToDegrees^1"),
        ScStatic<&Math_RaiseToPower>("Maths::RaiseToPower^2"),
        ScStatic<&Math_Random>("Maths::Random^1"),
        ScStatic<&Math_RandomFloat>("Maths::RandomFloat^0"),
        ScStatic<&Math_Round>("Maths::Round^2"),
        ScStatic<&Math_Sin>("Maths::Sin^1"),
        ScStatic<&Math_Sinh>("Maths::Sinh^1"),
        ScStatic<&Math_Sqrt>("Maths::Sqrt^1"),
        ScStatic<&Math_Tan>("Maths::Tan^1"),
        ScStatic<&Math_Tanh>("Maths::Tanh^1"),
        ScStatic<&Math_GetPi>("Maths::get_Pi"),
    };

    ccAddExternalFunctions(math_api);
}
