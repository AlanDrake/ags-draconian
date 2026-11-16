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
#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "game/dialog/speech.h"
#include "game/game_struct_defines.h"
#include "game/runtime_defines.h"
#include "game/world/move_list.h"
#include "gfx/graphicsdriver.h"
#include "gui/guidefines.h"
#include "media/audio/queuedaudioitem.h"
#include "util/geometry.h"
#include "util/keycode.h"
#include "util/string.h"
#include "util/string_types.h"
#include "util/timer.h"

// Forward declaration
namespace AGS
{
namespace Common
{
class Bitmap;
class Stream;
typedef std::shared_ptr<Bitmap> PBitmap;
} // namespace Common
namespace Engine
{
struct RestoredData;
}
} // namespace AGS
struct ScriptOverlay;

// The game state's save format (GameDef's saved fields and GamePlayState)
// Before 4.0: 0 to 3 (3.5.0 to 3.5.0.10), 4 (3.6.1.14), then 3060300, 3060302, 3060304, 3060313 (3.6.3).
enum GameStateSvgVersion
{
    kGSSvgVersion_400 = 4000000,
    kGSSvgVersion_400_03 = 4000003, // compat w 3.6.1.14 (game name of any length)
    kGSSvgVersion_400_08 = 4000008, // palette component range 64->256
    kGSSvgVersion_400_14 = 4000014, // 32-bit ARGB color properties
    kGSSvgVersion_400_17 = 4000017, // compat w 3.6.3.0 (dialog options position, alignment, font)
    kGSSvgVersion_400_18 = 4000018, // shaders
    kGSSvgVersion_400_24 = 4000024, // sync with 3.6.3.2 (dialog options z-order)
    kGSSvgVersion_400_26 = 4000026, // sync with 3.6.3.4 (script timers of any number)
    kGSSvgVersion_400_27 = 4000027, // 32-bit inventory cursor hotspot colors
    kGSSvgVersion_400_29 = 4000029, // rotation pivot
    kGSSvgVersion_400_33 = 4000033, // sync with 3.6.3.13 (mouse input enabled, speech z-order)
    kGSSvgVersion_400_34 = 4000034, // full ranges for ambient tint, light level and screen tint
    kGSSvgVersion_Current = kGSSvgVersion_400_34
};

// A location reference, used to remember where the mouse cursor was.
struct SceneLocationRef
{
    int LocationType = LOCTYPE_NOTHING;
    int ObjectIndex = -1;

    SceneLocationRef() = default;
    SceneLocationRef(int loc_type, int obj_index = -1) : LocationType(loc_type), ObjectIndex(obj_index) {}
    inline bool IsDefined() const { return LocationType != LOCTYPE_NOTHING && ObjectIndex >= 0; }
    bool operator==(const SceneLocationRef &ref) const
    {
        return LocationType == ref.LocationType && ObjectIndex == ref.ObjectIndex;
    }
    bool operator!=(const SceneLocationRef &ref) const
    {
        return LocationType != ref.LocationType || ObjectIndex != ref.ObjectIndex;
    }
};


// A GameState is a parent class for the game states.
class GameState
{
public:
    using String = AGS::Common::String;

    virtual ~GameState() = default;

    // Begin the state, initialize and prepare any resources
    virtual void Begin() = 0;
    // End the state, does final actions, releases all resources
    virtual void End() = 0;
    // Draw the state
    virtual void Draw() = 0;
    // Update the state during a game tick;
    // returns whether should continue to run state loop, or stop
    virtual bool Run() = 0;

    // Pause the state, makes sure that all related timers, threads etc
    // are also paused, avoiding any unexpected updates
    virtual void Pause() { /* do nothing */ };
    // Resume the state after pausing
    virtual void Resume() { /* do nothing */ };
};


