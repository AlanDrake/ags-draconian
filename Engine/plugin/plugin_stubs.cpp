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
// Stubs for plugin functions.
//
//=============================================================================

#include <cstring>

#include "util/string_compat.h"

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"

RuntimeScriptValue Sc_PluginStub_Void(const RuntimeScriptValue * /*params*/, int32_t /*param_count*/)
{
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_PluginStub_Int0(const RuntimeScriptValue * /*params*/, int32_t /*param_count*/)
{
    return RuntimeScriptValue().SetInt32(0);
}

RuntimeScriptValue Sc_PluginStub_IntNeg1(const RuntimeScriptValue * /*params*/, int32_t /*param_count*/)
{
    return RuntimeScriptValue().SetInt32(-1);
}

RuntimeScriptValue Sc_PluginStub_NullStr(const RuntimeScriptValue * /*params*/, int32_t /*param_count*/)
{
    return RuntimeScriptValue().SetStringLiteral(nullptr);
}

bool RegisterPluginStubs(const char *name)
{
    // Stubs for plugin functions.

    bool is_agsteam = (ags_stricmp(name, "agsteam") == 0) || (ags_stricmp(name, "agsteam-unified") == 0) ||
                      (ags_stricmp(name, "agsteam-disjoint") == 0);
    bool is_agsgalaxy = (ags_stricmp(name, "agsgalaxy") == 0) || (ags_stricmp(name, "agsgalaxy-unified") == 0) ||
                        (ags_stricmp(name, "agsgalaxy-disjoint") == 0);

    if (ags_stricmp(name, "ags_shell") == 0 || ags_stricmp(name, "agsshell") == 0)
    {
        // ags_shell.dll or agsshell.dll
        ccAddExternalStaticFunction("ShellExecute", Sc_PluginStub_Void);
        return true;
    }
    else if (ags_stricmp(name, "agsappopenurl") == 0)
    {
        // agsappopenurl.dll
        ccAddExternalStaticFunction("AppOpenURL", Sc_PluginStub_Int0);
        return true;
    }
    else if (ags_stricmp(name, "agsjoy") == 0)
    {
        // agsjoy.dll
        ccAddExternalStaticFunction("JoystickCount", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("JoystickName", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("JoystickRescan", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::Open^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::IsOpen^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::Click^1", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("Joystick::Close^0", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("Joystick::Valid^0", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::Unplugged^0", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::GetName^0", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::GetAxis^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::IsButtonDown^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::IsJoyBtnDown^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("Joystick::Update^0", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("Joystick::DisableEvents^0", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("Joystick::EnableEvents^1", Sc_PluginStub_Void);
        return true;
    }
    else if (ags_stricmp(name, "agswadjetutil") == 0)
    {
        // agswadjetutil.dll
        ccAddExternalStaticFunction("IsOnPhone", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("FakeKeypress", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("IosSetAchievementValue", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("IosGetAchievementValue", Sc_PluginStub_IntNeg1);
        ccAddExternalStaticFunction("IosShowAchievements", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("IosResetAchievements", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("MobileGetAchievement", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("MobileSetAchievement", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("MobileShowAchievements", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("MobileResetAchievements", Sc_PluginStub_Void);
        return true;
    }
    else if (is_agsteam || is_agsgalaxy)
    {
        // agsteam.dll or agsgalaxy.dll
        ccAddExternalStaticFunction("AGS2Client::IsAchievementAchieved^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::SetAchievementAchieved^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::ResetAchievement^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::GetIntStat^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::GetFloatStat^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::GetAverageRateStat^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::SetIntStat^2", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::SetFloatStat^2", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::UpdateAverageRateStat^3", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("AGS2Client::get_Initialized", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::get_CurrentLeaderboardName", Sc_PluginStub_NullStr);
        ccAddExternalStaticFunction("AGS2Client::RequestLeaderboard^3", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("AGS2Client::UploadScore^1", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::geti_LeaderboardNames", Sc_PluginStub_NullStr);
        ccAddExternalStaticFunction("AGS2Client::geti_LeaderboardScores", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::get_LeaderboardCount", Sc_PluginStub_Int0);
        ccAddExternalStaticFunction("AGS2Client::GetUserName^0", Sc_PluginStub_NullStr);
        ccAddExternalStaticFunction("AGS2Client::GetCurrentGameLanguage^0", Sc_PluginStub_NullStr);
        ccAddExternalStaticFunction("AGS2Client::FindLeaderboard^1", Sc_PluginStub_Void);
        ccAddExternalStaticFunction("AGS2Client::Initialize^2", Sc_PluginStub_Int0);
        if (is_agsteam)
        {
            ccAddExternalStaticFunction("Steam::AddAchievement^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::AddStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::GetIntStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::GetFloatStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::SetIntStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::SetFloatStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::ResetAchievements^0", Sc_PluginStub_Void);
            ccAddExternalStaticFunction("Steam::ResetStats^0", Sc_PluginStub_Void);
            ccAddExternalStaticFunction("Steam::IsAchievementAchieved^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::SetAchievementAchieved^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("Steam::ResetStatsAndAchievements^0", Sc_PluginStub_Void);

            ccAddExternalStaticFunction("AGSteam::IsAchievementAchieved^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::SetAchievementAchieved^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::ResetAchievement^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::GetIntStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::GetFloatStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::GetAverageRateStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::SetIntStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::SetFloatStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::UpdateAverageRateStat^3", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
            ccAddExternalStaticFunction("AGSteam::get_Initialized", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::get_CurrentLeaderboardName", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSteam::RequestLeaderboard^3", Sc_PluginStub_Void);
            ccAddExternalStaticFunction("AGSteam::UploadScore^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::geti_LeaderboardNames", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSteam::geti_LeaderboardScores", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::get_LeaderboardCount", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSteam::GetUserName^0", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSteam::GetCurrentGameLanguage^0", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSteam::FindLeaderboard^1", Sc_PluginStub_Void);
        }
        else // agsgalaxy
        {
            ccAddExternalStaticFunction("AGSGalaxy::IsAchievementAchieved^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::SetAchievementAchieved^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::ResetAchievement^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::GetIntStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::GetFloatStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::GetAverageRateStat^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::SetIntStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::SetFloatStat^2", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::UpdateAverageRateStat^3", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::ResetStatsAndAchievements^0", Sc_PluginStub_Void);
            ccAddExternalStaticFunction("AGSGalaxy::get_Initialized", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::get_CurrentLeaderboardName", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSGalaxy::RequestLeaderboard^3", Sc_PluginStub_Void);
            ccAddExternalStaticFunction("AGSGalaxy::UploadScore^1", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::geti_LeaderboardNames", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSGalaxy::geti_LeaderboardScores", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::get_LeaderboardCount", Sc_PluginStub_Int0);
            ccAddExternalStaticFunction("AGSGalaxy::GetUserName^0", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSGalaxy::GetCurrentGameLanguage^0", Sc_PluginStub_NullStr);
            ccAddExternalStaticFunction("AGSGalaxy::Initialize^2", Sc_PluginStub_Int0);
        }
        return true;
    }

    return false;
}
