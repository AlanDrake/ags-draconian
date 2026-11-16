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
// GameDef is the main game data.
//
//=============================================================================
#pragma once

#include <array>
#include <map>
#include <memory>
#include <vector>

#include <allegro.h> // RGB

#include "game/audio_clip_type.h"
#include "game/character_info.h" // TODO: constants to separate header
#include "game/customproperties.h"
#include "game/game_struct_defines.h"
#include "game/game_version.h"
#include "game/inventory_item_info.h"
#include "game/main_game_file.h" // TODO: constants to separate header or split out reading functions
#include "game/mouse_cursor.h"
#include "game/script_audio_clip.h"
#include "game/scripteventtable.h"
#include "game/words_dictionary.h"
#include "gui/guidefines.h"
#include "util/string.h"


using AGS::Common::HGameFileError;
using AGS::Common::HError;


// TODO: split GameDef into struct used to hold loaded game data, and actual runtime object
struct GameDef
{
    static const int LEGACY_GAME_NAME_LENGTH = 50;
    static const int MAX_OPTIONS = 100;
    static const int NUM_INTS_RESERVED = 14;
    // TODO: this is left purely to load older format version, revise later
    static const int NUM_LEGACY_GLOBALMES = 500;


    AGS::Common::String gamename;
    GameDataVersion gamedataver = kGameVersion_Undefined;
    AGS::Common::String compiled_with; // version of AGS this data was created by
    int options[MAX_OPTIONS] = {0};
    uint8_t paluses[256] = {0};
    RGB defpal[256] = {};
    int numviews = 0;
    int numcharacters = 0;
    int playercharacter = -1;
    uint16_t numinvitems = 0u;
    int numdialog = 0;
    int numdlgmessage = 0; // [DEPRECATED]
    int numfonts = 0;
    int color_depth = 0; // in bytes per pixel (ie. 1, 2, 4)
    int target_win = 0;
    int dialog_bullet = 0;       // 0 for none, otherwise slot num of bullet point
    int inv_hot_color = 0;       // inv cursor hotspot dot color
    int inv_hot_cross_color = 0; // inv cursor hotspot cross color
    int uniqueid = 0;            // random key identifying the game
    int numgui = 0;
    int numcursors = 0;
    int default_lipsync_frame = 0; // used for unknown chars
    int inv_hot_sprite = 0;
    int reserved[NUM_INTS_RESERVED] = {0};
    // Base text parser dictionary, contains parser words in the original game language.
    // This may be overridden by the parser dictionary read from translation, if one is applied.
    std::unique_ptr<AGS::Common::WordsDictionary> dict;
    std::vector<CharacterInfo> chars;


private:
    // Determines game's actual resolution.
    Size _gameResolution;
    // Multiplier for various UI drawin sizes, meant to keep UI elements readable
    int _relativeUIMult = 1;

public:
    // This array is used only to read data into;
    // font parameters are then put and queried in the fonts module
    // TODO: split into installation params (used only when reading) and runtime params
    std::vector<FontInfo> fonts;
    std::vector<InventoryItemInfo> invinfo;
    std::vector<MouseCursor> mcurs;
    char lipSyncFrameLetters[MAXLIPSYNCFRAMES][50] = {{0}};
    AGS::Common::PropertySchema propSchema;
    std::vector<AGS::Common::StringIMap> charProps;
    std::vector<AGS::Common::StringIMap> invProps;
    std::vector<AGS::Common::StringIMap> audioclipProps;
    std::vector<AGS::Common::StringIMap> dialogProps;
    std::vector<AGS::Common::StringIMap> guiProps;
    std::vector<AGS::Common::StringIMap> guicontrolProps[AGS::Common::kGUIControlTypeNum];
    // NOTE: although the view names are stored in game data, they are never
    // used, nor registered as script exports; numeric IDs are used to
    // reference views instead.
    std::vector<AGS::Common::String> viewNames;
    std::vector<AGS::Common::String> invScriptNames; // TODO: move to InventoryItemInfo
    // TODO: move this array out of Game struct, but need to read legacy data
    // into some sort of a temporary container for post-load upgrading.
    std::vector<AGS::Common::String> dialogScriptNames;
    char guid[MAX_GUID_LENGTH] = {0};
    char saveGameFileExtension[MAX_SG_EXT_LENGTH] = {0};
    // NOTE: saveGameFolderName is generally used to create game subdirs in common user directories
    AGS::Common::String saveGameFolderName;
    // Existing room numbers
    std::vector<int> roomNumbers;
    // Saved room names, known during the game compilation;
    // may be used to learn the total number of registered rooms
    std::map<int, AGS::Common::String> roomNames;
    std::vector<ScriptAudioClip> audioClips;
    std::vector<AudioClipType> audioClipTypes;
    // number of accessible game audio channels (the ones under direct user control)
    int numGameChannels = 0;
    // backward-compatible channel limit that may be exported to script and reserved by audiotypes
    int numCompatGameChannels = 0;

