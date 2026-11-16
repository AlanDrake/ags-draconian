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
#include "game/game_def.h"

#include "data/data_helpers.h"
#include "game/audio_clip_type.h"
#include "game/dialog_topic.h"
#include "game/script_audio_clip.h"
#include "game/view.h"
#include "script/cc_script.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

// Defined here, where the element types of the lists (ViewStruct, DialogTopic) are complete.
GameDef::GameDef() = default;
// Not noexcept, as when it was defaulted in the class: moving a std::map or an unordered_map member may allocate
// NOLINTNEXTLINE(bugprone-exception-escape)
GameDef::GameDef(GameDef &&gss) = default;
GameDef::~GameDef() = default;
GameDef &GameDef::operator=(GameDef &&gss) = default;

void GameDef::SetGameResolution(Size game_res)
{
    _gameResolution = game_res;
    OnResolutionSet();
}

void GameDef::OnResolutionSet()
{
    _relativeUIMult = 1; // NOTE: this is remains of old logic, currently unused.
}

void GameDef::ReadFromFile(Stream *in, GameDataVersion game_ver, SerializeInfo &info)
{
    // NOTE: historically the struct was saved by dumping whole memory
    // into the file stream, which added padding from memory alignment;
    // here we mark the padding bytes, as they do not belong to actual data.
    gamename.ReadCount(in, LEGACY_GAME_NAME_LENGTH);
    in->ReadInt16(); // alignment padding to int32 (gamename: 50 -> 52 bytes)
    in->ReadArrayOfInt32(options, MAX_OPTIONS);
    in->Read(&paluses[0], sizeof(paluses));
    // colors are an array of chars
    in->Read(&defpal[0], sizeof(defpal));
    numviews = in->ReadInt32();
    numcharacters = in->ReadInt32();
    playercharacter = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    numinvitems = static_cast<uint16_t>(in->ReadInt16());
    in->ReadInt16(); // alignment padding to int32
    numdialog = in->ReadInt32();
    numdlgmessage = in->ReadInt32();
    numfonts = in->ReadInt32();
    color_depth = in->ReadInt32();
    target_win = in->ReadInt32();
    dialog_bullet = in->ReadInt32();
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    uniqueid = in->ReadInt32();
    numgui = in->ReadInt32();
    numcursors = in->ReadInt32();
    [[maybe_unused]] GameResolutionType resolution_type = (GameResolutionType)in->ReadInt32();
    assert(resolution_type == kGameResolution_Custom);
    Size game_size;
    game_size.Width = in->ReadInt32();
    game_size.Height = in->ReadInt32();
    SetGameResolution(game_size);

    default_lipsync_frame = in->ReadInt32();
    inv_hot_sprite = in->ReadInt32();
    inv_hot_color = in->ReadInt32();
    inv_hot_cross_color = in->ReadInt32();
    in->ReadArrayOfInt32(reserved, NUM_INTS_RESERVED);
    info.ExtensionOffset = static_cast<uint32_t>(in->ReadInt32());

    if (game_ver < kGameVersion_400_33)
    {
        in->ReadArrayOfInt32(info.HasMessages.data(), NUM_LEGACY_GLOBALMES);
    }
    info.HasWordsDict = in->ReadInt32() != 0;
    in->ReadInt32(); // globalscript (dummy 32-bit pointer value)
    in->ReadInt32(); // chars (dummy 32-bit pointer value)
    info.HasCCScript = in->ReadInt32() != 0;
}

const char *GetScriptAPIName(ScriptAPIVersion v)
{
    switch (v)
    {
    case kScriptAPI_v321: return "v3.2.1";
    case kScriptAPI_v330: return "v3.3.0";
    case kScriptAPI_v334: return "v3.3.4";
    case kScriptAPI_v335: return "v3.3.5";
    case kScriptAPI_v340: return "v3.4.0";
    case kScriptAPI_v341: return "v3.4.1";
    case kScriptAPI_v350: return "v3.5.0-alpha";
    case kScriptAPI_v3507: return "v3.5.0-final";
    case kScriptAPI_v360: return "v3.6.0-alpha";
    case kScriptAPI_v36026: return "v3.6.0-final";
    case kScriptAPI_v361: return "v3.6.1";
    case kScriptAPI_v362: return "v3.6.2";
    case kScriptAPI_v363: return "v3.6.3";
    case kScriptAPI_v399: return "3.99.x";
    case kScriptAPI_v400: return "4.0.0-alpha8";
    case kScriptAPI_v400_07: return "4.0.0-alpha12";
    case kScriptAPI_v400_14: return "4.0.0-alpha18";
    case kScriptAPI_v400_16: return "4.0.0-alpha20";
    case kScriptAPI_v400_18: return "4.0.0-alpha22";
    case kScriptAPI_v400_24: return "4.0.0-alpha28";
    case kScriptAPI_v400_33: return "4.0.0-beta";
    default: return "unknown";
    }
}


