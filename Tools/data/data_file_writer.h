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

#include <memory>
#include <vector>

#include "data/game_utils.h"
#include "util/error.h"
#include "util/stream.h"

namespace AGS
{
namespace DataFileWriter
{

using AGS::Common::Stream;

// Each function below writes the entity or block that its name gives, in the form the matching reader in Common
// takes (the .cpp names the reader above every definition). The "Ext" ones write the payload of an extension block
// whose name is the digits in the function name (WriteExt363GameInfo: "v363_gameinfo"; WriteExtension() frames it).

// Writes one character's CharacterInfo record
void WriteCharacter(Stream *out, const DataUtil::GameData &game, const DataUtil::CharacterData &character, int index);
// Writes one inventory item record
void WriteInventoryItem(Stream *out, const DataUtil::InventoryItemData &item);
// Writes one mouse cursor record
void WriteCursor(Stream *out, const DataUtil::CursorData &cursor);
// Writes the text parser's word dictionary
void WriteTextParserDictionary(const DataUtil::GameData &game, Stream *out);
// Writes one view with its loops and frames
void WriteView(Stream *out, const DataUtil::GameData &game, const DataUtil::ViewData &view);
// Writes one audio clip type
void WriteAudioType(Stream *out, const DataUtil::AudioTypeData &type, int id);
// Writes one audio clip
void WriteAudioClip(Stream *out, const DataUtil::AudioClipData &clip, int index);
// Writes the "v363_dialogsnew" extension payload: the dialog topics
void WriteExt363Dialogs(Stream *out, const DataUtil::GameData &game);
// Writes a custom property schema
void WritePropertySchemaBlock(Stream *out, const std::vector<DataUtil::CustomPropertySchemaItem> &schema);
// Writes one object's custom property values
void WritePropertyValues(Stream *out, const std::vector<DataUtil::CustomPropertyValue> &properties);
// Returns the code page of the game text encoding setting ("UTF-8" or "ANSI")
int ParseGameTextEncodingCodePage(const Common::String &value);

// Block-level writers used by WriteGameData28, one per section of the main data
// in file order (the names say which). These are exposed separately so
// that their count, ordering and reserved-slot framing may be unit-tested.
// WriteGlobalMessagesBlock: the legacy global messages, which the reader does not read at the current
// data version (see the .cpp).
void WriteInventoryBlock(const DataUtil::GameData &game, Stream *out);
void WriteCursorBlock(const DataUtil::GameData &game, Stream *out);
void WriteViewsBlock(const DataUtil::GameData &game, Stream *out);
void WriteCharactersBlock(const DataUtil::GameData &game, Stream *out);
void WriteGlobalMessagesBlock(const DataUtil::GameData &game, Stream *out);
void WriteCustomPropertiesBlock(const DataUtil::GameData &game, Stream *out);
void WriteAudioBlock(const DataUtil::GameData &game, Stream *out);
void WriteFontBlock(const DataUtil::GameData &game, Stream *out);
void WriteSpriteFlags(const DataUtil::GameData &game, Stream *out);
void WriteInteractionScriptsBlock(const DataUtil::GameData &game, Stream *out);
void WriteLipSyncBlock(const DataUtil::GameData &game, Stream *out);
void WriteSaveGameInfo(const DataUtil::GameData &game, Stream *out);
void WriteLegacyScriptNamesBlock(const DataUtil::GameData &game, Stream *out);
void WritePluginsBlock(const DataUtil::GameData &game, Stream *out);
void WriteRoomNamesBlock(const DataUtil::GameData &game, Stream *out);
void WriteGuiBlock(const DataUtil::GameData &game, Stream *out);

// Extension block payloads, one per block id
void WriteExt360Fonts(Stream *out, const DataUtil::GameData &game);
void WriteExt360Cursors(Stream *out, const DataUtil::GameData &game);
void WriteExt361ObjNames(Stream *out, const DataUtil::GameData &game);
void WriteExt362Interactions(Stream *out, const DataUtil::GameData &game);
void WriteExt363GameInfo(Stream *out, const DataUtil::GameData &game);
// Writes the look properties of one GUI control (the part of "v363_guictrls2" common to all control types)
void WriteGuiControlLooks363(Stream *out, const DataUtil::GUIControlData &control);
void WriteExt363GuiControls(Stream *out, const DataUtil::GameData &game);

using ExtensionWriter = void (*)(Stream *, const DataUtil::GameData &);
// Writes an extension record: the id, the payload's length and the payload that `writer` produces
void WriteExtension(Stream *out, const char *id, const DataUtil::GameData &game, ExtensionWriter writer);

// Serializes the legacy GameSetupStructBase block.
void WriteGameSetupStructBase(const DataUtil::GameData &game, Stream *out, soff_t &ext_offset_pos);

} // namespace DataFileWriter

namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::Stream;

// Serializes the game data to the game28.dta format.
HError WriteGameData28(const GameData &game, std::unique_ptr<Stream> &&out);

} // namespace DataUtil
} // namespace AGS
