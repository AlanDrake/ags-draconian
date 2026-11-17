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
#include "game/game_state.h"

#include <algorithm>

#include "debug/debug_log.h"
#include "game/customproperties.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_version.h"
#include "game/save/savegame_internal.h"
#include "game/script.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "gfx/gfx_def.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "util/field_table.h"
#include "util/string_utils.h"
#include "util/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

const std::array<const char *, kNum_RBS> kRBSwitchNames = {{
    "dummy",                     // kRBS_Dummy
    "smooth_walk",               // kRBO_SmoothWalkTransition
    "gui_text_direction",        // kRBO_ApplyGUITextDirection
    "dialog_opt_text_alignment", // kRBO_ApplyDialogOptionTextAlignment
    "no_textprop_autotranslate", // kRBO_NoTextPropertyAutoTranslate
}};

// The operations of a location reference, for the rows of GameState that hold one. They are here, not beside the
// struct in game_state.h, so that the header does not include field_table.h: only this file's table names the type.
namespace AGS
{
namespace Common
{

template <>
struct FieldValue<SceneLocationRef>
{
    static bool Equal(const SceneLocationRef &a, const SceneLocationRef &b) { return a == b; }
    static String Print(const SceneLocationRef &value)
    {
        return String::FromFormat("(%d, %d)", value.LocationType, value.ObjectIndex);
    }
    static void Alter(SceneLocationRef &value)
    {
        value.LocationType++;
        value.ObjectIndex++;
    }
};

// The timers print and compare as their map does; Alter starts one that is not running
template <>
struct FieldValue<ScriptTimerSet>
{
    using Map = std::unordered_map<int32_t, int32_t>;
    static bool Equal(const ScriptTimerSet &a, const ScriptTimerSet &b)
    {
        return FieldValue<Map>::Equal(a.Running(), b.Running());
    }
    static String Print(const ScriptTimerSet &value) { return FieldValue<Map>::Print(value.Running()); }
    static void Alter(ScriptTimerSet &value)
    {
        int timer_id = 0;
        while (value.GetPos(timer_id) != 0)
            timer_id++;
        value.Start(timer_id, 1);
    }
};

} // namespace Common
} // namespace AGS

