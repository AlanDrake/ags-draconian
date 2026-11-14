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
#include "game/lib/math.h"

#include <cmath>

#include "debug/debug_log.h"
#include "util/common.h" // quit
#include "util/math.h"

using namespace AGS::Common;

float RoundImpl(const char *apiname, float value, int roundDirection)
{
    switch (roundDirection)
    {
    case eRoundDown: return std::floor(value);
    case eRoundUp: return std::ceil(value);
    case eRoundNearest: return std::round(value);
    case eRoundTowardsZero: return std::trunc(value);
    case eRoundAwayFromZero: return value < 0.f ? std::floor(value) : std::ceil(value);
    default: debug_script_warn("!%s: invalid round direction %d", apiname, roundDirection); return value;
    }
    return 0.f;
}

int FloatToInt(float value, int roundDirection)
{
    return static_cast<int>(RoundImpl("FloatToInt", value, roundDirection));
}

float IntToFloat(int value)
{
    return static_cast<float>(value);
}

float Math_Cos(float value)
{
    return cos(value);
}

float Math_Sin(float value)
{
    return sin(value);
}

float Math_Tan(float value)
{
    return tan(value);
}

float Math_ArcCos(float value)
{
    return acos(value);
}

float Math_ArcSin(float value)
{
    return asin(value);
}

float Math_ArcTan(float value)
{
    return atan(value);
}

float Math_ArcTan2(float yval, float xval)
{
    return atan2(yval, xval);
}

float Math_Log(float value)
{
    return log(value);
}

float Math_Log10(float value)
{
    return ::log10(value);
}

float Math_Exp(float value)
{
    return exp(value);
}

float Math_Cosh(float value)
{
    return cosh(value);
}

float Math_Sinh(float value)
{
    return sinh(value);
}

float Math_Tanh(float value)
{
    return tanh(value);
}

float Math_RaiseToPower(float base, float exp)
{
    return ::pow(base, exp);
}

float Math_DegreesToRadians(float value)
{
    return static_cast<float>(Math::DegreesToRadians(value));
}

float Math_RadiansToDegrees(float value)
{
    return static_cast<float>(Math::RadiansToDegrees(value));
}

int Math_Random(int limit)
{
    // NOTE: we clamp rand max to INT16_MAX for cross-platform compatibility;
    // perhaps replacing rand with another random number generator would solve this problem.
    if (limit <= 0 || limit > INT16_MAX + 1)
    {
        debug_script_warn("!Maths.Random: invalid parameter %d -- must be in range (1..%d)", limit, INT16_MAX + 1);
        return 0;
    }

    return rand() % (Math::Clamp<int>(limit, 0, INT16_MAX + 1));
}

float Math_RandomFloat()
{
    return static_cast<float>(static_cast<double>(rand()) / (RAND_MAX + 1.0));
}

float Math_Round(float value, int roundDirection)
{
    return RoundImpl("Math.Round", value, roundDirection);
}

float Math_GetPi()
{
    return static_cast<float>(M_PI);
}

float Math_Sqrt(float value)
{
    if (value < 0.0)
        quit("!Sqrt: cannot perform square root of negative number");

    return ::sqrt(value);
}

int Random(int upto)
{
    if (upto < 0 || upto > RAND_MAX)
    {
        debug_script_warn("!Random: invalid parameter %d -- must be in range (0..%d)", upto, RAND_MAX);
        return 0;
    }

    return rand() % (++upto);
}


#include "script/script_runtime.h"