// Runtime game state, held by the game context as Game::Play (game/game_context.h).
// ReadFromSavegame and WriteForSavegame keep the save layout of the fields below; it is not the order of the
// fields here, and has gaps for removed fields, so it changes only with a new GameStateSvgVersion.
// Not saved by them: stop_dialog_at_end, screen_is_faded_out, wait_skipped_by, wait_skipped_by_data,
// std_gui_textheight, swap_portrait_lastlastchar, room_changes,
// speech_in_post_state, overlay_count, complete_overlay_on, text_overlay_on, speech_text_schandle,
// speech_face_schandle, shake_screen_yoff, get_loc_name_last_time, get_loc_name_save_cursor and gui_draw_order.
// Saved by other save components: default_audio_type_volumes and the custom property vectors (charProps,
// dialogProps, guiProps, guicontrolProps, invProps); audioclipProps are not saved.
// The save layout also holds the room's ScriptFinished, AnimBackgroundDelay, BgFrame, BgFrameLocked,
// WalkableAreasOn, EnteredAtX, EnteredAtY and EnteredEdge (game/world/room_context.h), at the places their
// GamePlayState fields had: ReadFromSavegame and WriteForSavegame read and write them in the current Room.
struct GamePlayState
{
    // TODO: this is left purely to load older save version, revise later
    static const int LEGACY_GAMENAMELENGTH = 100;
    static const int LEGACY_MAXGLOBALVARS = 50;
    static const int LEGACY_MAXGSVALUES = 500;
    static const int LEGACY_MAXGLOBALSTRINGS = 51;
    static const int LEGACY_MAXSAVEGAMES = 50;

    int usedmode = 0;                                   // set by ProcessClick to last cursor mode used
    int disabled_user_interface = 0;                    // >0 while in cutscene/etc
    int gscript_timer = 0;                              // obsolete; counts down each update while above 0
    int debug_mode = 0;                                 // whether we're in debug mode
    int messagetime = 0;                                // time left for auto-remove messages
    int usedinv = 0;                                    // inventory item last used
    int text_speed = 0;                                 // how quickly text is removed
    int talkanim_speed = 0;                             // animation speed of talking anims
    int inv_item_wid, inv_item_hit = 0;                 // set by SetInvDimensions
    int speech_text_shadow = 0;                         // colour of outline fonts (default black)
    int swap_portrait_side = 0;                         // sierra-style speech swap sides
    int speech_textwindow_gui = 0;                      // textwindow used for sierra-style speech
    int follow_change_room_timer = 0;                   // delay before moving following characters into new room
    SkipSpeechStyle skip_display = kSkipSpeechKeyMouse; // how the user can skip normal Display windows
    int no_multiloop_repeat = 0;                        // for backwards compatibility
    int used_inv_on = 0;                                // inv item they clicked on
    int no_textbg_when_voice = 0;                       // no textwindow bgrnd when voice speech is used
    int max_dialogoption_width = 0;                     // max width of dialog options text window
    int no_hicolor_fadein = 0;               // (DEPRECATED, lo-end optimization) fade out but instant in for hi-color
    int bgspeech_game_speed = 0;             // is background speech relative to game speed
    int bgspeech_stay_on_display = 0;        // whether to remove bg speech when DisplaySpeech is used
    int unfactor_speech_from_textlength = 0; // remove "&10" when calculating time for text to stay
    int speech_music_drop = 0;               // how much to drop music volume by when speech is played
    int in_cutscene = 0;                     // we are between a StartCutscene and EndCutscene
    int fast_forward = 0;                    // player has elected to skip cutscene
    int game_speed_modifier = 0;
    int takeover_data = 0;        // value passed to RunAGSGame in previous game
    int replay_hotkey_unused = 0; // (UNUSED!) StartRecording: not supported
    int dialog_options_pad_x = 0; // dialog options x padding within the gui
    int dialog_options_pad_y = 0; // dialog options y padding within the gui
    int narrator_speech = 0;
    int lipsync_speed = 0;
    int close_mouth_speech_time = 0; // the speech animation closes the mouth once messagetime falls below this
                                     // (this is designed to work in text-only mode); 0 animates until the end
    int disable_antialiasing = 0;
    int text_speed_modifier = 0;
    HorAlignment text_align = kHAlignNone;
    int speech_bubble_width = 0;
    int min_dialogoption_width = 0;
    int disable_dialog_parser = 0;
    int top_bar_backcolor = 0;
    int top_bar_textcolor = 0;
    int top_bar_bordercolor = 0;
    int top_bar_borderwidth = 0;
    int top_bar_ypos = 0;
    int screenshot_width = 0;
    int screenshot_height = 0;
    int top_bar_font = 0;
    HorAlignment speech_text_align = kHAlignNone;
    int auto_use_walkto_points = 0;
    int inventory_greys_out = 0;
    int skip_speech_specific_key = 0;
    int abort_key = 0;
    int fade_to_red = 0;
    int fade_to_green = 0;
    int fade_to_blue = 0;
    int show_single_dialog_option = 0;
    int keep_screen_during_instant_transition = 0;
    int read_dialog_option_colour = 0;
    int stop_dialog_at_end = 0; // used to record state-changing commands inside "dialog_request" callback
        // (a DIALOG_* value, see dialog.cpp)
        // FIXME: StopDialog, RunDialog and NewRoom called inside "dialog_request" are recorded here, and only the first
        // one counts, unlike the other scheduled script commands that are queued; merge with that handling
    int speech_portrait_placement = 0;   // speech portrait placement mode (automatic/custom)
    int speech_portrait_x = 0;           // a speech portrait x offset from corresponding screen side
    int speech_portrait_y = 0;           // a speech portrait y offset
    int speech_display_post_time_ms = 0; // keep speech text/portrait on screen after text/voice has finished playing;
                                         // no speech animation is supposed to be played at this time
    int dialog_options_highlight_color = 0; // The colour used for highlighted (hovered over) text in dialog options
    long randseed = 0;                      // random seed
    int player_on_region = 0;               // player's current region
    bool screen_is_faded_out = false;       // the screen is currently black
    int check_interaction_only = 0;
    int bg_anim_delay = 0;
    short wait_counter = 0;
    char wait_skipped_by = 0;     // tells how last blocking wait was skipped [not serialized]
    int wait_skipped_by_data = 0; // extended data telling how last blocking wait was skipped [not serialized]
    Rect mbounds;                 // mouse cursor bounds
    int fade_effect = 0;
    int audio_master_volume = 0; // in 0-100
    short screen_flipped = 0;
    SpeechMode speech_mode = kSpeech_TextOnly; // speech mode (text, voice, or both)
    int speech_skip_style = 0;                 // stores SKIP_* flags
    bool speech_always_wait_for_text = false;
    int speech_zorder = INT32_MAX; // z-order for the blocking speech overlays (text & portrait)
    int speech_volume = 0;         // in 0-255 !!
    int normal_font = 0;
    int speech_font = 0;
    int std_gui_textheight = 0; // text height for built-in gui dialogs (TODO: use font setting instead)
    char key_skip_wait = 0;
    int swap_portrait_lastchar = 0;
    int swap_portrait_lastlastchar = 0;
    int in_conversation = 0;
    int screen_tint = 0;
    std::vector<uint16_t> parsed_words;
    AGS::Common::String bad_parsed_word;
    int room_changes = 0;