namespace
{

// Saved: by the "Game State" component (ReadFromSavegame, WriteForSavegame) unless the row names another one.
// Transient: set and cleared as the game runs; the row says what a restore gives it today, the running game's value
// (kept) or one of its own.
constexpr FieldRow<GameState> kGameStateFields[] = {
    Field<&GameState::UsedMode>("UsedMode"),
    Field<&GameState::UserInterfaceDisabledCount>("UserInterfaceDisabledCount"),
    Field<&GameState::GlobalScriptTimer>("GlobalScriptTimer"),
    Field<&GameState::DebugMode>("DebugMode"),
    Field<&GameState::MessageTimeLeft>("MessageTimeLeft"),
    Field<&GameState::UsedInventory>("UsedInventory"),
    Field<&GameState::TextReadingSpeed>("TextReadingSpeed"),
    Field<&GameState::GlobalSpeechAnimationDelay>("GlobalSpeechAnimationDelay"),
    Field<&GameState::InventoryItemWidth>("InventoryItemWidth"),
    Field<&GameState::InventoryItemHeight>("InventoryItemHeight"),
    Field<&GameState::TextShadowColor>("TextShadowColor"),
    Field<&GameState::SwapPortraitSide>("SwapPortraitSide"),
    Field<&GameState::SpeechTextWindowGui>("SpeechTextWindowGui"),
    Field<&GameState::FollowingRoomDelay>("FollowingRoomDelay"),
    Field<&GameState::SkipDisplay>("SkipDisplay"),
    Field<&GameState::NoMultiLoopRepeat>("NoMultiLoopRepeat"),
    Field<&GameState::InventoryActivated>("InventoryActivated"),
    Field<&GameState::NoTextBackgroundWhenVoice>("NoTextBackgroundWhenVoice"),
    Field<&GameState::MaxDialogOptionWidth>("MaxDialogOptionWidth"),
    Field<&GameState::NoHighColorFadeIn>("NoHighColorFadeIn"),
    Field<&GameState::BackgroundSpeechGameSpeed>("BackgroundSpeechGameSpeed"),
    Field<&GameState::KeepBackgroundSpeechOnDisplay>("KeepBackgroundSpeechOnDisplay"),
    Field<&GameState::UnfactorSpeechFromTextLength>("UnfactorSpeechFromTextLength"),
    Field<&GameState::SpeechMusicDrop>("SpeechMusicDrop"),
    Field<&GameState::RunningCutsceneSkipStyle>("RunningCutsceneSkipStyle"),
    Field<&GameState::FastForward>("FastForward"),
    Field<&GameState::GameSpeedModifier>("GameSpeedModifier"),
    Field<&GameState::PreviousGameData>("PreviousGameData"),
    Field<&GameState::ReplayHotkeyUnused>("ReplayHotkeyUnused"),
    Field<&GameState::DialogOptionsPaddingX>("DialogOptionsPaddingX"),
    Field<&GameState::DialogOptionsPaddingY>("DialogOptionsPaddingY"),
    Field<&GameState::NarratorSpeech>("NarratorSpeech"),
    Field<&GameState::LipSyncSpeed>("LipSyncSpeed"),
    Field<&GameState::SpeechAnimationStopTimeMargin>("SpeechAnimationStopTimeMargin"),
    Field<&GameState::DisableAntialiasing>("DisableAntialiasing"),
    Field<&GameState::TextReadingSpeedModifier>("TextReadingSpeedModifier"),
    Field<&GameState::TextAlignment>("TextAlignment"),
    Field<&GameState::SpeechBubbleWidth>("SpeechBubbleWidth"),
    Field<&GameState::MinDialogOptionWidth>("MinDialogOptionWidth"),
    Field<&GameState::DisableDialogParser>("DisableDialogParser"),
    Field<&GameState::TopBarBackColor>("TopBarBackColor"),
    Field<&GameState::TopBarTextColor>("TopBarTextColor"),
    Field<&GameState::TopBarBorderColor>("TopBarBorderColor"),
    Field<&GameState::TopBarBorderWidth>("TopBarBorderWidth"),
    Field<&GameState::TopBarY>("TopBarY"),
    Field<&GameState::ScreenshotWidth>("ScreenshotWidth"),
    Field<&GameState::ScreenshotHeight>("ScreenshotHeight"),
    Field<&GameState::TopBarFont>("TopBarFont"),
    Field<&GameState::SpeechTextAlignment>("SpeechTextAlignment"),
    Field<&GameState::AutoUseWalkToPoints>("AutoUseWalkToPoints"),
    Field<&GameState::GreyOutInventory>("GreyOutInventory"),
    Field<&GameState::SpeechSkipKey>("SpeechSkipKey"),
    Field<&GameState::AbortKey>("AbortKey"),
    Field<&GameState::FadeColorRed>("FadeColorRed"),
    Field<&GameState::FadeColorGreen>("FadeColorGreen"),
    Field<&GameState::FadeColorBlue>("FadeColorBlue"),
    Field<&GameState::ShowSingleDialogOption>("ShowSingleDialogOption"),
    Field<&GameState::KeepScreenDuringInstantTransition>("KeepScreenDuringInstantTransition"),
    Field<&GameState::ReadDialogOptionColor>("ReadDialogOptionColor"),
    // during a "dialog_request" callback only; a restore keeps the running game's value
    Field<&GameState::DialogRequestResult>("DialogRequestResult", FieldRole::Transient),
    Field<&GameState::IsSpeechCustomPortraitPlacement>("IsSpeechCustomPortraitPlacement"),
    Field<&GameState::SpeechPortraitOffsetX>("SpeechPortraitOffsetX"),
    Field<&GameState::SpeechPortraitY>("SpeechPortraitY"),
    Field<&GameState::SpeechDisplayPostTimeMs>("SpeechDisplayPostTimeMs"),
    Field<&GameState::DialogOptionsHighlightColor>("DialogOptionsHighlightColor"),
    Field<&GameState::RandomSeed>("RandomSeed"),
    Field<&GameState::PlayerRegionID>("PlayerRegionID"),
    // the fades and the transitions; a restore keeps the running game's value
    Field<&GameState::IsScreenFadedOut>("IsScreenFadedOut", FieldRole::Transient),
    Field<&GameState::CheckInteractionOnly>("CheckInteractionOnly"),
    Field<&GameState::BgAnimWait>("BgAnimWait"),
    Field<&GameState::WaitCounter>("WaitCounter"),
    // how the last blocking wait ended (SetWaitSkipResult); a restore keeps the running game's values
    Field<&GameState::WaitSkippedBy>("WaitSkippedBy", FieldRole::Transient),
    Field<&GameState::WaitSkippedByData>("WaitSkippedByData", FieldRole::Transient),
    Field<&GameState::MouseBounds>("MouseBounds"),
    Field<&GameState::FadeEffect>("FadeEffect"),
    Field<&GameState::AudioMasterVolume>("AudioMasterVolume"),
    Field<&GameState::ScreenFlip>("ScreenFlip"),
    Field<&GameState::SpeechVoiceMode>("SpeechVoiceMode"),
    Field<&GameState::SpeechSkipStyle>("SpeechSkipStyle"),
    Field<&GameState::AlwaysWaitForSpeechText>("AlwaysWaitForSpeechText"),
    Field<&GameState::SpeechOverlayZOrder>("SpeechOverlayZOrder"),
    Field<&GameState::SpeechVolume>("SpeechVolume"),
    Field<&GameState::NormalFont>("NormalFont"),
    Field<&GameState::SpeechFont>("SpeechFont"),
    // the height of font 0 (InitGameState)
    Field<&GameState::StandardGuiTextHeight>("StandardGuiTextHeight", FieldRole::Derived),
    Field<&GameState::WaitSkipType>("WaitSkipType"),
    Field<&GameState::SwapPortraitLastCharacter>("SwapPortraitLastCharacter"),
    // the speech portrait's side; a restore sets it to -1 (unload_old_room), not the initializer (0)
    Field<&GameState::SwapPortraitLastLastCharacter>("SwapPortraitLastLastCharacter", FieldRole::Transient),
    Field<&GameState::ConversationDepth>("ConversationDepth"),
    Field<&GameState::ScreenTint>("ScreenTint"),
    Field<&GameState::ParsedWords>("ParsedWords"),
    Field<&GameState::BadParsedWord>("BadParsedWord"),
    // counts the rooms loaded; a restore keeps the running game's value and adds one (load_new_room)
    Field<&GameState::RoomChanges>("RoomChanges", FieldRole::Transient),
    Field<&GameState::IsMouseInputEnabled>("IsMouseInputEnabled"),
    Field<&GameState::IsMouseCursorShown>("IsMouseCursorShown"),
    Field<&GameState::ShakeScreenDelay>("ShakeScreenDelay"),
    Field<&GameState::ShakeScreenAmount>("ShakeScreenAmount"),
    Field<&GameState::ShakeScreenLength>("ShakeScreenLength"),
    Field<&GameState::AmbientTintRed>("AmbientTintRed"),
    Field<&GameState::AmbientTintGreen>("AmbientTintGreen"),
    Field<&GameState::AmbientTintBlue>("AmbientTintBlue"),
    Field<&GameState::AmbientTintLevel>("AmbientTintLevel"),
    Field<&GameState::AmbientTintLight>("AmbientTintLight"),
    Field<&GameState::IsAmbientTintEnabled>("IsAmbientTintEnabled"),
    Field<&GameState::SkipUntilCharacterStops>("SkipUntilCharacterStops"),
    // the location under the mouse (GetLocationName, the cursor's update); a restore keeps the running game's values
    Field<&GameState::LastNamedLocation>("LastNamedLocation", FieldRole::Transient),
    Field<&GameState::LocationNameSaveCursor>("LocationNameSaveCursor", FieldRole::Transient),
    Field<&GameState::RestoreCursorModeTo>("RestoreCursorModeTo"),
    Field<&GameState::RestoreCursorImageTo>("RestoreCursorImageTo"),
    Field<&GameState::CrossfadingOutChannel>("CrossfadingOutChannel"),
    Field<&GameState::CrossfadeStep>("CrossfadeStep"),
    Field<&GameState::CrossfadeOutVolumePerStep>("CrossfadeOutVolumePerStep"),
    Field<&GameState::CrossfadeInitialVolumeOut>("CrossfadeInitialVolumeOut"),
    Field<&GameState::CrossfadingInChannel>("CrossfadingInChannel"),
    Field<&GameState::CrossfadeInVolumePerStep>("CrossfadeInVolumePerStep"),
    Field<&GameState::CrossfadeFinalVolumeIn>("CrossfadeFinalVolumeIn"),
    Field<&GameState::NewMusicQueueSize>("NewMusicQueueSize"),
    Field<&GameState::NewMusicQueue>("NewMusicQueue"),
    Field<&GameState::LastParserEntry>("LastParserEntry"),
    Field<&GameState::GameName>("GameName"),
    Field<&GameState::GroundLevelAreasDisabled>("GroundLevelAreasDisabled"),
    Field<&GameState::NextScreenTransition>("NextScreenTransition"),
    // written, and dropped by the reader on purpose: the running game's gamma stays
    Field<&GameState::GammaAdjustment>("GammaAdjustment"),
    Field<&GameState::TemporarilyTurnedOffCharacter>("TemporarilyTurnedOffCharacter"),
    // the GUIs by their z-order (prepare_gui_runtime, which a restore calls, and update_gui_zorder)
    Field<&GameState::GuiDrawOrder>("GuiDrawOrder", FieldRole::Derived),
    Field<&GameState::DoOnceTokens>("DoOnceTokens"),
    Field<&GameState::MinimumTextDisplayTimeMs>("MinimumTextDisplayTimeMs"),
    Field<&GameState::IgnoreUserInputAfterTextTimeoutMs>("IgnoreUserInputAfterTextTimeoutMs"),
    // by the "Audio" component
    Field<&GameState::DefaultAudioTypeVolumes>("DefaultAudioTypeVolumes"),
    Field<&GameState::DialogOptionsGuiX>("DialogOptionsGuiX"),
    Field<&GameState::DialogOptionsGuiY>("DialogOptionsGuiY"),
    Field<&GameState::DialogOptionsTextAlignment>("DialogOptionsTextAlignment"),
    Field<&GameState::DialogOptionsFont>("DialogOptionsFont"),
    Field<&GameState::DialogOptionsZOrder>("DialogOptionsZOrder"),
    Field<&GameState::FaceDirectionRatio>("FaceDirectionRatio"),
    // by the "GUI" component
    Field<&GameState::GuiProperties>("GuiProperties"),
    Field<&GameState::GuiControlProperties>("GuiControlProperties"),
    Field<&GameState::HasSpeechVoice>("HasSpeechVoice"),
    Field<&GameState::IsSpeechVoiceBlocking>("IsSpeechVoiceBlocking"),
    // the speech's time on screen after its text or voice (update.cpp); a restore keeps the running game's value
    Field<&GameState::IsSpeechInPostState>("IsSpeechInPostState", FieldRole::Transient),
    // counted as overlays are added and removed; a restore removes them all, which gives the initializer (0), and does
    // not count the restored ones
    Field<&GameState::OverlayCount>("OverlayCount", FieldRole::Transient),
    // the overlays on screen and the speech's script handles (add_screen_overlay, remove_screen_overlay); a restore
    // removes all overlays, which gives them the initializer (0), and the restored overlays do not set them
    Field<&GameState::CompleteOverlayID>("CompleteOverlayID", FieldRole::Transient),
    Field<&GameState::TextOverlayID>("TextOverlayID", FieldRole::Transient),
    Field<&GameState::SpeechTextScriptHandle>("SpeechTextScriptHandle", FieldRole::Transient),
    Field<&GameState::SpeechPortraitScriptHandle>("SpeechPortraitScriptHandle", FieldRole::Transient),
    // from the shake's fields and the loop counter, at every update (update_shakescreen)
    Field<&GameState::ShakeScreenOffsetY>("ShakeScreenOffsetY", FieldRole::Derived),
    // the game's "dev_error_text", translated (InitGameState, a change of translation)
    Field<&GameState::GameErrorText>("GameErrorText", FieldRole::Derived),
    // from the engine's configuration (ApplyBehaviorOptions, which a restore calls)
    Field<&GameState::RBSwitches>("RBSwitches", FieldRole::Derived),
    Field<&GameState::ScreenShaderID>("ScreenShaderID"),
    Field<&GameState::ScreenShaderHandle>("ScreenShaderHandle"),
    Field<&GameState::CursorShaderID>("CursorShaderID"),
    Field<&GameState::CursorShaderHandle>("CursorShaderHandle"),
    // a point in real time (SetIgnoreInput); a restore sets it to now (ClearIgnoreInput), not the initializer (the
    // clock's epoch): either ends the wait
    Field<&GameState::IgnoreUserInputUntilTime>("IgnoreUserInputUntilTime", FieldRole::Transient),
    Field<&GameState::ScriptTimers>("ScriptTimers"),
};
static_assert(FieldsCoverStruct(kGameStateFields), "GameState: a field has no row in kGameStateFields");

} // namespace

