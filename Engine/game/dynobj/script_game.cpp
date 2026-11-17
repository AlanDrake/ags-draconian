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
#include "game/dynobj/script_game.h"

#include "debug/debug_log.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/ui/gui.h"
#include "game/world/room_context.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "script/cc_common.h" // cc_error

using namespace AGS::Engine;

CCScriptGame GameStaticManager;


int32_t CCScriptGame::ReadInt32(const void *address, intptr_t offset)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    const int index = offset / sizeof(int32_t);
    if (index >= 5 && index < 5 + GameState::LEGACY_MAXGLOBALVARS)
        return 0; // [DEPRECATED]

    switch (index)
    {
    case 0: return 0; // [DEPRECATED]
    case 1: return game.Play.UsedMode;
    case 2: return game.Play.UserInterfaceDisabledCount;
    case 3: return game.Play.GlobalScriptTimer;
    case 4: return game.Play.DebugMode;
    // 5 -> 54: game.Play.globalvars
    case 55: return game.Play.MessageTimeLeft;
    case 56: return game.Play.UsedInventory;
    case 57: return 0; // [DEPRECATED]
    case 58: return 0; // [DEPRECATED]
    case 59: return 0; // [DEPRECATED]
    case 60: return 0; // [DEPRECATED]
    case 61: return game.Play.TextReadingSpeed;
    case 62: return 0; // [DEPRECATED]
    case 63: return game.Play.GlobalSpeechAnimationDelay;
    case 64: return game.Play.InventoryItemWidth;
    case 65: return game.Play.InventoryItemHeight;
    case 66: return game.Play.TextShadowColor;
    case 67: return game.Play.SwapPortraitSide;
    case 68: return game.Play.SpeechTextWindowGui;
    case 69: return game.Play.FollowingRoomDelay;
    case 70: return 0; // [DEPRECATED]
    case 71: return game.Play.SkipDisplay;
    case 72: return game.Play.NoMultiLoopRepeat;
    case 73: return room.ScriptFinished;
    case 74: return game.Play.InventoryActivated;
    case 75: return game.Play.NoTextBackgroundWhenVoice;
    case 76: return game.Play.MaxDialogOptionWidth;
    case 77: return game.Play.NoHighColorFadeIn;
    case 78: return game.Play.BackgroundSpeechGameSpeed;
    case 79: return game.Play.KeepBackgroundSpeechOnDisplay;
    case 80: return game.Play.UnfactorSpeechFromTextLength;
    case 81: return 0; // [DEPRECATED]
    case 82: return game.Play.SpeechMusicDrop;
    case 83: return game.Play.RunningCutsceneSkipStyle;
    case 84: return game.Play.FastForward;
    case 85: return room.LoadedWidth;
    case 86: return room.LoadedHeight;
    case 87: return game.Play.GameSpeedModifier;
    case 88: return 0; // [DEPRECATED]
    case 89: return game.Play.PreviousGameData;
    case 90: return 0; // [DEPRECATED]
    case 91: return game.Play.DialogOptionsPaddingX;
    case 92: return game.Play.DialogOptionsPaddingY;
    case 93: return game.Play.NarratorSpeech;
    case 94: return 0; // [DEPRECATED]
    case 95: return game.Play.LipSyncSpeed;
    case 96: return game.Play.SpeechAnimationStopTimeMargin;
    case 97: return game.Play.DisableAntialiasing;
    case 98: return game.Play.TextReadingSpeedModifier;
    case 99: return game.Play.TextAlignment;
    case 100: return game.Play.SpeechBubbleWidth;
    case 101: return game.Play.MinDialogOptionWidth;
    case 102: return game.Play.DisableDialogParser;
    case 103: return room.AnimBackgroundDelay;
    case 104: return game.Play.TopBarBackColor;
    case 105: return game.Play.TopBarTextColor;
    case 106: return game.Play.TopBarBorderColor;
    case 107: return game.Play.TopBarBorderWidth;
    case 108: return game.Play.TopBarY;
    case 109: return game.Play.ScreenshotWidth;
    case 110: return game.Play.ScreenshotHeight;
    case 111: return game.Play.TopBarFont;
    case 112: return game.Play.SpeechTextAlignment;
    case 113: return game.Play.AutoUseWalkToPoints;
    case 114: return game.Play.GreyOutInventory;
    case 115: return game.Play.SpeechSkipKey;
    case 116: return game.Play.AbortKey;
    case 117: return game.Play.FadeColorRed;
    case 118: return game.Play.FadeColorGreen;
    case 119: return game.Play.FadeColorBlue;
    case 120: return game.Play.ShowSingleDialogOption;
    case 121: return game.Play.KeepScreenDuringInstantTransition;
    case 122: return game.Play.ReadDialogOptionColor;
    case 123: return 0; // [DEPRECATED]
    case 124: return game.Play.IsSpeechCustomPortraitPlacement;
    case 125: return game.Play.SpeechPortraitOffsetX;
    case 126: return game.Play.SpeechPortraitY;
    case 127: return game.Play.SpeechDisplayPostTimeMs;
    case 128: return game.Play.DialogOptionsHighlightColor;
    default: cc_error("ScriptGame: unsupported variable offset %" PRIdPTR, offset); return 0;
    }
}