    // Disables mouse input *in game*. Note that the mouse input may still affect the "engine layer",
    // if it's used in debugging menus or whatever other purposes.
    bool mouse_input_enabled = true;
    bool mouse_cursor_shown = true;
    uint32_t shakesc_delay = 0; // unsigned to match the loop counter (get_loop_counter)
    int shakesc_amount = 0;
    int shakesc_length = 0;
    int rtint_red = 0;
    int rtint_green = 0;
    int rtint_blue = 0;
    int rtint_level = 0;
    int rtint_light = 0;
    bool rtint_enabled = false;
    int skip_until_char_stops = 0;
    SceneLocationRef get_loc_name_last_time;
    SceneLocationRef get_loc_name_save_cursor;
    int restore_cursor_mode_to = 0;
    int restore_cursor_image_to = 0;
    short crossfading_out_channel = 0;
    short crossfade_step = 0;
    short crossfade_out_volume_per_step = 0;
    short crossfade_initial_volume_out = 0;
    short crossfading_in_channel = 0;
    short crossfade_in_volume_per_step = 0;
    short crossfade_final_volume_in = 0;
    short new_music_queue_size = 0;
    QueuedAudioItem new_music_queue[MAX_QUEUED_MUSIC]{};
    char lastParserEntry[MAX_MAXSTRLEN]{};
    AGS::Common::String game_name;
    int ground_level_areas_disabled = 0;
    int next_screen_transition = 0;
    int gamma_adjustment = 0;
    short temporarily_turned_off_character = 0; // Hide Player Charactr ticked
    std::vector<int> gui_draw_order;            // used only for hit detection now
    std::unordered_set<AGS::Common::String> do_once_tokens;
    int text_min_display_time_ms = 0;
    int ignore_user_input_after_text_timeout_ms = 0;
    std::vector<int> default_audio_type_volumes;
    // GUI position for dialog options, -1 to use default pos
    int dialog_options_gui_x = -1;
    // GUI position for dialog options, -1 to use default pos
    int dialog_options_gui_y = -1;
    // Horizontal alignment of dialog options texts
    HorAlignment dialog_options_textalign = kHAlignLeft;
    // Font of dialog options texts, if FONT_UNDEFINED then use "normal font"
    int dialog_options_font = FONT_UNDEFINED;
    int dialog_options_zorder = INT32_MAX;
    float face_dir_ratio = 1.f; // character face direction ratio, defines y/x relation