FieldTable<GameState> GameState::Fields()
{
    return kGameStateFields;
}

void GameState::SetRBSwitches(const std::array<bool, kNum_RBS> &overrides)
{
    std::copy(overrides.begin(), overrides.end(), RBSwitches.begin());
}

void GameState::SetScreenShader(int shader_id, int shader_handle)
{
    ScreenShaderID = shader_id;
    ScreenShaderHandle = shader_handle;
}

void GameState::SetCursorShader(int shader_id, int shader_handle)
{
    CursorShaderID = shader_id;
    CursorShaderHandle = shader_handle;
}

bool GameState::IsIgnoringInput() const
{
    return Clock::now() < IgnoreUserInputUntilTime;
}

void GameState::SetIgnoreInput(int timeout_ms)
{
    if (Clock::now() + std::chrono::milliseconds(timeout_ms) > IgnoreUserInputUntilTime)
        IgnoreUserInputUntilTime = Clock::now() + std::chrono::milliseconds(timeout_ms);
}

void GameState::ClearIgnoreInput()
{
    IgnoreUserInputUntilTime = Clock::now();
}

bool GameState::IsInWait() const
{
    // wait counter is either
    // * positive, and then there's a wait with timeout;
    // * negative, and then there's a wait without timeout;
    // * zero, which means there's no wait
    return WaitCounter != 0;
}

