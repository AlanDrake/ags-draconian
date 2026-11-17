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
#include "game/audio_clip_def.h"
#include "game/audio_clip_type_def.h"
#include "game/character_def.h"
#include "game/dialog_def.h"
#include "game/inventory_item_def.h"
#include "game/view_def.h"
#include "game/words_dictionary.h"
#include "script/cc_script.h"
#include "util/field_table.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

namespace
{

// FontDef (game/game_struct_defines.h, which has no source file of its own), read here (read_font_infos). The live
// font is the font module's copy (Common/font/fonts.cpp), made from the definition whenever the game's fonts are
// loaded (at the game's start and at a change of translation, which may override some fields); no save holds a field
// of it, and nothing writes the definition during play
constexpr FieldRow<FontDef> kFontDefFields[] = {
    Field<&FontDef::ID>("ID", FieldRole::FromDefinition),
    Field<&FontDef::SourceFileName>("SourceFileName", FieldRole::FromDefinition),
    Field<&FontDef::AssetFileName>("AssetFileName", FieldRole::FromDefinition),
    Field<&FontDef::Flags>("Flags", FieldRole::FromDefinition),
    Field<&FontDef::Size>("Size", FieldRole::FromDefinition),
    Field<&FontDef::SizeMultiplier>("SizeMultiplier", FieldRole::FromDefinition),
    Field<&FontDef::Outline>("Outline", FieldRole::FromDefinition),
    Field<&FontDef::CustomHeight>("CustomHeight", FieldRole::FromDefinition),
    Field<&FontDef::OffsetY>("OffsetY", FieldRole::FromDefinition),
    Field<&FontDef::LineSpacing>("LineSpacing", FieldRole::FromDefinition),
    Field<&FontDef::CharacterSpacing>("CharacterSpacing", FieldRole::FromDefinition),
    Field<&FontDef::AutoOutlineStyle>("AutoOutlineStyle", FieldRole::FromDefinition),
    Field<&FontDef::AutoOutlineThickness>("AutoOutlineThickness", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kFontDefFields), "FontDef: a field has no row in kFontDefFields");

} // namespace

FieldTable<FontDef> FontDef::Fields()
{
    return kFontDefFields;
}

// Defined here, where the element types of the lists (ViewDef, DialogDef) are complete.
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
    Name.ReadCount(in, LEGACY_GAME_NAME_LENGTH);
    in->ReadInt16(); // alignment padding to int32 (gamename: 50 -> 52 bytes)
    in->ReadArrayOfInt32(Options, MAX_OPTIONS);
    in->Read(&PaletteUses[0], sizeof(PaletteUses));
    // colors are an array of chars
    in->Read(&DefaultPalette[0], sizeof(DefaultPalette));
    ViewCount = in->ReadInt32();
    CharacterCount = in->ReadInt32();
    PlayerCharacterID = in->ReadInt32();
    in->ReadInt32(); // [DEPRECATED]
    InventoryItemCount = static_cast<uint16_t>(in->ReadInt16());
    in->ReadInt16(); // alignment padding to int32
    DialogCount = in->ReadInt32();
    DialogMessageCount = in->ReadInt32();
    FontCount = in->ReadInt32();
    BytesPerPixel = in->ReadInt32();
    TargetWindows = in->ReadInt32();
    DialogBulletSprite = in->ReadInt32();
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    in->ReadInt16(); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    UniqueID = in->ReadInt32();
    GuiCount = in->ReadInt32();
    MouseCursorCount = in->ReadInt32();
    [[maybe_unused]] GameResolutionType resolution_type = (GameResolutionType)in->ReadInt32();
    assert(resolution_type == kGameResolution_Custom);
    Size game_size;
    game_size.Width = in->ReadInt32();
    game_size.Height = in->ReadInt32();
    SetGameResolution(game_size);

    DefaultLipSyncFrame = in->ReadInt32();
    InventoryCursorHotspotSprite = in->ReadInt32();
    InventoryCursorHotspotColor = in->ReadInt32();
    InventoryCursorHotspotCrossColor = in->ReadInt32();
    in->ReadArrayOfInt32(Reserved, NUM_INTS_RESERVED);
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
        for (int i = 0; i < MouseCursorCount; ++i)
        {
            if (MouseCursors[i].Role == role)
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
    StrUtil::ReadCStrCount(Guid, in, MAX_GUID_LENGTH);
    StrUtil::ReadCStrCount(SaveGameFileExtension, in, MAX_SG_EXT_LENGTH);
    SaveGameFolderName.ReadCount(in, LEGACY_MAX_SG_FOLDER_LEN);
}

