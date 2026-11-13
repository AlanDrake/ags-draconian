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
#include "ac/datetime.h"

#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "platform/base/agsplatformdriver.h"


ScriptDateTime *DateTime_Now()
{
    ScriptDateTime *sdt = new ScriptDateTime(ScriptDateTime::SystemClock::now());
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

ScriptDateTime *DateTime_CreateFromDate(int year, int month, int day, int hour, int minute, int second)
{
    // NOTE: implementation cannot handle years before 1970,
    // and since DateTime.RawTime is int32, years from 2038 and above cause overflow.
    ScriptDateTime *sdt;
    if (year < 1970 || year >= 2038 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        debug_script_warn(
            "DateTime.CreateFromDate: requested date or time is not in a supported range: %dy,%dm,%dd %dh:%dm:%ds",
            year, month, day, hour, minute, second);
        sdt = new ScriptDateTime();
    }
    else
    {
        sdt = ScriptDateTime::FromFullDate(year, month, day, hour, minute, second);
    }
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

ScriptDateTime *DateTime_CreateFromRawTime(int raw_time)
{
    ScriptDateTime *sdt;
    if (raw_time < 0)
    {
        debug_script_warn("DateTime.CreateFromRawTime: raw time is not in a supported range: %d", raw_time);
        sdt = new ScriptDateTime();
    }
    else
    {
        sdt = ScriptDateTime::FromRawTime(raw_time);
    }
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int DateTime_GetYear(ScriptDateTime *sdt)
{
    return sdt->Year();
}

int DateTime_GetMonth(ScriptDateTime *sdt)
{
    return sdt->Month();
}

int DateTime_GetDayOfMonth(ScriptDateTime *sdt)
{
    return sdt->Day();
}

int DateTime_GetHour(ScriptDateTime *sdt)
{
    return sdt->Hour();
}

int DateTime_GetMinute(ScriptDateTime *sdt)
{
    return sdt->Minute();
}

int DateTime_GetSecond(ScriptDateTime *sdt)
{
    return sdt->Second();
}

int DateTime_GetMillisecond(ScriptDateTime *sdt)
{
    return sdt->Millisecond();
}

int DateTime_GetRawTime(ScriptDateTime *sdt)
{
    return sdt->RawTime();
}