int GameState::GetWaitCounter() const
{
    return WaitCounter;
}

int GameState::GetWaitSkipType() const
{
    return WaitSkipType;
}

void GameState::SetWaitSkipResult(int skipped_by, int data)
{
    WaitCounter = 0;
    WaitSkippedBy = skipped_by;
    WaitSkippedByData = data;
}

int GameState::GetWaitSkipResult() const
{ // NOTE: we remove timer flag to make timeout reason = 0
    return ((WaitSkippedBy & ~SKIP_AUTOTIMER) << SKIP_RESULT_TYPE_SHIFT) | (WaitSkippedByData & SKIP_RESULT_DATA_MASK);
}

bool GameState::IsBlockingVoiceSpeech() const
{
    return HasSpeechVoice && IsSpeechVoiceBlocking;
}

bool GameState::IsNonBlockingVoiceSpeech() const
{
    return HasSpeechVoice && !IsSpeechVoiceBlocking;
}

bool GameState::ShouldPlayVoiceSpeech() const
{
    return !FastForward && (SpeechVoiceMode != kSpeech_TextOnly) && EngineServices::Current().VoiceAvail;
}

bool GameState::ShouldPlayVoiceSpeechNonBlocking() const
{
    return !FastForward && EngineServices::Current().VoiceAvail;
}

bool GameState::ShouldAASprites() const
{
    return EngineServices::Current().Config.AntialiasSprites && (DisableAntialiasing == 0);
}

void ScriptTimerSet::Start(int timer_id, int timeout)
{
    assert(timer_id >= 0 && timeout > 0);
    if (timer_id >= 0 && timeout > 0)
    {
        _timers[timer_id] = timeout;
    }
}

void ScriptTimerSet::Stop(int timer_id)
{
    assert(timer_id >= 0);
    auto it = _timers.find(timer_id);
    if (it != _timers.end())
        _timers.erase(it);
}

int ScriptTimerSet::GetPos(int timer_id) const
{
    auto it = _timers.find(timer_id);
    if (it != _timers.end())
        return it->second;
    return 0;
}

bool ScriptTimerSet::Check(int timer_id)
{
    // The timer logic is that it counts down to 1, where stays
    // *until* user checks for its expiration in script.
    // Only then the timer is removed.
    auto it = _timers.find(timer_id);
    if (it != _timers.end())
    {
        if (it->second == 1)
        {
            _timers.erase(it);
            return true;
        }
    }
    return false;
}

void ScriptTimerSet::Update()
{
    // The timer logic is that it counts down to 1, where stays
    // *until* user checks for its expiration in script.
    for (auto &timer : _timers)
    {
        if (timer.second > 1)
            timer.second--;
    }
}

void ScriptTimerSet::Restore(int timer_id, int ticks_left)
{
    if (timer_id >= 0 && ticks_left > 0)
        _timers[timer_id] = ticks_left;
}