void GameDef::read_font_infos(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    Fonts.resize(FontCount);
    for (int i = 0; i < FontCount; ++i)
    {
        Fonts[i].ID = i;
        uint32_t flags = in->ReadInt32();
        Fonts[i].Size = in->ReadInt32();
        Fonts[i].Outline = in->ReadInt32();
        Fonts[i].OffsetY = in->ReadInt32();
        Fonts[i].LineSpacing = std::max(0, in->ReadInt32());
        Fonts[i].SetFlags(flags);
    }
}

void GameDef::ReadInvInfo(Stream *in, GameDataVersion data_ver)
{
    InventoryItems.resize(InventoryItemCount);
    for (int i = 0; i < InventoryItemCount; ++i)
    {
        InventoryItems[i].ReadFromFile(in, data_ver);
    }
}

HGameFileError GameDef::read_cursors(Common::Stream *in)
{
    MouseCursors.resize(MouseCursorCount);
    ReadMouseCursors(in);
    return HGameFileError::None();
}

HError GameDef::read_interaction_scripts(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    Characters.resize(CharacterCount); // safety precaution, because chars may be read later
    HError err;
    for (size_t i = 0; i < (size_t)CharacterCount; ++i)
    {
        err = Characters[i].Interactions.Read_v361(in);
        if (!err)
            return err;
    }
    // NOTE: inventory item events are loaded starting from index 1, because index 0 is an unused item
    InventoryItems.resize(InventoryItemCount); // safety precaution
    for (size_t i = 1; i < (size_t)InventoryItemCount; ++i)
    {
        err = InventoryItems[i].Interactions.Read_v361(in);
        if (!err)
            return err;
    }
    return HError::None();
}

void GameDef::read_words_dictionary(Common::Stream *in)
{
    ParserDictionary.reset(new WordsDictionary());
    ParserDictionary->ReadFromFile(in);
}

void GameDef::ReadMouseCursors(Stream *in)
{
    for (int i = 0; i < MouseCursorCount; ++i)
    {
        MouseCursors[i].ReadFromFile(in);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 2

void GameDef::read_characters(Common::Stream *in, GameDataVersion data_ver)
{
    Characters.resize(CharacterCount);
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
    for (int i = 0; i < CharacterCount; ++i)
    {
        Characters[i].ReadFromFile(in, data_ver);
    }
}

//-----------------------------------------------------------------------------
// Reading Part 3

HGameFileError GameDef::read_customprops(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    if (Properties::ReadSchema(CustomPropertySchema, in) != kPropertyErr_NoError)
        return new MainGameFileError(kMGFErr_InvalidPropertySchema);

    int errors = 0;

    charProps.resize(CharacterCount);
    for (int i = 0; i < CharacterCount; ++i)
    {
        errors += Properties::ReadValues(charProps[i], in);
    }
    invProps.resize(InventoryItemCount);
    for (int i = 0; i < InventoryItemCount; ++i)
    {
        errors += Properties::ReadValues(invProps[i], in);
    }

    if (errors > 0)
        return new MainGameFileError(kMGFErr_InvalidPropertyValues);

    ViewNames.resize(ViewCount);
    for (int i = 0; i < ViewCount; ++i)
        ViewNames[i] = String::FromStream(in);

    InventoryItemScriptNames.resize(InventoryItemCount);
    for (int i = 0; i < InventoryItemCount; ++i)
        InventoryItemScriptNames[i] = String::FromStream(in);

    DialogScriptNames.resize(DialogCount);
    for (int i = 0; i < DialogCount; ++i)
        DialogScriptNames[i] = String::FromStream(in);
    return HGameFileError::None();
}

HGameFileError GameDef::read_audio(Common::Stream *in, GameDataVersion /*data_ver*/)
{
    size_t audiotype_count = in->ReadInt32();
    AudioClipTypes.resize(audiotype_count);
    for (size_t i = 0; i < audiotype_count; ++i)
    {
        AudioClipTypes[i].ReadFromFile(in);
    }

    size_t audioclip_count = in->ReadInt32();
    AudioClips.resize(audioclip_count);
    ReadAudioClips(in, audioclip_count);

    in->ReadInt32(); // [DEPRECATED]
    return HGameFileError::None();
}

void GameDef::read_room_names(Stream *in, GameDataVersion data_ver)
{
    if ((data_ver >= kGameVersion_400_13) || (Options[OPT_DEBUGMODE] != 0))
    {
        uint32_t room_count = in->ReadInt32();
        RoomNumbers.resize(room_count);
        for (uint32_t i = 0; i < room_count; ++i)
        {
            int room_number = in->ReadInt32();
            String room_name = String::FromStream(in);
            RoomNumbers[i] = room_number;
            RoomNames.insert(std::make_pair(room_number, room_name));
        }
    }
}

void GameDef::ReadAudioClips(Common::Stream *in, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        AudioClips[i].ReadFromFile(in);
        // a clip's id is its index in the list; the id that the file holds is not used
        AudioClips[i].ID = static_cast<int>(i);
    }
}
