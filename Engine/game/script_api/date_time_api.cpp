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
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

void RegisterDateTimeAPI()
{
    ScFnRegister datetime_api[] = {
        ScStatic<&DateTime_Now>("DateTime::get_Now"),
        ScStatic<&DateTime_CreateFromDate>("DateTime::CreateFromDate"),
        ScStatic<&DateTime_CreateFromRawTime>("DateTime::CreateFromRawTime"),

        ScMethod<&DateTime_GetDayOfMonth>("DateTime::get_DayOfMonth"),
        ScMethod<&DateTime_GetHour>("DateTime::get_Hour"),
        ScMethod<&DateTime_GetMillisecond>("DateTime::get_Millisecond"),
        ScMethod<&DateTime_GetMinute>("DateTime::get_Minute"),
        ScMethod<&DateTime_GetMonth>("DateTime::get_Month"),
        ScMethod<&DateTime_GetRawTime>("DateTime::get_RawTime"),
        ScMethod<&DateTime_GetSecond>("DateTime::get_Second"),
        ScMethod<&DateTime_GetYear>("DateTime::get_Year"),
    };

    ccAddExternalFunctions(datetime_api);
}