void GameState::ReadFromSavegame(Stream *in, GameDataVersion /*data_ver*/, GameStateSvgVersion svg_ver,
                                 RestoredData & /*r_data*/)
{
    Room &room = Room::Current();
    in->ReadInt32(); // [DEPRECATED]
    UsedMode = in->ReadInt32();
    UserInterfaceDisabledCount = in->ReadInt32();
    GlobalScriptTimer = in->ReadInt32();
    DebugMode = in->ReadInt32();
    in->Seek(sizeof(int32_t) * LEGACY_MAXGLOBALVARS);
    MessageTimeLeft = in->ReadInt32();
    UsedInventory = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();
    TextReadingSpeed = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    GlobalSpeechAnimationDelay = in->ReadInt32();
    InventoryItemWidth = in->ReadInt32();
    InventoryItemHeight = in->ReadInt32();
    TextShadowColor = in->ReadInt32();
    SwapPortraitSide = in->ReadInt32();
    SpeechTextWindowGui = in->ReadInt32();
    FollowingRoomDelay = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    SkipDisplay = static_cast<SkipSpeechStyle>(in->ReadInt32());
    NoMultiLoopRepeat = in->ReadInt32();
    room.ScriptFinished = in->ReadInt32();
    InventoryActivated = in->ReadInt32();
    NoTextBackgroundWhenVoice = in->ReadInt32();
    MaxDialogOptionWidth = in->ReadInt32();
    NoHighColorFadeIn = in->ReadInt32();
    BackgroundSpeechGameSpeed = in->ReadInt32();
    KeepBackgroundSpeechOnDisplay = in->ReadInt32();
    UnfactorSpeechFromTextLength = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    SpeechMusicDrop = in->ReadInt32();
    RunningCutsceneSkipStyle = in->ReadInt32();
    FastForward = in->ReadInt32();
    room.LoadedWidth = in->ReadInt32();
    room.LoadedHeight = in->ReadInt32();
    GameSpeedModifier = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    PreviousGameData = in->ReadInt32();
    ReplayHotkeyUnused = in->ReadInt32();
    DialogOptionsPaddingX = in->ReadInt32();
    DialogOptionsPaddingY = in->ReadInt32();
    NarratorSpeech = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    LipSyncSpeed = in->ReadInt32();
    SpeechAnimationStopTimeMargin = in->ReadInt32();
    DisableAntialiasing = in->ReadInt32();
    TextReadingSpeedModifier = in->ReadInt32();
    TextAlignment = (HorAlignment)in->ReadInt32();
    SpeechBubbleWidth = in->ReadInt32();
    MinDialogOptionWidth = in->ReadInt32();
    DisableDialogParser = in->ReadInt32();
    room.AnimBackgroundDelay = in->ReadInt32(); // the setting for this room
    TopBarBackColor = in->ReadInt32();
    TopBarTextColor = in->ReadInt32();
    TopBarBorderColor = in->ReadInt32();
    TopBarBorderWidth = in->ReadInt32();
    TopBarY = in->ReadInt32();
    ScreenshotWidth = in->ReadInt32();
    ScreenshotHeight = in->ReadInt32();
    TopBarFont = in->ReadInt32();
    SpeechTextAlignment = (HorAlignment)in->ReadInt32();
    AutoUseWalkToPoints = in->ReadInt32();
    GreyOutInventory = in->ReadInt32();
    SpeechSkipKey = in->ReadInt32();
    AbortKey = in->ReadInt32();
    FadeColorRed = in->ReadInt32();
    FadeColorGreen = in->ReadInt32();
    FadeColorBlue = in->ReadInt32();
    ShowSingleDialogOption = in->ReadInt32();
    KeepScreenDuringInstantTransition = in->ReadInt32();
    ReadDialogOptionColor = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    IsSpeechCustomPortraitPlacement = in->ReadInt32();
    SpeechPortraitOffsetX = in->ReadInt32();
    SpeechPortraitY = in->ReadInt32();
    SpeechDisplayPostTimeMs = in->ReadInt32();
    DialogOptionsHighlightColor = in->ReadInt32();
    RandomSeed = in->ReadInt32();     // random seed
    PlayerRegionID = in->ReadInt32(); // player's current region
    CheckInteractionOnly = in->ReadInt32();
    room.BgFrame = in->ReadInt32();
    BgAnimWait = in->ReadInt32(); // for animating backgrounds
    in->ReadInt32();              // [DEPRECATED]
    WaitCounter = in->ReadInt16();
    MouseBounds.Left = in->ReadInt16();
    MouseBounds.Right = in->ReadInt16();
    MouseBounds.Top = in->ReadInt16();
    MouseBounds.Bottom = in->ReadInt16();
    FadeEffect = in->ReadInt32();
    room.BgFrameLocked = in->ReadInt32();
    in->Seek(sizeof(int32_t) * LEGACY_MAXGSVALUES);
    in->ReadInt32(); // [DEPRECATED]
    in->ReadInt32();
    in->ReadInt32();
    AudioMasterVolume = in->ReadInt32();
    in->Read(room.WalkableAreasOn, MAX_WALK_AREAS);
    ScreenFlip = in->ReadInt16();
    room.EnteredAtX = in->ReadInt32();
    room.EnteredAtY = in->ReadInt32();
    room.EnteredEdge = in->ReadInt32();
    SpeechVoiceMode = (SpeechMode)in->ReadInt32();
    SpeechSkipStyle = in->ReadInt32();
    // Old-style timers, limited to 20, with 1-based ID
    if (svg_ver < kGSSvgVersion_400_26)
    {
        int old_script_timers[LEGACY_MAX_TIMERS];
        in->ReadArrayOfInt32(old_script_timers, LEGACY_MAX_TIMERS);
        for (int timer_id = 1; timer_id < LEGACY_MAX_TIMERS; ++timer_id)
        {
            ScriptTimers.Restore(timer_id, old_script_timers[timer_id]);
        }
    }
    in->ReadInt32(); // [DEPRECATED]
    SpeechVolume = in->ReadInt32();
    NormalFont = in->ReadInt32();
    SpeechFont = in->ReadInt32();
    WaitSkipType = in->ReadInt8();
    SwapPortraitLastCharacter = in->ReadInt32();
    in->ReadInt32(); // separate_music_lib: says which packs the saving engine had, not kept
    ConversationDepth = in->ReadInt32();
    ScreenTint = in->ReadInt32();
    // TODO: investigate if parsed words should be serialized at all
    const size_t parsed_words_count = in->ReadInt32();
    std::array<uint16_t, MAX_PARSED_WORDS> parsed_words_temp;
    in->ReadArrayOfInt16(reinterpret_cast<int16_t *>(parsed_words_temp.data()), MAX_PARSED_WORDS);
    std::copy_n(parsed_words_temp.begin(), parsed_words_count, std::back_inserter(ParsedWords));
    BadParsedWord.ReadCount(in, 100);
    in->ReadInt32();                                 // [DEPRECATED]
    in->Seek(LEGACY_MAXSAVEGAMES * sizeof(int16_t)); // [DEPRECATED]
    uint32_t mouse_opts = in->ReadInt32();
    IsMouseCursorShown = (mouse_opts & 0x01) == 0;  // previously this field had a reverse meaning
    IsMouseInputEnabled = (mouse_opts & 0x02) != 0; // kGSSvgVersion_400_33
    if (svg_ver < kGSSvgVersion_400_33)
    {
        IsMouseInputEnabled = true;
    }
    in->ReadInt32(); // [DEPRECATED]
    in->ReadInt32();
    in->ReadInt32();
    ShakeScreenDelay = in->ReadInt32();
    ShakeScreenAmount = in->ReadInt32();
    ShakeScreenLength = in->ReadInt32();
    AmbientTintRed = in->ReadInt32();
    AmbientTintGreen = in->ReadInt32();
    AmbientTintBlue = in->ReadInt32();
    AmbientTintLevel = in->ReadInt32();
    AmbientTintLight = in->ReadInt32();
    IsAmbientTintEnabled = in->ReadBool();
    if (svg_ver < kGSSvgVersion_400_34)
    {
        // Ambient tint saturation was 0-100 and luminance 0-250,
        // ambient light level was -100 to 100
        if (AmbientTintLevel > 0)
        {
            AmbientTintLevel = GfxDef::Value100ToValue255(AmbientTintLevel);
            AmbientTintLight = GfxDef::Value250ToValue255(AmbientTintLight);
        }
        else
        {
            AmbientTintLight = GfxDef::Value100ToValue255(AmbientTintLight);
        }
        // Screen tint rgb was 0-250
        if (ScreenTint > 0)
        {
            ScreenTint = GfxDef::Value250ToValue255(ScreenTint & 0xFF) |
                         (GfxDef::Value250ToValue255((ScreenTint >> 8) & 0xFF) << 8) |
                         (GfxDef::Value250ToValue255((ScreenTint >> 16) & 0xFF) << 16);
        }
    }
    in->ReadInt32(); // [DEPRECATED]
    SkipUntilCharacterStops = in->ReadInt32();
    in->ReadInt32(); // was LastNamedLocation, should not serialize
    in->ReadInt32(); // was LocationNameSaveCursor, should not serialize
    RestoreCursorModeTo = in->ReadInt32();
    RestoreCursorImageTo = in->ReadInt32();
    in->ReadInt16(); // legacy music queue
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
        in->ReadInt16();
    NewMusicQueueSize = in->ReadInt16();
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        NewMusicQueue[i].ReadFromSavegame(in);
    }

    CrossfadingOutChannel = in->ReadInt16();
    CrossfadeStep = in->ReadInt16();
    CrossfadeOutVolumePerStep = in->ReadInt16();
    CrossfadeInitialVolumeOut = in->ReadInt16();
    CrossfadingInChannel = in->ReadInt16();
    CrossfadeInVolumePerStep = in->ReadInt16();
    CrossfadeFinalVolumeIn = in->ReadInt16();

    in->Seek(50); // [DEPRECATED]
    in->Seek(50); // [DEPRECATED]
    in->Seek(LEGACY_MAXGLOBALSTRINGS * MAX_MAXSTRLEN);
    in->Read(LastParserEntry, MAX_MAXSTRLEN);
    if (svg_ver < kGSSvgVersion_400_03)
    {
        GameName.ReadCount(in, LEGACY_GAMENAMELENGTH);
    }
    else
    {
        GameName = StrUtil::ReadString(in);
    }
    GroundLevelAreasDisabled = in->ReadInt32();
    NextScreenTransition = in->ReadInt32();
    in->ReadInt32(); // GammaAdjustment -- do not apply gamma level from savegame
    TemporarilyTurnedOffCharacter = in->ReadInt16();
    in->ReadInt16(); // [DEPRECATED]
    uint32_t num_do_once_tokens = static_cast<uint32_t>(in->ReadInt32());
    for (size_t i = 0; i < num_do_once_tokens; ++i)
    {
        DoOnceTokens.insert(StrUtil::ReadString(in));
    }
    MinimumTextDisplayTimeMs = in->ReadInt32();
    IgnoreUserInputAfterTextTimeoutMs = in->ReadInt32();
    int voice_speech_flags = in->ReadInt32();
    HasSpeechVoice = voice_speech_flags != 0;
    IsSpeechVoiceBlocking = (voice_speech_flags & 0x02) != 0;

    if (svg_ver >= kGSSvgVersion_400_17)
    {
        DialogOptionsGuiX = in->ReadInt32();
        DialogOptionsGuiY = in->ReadInt32();
        DialogOptionsTextAlignment = (HorAlignment)in->ReadInt32();
        DialogOptionsFont = in->ReadInt32();
    }
    else
    {
        DialogOptionsGuiX = -1;
        DialogOptionsGuiY = -1;
        DialogOptionsTextAlignment = kHAlignLeft;
        DialogOptionsFont = FONT_UNDEFINED;
    }

    if (svg_ver >= kGSSvgVersion_400_24)
    {
        DialogOptionsZOrder = in->ReadInt32();
        SpeechOverlayZOrder = in->ReadInt32(); // since kGSSvgVersion_400_33
        uint32_t speech_ex_flags = in->ReadInt32();
        AlwaysWaitForSpeechText = speech_ex_flags & 0x01;
        in->ReadInt32(); // reserved
    }

    if (svg_ver < kGSSvgVersion_400_33)
    {
        SpeechOverlayZOrder = INT32_MAX;
        AlwaysWaitForSpeechText = false;
    }

    // New-style timers
    if (svg_ver >= kGSSvgVersion_400_26)
    {
        uint32_t timer_count = in->ReadInt32();
        ScriptTimers.Clear();
        for (uint32_t i = 0; i < timer_count; ++i)
        {
            int timer_id = in->ReadInt32();
            int timer_time = in->ReadInt32();
            ScriptTimers.Restore(timer_id, timer_time);
        }
    }

    if (svg_ver >= kGSSvgVersion_400_03)
    {
        FaceDirectionRatio = in->ReadFloat32();
        // used since kGSSvgVersion_400_18
        ScreenShaderID = in->ReadInt32();
        ScreenShaderHandle = in->ReadInt32();
        CursorShaderID = in->ReadInt32();
        CursorShaderHandle = in->ReadInt32();
        // reserve few more 32-bit values (for a total of 10)
        in->Seek(sizeof(int32_t) * 5);
    }
    else
    {
        FaceDirectionRatio = 1.f;
        ScreenShaderID = 0;
        ScreenShaderHandle = 0;
        CursorShaderID = 0;
        CursorShaderHandle = 0;
    }
}