void CCScriptGame::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    Room &room = Room::Current();
    const int index = offset / sizeof(int32_t);
    if (index >= 5 && index < 5 + GameState::LEGACY_MAXGLOBALVARS)
        return; // [DEPRECATED]

    switch (index)
    {
    case 0: break; // [DEPRECATED]
    case 1: game.Play.UsedMode = val; break;
    case 2: game.Play.UserInterfaceDisabledCount = val; break;
    case 3: game.Play.GlobalScriptTimer = val; break;
    case 4:
        set_debug_mode(val != 0);
        break; // game.Play.DebugMode
    // 5 -> 54: game.Play.globalvars
    case 55: game.Play.MessageTimeLeft = val; break;
    case 56: game.Play.UsedInventory = val; break;
    case 57: break; // [DEPRECATED]
    case 58: break; // [DEPRECATED]
    case 59: break; // [DEPRECATED]
    case 60: break; // [DEPRECATED]
    case 61:
        if (services.Config.Access.TextReadSpeed <= 0)
            game.Play.TextReadingSpeed = val;
        break;
    case 62: break; // [DEPRECATED]
    case 63: game.Play.GlobalSpeechAnimationDelay = val; break;
    case 64: game.Play.InventoryItemWidth = val; break;
    case 65: game.Play.InventoryItemHeight = val; break;
    case 66: game.Play.TextShadowColor = val; break;
    case 67: game.Play.SwapPortraitSide = val; break;
    case 68: game.Play.SpeechTextWindowGui = val; break;
    case 69: game.Play.FollowingRoomDelay = val; break;
    case 70: break; // [DEPRECATED]
    case 71:
        if (services.Config.Access.TextSkipStyle == kSkipSpeechNone)
            game.Play.SkipDisplay = static_cast<SkipSpeechStyle>(val);
        break;
    case 72: game.Play.NoMultiLoopRepeat = val; break;
    case 73: room.ScriptFinished = val; break;
    case 74: game.Play.InventoryActivated = val; break;
    case 75: game.Play.NoTextBackgroundWhenVoice = val; break;
    case 76: game.Play.MaxDialogOptionWidth = val; break;
    case 77: game.Play.NoHighColorFadeIn = val; break;
    case 78: game.Play.BackgroundSpeechGameSpeed = val; break;
    case 79: game.Play.KeepBackgroundSpeechOnDisplay = val; break;
    case 80: game.Play.UnfactorSpeechFromTextLength = val; break;
    case 81: break; // [DEPRECATED]
    case 82: game.Play.SpeechMusicDrop = val; break;
    case 83: // game.Play.RunningCutsceneSkipStyle
    case 84: // game.Play.FastForward;
    case 85: // Room::LoadedWidth;
    case 86: // Room::LoadedHeight;
        debug_script_warn("ScriptGame: attempt to write in readonly variable at offset %" PRIdPTR ", value %d", offset,
                          val);
        break;
    case 87: game.Play.GameSpeedModifier = val; break;
    case 88: break; // [DEPRECATED]
    case 89: game.Play.PreviousGameData = val; break;
    case 90: break; // [DEPRECATED]
    case 91: game.Play.DialogOptionsPaddingX = val; break;
    case 92: game.Play.DialogOptionsPaddingY = val; break;
    case 93: game.Play.NarratorSpeech = val; break;
    case 94: break; // [DEPRECATED]
    case 95: game.Play.LipSyncSpeed = val; break;
    case 96: game.Play.SpeechAnimationStopTimeMargin = val; break;
    case 97: game.Play.DisableAntialiasing = val; break;
    case 98: game.Play.TextReadingSpeedModifier = val; break;
    case 99: game.Play.TextAlignment = (HorAlignment)val; break;
    case 100: game.Play.SpeechBubbleWidth = val; break;
    case 101: game.Play.MinDialogOptionWidth = val; break;
    case 102: game.Play.DisableDialogParser = val; break;
    case 103: room.AnimBackgroundDelay = val; break;
    case 104: game.Play.TopBarBackColor = val; break;
    case 105: game.Play.TopBarTextColor = val; break;
    case 106: game.Play.TopBarBorderColor = val; break;
    case 107: game.Play.TopBarBorderWidth = val; break;
    case 108: game.Play.TopBarY = val; break;
    case 109: game.Play.ScreenshotWidth = val; break;
    case 110: game.Play.ScreenshotHeight = val; break;
    case 111: game.Play.TopBarFont = val; break;
    case 112: game.Play.SpeechTextAlignment = (HorAlignment)val; break;
    case 113: game.Play.AutoUseWalkToPoints = val; break;
    case 114: game.Play.GreyOutInventory = val; break;
    case 115: game.Play.SpeechSkipKey = val; break;
    case 116: game.Play.AbortKey = val; break;
    case 117: // game.Play.FadeColorRed;
    case 118: // game.Play.FadeColorGreen;
    case 119: // game.Play.FadeColorBlue;
        debug_script_warn("ScriptGame: attempt to write in readonly variable at offset %" PRIdPTR ", value %d", offset,
                          val);
        break;
    case 120:
        game.Play.ShowSingleDialogOption = val;
        game.Options[OPT_DISPLAYSINGLEDIALOGOPTION] = val;
        break;
    case 121: game.Play.KeepScreenDuringInstantTransition = val; break;
    case 122: game.Play.ReadDialogOptionColor = val; break;
    case 123: break; // [DEPRECATED]
    case 124: game.Play.IsSpeechCustomPortraitPlacement = val; break;
    case 125: game.Play.SpeechPortraitOffsetX = val; break;
    case 126: game.Play.SpeechPortraitY = val; break;
    case 127: game.Play.SpeechDisplayPostTimeMs = val; break;
    case 128: game.Play.DialogOptionsHighlightColor = val; break;
    default: cc_error("ScriptGame: unsupported variable offset %" PRIdPTR, offset); break;
    }
}
