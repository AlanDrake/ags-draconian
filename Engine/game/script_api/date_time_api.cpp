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
// Script bindings registered by RegisterDateTimeAPI().
//
//=============================================================================
#include "game/lib/date_time.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// ScriptDateTime* ()
RuntimeScriptValue Sc_DateTime_Now(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDateTime, DateTime_Now);
}

RuntimeScriptValue Sc_DateTime_CreateFromRawTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDateTime, DateTime_CreateFromRawTime);
}

RuntimeScriptValue Sc_DateTime_CreateFromDate(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT6(ScriptDateTime, DateTime_CreateFromDate);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetYear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetYear);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetMonth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetMonth);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetDayOfMonth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetDayOfMonth);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetHour(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetHour);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetMinute(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetMinute);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetSecond(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetSecond);
}

RuntimeScriptValue Sc_DateTime_GetMillisecond(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetMillisecond);
}


// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetRawTime(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetRawTime);
}

void RegisterDateTimeAPI()
{
    ScFnRegister datetime_api[] = {
        {"DateTime::get_Now", API_FN_PAIR(DateTime_Now)},
        {"DateTime::CreateFromDate", API_FN_PAIR(DateTime_CreateFromDate)},
        {"DateTime::CreateFromRawTime", API_FN_PAIR(DateTime_CreateFromRawTime)},

        {"DateTime::get_DayOfMonth", API_FN_PAIR(DateTime_GetDayOfMonth)},
        {"DateTime::get_Hour", API_FN_PAIR(DateTime_GetHour)},
        {"DateTime::get_Millisecond", API_FN_PAIR(DateTime_GetMillisecond)},
        {"DateTime::get_Minute", API_FN_PAIR(DateTime_GetMinute)},
        {"DateTime::get_Month", API_FN_PAIR(DateTime_GetMonth)},
        {"DateTime::get_RawTime", API_FN_PAIR(DateTime_GetRawTime)},
        {"DateTime::get_Second", API_FN_PAIR(DateTime_GetSecond)},
        {"DateTime::get_Year", API_FN_PAIR(DateTime_GetYear)},
    };

    ccAddExternalFunctions(datetime_api);
}
