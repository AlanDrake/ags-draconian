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

// The game state's save format (GameDef's saved fields and GameState)
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


// A GameLoopState is a parent class for the game states.
class GameLoopState
{
public:
    using String = AGS::Common::String;

    virtual ~GameLoopState() = default;

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
// Not saved by them: DialogRequestResult, IsScreenFadedOut, WaitSkippedBy, WaitSkippedByData,
// StandardGuiTextHeight, SwapPortraitLastLastCharacter, RoomChanges,
// IsSpeechInPostState, OverlayCount, CompleteOverlayID, TextOverlayID, SpeechTextScriptHandle,
// SpeechPortraitScriptHandle, ShakeScreenOffsetY, LastNamedLocation, LocationNameSaveCursor and GuiDrawOrder.
// Saved by other save components: DefaultAudioTypeVolumes and the custom property vectors (GuiProperties,
// GuiControlProperties).
// The save layout also holds the room's ScriptFinished, AnimBackgroundDelay, BgFrame, BgFrameLocked,
// WalkableAreasOn, EnteredAtX, EnteredAtY and EnteredEdge (game/world/room_context.h), at the places their
// GameState fields had: ReadFromSavegame and WriteForSavegame read and write them in the current Room.
struct GameState
{
    // TODO: this is left purely to load older save version, revise later
    static const int LEGACY_GAMENAMELENGTH = 100;
    static const int LEGACY_MAXGLOBALVARS = 50;
    static const int LEGACY_MAXGSVALUES = 500;
    static const int LEGACY_MAXGLOBALSTRINGS = 51;
    static const int LEGACY_MAXSAVEGAMES = 50;