    // Extended global game properties
    // Character face direction ratio (y/x)
    float faceDirectionRatio = 1.f;

    // A dictionary of semi-arbitrary game info properties: title, developer's name, etc
    // Following keys are currently supported:
    // - title - game title;
    // - description - game description;
    // - dev_error_text - custom text to use when the game bails out with error;
    // - dev_name - game developer's name (person or company);
    // - dev_url - game developer's url;
    // - genre - game's genre;
    // - release_date - "dd.MM.yyyy" string;
    // - version - game's version (assumes X.X.X.X string, but may be anything);
    // - text_lang - 'en_US' format string which defines game language;
    AGS::Common::StringMap GameInfo;
    // Game text language definition, in the form of locale name ('en', 'en_US' etc)
    AGS::Common::String GameTextLanguage;

    // TODO: I converted original array of sprite infos to vector here, because
    // statistically in most games sprites go in long continious sequences with minimal
    // gaps, and standard hash-map will have relatively big memory overhead compared.
    // Of course vector will not behave very well if user has created e.g. only
    // sprite #1 and sprite #1000000. For that reason I decided to still limit static
    // sprite count to some reasonable number for the time being. Dynamic sprite IDs are
    // added in sequence, so there won't be any issue with these.
    // There could be other collection types, more optimal for this case. For example,
    // we could use a kind of hash map containing fixed-sized arrays, where size of
    // array is calculated based on key spread factor (sparse list?).
    std::vector<SpriteInfo> SpriteInfos;

    // Tells if the given game sprite is valid
    inline bool IsSpriteValid(int sprnum) const
    {
        return (sprnum >= 0) && (static_cast<size_t>(sprnum) < SpriteInfos.size()) && SpriteInfos[sprnum].IsValid();
    }

    // Returns a cursor index with standard Walk mode, or -1 if there's none
    int GetModeWalk() const { return _roleCursors[kCursorRole_Walk]; }
    // Returns a cursor index with standard Look mode, or -1 if there's none
    int GetModeLook() const { return _roleCursors[kCursorRole_Look]; }
    // Returns a cursor index with standard Interact mode, or -1 if there's none
    int GetModeInteract() const { return _roleCursors[kCursorRole_Interact]; }
    // Returns a cursor index with standard Use Inventory mode, or -1 if there's none
    int GetModeUseInv() const { return _roleCursors[kCursorRole_UseInv]; }
    // Returns a cursor index with standard Pointer mode, or -1 if there's none
    int GetModePointer() const { return _roleCursors[kCursorRole_Pointer]; }
    // Returns a cursor index with standard Wait mode, or -1 if there's none
    int GetModeWait() const { return _roleCursors[kCursorRole_Wait]; }
    // Tells if the given cursor has the specific role
    bool HasCursorRole(int curs, CursorRole role) const { return mcurs[curs].role == role; }

    GameDef() = default;
    GameDef(GameDef &&gss) = default;
    ~GameDef() = default;

    GameDef &operator=(GameDef &&gss) = default;

    // Tells whether the serialized game data contains certain components
    struct SerializeInfo
    {
        bool HasCCScript = false;
        bool HasWordsDict = false;
        // NOTE: Global messages are cut out, but we still have to check them
        // so long as we keep support of loading an older game data
        std::array<int, NUM_LEGACY_GLOBALMES> HasMessages = {};
        // File offset at which game data extensions begin
        uint32_t ExtensionOffset = 0u;
    };

    void ReadFromFile(AGS::Common::Stream *in, GameDataVersion game_ver, SerializeInfo &info);