    // Dynamic custom property values for global game objects
    std::vector<AGS::Common::StringIMap> charProps;
    std::vector<AGS::Common::StringIMap> dialogProps;
    std::vector<AGS::Common::StringIMap> guiProps;
    std::vector<AGS::Common::StringIMap> guicontrolProps[AGS::Common::kGUIControlTypeNum];
    std::vector<AGS::Common::StringIMap> invProps;
    // NOTE: audioclip custom properties are not written into game saves;
    // this is done on purpose, as audio clips are resources and not a part of a game state.
    std::vector<AGS::Common::StringIMap> audioclipProps;

    // Dynamic speech state
    //
    // Tells whether there is a voice-over played during current speech
    bool speech_has_voice = false;
    // Tells whether the voice was played in blocking mode;
    // atm blocking speech handles itself, and we only need to finalize
    // non-blocking voice speech during game update; speech refactor would be
    // required to get rid of this rule.
    bool speech_voice_blocking = false;
    // Tells whether character speech stays on screen not animated for additional time
    bool speech_in_post_state = false;

    // Special overlays
    //
    // Total number of existing overlays, only for the reference
    int overlay_count = 0;
    // Is there a QFG4-style dialog overlay on screen (contains overlay ID)
    int complete_overlay_on = 0;
    // Is there a blocking text overlay on screen (contains overlay ID)
    int text_overlay_on = 0;
    // Script overlay handles, because we must return same script objects
    // whenever user script queries for them.
    // Blocking speech overlay managed handle
    int speech_text_schandle = 0;
    // Speech portrait overlay managed handle
    int speech_face_schandle = 0;

    // y offset of the shaking screen
    int shake_screen_yoff = 0;


    GamePlayState();

    // Custom error text, to use when displaying a quit message (may be translated)
    const AGS::Common::String &GetGameErrorText() const { return _gameErrorText; }

    void SetGameErrorText(const AGS::Common::String &text) { _gameErrorText = text; }

    // Runtime behavior switches: allow to configure engine behavior
    // for modern and backwards-compatibility mode on a per-operation basis.
    const std::array<bool, kNum_RBS> &GetRBSwitches() const { return _rbSwitches; }
    // Replaces all the switches with the given ones.
    void SetRBSwitches(const std::array<bool, kNum_RBS> &overrides);

    // The shader (ID and managed handle) applied to the whole screen
    int GetScreenShaderID() const { return _screenShaderID; }
    int GetScreenShaderHandle() const { return _screenShaderHandle; }
    void SetScreenShader(int shader_id, int shader_handle);

    // The shader (ID and managed handle) applied to the mouse cursor
    int GetCursorShaderID() const { return _cursorShaderID; }
    int GetCursorShaderHandle() const { return _cursorShaderHandle; }
    void SetCursorShader(int shader_id, int shader_handle);

    // Tells if engine should apply AA (linear) scaling to the game sprites: the config allows it and the game
    // has not disabled it
    bool ShouldAASprites() const;

    // Tells if the mouse cursor should be drawn on screen
    bool ShouldDrawMouseCursor() const { return mouse_input_enabled && mouse_cursor_shown; }