void GameDef::ResolveStandardModes()
{
    for (int role = 0; role < kNumCursorRoles; ++role)
    {
        for (int i = 0; i < numcursors; ++i)
        {
            if (mcurs[i].role == role)
            {
                _roleCursors[role] = i;
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Reading Part 1

void GameDef::read_savegame_info(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    StrUtil::ReadCStrCount(guid, in, MAX_GUID_LENGTH);
    StrUtil::ReadCStrCount(saveGameFileExtension, in, MAX_SG_EXT_LENGTH);
    saveGameFolderName.ReadCount(in, LEGACY_MAX_SG_FOLDER_LEN);
}

void GameDef::read_font_infos(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    fonts.resize(numfonts);
    for (int i = 0; i < numfonts; ++i)
    {
        fonts[i].FontID = i;
        uint32_t flags = in->ReadInt32();
        fonts[i].Size = in->ReadInt32();
        fonts[i].Outline = in->ReadInt32();
        fonts[i].YOffset = in->ReadInt32();
        fonts[i].LineSpacing = std::max(0, in->ReadInt32());
        fonts[i].SetFlags(flags);
    }
}

void GameDef::ReadInvInfo(Stream *in, GameDataVersion data_ver)
{
    invinfo.resize(numinvitems);
    for (int i = 0; i < numinvitems; ++i)
    {
        invinfo[i].ReadFromFile(in, data_ver);
    }
}

HGameFileError GameDef::read_cursors(Common::Stream *in)
{
    mcurs.resize(numcursors);
    ReadMouseCursors(in);
    return HGameFileError::None();
}

HError GameDef::read_interaction_scripts(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    Characters.resize(numcharacters); // safety precaution, because chars may be read later
    HError err;
    for (size_t i = 0; i < (size_t)numcharacters; ++i)
    {
        err = Characters[i].interactions.Read_v361(in);
        if (!err)
            return err;
    }
    // NOTE: inventory item events are loaded starting from index 1, because index 0 is an unused item
    invinfo.resize(numinvitems); // safety precaution
    for (size_t i = 1; i < (size_t)numinvitems; ++i)
    {
        err = invinfo[i].interactions.Read_v361(in);
        if (!err)
            return err;
    }
    return HError::None();
}

void GameDef::read_words_dictionary(Common::Stream *in)
{
    dict.reset(new WordsDictionary());
    dict->ReadFromFile(in);
}

void GameDef::ReadMouseCursors(Stream *in)
{
    for (int i = 0; i < numcursors; ++i)
    {
        mcurs[i].ReadFromFile(in);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameDef::read_characters(Common::Stream *in, GameDataVersion data_ver)
{
    Characters.resize(numcharacters);
    ReadCharacters(in, data_ver);
}

void GameDef::read_lipsync(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    in->ReadArray(&lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50);
}

void GameDef::skip_messages(Common::Stream *in, const std::array<int, NUM_LEGACY_GLOBALMES> &load_messages,
                            GameDataVersion data_ver)
{
    for (int i = 0; i < NUM_LEGACY_GLOBALMES; ++i)
    {
        if (!load_messages[i])
            continue;
        SkipStringDecrypt(in);
    }
}

void GameDef::ReadCharacters(Stream *in, GameDataVersion data_ver)
{
    for (int i = 0; i < numcharacters; ++i)
    {
        Characters[i].ReadFromFile(in, data_ver);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 3

HGameFileError GameDef::read_customprops(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    if (Properties::ReadSchema(propSchema, in) != kPropertyErr_NoError)
        return new MainGameFileError(kMGFErr_InvalidPropertySchema);

    int errors = 0;

    charProps.resize(numcharacters);
    for (int i = 0; i < numcharacters; ++i)
    {
        errors += Properties::ReadValues(charProps[i], in);
    }
    invProps.resize(numinvitems);
    for (int i = 0; i < numinvitems; ++i)
    {
        errors += Properties::ReadValues(invProps[i], in);
    }

    if (errors > 0)
        return new MainGameFileError(kMGFErr_InvalidPropertyValues);

    viewNames.resize(numviews);
    for (int i = 0; i < numviews; ++i)
        viewNames[i] = String::FromStream(in);

    invScriptNames.resize(numinvitems);
    for (int i = 0; i < numinvitems; ++i)
        invScriptNames[i] = String::FromStream(in);

    dialogScriptNames.resize(numdialog);
    for (int i = 0; i < numdialog; ++i)
        dialogScriptNames[i] = String::FromStream(in);
    return HGameFileError::None();
}

HGameFileError GameDef::read_audio(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    size_t audiotype_count = in->ReadInt32();
    audioClipTypes.resize(audiotype_count);
    for (size_t i = 0; i < audiotype_count; ++i)
    {
        audioClipTypes[i].ReadFromFile(in);
    }

    size_t audioclip_count = in->ReadInt32();
    audioClips.resize(audioclip_count);
    ReadAudioClips(in, audioclip_count);

    in->ReadInt32(); // [DEPRECATED]
    return HGameFileError::None();
}

void GameDef::read_room_names(Stream *in, GameDataVersion data_ver)
{
    if ((data_ver >= kGameVersion_400_13) || (options[OPT_DEBUGMODE] != 0))
    {
        uint32_t room_count = in->ReadInt32();
        roomNumbers.resize(room_count);
        for (uint32_t i = 0; i < room_count; ++i)
        {
            int room_number = in->ReadInt32();
            String room_name = String::FromStream(in);
            roomNumbers[i] = room_number;
            roomNames.insert(std::make_pair(room_number, room_name));
        }
    }
}

void GameDef::ReadAudioClips(Common::Stream *in, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        audioClips[i].ReadFromFile(in);
    }
}