void GameState::WriteForSavegame(Stream *out) const
{
    const Room &room = Room::Current();
    // NOTE: the fields that are never saved here are listed above the GameState struct;
    // the order below is the save layout, ReadFromSavegame reads the same
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(UsedMode);
    out->WriteInt32(UserInterfaceDisabledCount);
    out->WriteInt32(GlobalScriptTimer);
    out->WriteInt32(DebugMode);
    out->WriteByteCount(0, sizeof(int32_t) * LEGACY_MAXGLOBALVARS);
    out->WriteInt32(MessageTimeLeft);
    out->WriteInt32(UsedInventory);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(TextReadingSpeed);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(GlobalSpeechAnimationDelay);
    out->WriteInt32(InventoryItemWidth);
    out->WriteInt32(InventoryItemHeight);
    out->WriteInt32(TextShadowColor);
    out->WriteInt32(SwapPortraitSide);
    out->WriteInt32(SpeechTextWindowGui);
    out->WriteInt32(FollowingRoomDelay);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(SkipDisplay);
    out->WriteInt32(NoMultiLoopRepeat);
    out->WriteInt32(room.ScriptFinished);
    out->WriteInt32(InventoryActivated);
    out->WriteInt32(NoTextBackgroundWhenVoice);
    out->WriteInt32(MaxDialogOptionWidth);
    out->WriteInt32(NoHighColorFadeIn);
    out->WriteInt32(BackgroundSpeechGameSpeed);
    out->WriteInt32(KeepBackgroundSpeechOnDisplay);
    out->WriteInt32(UnfactorSpeechFromTextLength);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(SpeechMusicDrop);
    out->WriteInt32(RunningCutsceneSkipStyle);
    out->WriteInt32(FastForward);
    out->WriteInt32(room.LoadedWidth);
    out->WriteInt32(room.LoadedHeight);
    out->WriteInt32(GameSpeedModifier);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(PreviousGameData);
    out->WriteInt32(ReplayHotkeyUnused); // StartRecording: not supported
    out->WriteInt32(DialogOptionsPaddingX);
    out->WriteInt32(DialogOptionsPaddingY);
    out->WriteInt32(NarratorSpeech);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(LipSyncSpeed);
    out->WriteInt32(SpeechAnimationStopTimeMargin);
    out->WriteInt32(DisableAntialiasing);
    out->WriteInt32(TextReadingSpeedModifier);
    out->WriteInt32(TextAlignment);
    out->WriteInt32(SpeechBubbleWidth);
    out->WriteInt32(MinDialogOptionWidth);
    out->WriteInt32(DisableDialogParser);
    out->WriteInt32(room.AnimBackgroundDelay); // the setting for this room
    out->WriteInt32(TopBarBackColor);
    out->WriteInt32(TopBarTextColor);
    out->WriteInt32(TopBarBorderColor);
    out->WriteInt32(TopBarBorderWidth);
    out->WriteInt32(TopBarY);
    out->WriteInt32(ScreenshotWidth);
    out->WriteInt32(ScreenshotHeight);
    out->WriteInt32(TopBarFont);
    out->WriteInt32(SpeechTextAlignment);
    out->WriteInt32(AutoUseWalkToPoints);
    out->WriteInt32(GreyOutInventory);
    out->WriteInt32(SpeechSkipKey);
    out->WriteInt32(AbortKey);
    out->WriteInt32(FadeColorRed);
    out->WriteInt32(FadeColorGreen);
    out->WriteInt32(FadeColorBlue);
    out->WriteInt32(ShowSingleDialogOption);
    out->WriteInt32(KeepScreenDuringInstantTransition);
    out->WriteInt32(ReadDialogOptionColor);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(IsSpeechCustomPortraitPlacement);
    out->WriteInt32(SpeechPortraitOffsetX);
    out->WriteInt32(SpeechPortraitY);
    out->WriteInt32(SpeechDisplayPostTimeMs);
    out->WriteInt32(DialogOptionsHighlightColor);
    // ** up to here is referenced in the script "game." object
    out->WriteInt32(RandomSeed);     // random seed
    out->WriteInt32(PlayerRegionID); // player's current region
    out->WriteInt32(CheckInteractionOnly);
    out->WriteInt32(room.BgFrame);
    out->WriteInt32(BgAnimWait); // for animating backgrounds
    out->WriteInt32(0);          // [DEPRECATED]
    out->WriteInt16(WaitCounter);
    out->WriteInt16(MouseBounds.Left);
    out->WriteInt16(MouseBounds.Right);
    out->WriteInt16(MouseBounds.Top);
    out->WriteInt16(MouseBounds.Bottom);
    out->WriteInt32(FadeEffect);
    out->WriteInt32(room.BgFrameLocked);
    out->WriteByteCount(0, sizeof(int32_t) * LEGACY_MAXGSVALUES);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(AudioMasterVolume);
    out->Write(room.WalkableAreasOn, MAX_WALK_AREAS);
    out->WriteInt16(ScreenFlip);
    out->WriteInt32(room.EnteredAtX);
    out->WriteInt32(room.EnteredAtY);
    out->WriteInt32(room.EnteredEdge);
    out->WriteInt32(SpeechVoiceMode);
    out->WriteInt32(SpeechSkipStyle);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(SpeechVolume);
    out->WriteInt32(NormalFont);
    out->WriteInt32(SpeechFont);
    out->WriteInt8(WaitSkipType);
    out->WriteInt32(SwapPortraitLastCharacter);
    out->WriteInt32(EngineServices::Current().SeparateMusicLib ? 1 : 0);
    out->WriteInt32(ConversationDepth);
    out->WriteInt32(ScreenTint);
    // TODO: investigate if parsed words should be serialized at all
    out->WriteInt32(ParsedWords.size());
    out->WriteArrayOfInt16(reinterpret_cast<const int16_t *>(ParsedWords.data()),
                           std::min<size_t>(ParsedWords.size(), MAX_PARSED_WORDS));
    if (ParsedWords.size() < MAX_PARSED_WORDS)
        out->WriteByteCount(0, (MAX_PARSED_WORDS - ParsedWords.size()) * sizeof(int16_t));
    BadParsedWord.WriteCount(out, 100);
    out->WriteInt32(0);                                            // [DEPRECATED]
    out->WriteByteCount(0, LEGACY_MAXSAVEGAMES * sizeof(int16_t)); // [DEPRECATED]
    uint32_t mouse_opts = 0x01 * (!IsMouseCursorShown)             // previously this field had a reverse meaning
                          | 0x02 * IsMouseInputEnabled;            // kGSSvgVersion_400_33
    out->WriteInt32(mouse_opts);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(ShakeScreenDelay);
    out->WriteInt32(ShakeScreenAmount);
    out->WriteInt32(ShakeScreenLength);
    out->WriteInt32(AmbientTintRed);
    out->WriteInt32(AmbientTintGreen);
    out->WriteInt32(AmbientTintBlue);
    out->WriteInt32(AmbientTintLevel);
    out->WriteInt32(AmbientTintLight);
    out->WriteBool(IsAmbientTintEnabled);
    out->WriteInt32(0); // [DEPRECATED]
    out->WriteInt32(SkipUntilCharacterStops);
    out->WriteInt32(0); // was LastNamedLocation, should not serialize
    out->WriteInt32(0); // was LocationNameSaveCursor, should not serialize
    out->WriteInt32(RestoreCursorModeTo);
    out->WriteInt32(RestoreCursorImageTo);
    out->WriteInt16(0); // legacy music queue
    out->WriteByteCount(0, sizeof(int16_t) * MAX_QUEUED_MUSIC);
    out->WriteInt16(NewMusicQueueSize);
    for (int i = 0; i < MAX_QUEUED_MUSIC; ++i)
    {
        NewMusicQueue[i].WriteToSavegame(out);
    }

    out->WriteInt16(CrossfadingOutChannel);
    out->WriteInt16(CrossfadeStep);
    out->WriteInt16(CrossfadeOutVolumePerStep);
    out->WriteInt16(CrossfadeInitialVolumeOut);
    out->WriteInt16(CrossfadingInChannel);
    out->WriteInt16(CrossfadeInVolumePerStep);
    out->WriteInt16(CrossfadeFinalVolumeIn);

    out->WriteByteCount(0, 50); // [DEPRECATED]
    out->WriteByteCount(0, 50); // [DEPRECATED]
    out->WriteByteCount(0, LEGACY_MAXGLOBALSTRINGS * MAX_MAXSTRLEN);
    out->Write(LastParserEntry, MAX_MAXSTRLEN);
    StrUtil::WriteString(GameName, out);
    out->WriteInt32(GroundLevelAreasDisabled);
    out->WriteInt32(NextScreenTransition);
    out->WriteInt32(GammaAdjustment);
    out->WriteInt16(TemporarilyTurnedOffCharacter);
    out->WriteInt16(0); // [DEPRECATED]
    out->WriteInt32(static_cast<uint32_t>(DoOnceTokens.size()));
    for (const auto &token : DoOnceTokens)
    {
        StrUtil::WriteString(token, out);
    }
    out->WriteInt32(MinimumTextDisplayTimeMs);
    out->WriteInt32(IgnoreUserInputAfterTextTimeoutMs);

    int voice_speech_flags = HasSpeechVoice ? 0x01 : 0;
    if (IsSpeechVoiceBlocking)
        voice_speech_flags |= 0x02;
    out->WriteInt32(voice_speech_flags);

    // kGSSvgVersion_400_17
    out->WriteInt32(DialogOptionsGuiX);
    out->WriteInt32(DialogOptionsGuiY);
    out->WriteInt32(DialogOptionsTextAlignment);
    out->WriteInt32(DialogOptionsFont);
    // kGSSvgVersion_400_24
    out->WriteInt32(DialogOptionsZOrder);
    out->WriteInt32(SpeechOverlayZOrder); // since kGSSvgVersion_400_33
    uint32_t speech_ex_flags = (0x01 * AlwaysWaitForSpeechText);
    out->WriteInt32(speech_ex_flags);
    out->WriteInt32(0); // reserved

    // kGSSvgVersion_400_26
    out->WriteInt32(ScriptTimers.Running().size());
    for (const auto &timer : ScriptTimers.Running())
    {
        out->WriteInt32(timer.first);
        out->WriteInt32(timer.second);
    }

    // kGSSvgVersion_400_03
    out->WriteFloat32(FaceDirectionRatio);
    out->WriteInt32(ScreenShaderID); // used since kGSSvgVersion_400_18
    out->WriteInt32(ScreenShaderHandle);
    out->WriteInt32(CursorShaderID);
    out->WriteInt32(CursorShaderHandle);
    // reserve few more 32-bit values (for a total of 10)
    out->WriteByteCount(0, sizeof(int32_t) * 5);
}

void GameState::FreeProperties()
{
    for (auto &p : GuiProperties)
        p.clear();
    for (auto &list : GuiControlProperties)
    {
        for (auto &p : list)
            p.clear();
    }
}