    //
    // User input management
    //
    // Tells if game should ignore user input right now. Note that some of the parent states
    // may not ignore it at the same time, such as cutscene state, which may still be skipped
    // with a key press or a mouse button.
    bool IsIgnoringInput() const;
    // Sets ignore input state, for the given time; if there's one already, chooses max timeout
    void SetIgnoreInput(int timeout_ms);
    // Clears ignore input state
    void ClearIgnoreInput();

    //
    // Wait and blocking action state
    //
    // Tells if a blocking wait is running now (the wait counter is not 0, whether it has a timeout or not)
    bool IsInWait() const;
    // Set how the last blocking wait was skipped; ends the wait (resets the wait counter)
    void SetWaitSkipResult(int skipped_by, int data = 0);
    // Ends the wait as skipped by the given key press
    void SetWaitKeySkip(const KeyInput &key_input)
    {
        SetWaitSkipResult(SKIP_KEYPRESS, AGSKeyToScriptKey(key_input.Key) | key_input.Mod);
    }
    // Returns the wait counter: above 0 for a wait with a timeout (the ticks left), below 0 for a wait without
    // a timeout, 0 if there is no wait
    int GetWaitCounter() const;
    // Returns which kinds of input may skip the current wait (the SKIP_* flags, key_skip_wait)
    int GetWaitSkipType() const;
    // Returns the information about how the latest blocking wait was skipped.
    // The information is packed into int32 value like this:
    // | 0xFF       | 0xFF    | 0xF      | 0xFFF                     |
    // | eInputType | eKeyMod | reserved | eKeyCode, MouseButton etc |
    int GetWaitSkipResult() const;

    //
    // Voice speech management
    //
    // Tells if there's a blocking voice speech playing right now
    bool IsBlockingVoiceSpeech() const;
    // Tells whether we have to finalize voice speech when stopping or reusing the channel
    bool IsNonBlockingVoiceSpeech() const;
    // Speech helpers: tell if the speech voice-over should be played, which it should not while fast-forwarding or
    // if the voice is not available; the first one also not in the text-only speech mode
    bool ShouldPlayVoiceSpeech() const;
    bool ShouldPlayVoiceSpeechNonBlocking() const;

    //
    // Script Timers
    //
    // Starts the timer (re-starts if it is running), to count down timeout game ticks; timer_id must not be
    // negative and timeout must be positive
    void StartScriptTimer(int timer_id, int timeout);
    // Stops the timer, if it is running
    void StopScriptTimer(int timer_id);
    // Returns the ticks left on the timer, 0 if it is not running
    int GetScriptTimerPos(int timer_id);
    // Tells if the timer has run out, which it does only once: the timer is stopped when it is found run out
    bool CheckScriptTimer(int timer_id);
    // Counts all the running timers down by one tick; a timer stops at 1 and stays there until it is checked
    void UpdateScriptTimers();

    //
    // Serialization
    //
    // Reads the state from a save, written by WriteForSavegame (or by an older save version, see the
    // GameStateSvgVersion checks), with the room's fields that the layout holds into Room::Current().
    // Skips the legacy fields that are no longer used. data_ver and r_data are not used.
    void ReadFromSavegame(AGS::Common::Stream *in, GameDataVersion data_ver, GameStateSvgVersion svg_ver,
                          AGS::Engine::RestoredData &r_data);
    // Writes the state into a save, with the room's fields that the layout holds from Room::Current(); the
    // fields that are not written are listed above the struct.
    void WriteForSavegame(AGS::Common::Stream *out) const;
    // Required for freeing only particular parts when restoring the game: clears the custom property values of
    // audio clips, characters, dialogs, GUIs and inventory items;
    // TODO: restoring a game resets the state part by part (see DoBeforeRestore in savegame.cpp), reset the whole
    // object instead
    void FreeProperties();

private:
    AGS::Common::String _gameErrorText;

    std::array<bool, kNum_RBS> _rbSwitches = {{0}};

    // TODO: a RAII wrapper over managed handle, that auto releases the reference
    int _screenShaderID = 0;
    int _screenShaderHandle = 0;
    int _cursorShaderID = 0;
    int _cursorShaderHandle = 0;

    // A time point until which the user input will be ignored
    AGS::Engine::Clock::time_point _ignoreUserInputUntilTime{};

    // Script Timers: simple game tick counters with arbitrary ID
    std::unordered_map<int32_t, int32_t> _scriptTimers;
};
