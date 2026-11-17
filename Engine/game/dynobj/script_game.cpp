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
    case 1: return game.Play.usedmode;
    case 2: return game.Play.disabled_user_interface;
    case 3: return game.Play.gscript_timer;
    case 4: return game.Play.debug_mode;
    // 5 -> 54: game.Play.globalvars
    case 55: return game.Play.messagetime;
    case 56: return game.Play.usedinv;
    case 57: return 0; // [DEPRECATED]
    case 58: return 0; // [DEPRECATED]
    case 59: return 0; // [DEPRECATED]
    case 60: return 0; // [DEPRECATED]
    case 61: return game.Play.text_speed;
    case 62: return 0; // [DEPRECATED]
    case 63: return game.Play.talkanim_speed;
    case 64: return game.Play.inv_item_wid;
    case 65: return game.Play.inv_item_hit;
    case 66: return game.Play.speech_text_shadow;
    case 67: return game.Play.swap_portrait_side;
    case 68: return game.Play.speech_textwindow_gui;
    case 69: return game.Play.follow_change_room_timer;
    case 70: return 0; // [DEPRECATED]
    case 71: return game.Play.skip_display;
    case 72: return game.Play.no_multiloop_repeat;
    case 73: return room.ScriptFinished;
    case 74: return game.Play.used_inv_on;
    case 75: return game.Play.no_textbg_when_voice;
    case 76: return game.Play.max_dialogoption_width;
    case 77: return game.Play.no_hicolor_fadein;
    case 78: return game.Play.bgspeech_game_speed;
    case 79: return game.Play.bgspeech_stay_on_display;
    case 80: return game.Play.unfactor_speech_from_textlength;
    case 81: return 0; // [DEPRECATED]
    case 82: return game.Play.speech_music_drop;
    case 83: return game.Play.in_cutscene;
    case 84: return game.Play.fast_forward;
    case 85: return room.LoadedWidth;
    case 86: return room.LoadedHeight;
    case 87: return game.Play.game_speed_modifier;
    case 88: return 0; // [DEPRECATED]
    case 89: return game.Play.takeover_data;
    case 90: return 0; // [DEPRECATED]
    case 91: return game.Play.dialog_options_pad_x;
    case 92: return game.Play.dialog_options_pad_y;
    case 93: return game.Play.narrator_speech;
    case 94: return 0; // [DEPRECATED]
    case 95: return game.Play.lipsync_speed;
    case 96: return game.Play.close_mouth_speech_time;
    case 97: return game.Play.disable_antialiasing;
    case 98: return game.Play.text_speed_modifier;
    case 99: return game.Play.text_align;
    case 100: return game.Play.speech_bubble_width;
    case 101: return game.Play.min_dialogoption_width;
    case 102: return game.Play.disable_dialog_parser;
    case 103: return room.AnimBackgroundDelay;
    case 104: return game.Play.top_bar_backcolor;
    case 105: return game.Play.top_bar_textcolor;
    case 106: return game.Play.top_bar_bordercolor;
    case 107: return game.Play.top_bar_borderwidth;
    case 108: return game.Play.top_bar_ypos;
    case 109: return game.Play.screenshot_width;
    case 110: return game.Play.screenshot_height;
    case 111: return game.Play.top_bar_font;
    case 112: return game.Play.speech_text_align;
    case 113: return game.Play.auto_use_walkto_points;
    case 114: return game.Play.inventory_greys_out;
    case 115: return game.Play.skip_speech_specific_key;
    case 116: return game.Play.abort_key;
    case 117: return game.Play.fade_to_red;
    case 118: return game.Play.fade_to_green;
    case 119: return game.Play.fade_to_blue;
    case 120: return game.Play.show_single_dialog_option;
    case 121: return game.Play.keep_screen_during_instant_transition;
    case 122: return game.Play.read_dialog_option_colour;
    case 123: return 0; // [DEPRECATED]
    case 124: return game.Play.speech_portrait_placement;
    case 125: return game.Play.speech_portrait_x;
    case 126: return game.Play.speech_portrait_y;
    case 127: return game.Play.speech_display_post_time_ms;
    case 128: return game.Play.dialog_options_highlight_color;
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
    case 1: game.Play.usedmode = val; break;
    case 2: game.Play.disabled_user_interface = val; break;
    case 3: game.Play.gscript_timer = val; break;
    case 4:
        set_debug_mode(val != 0);
        break; // game.Play.debug_mode
    // 5 -> 54: game.Play.globalvars
    case 55: game.Play.messagetime = val; break;
    case 56: game.Play.usedinv = val; break;
    case 57: break; // [DEPRECATED]
    case 58: break; // [DEPRECATED]
    case 59: break; // [DEPRECATED]
    case 60: break; // [DEPRECATED]
    case 61:
        if (services.Config.Access.TextReadSpeed <= 0)
            game.Play.text_speed = val;
        break;
    case 62: break; // [DEPRECATED]
    case 63: game.Play.talkanim_speed = val; break;
    case 64: game.Play.inv_item_wid = val; break;
    case 65: game.Play.inv_item_hit = val; break;
    case 66: game.Play.speech_text_shadow = val; break;
    case 67: game.Play.swap_portrait_side = val; break;
    case 68: game.Play.speech_textwindow_gui = val; break;
    case 69: game.Play.follow_change_room_timer = val; break;
    case 70: break; // [DEPRECATED]
    case 71:
        if (services.Config.Access.TextSkipStyle == kSkipSpeechNone)
            game.Play.skip_display = static_cast<SkipSpeechStyle>(val);
        break;
    case 72: game.Play.no_multiloop_repeat = val; break;
    case 73: room.ScriptFinished = val; break;
    case 74: game.Play.used_inv_on = val; break;
    case 75: game.Play.no_textbg_when_voice = val; break;
    case 76: game.Play.max_dialogoption_width = val; break;
    case 77: game.Play.no_hicolor_fadein = val; break;
    case 78: game.Play.bgspeech_game_speed = val; break;
    case 79: game.Play.bgspeech_stay_on_display = val; break;
    case 80: game.Play.unfactor_speech_from_textlength = val; break;
    case 81: break; // [DEPRECATED]
    case 82: game.Play.speech_music_drop = val; break;
    case 83: // game.Play.in_cutscene
    case 84: // game.Play.fast_forward;
    case 85: // Room::LoadedWidth;
    case 86: // Room::LoadedHeight;
        debug_script_warn("ScriptGame: attempt to write in readonly variable at offset %" PRIdPTR ", value %d", offset,
                          val);
        break;
    case 87: game.Play.game_speed_modifier = val; break;
    case 88: break; // [DEPRECATED]
    case 89: game.Play.takeover_data = val; break;
    case 90: break; // [DEPRECATED]
    case 91: game.Play.dialog_options_pad_x = val; break;
    case 92: game.Play.dialog_options_pad_y = val; break;
    case 93: game.Play.narrator_speech = val; break;
    case 94: break; // [DEPRECATED]
    case 95: game.Play.lipsync_speed = val; break;
    case 96: game.Play.close_mouth_speech_time = val; break;
    case 97: game.Play.disable_antialiasing = val; break;
    case 98: game.Play.text_speed_modifier = val; break;
    case 99: game.Play.text_align = (HorAlignment)val; break;
    case 100: game.Play.speech_bubble_width = val; break;
    case 101: game.Play.min_dialogoption_width = val; break;
    case 102: game.Play.disable_dialog_parser = val; break;
    case 103: room.AnimBackgroundDelay = val; break;
    case 104: game.Play.top_bar_backcolor = val; break;
    case 105: game.Play.top_bar_textcolor = val; break;
    case 106: game.Play.top_bar_bordercolor = val; break;
    case 107: game.Play.top_bar_borderwidth = val; break;
    case 108: game.Play.top_bar_ypos = val; break;
    case 109: game.Play.screenshot_width = val; break;
    case 110: game.Play.screenshot_height = val; break;
    case 111: game.Play.top_bar_font = val; break;
    case 112: game.Play.speech_text_align = (HorAlignment)val; break;
    case 113: game.Play.auto_use_walkto_points = val; break;
    case 114: game.Play.inventory_greys_out = val; break;
    case 115: game.Play.skip_speech_specific_key = val; break;
    case 116: game.Play.abort_key = val; break;
    case 117: // game.Play.fade_to_red;
    case 118: // game.Play.fade_to_green;
    case 119: // game.Play.fade_to_blue;
        debug_script_warn("ScriptGame: attempt to write in readonly variable at offset %" PRIdPTR ", value %d", offset,
                          val);
        break;
    case 120:
        game.Play.show_single_dialog_option = val;
        game.Options[OPT_DISPLAYSINGLEDIALOGOPTION] = val;
        break;
    case 121: game.Play.keep_screen_during_instant_transition = val; break;
    case 122: game.Play.read_dialog_option_colour = val; break;
    case 123: break; // [DEPRECATED]
    case 124: game.Play.speech_portrait_placement = val; break;
    case 125: game.Play.speech_portrait_x = val; break;
    case 126: game.Play.speech_portrait_y = val; break;
    case 127: game.Play.speech_display_post_time_ms = val; break;
    case 128: game.Play.dialog_options_highlight_color = val; break;
    default: cc_error("ScriptGame: unsupported variable offset %" PRIdPTR, offset); break;
    }
}