    // Game resolution is a size of a native game screen in pixels.
    // This is the "game resolution" that developer sets up in AGS Editor.
    // It is in the same units in which sprite and font sizes are defined.
    //
    // Graphic renderer may scale and stretch game's frame as requested by
    // player or system, which will not affect native coordinates in any way.
    void SetGameResolution(Size game_res);
    const Size &GetGameRes() const { return _gameResolution; }
    // Get game's native color depth (bits per pixel)
    inline int GetColorDepth() const { return color_depth * 8; }

    // Returns the expected filename of a digital audio package
    inline AGS::Common::String GetAudioVOXName() const { return "audio.vox"; }

    // Returns a list of game options that are forbidden to change at runtime
    inline static std::array<int, 18> GetRestrictedOptions()
    {
        return std::array<int, 18>{{OPT_DEBUGMODE, OPT_OBSOLETE_LETTERBOX, OPT_OBSOLETE_HIRES_FONTS, OPT_SPLITRESOURCES,
                                    OPT_OBSOLETE_STRICTSCRIPTING, OPT_OBSOLETE_LEFTTORIGHTEVAL, OPT_COMPRESSSPRITES,
                                    OPT_OBSOLETE_STRICTSTRINGS, OPT_OBSOLETE_NATIVECOORDINATES,
                                    OPT_OBSOLETE_SAFEFILEPATHS, OPT_DIALOGOPTIONSAPI, OPT_BASESCRIPTAPI,
                                    OPT_SCRIPTCOMPATLEV, OPT_OBSOLETE_RELATIVEASSETRES, OPT_GAMETEXTENCODING,
                                    OPT_KEYHANDLEAPI, OPT_CUSTOMENGINETAG, OPT_VOICECLIPNAMERULE}};
    }

    // Returns a list of game options that must be preserved when restoring a save;
    // NOTE: restricted options are always preserved, so no need to mention them here
    inline static std::array<int, 2> GetPreservedOptions()
    {
        return std::array<int, 2>{{OPT_SAVECOMPONENTSIGNORE, OPT_AUTOTRANSPARSERSAID}};
    }

    void ResolveStandardModes();

    // [IKM] Game struct loading code is moved here from Engine's load_game_file
    // function; for now it is not supposed to be called by Editor; although it
    // is possible that eventually will be.
    //
    // Since reading game data is made in a bit inconvenient way I had to
    // a) divide process into three functions (there's some extra stuff
    // being read between them;
    // b) use a helper struct to pass some arguments
    //

    //--------------------------------------------------------------------
    // Do not call these directly
    //------------------------------
    // Part 1
    void read_savegame_info(AGS::Common::Stream *in, GameDataVersion data_ver);
    void read_font_infos(AGS::Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_cursors(AGS::Common::Stream *in);
    HError read_interaction_scripts(AGS::Common::Stream *in, GameDataVersion data_ver);
    void read_words_dictionary(AGS::Common::Stream *in);

    void ReadInvInfo(AGS::Common::Stream *in, GameDataVersion data_ver);
    void ReadMouseCursors(AGS::Common::Stream *in);
    //------------------------------
    // Part 2
    void read_characters(AGS::Common::Stream *in, GameDataVersion data_ver);
    void read_lipsync(AGS::Common::Stream *in, GameDataVersion data_ver);
    // NOTE: Global messages are cut out, but we still have to check them
    // so long as we keep support of loading an older game data
    void skip_messages(AGS::Common::Stream *in, const std::array<int, NUM_LEGACY_GLOBALMES> &load_messages,
                       GameDataVersion data_ver);

    void ReadCharacters(AGS::Common::Stream *in, GameDataVersion data_ver);
    //------------------------------
    // Part 3
    HGameFileError read_customprops(AGS::Common::Stream *in, GameDataVersion data_ver);
    HGameFileError read_audio(AGS::Common::Stream *in, GameDataVersion data_ver);
    void read_room_names(AGS::Common::Stream *in, GameDataVersion data_ver);

    void ReadAudioClips(AGS::Common::Stream *in, size_t count);
    //--------------------------------------------------------------------

private:
    void OnResolutionSet();
    std::array<int, kNumCursorRoles> _roleCursors = {{-1, -1, -1, -1, -1, -1, -1, -1}};
};