    int UsedMode = 0;                   // set by ProcessClick to last cursor mode used; script: game.used_mode
    int UserInterfaceDisabledCount = 0; // >0 while in cutscene/etc; script: game.disabled_user_interface
    int GlobalScriptTimer = 0;          // obsolete; counts down each update while above 0; script: game.gscript_timer
    int DebugMode = 0;                  // whether we're in debug mode
    int MessageTimeLeft = 0;            // time left for auto-remove messages; script: game.messagetime
    int UsedInventory = 0;              // inventory item last used; script: game.usedinv
    int TextReadingSpeed = 0;           // how quickly text is removed; script: game.text_speed
    int GlobalSpeechAnimationDelay = 0; // animation speed of talking anims; script: game.talkanim_speed
    // script: game.inv_item_wid, game.inv_item_hit
    int InventoryItemWidth, InventoryItemHeight = 0; // set by SetInvDimensions
    int TextShadowColor = 0;     // colour of outline fonts (default black); script: game.text_shadow_color
    int SwapPortraitSide = 0;    // sierra-style speech swap sides; script: game.swap_portrait
    int SpeechTextWindowGui = 0; // textwindow used for sierra-style speech; script: game.speech_text_gui
    // script: game.following_room_timer
    int FollowingRoomDelay = 0;                        // delay before moving following characters into new room
    SkipSpeechStyle SkipDisplay = kSkipSpeechKeyMouse; // how the user can skip normal Display windows
    int no_multiloop_repeat = 0;                       // for backwards compatibility
    int InventoryActivated = 0;                        // inv item they clicked on; script: game.inv_activated
    // script: game.no_textbg_when_voice
    int NoTextBackgroundWhenVoice = 0; // no textwindow bgrnd when voice speech is used
    int MaxDialogOptionWidth = 0;      // max width of dialog options text window; script: game.max_dialogoption_width
    // script: game.no_hicolor_fadein
    int NoHighColorFadeIn = 0;         // (DEPRECATED, lo-end optimization) fade out but instant in for hi-color
    int BackgroundSpeechGameSpeed = 0; // is background speech relative to game speed; script: game.bgspeech_game_speed
    // script: game.bgspeech_stay_on_display
    int KeepBackgroundSpeechOnDisplay = 0; // whether to remove bg speech when DisplaySpeech is used
    // script: game.unfactor_speech_from_textlength
    int UnfactorSpeechFromTextLength = 0; // remove "&10" when calculating time for text to stay
    int SpeechMusicDrop = 0;              // how much to drop music volume by when speech is played
    int RunningCutsceneSkipStyle = 0;     // we are between a StartCutscene and EndCutscene; script: game.in_cutscene
    int FastForward = 0;                  // player has elected to skip cutscene; script: game.skipping_cutscene
    int GameSpeedModifier = 0;
    int PreviousGameData = 0;      // value passed to RunAGSGame in previous game; script: game.previous_game_data
    int ReplayHotkeyUnused = 0;    // (UNUSED!) StartRecording: not supported
    int DialogOptionsPaddingX = 0; // dialog options x padding within the gui; script: game.dialog_options_x
    int DialogOptionsPaddingY = 0; // dialog options y padding within the gui; script: game.dialog_options_y
    int NarratorSpeech = 0;
    int LipSyncSpeed = 0; // script: game.lipsync_speed
    // The speech animation closes the mouth once MessageTimeLeft falls below this
    // (this is designed to work in text-only mode); 0 animates until the end; script: Speech.AnimationStopTimeMargin
    int SpeechAnimationStopTimeMargin = 0;
    int disable_antialiasing = 0;
    int TextReadingSpeedModifier = 0;         // script: game.text_speed_modifier
    HorAlignment TextAlignment = kHAlignNone; // script: game.text_align
    int SpeechBubbleWidth = 0;
    int MinDialogOptionWidth = 0; // script: game.min_dialogoption_width
    int disable_dialog_parser = 0;
    int TopBarBackColor = 0;   // script: game.top_bar_backcolor
    int TopBarTextColor = 0;   // script: game.top_bar_textcolor
    int TopBarBorderColor = 0; // script: game.top_bar_bordercolor
    int TopBarBorderWidth = 0; // script: game.top_bar_borderwidth
    int TopBarY = 0;           // script: game.top_bar_ypos
    int ScreenshotWidth = 0;
    int ScreenshotHeight = 0;
    int TopBarFont = 0;
    HorAlignment SpeechTextAlignment = kHAlignNone; // script: Speech.TextAlignment
    int AutoUseWalkToPoints = 0;                    // script: game.auto_use_walkto_points
    int GreyOutInventory = 0;                       // script: game.inventory_greys_out
    int SpeechSkipKey = 0;                          // script: Speech.SkipKey
    int AbortKey = 0;
    int FadeColorRed = 0;   // script: game.fade_color_red
    int FadeColorGreen = 0; // script: game.fade_color_green
    int FadeColorBlue = 0;  // script: game.fade_color_blue
    int ShowSingleDialogOption = 0;
    int KeepScreenDuringInstantTransition = 0;
    int ReadDialogOptionColor = 0; // script: game.read_dialog_option_color
    int DialogRequestResult = 0;   // used to record state-changing commands inside "dialog_request" callback
        // (a DIALOG_* value, see dialog.cpp)
        // FIXME: StopDialog, RunDialog and NewRoom called inside "dialog_request" are recorded here, and only the first
        // one counts, unlike the other scheduled script commands that are queued; merge with that handling
    // script: Speech.CustomPortraitPlacement
    int IsSpeechCustomPortraitPlacement = 0; // speech portrait placement mode (automatic/custom)
    // script: Speech.PortraitXOffset
    int SpeechPortraitOffsetX = 0; // a speech portrait x offset from corresponding screen side
    int SpeechPortraitY = 0;       // a speech portrait y offset; script: Speech.PortraitY
    // script: Speech.DisplayPostTimeMs
    int SpeechDisplayPostTimeMs = 0;     // keep speech text/portrait on screen after text/voice has finished playing;
                                         // no speech animation is supposed to be played at this time
    int DialogOptionsHighlightColor = 0; // The colour used for highlighted (hovered over) text in dialog options
    long RandomSeed = 0;                 // random seed
    int PlayerRegionID = 0;              // player's current region
    bool IsScreenFadedOut = false;       // the screen is currently black
    int CheckInteractionOnly = 0;
    int BgAnimWait = 0;
    short WaitCounter = 0;     // script: Game.BlockingWaitCounter
    char WaitSkippedBy = 0;    // tells how last blocking wait was skipped [not serialized]
    int WaitSkippedByData = 0; // extended data telling how last blocking wait was skipped [not serialized]
    Rect MouseBounds;          // mouse cursor bounds
    int FadeEffect = 0;
    int AudioMasterVolume = 0; // in 0-100
    short ScreenFlip = 0;
    SpeechMode SpeechVoiceMode = kSpeech_TextOnly; // speech mode (text, voice, or both); script: Speech.VoiceMode
    int SpeechSkipStyle = 0;                       // stores SKIP_* flags; script: Speech.SkipStyle
    bool AlwaysWaitForSpeechText = false;          // script: Speech.AlwaysWaitForText
    // script: Speech.OverlayZOrder
    int SpeechOverlayZOrder = INT32_MAX; // z-order for the blocking speech overlays (text & portrait)
    int SpeechVolume = 0;                // in 0-255 !!
    int NormalFont = 0;
    int SpeechFont = 0;
    int StandardGuiTextHeight = 0; // text height for built-in gui dialogs (TODO: use font setting instead)
    char WaitSkipType = 0;         // script: Game.BlockingWaitSkipType
    int SwapPortraitLastCharacter = 0;
    int SwapPortraitLastLastCharacter = 0;
    int ConversationDepth = 0;
    int ScreenTint = 0;
    std::vector<uint16_t> ParsedWords;
    AGS::Common::String BadParsedWord;
    int RoomChanges = 0;

    // Disables mouse input *in game*. Note that the mouse input may still affect the "engine layer",
    // if it's used in debugging menus or whatever other purposes.
    bool IsMouseInputEnabled = true; // script: Mouse.ControlEnabled
    bool IsMouseCursorShown = true;  // script: Mouse.Visible
    uint32_t ShakeScreenDelay = 0;   // unsigned to match the loop counter (get_loop_counter)
    int ShakeScreenAmount = 0;
    int ShakeScreenLength = 0;
    int AmbientTintRed = 0;
    int AmbientTintGreen = 0;
    int AmbientTintBlue = 0;
    int AmbientTintLevel = 0;
    int AmbientTintLight = 0;
    bool IsAmbientTintEnabled = false;
    int SkipUntilCharacterStops = 0;
    SceneLocationRef LastNamedLocation;
    SceneLocationRef LocationNameSaveCursor;
    int RestoreCursorModeTo = 0;
    int RestoreCursorImageTo = 0;
    short CrossfadingOutChannel = 0;
    short CrossfadeStep = 0;
    short CrossfadeOutVolumePerStep = 0;
    short CrossfadeInitialVolumeOut = 0;
    short CrossfadingInChannel = 0;
    short CrossfadeInVolumePerStep = 0;
    short CrossfadeFinalVolumeIn = 0;
    short NewMusicQueueSize = 0;
    QueuedAudioItem NewMusicQueue[MAX_QUEUED_MUSIC]{};
    char LastParserEntry[MAX_MAXSTRLEN]{};
    AGS::Common::String GameName; // script: Game.Name
    int GroundLevelAreasDisabled = 0;
    int NextScreenTransition = 0;
    int GammaAdjustment = 0;
    short TemporarilyTurnedOffCharacter = 0; // Hide Player Charactr ticked
    std::vector<int> GuiDrawOrder;           // used only for hit detection now
    std::unordered_set<AGS::Common::String> DoOnceTokens;
    int MinimumTextDisplayTimeMs = 0;
    int IgnoreUserInputAfterTextTimeoutMs = 0;
    std::vector<int> DefaultAudioTypeVolumes;
    // GUI position for dialog options, -1 to use default pos
    int DialogOptionsGuiX = -1;
    // GUI position for dialog options, -1 to use default pos
    int DialogOptionsGuiY = -1;
    // Horizontal alignment of dialog options texts
    HorAlignment DialogOptionsTextAlignment = kHAlignLeft;
    // Font of dialog options texts, if FONT_UNDEFINED then use "normal font"
    int DialogOptionsFont = FONT_UNDEFINED;
    int DialogOptionsZOrder = INT32_MAX;
    float FaceDirectionRatio = 1.f; // character face direction ratio, defines y/x relation

    // The custom property values that the script set during play on the GUIs and, by control type, on the GUI
    // controls (the other entities hold theirs: Character, InventoryItem, Dialog, AudioClip::PropertyOverrides)
    std::vector<AGS::Common::StringIMap> GuiProperties;
    std::vector<AGS::Common::StringIMap> GuiControlProperties[AGS::Common::kGUIControlTypeNum];

    // Dynamic speech state
    //
    // Tells whether there is a voice-over played during current speech
    bool HasSpeechVoice = false;
    // Tells whether the voice was played in blocking mode;
    // atm blocking speech handles itself, and we only need to finalize
    // non-blocking voice speech during game update; speech refactor would be
    // required to get rid of this rule.
    bool IsSpeechVoiceBlocking = false;
    // Tells whether character speech stays on screen not animated for additional time
    bool IsSpeechInPostState = false;

    // Special overlays
    //
    // Total number of existing overlays, only for the reference
    int OverlayCount = 0;
    // Is there a QFG4-style dialog overlay on screen (contains overlay ID)
    int CompleteOverlayID = 0;
    // Is there a blocking text overlay on screen (contains overlay ID)
    int TextOverlayID = 0; // script: Speech.TextOverlay
    // Script overlay handles, because we must return same script objects
    // whenever user script queries for them.
    // Blocking speech overlay managed handle
    int SpeechTextScriptHandle = 0;
    // Speech portrait overlay managed handle
    int SpeechPortraitScriptHandle = 0; // script: Speech.PortraitOverlay

    // y offset of the shaking screen
    int ShakeScreenOffsetY = 0;


    GameState();

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
    bool ShouldDrawMouseCursor() const { return IsMouseInputEnabled && IsMouseCursorShown; }

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
    // Returns which kinds of input may skip the current wait (the SKIP_* flags, WaitSkipType)
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
    // Required for freeing only particular parts when restoring the game: clears the custom property values of the
    // GUIs and of the GUI controls;
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
