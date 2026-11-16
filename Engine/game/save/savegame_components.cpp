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
#include "game/save/savegame_components.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "debug/out.h"
#include "game/audio_clip_type_def.h"
#include "game/character_def.h"
#include "game/common_defines.h"
#include "game/dialog/dialog.h"
#include "game/dialog_def.h"
#include "game/dynobj/cc_serializer.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/inventory_item_def.h"
#include "game/main_game_file.h"
#include "game/mouse_cursor_def.h"
#include "game/render/draw.h"
#include "game/render/dynamic_sprite.h"
#include "game/render/overlay.h"
#include "game/render/screen_overlay.h"
#include "game/save/savegame_internal.h"
#include "game/script.h"
#include "game/system.h"
#include "game/ui/animating_gui_button.h"
#include "game/ui/button.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/view_def.h"
#include "game/world/character.h"
#include "game/world/inventory_item.h"
#include "game/world/move_list.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_state.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/sprite_cache.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guimain.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/common.h"
#include "util/deflatestream.h"
#include "util/geometry.h"
#include "util/memory_compat.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

// Set to 1 to print each component's offsets and sizes while reading and writing
#define DEBUG_SAVECOMPONENTS (0)

//-----------------------------------------------------------------------------
//
// The save format of the game records that Common defines; the records do not
// know it.
//
//-----------------------------------------------------------------------------

// Inventory items' save format
enum InvitemSvgVersion
{
    kInvitemSvgVersion_Initial = 0,
    kInvitemSvgVersion_36304 = 3060304, // hotspot
    kInvitemSvgVersion_36314 = 3060314, // hotspot align
    kInvitemSvgVersion_Current = kInvitemSvgVersion_36314
};

// Dialog topics' save format
enum DialogTopicSvgVersion
{
    kDialogTopicSvgVer_Initial = 0,
    kDialogTopicSvgVer_363 = 3060304,   // new save format
    kDialogTopicSvgVer_40008 = 4000008, // custom properties
    kDialogTopicSvgVer_40026 = 4000026, // sync with kDialogTopicSvgVer_363
};

// Mouse cursors' save format; 0 (initial) is no longer read
enum CursorSvgVersion
{
    kCursorSvgVersion_36016 = 1, // animation delay
};

namespace
{

// What a save holds for a character that the character does not have any more
struct CharacterLegacyFields
{
    Point ViewOffset;
};

void ReadCharacterDef(Character &ch, Stream *in, CharacterLegacyFields &old_fields, CharacterSvgVersion save_ver)
{
    ch.NormalView = in->ReadInt32();
    ch.SpeechView = in->ReadInt32();
    ch.View = in->ReadInt32();
    ch.Room = in->ReadInt32();
    ch.PreviousRoom = in->ReadInt32();
    ch.X = in->ReadInt32();
    ch.Y = in->ReadInt32();
    ch.AnimWait = in->ReadInt32();
    ch.Flags = in->ReadInt32();
    in->ReadInt16(); // legacy following
    in->ReadInt16(); // legacy followinginfo
    ch.IdleView = in->ReadInt32();
    ch.IdleDelay = in->ReadInt16();
    ch.IdleTimeLeft = in->ReadInt16();
    ch.Opacity = in->ReadInt16();
    ch.Baseline = in->ReadInt16();
    ch.ActiveInventory = in->ReadInt32();
    ch.SpeechColor = in->ReadInt32();
    ch.ThinkView = in->ReadInt32();
    ch.BlinkView = in->ReadInt16();
    ch.BlinkInterval = in->ReadInt16();
    ch.BlinkTimer = in->ReadInt16();
    ch.BlinkFrame = in->ReadInt16();
    ch.WalkSpeedY = in->ReadInt16();
    old_fields.ViewOffset.Y = in->ReadInt16(); // legacy 16-bit view offset
    ch.Z = in->ReadInt32();
    ch.WalkWait = in->ReadInt32();
    ch.SpeechAnimationDelay = in->ReadInt16();
    ch.IdleAnimationDelay = in->ReadInt16();
    ch.BlockingWidth = in->ReadInt16();
    ch.BlockingHeight = in->ReadInt16();
    ch.ID = in->ReadInt32();
    old_fields.ViewOffset.X = in->ReadInt16(); // legacy 16-bit view offset
    ch.WalkWaitCounter = in->ReadInt16();
    ch.Loop = in->ReadInt16();
    ch.Frame = in->ReadInt16();
    ch.MoveListID = in->ReadInt16();
    in->ReadInt16(); // legacy animating
    ch.WalkSpeedX = in->ReadInt16();
    ch.AnimationDelay = in->ReadInt16();
    if (save_ver < kCharSvgVersion_400_33)
    {
        int16_t legacy_invq[LEGACY_MAX_INV];
        in->ReadArrayOfInt16(legacy_invq, LEGACY_MAX_INV);
        for (int i = 0; i < LEGACY_MAX_INV; ++i)
            if (legacy_invq[i] > 0)
                ch.InventoryQuantity[i] = legacy_invq[i];
    }
    else
    {
        int invitems = in->ReadInt32();
        for (int i = 0; i < invitems; ++i)
        {
            int itemid = in->ReadInt32();
            int itemq = in->ReadInt32();
            ch.InventoryQuantity[itemid] = itemq;
        }
    }
    in->ReadInt16(); // actx__
    in->ReadInt16(); // acty__

    if (save_ver < kCharSvgVersion_400_03)
    { // Fixed-size name and scriptname
        ch.Name.ReadCount(in, LEGACY_MAX_CHAR_NAME_LEN);
        in->Seek(LEGACY_MAX_SCRIPT_NAME_LEN); // skip legacy scriptname
                                              // (don't overwrite static data from save!)
    }
    else
    {
        ch.Name = StrUtil::ReadString(in);
    }

    if (save_ver >= kCharSvgVersion_400_03)
    {
        in->ReadInt8(); // [OBSOLETE] old enabled + visible flag
    }
    else
    {
        uint8_t on = in->ReadInt8(); // old enabled + visible flag
        // Only treat "on" as visible and make characters from old saves all enabled
        ch.Flags |= CHF_ENABLED | (CHF_VISIBLE)*on;
    }

    if (save_ver >= kCharSvgVersion_400_26)
    {
        ch.BlockingX = in->ReadInt16();
        ch.BlockingY = in->ReadInt16();
    }
    else
    {
        ch.BlockingX = 0;
        ch.BlockingY = 0;
    }
}

void WriteCharacterDef(const Character &ch, Stream *out)
{
    out->WriteInt32(ch.NormalView);
    out->WriteInt32(ch.SpeechView);
    out->WriteInt32(ch.View);
    out->WriteInt32(ch.Room);
    out->WriteInt32(ch.PreviousRoom);
    out->WriteInt32(ch.X);
    out->WriteInt32(ch.Y);
    out->WriteInt32(ch.AnimWait);
    out->WriteInt32(ch.Flags);
    out->WriteInt16(0); // legacy following
    out->WriteInt16(0); // legacy followinfo
    out->WriteInt32(ch.IdleView);
    out->WriteInt16(ch.IdleDelay);
    out->WriteInt16(ch.IdleTimeLeft);
    out->WriteInt16(ch.Opacity);
    out->WriteInt16(ch.Baseline);
    out->WriteInt32(ch.ActiveInventory);
    out->WriteInt32(ch.SpeechColor);
    out->WriteInt32(ch.ThinkView);
    out->WriteInt16(ch.BlinkView);
    out->WriteInt16(ch.BlinkInterval);
    out->WriteInt16(ch.BlinkTimer);
    out->WriteInt16(ch.BlinkFrame);
    out->WriteInt16(ch.WalkSpeedY);
    out->WriteInt16(0); // legacy view offset y
    out->WriteInt32(ch.Z);
    out->WriteInt32(ch.WalkWait);
    out->WriteInt16(ch.SpeechAnimationDelay);
    out->WriteInt16(ch.IdleAnimationDelay);
    out->WriteInt16(ch.BlockingWidth);
    out->WriteInt16(ch.BlockingHeight);
    out->WriteInt32(ch.ID);
    out->WriteInt16(0); // old view offset x
    out->WriteInt16(ch.WalkWaitCounter);
    out->WriteInt16(ch.Loop);
    out->WriteInt16(ch.Frame);
    out->WriteInt16(ch.MoveListID);
    out->WriteInt16(0); // legacy animating
    out->WriteInt16(ch.WalkSpeedX);
    out->WriteInt16(ch.AnimationDelay);
    // new inv item quantity format since kCharSvgVersion_400_33;
    // written in the order of item IDs, so that the same inventory always gives the same bytes
    std::vector<std::pair<int, int>> items(ch.InventoryQuantity.begin(), ch.InventoryQuantity.end());
    std::sort(items.begin(), items.end());
    out->WriteInt32(items.size());
    for (const auto &iq : items)
    {
        out->WriteInt32(iq.first);
        out->WriteInt32(iq.second);
    }
    out->WriteInt16(0); // actx__
    out->WriteInt16(0); // acty__

    StrUtil::WriteString(ch.Name, out); // kCharSvgVersion_400_03
    out->WriteInt8(0);                  // [OBSOLETE], old enabled + visible flag
    // kCharSvgVersion_400_26
    out->WriteInt16(ch.BlockingX);
    out->WriteInt16(ch.BlockingY);
}

void ReadInventoryItemDef(InventoryItem &item, Stream *in, InvitemSvgVersion svg_ver)
{
    item.name = StrUtil::ReadString(in);
    item.pic = in->ReadInt32();
    item.cursorPic = in->ReadInt32();
    if (svg_ver >= kInvitemSvgVersion_36304)
    {
        item.hotx = in->ReadInt16();
        item.hoty = in->ReadInt16();
    }

    if (svg_ver < kInvitemSvgVersion_36304)
    {
        item.hotAlign = ((item.hotx == 0) && (item.hoty == 0)) ? kAlignMiddleCenter : kAlignTopLeft;
    }
    else
    {
        item.hotAlign = kAlignTopLeft;
    }

    if (svg_ver >= kInvitemSvgVersion_36314)
    {
        item.hotAlign = static_cast<FrameAlignment>(in->ReadInt32());
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
}

void WriteInventoryItemDef(const InventoryItem &item, Stream *out)
{
    StrUtil::WriteString(item.name, out);
    out->WriteInt32(item.pic);
    out->WriteInt32(item.cursorPic);
    // kInvitemSvgVersion_36304
    out->WriteInt16(item.hotx);
    out->WriteInt16(item.hoty);
    // kInvitemSvgVersion_36314
    out->WriteInt32(item.hotAlign);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
}

void ReadMouseCursor(MouseCursor &cursor, Stream *in)
{
    cursor.pic = in->ReadInt32();
    cursor.hotx = static_cast<int16_t>(in->ReadInt32());
    cursor.hoty = static_cast<int16_t>(in->ReadInt32());
    cursor.view = static_cast<int16_t>(in->ReadInt32());
    cursor.flags = static_cast<int8_t>(in->ReadInt32());
    cursor.animdelay = in->ReadInt32();
}

void WriteMouseCursor(const MouseCursor &cursor, Stream *out)
{
    out->WriteInt32(cursor.pic);
    out->WriteInt32(cursor.hotx);
    out->WriteInt32(cursor.hoty);
    out->WriteInt32(cursor.view);
    out->WriteInt32(cursor.flags);
    out->WriteInt32(cursor.animdelay);
}

void ReadDialogOption(DialogOption &opt, Stream *in)
{
    opt.Text = StrUtil::ReadString(in);
    opt.Flags = in->ReadInt32();
    if ((opt.Flags & DFLG_TEXTSET) == 0)
        opt.Text = opt.OriginalText;
}

void ReadDialogTopic(Dialog &topic, Stream *in, DialogTopicSvgVersion svg_ver, uint32_t *read_opt_count)
{
    if ((svg_ver <= kDialogTopicSvgVer_Initial) || (svg_ver == kDialogTopicSvgVer_40008))
    {
        uint32_t opt_idx = 0;
        for (; opt_idx < LEGACY_MAXTOPICOPTIONS && opt_idx < topic.Options.size(); ++opt_idx)
            topic.Options[opt_idx].Flags = in->ReadInt32();
        // skip if there's any mismatched count (?)
        for (; opt_idx < LEGACY_MAXTOPICOPTIONS; ++opt_idx)
            in->ReadInt32();

        if (read_opt_count)
            *read_opt_count = std::min<uint32_t>(topic.Options.size(), LEGACY_MAXTOPICOPTIONS);
    }
    else
    {
        uint32_t option_records = in->ReadInt32();
        uint32_t opt_idx = 0;
        for (opt_idx = 0; opt_idx < option_records && opt_idx < topic.Options.size(); ++opt_idx)
            ReadDialogOption(topic.Options[opt_idx], in);
        // skip if there's any mismatched count (?)
        DialogOption dummy;
        for (; opt_idx < option_records; ++opt_idx)
            ReadDialogOption(dummy, in);

        if (read_opt_count)
            *read_opt_count = option_records;
    }
}

void WriteDialogTopic(const Dialog &topic, Stream *out)
{
    out->WriteInt32(topic.Options.size());
    for (auto &opt : topic.Options)
    {
        if ((opt.Flags & DFLG_TEXTSET) != 0)
            StrUtil::WriteString(opt.Text, out);
        else
            StrUtil::WriteString(nullptr, 0, out);
        out->WriteInt32(opt.Flags);
    }
}

void ReadAudioClipType(AudioClipType &type, Stream *in)
{
    type.volume_reduction_while_speech_playing = in->ReadInt32();
    type.crossfadeSpeed = in->ReadInt32();
}

void WriteAudioClipType(const AudioClipType &type, Stream *out)
{
    out->WriteInt32(type.volume_reduction_while_speech_playing);
    out->WriteInt32(type.crossfadeSpeed);
}

void ReadGameSetupStruct(GameDef &setup, Stream *in, GameStateSvgVersion svg_ver)
{
    // of GameDef
    in->ReadArrayOfInt32(setup.Options, OPT_HIGHESTOPTION_321 + 1);
    setup.Options[OPT_LIPSYNCTEXT] = in->ReadInt32();
    setup.PlayerCharacterID = in->ReadInt32();
    setup.DialogBulletSprite = in->ReadInt32();
    // legacy uint16 values of a inv cursor hotdot and cross colors
    setup.InventoryCursorHotspotColor = RemapFromLegacyColourNumber(setup, in->ReadInt16());
    setup.InventoryCursorHotspotCrossColor = RemapFromLegacyColourNumber(setup, in->ReadInt16());
    setup.InventoryCursorHotspotSprite = in->ReadInt32();
    setup.DefaultLipSyncFrame = in->ReadInt32();
    if (svg_ver >= kGSSvgVersion_400_27)
    {
        setup.InventoryCursorHotspotColor = in->ReadInt32();
        setup.InventoryCursorHotspotCrossColor = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
    }
}

void WriteGameSetupStruct(const GameDef &setup, Stream *out)
{
    // of GameDef
    out->WriteArrayOfInt32(setup.Options, OPT_HIGHESTOPTION_321 + 1);
    out->WriteInt32(setup.Options[OPT_LIPSYNCTEXT]);
    out->WriteInt32(setup.PlayerCharacterID);
    out->WriteInt32(setup.DialogBulletSprite);
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hotdot color
    out->WriteInt16(0); // [DEPRECATED] uint16 value of a inv cursor hot cross color
    out->WriteInt32(setup.InventoryCursorHotspotSprite);
    out->WriteInt32(setup.DefaultLipSyncFrame);
    // kGSSvgVersion_400_27
    out->WriteInt32(setup.InventoryCursorHotspotColor);
    out->WriteInt32(setup.InventoryCursorHotspotCrossColor);
    out->WriteInt32(0);
    out->WriteInt32(0);
}

} // namespace

namespace AGS
{
namespace Engine
{

namespace SavegameComponents
{

//-----------------------------------------------------------------------------
//
// Helper functions: component tags and game content assertions.
//
//-----------------------------------------------------------------------------

// Tag used to mark the beginning of a save component list
const String ComponentListTag = "Components";

// Writes a opening or closing tag for a save component
void WriteFormatTag(Stream *out, const String &tag, bool open = true)
{
    String full_tag = String::FromFormat(open ? "<%s>" : "</%s>", tag.GetCStr());
    out->Write(full_tag.GetCStr(), full_tag.GetLength());
}

// Reads a opening or closing save component tag and asserts its format
bool ReadFormatTag(Stream *in, String &tag, bool open = true)
{
    if (in->ReadByte() != '<')
        return false;
    if (!open && in->ReadByte() != '/')
        return false;
    tag.Empty();
    while (!in->EOS())
    {
        char c = in->ReadByte();
        if (c == '>')
            return true;
        tag.AppendChar(c);
    }
    return false; // reached EOS before closing symbol
}

// Reads a component tag and asserts that it matches expected name
bool AssertFormatTag(Stream *in, const String &tag, bool open = true)
{
    String read_tag;
    if (!ReadFormatTag(in, read_tag, open))
        return false;
    return read_tag.Compare(tag) == 0;
}

// Reads a component tag and asserts that it matches expected name; formats error message on failure
bool AssertFormatTagStrict(Stream *in, HSaveError &err, const String &tag, const String &info, bool open = true)
{
    String read_tag;
    if (!ReadFormatTag(in, read_tag, open) || read_tag.Compare(tag) != 0)
    {
        err = new SavegameError(kSvgErr_InconsistentFormat,
                                info.IsEmpty()
                                    ? String::FromFormat("Mismatching tag: %s.", tag.GetCStr())
                                    : String::FromFormat("Mismatching tag: %s (%s).", tag.GetCStr(), info.GetCStr()));
        return false;
    }
    return true;
}

inline bool AssertFormatTagStrict(Stream *in, HSaveError &err, const String &tag, bool open = true)
{
    return AssertFormatTagStrict(in, err, tag, String(), open);
}

// Asserts that the read data count does not exceed engine capabilities
inline bool AssertCompatLimit(HSaveError &err, int count, int max_count, const char *content_name)
{
    if (count > max_count)
    {
        err = new SavegameError(kSvgErr_IncompatibleEngine, "Incompatible number of %s (count: %d, max: %d).",
                                content_name, count, max_count);
        return false;
    }
    return true;
}

// Asserts that the read data range does not exceed engine capabilities
inline bool AssertCompatRange(HSaveError &err, int value, int min_value, int max_value, const char *content_name)
{
    if (value < min_value || value > max_value)
    {
        err = new SavegameError(kSvgErr_IncompatibleEngine,
                                "Restore game error: incompatible %s (id: %d, range: %d - %d).", content_name, value,
                                min_value, max_value);
        return false;
    }
    return true;
}

// Handles save-game mismatch; chosen action depends on SaveRestorationFlags
inline bool HandleGameContentMismatch(HSaveError &err, const uint32_t new_val, const uint32_t original_val,
                                      const String &error_text, SaveRestoreResult &result)
{
    SaveRestorationFlags &restore_flags = result.RestoreFlags;
    if (((new_val > original_val) && (restore_flags & kSaveRestore_AllowMismatchExtra) == 0) ||
        ((new_val < original_val) && (restore_flags & kSaveRestore_AllowMismatchLess) == 0))
    {
        err = new SavegameError(kSvgErr_GameContentAssertion, error_text);
        return false; // numbers mismatch, and not allowed to, fail
    }

    if (result.FirstMismatchError.IsEmpty())
        result.FirstMismatchError = error_text;
    Debug::Printf(kDbgMsg_Warn, "Restored save mismatches game: %s", error_text.GetCStr());

    if (new_val > original_val)
    {
        restore_flags = (SaveRestorationFlags)(restore_flags | kSaveRestore_ExtraDataInSave);
    }
    else
    {
        restore_flags = (SaveRestorationFlags)(restore_flags | kSaveRestore_MissingDataInSave);
        if ((restore_flags & kSaveRestore_ClearData) == 0)
        {
            result.Feedback.RetryWithClearGame = true;
            result.Feedback.RetryWithoutComponents = kSaveCmp_All;
            err = new SavegameError(kSvgErr_GameContentAssertion);
            return false; // mismatch is allowed, but we require a clear game data to proceed
        }
    }
    return true;
}

// Handles save-game mismatch where save has an extra object that the game does not
inline bool HandleExtraGameComponent(HSaveError &err, const char *content_name, const String &obj_name,
                                     SaveRestoreResult &result)
{
    const String error_text = String::FromFormat("Extra %s found in save that does not exist in the game: %s.",
                                                 content_name, obj_name.GetCStr());
    return HandleGameContentMismatch(err, 1, 0, error_text, result);
}

// Handles save-game mismatch where save is missing an object that the game has
inline bool HandleMissingGameComponent(HSaveError &err, const char *content_name, const String &obj_name,
                                       SaveRestoreResult &result)
{
    const String error_text =
        String::FromFormat("Missing %s in save that exists in the game: %s.", content_name, obj_name.GetCStr());
    return HandleGameContentMismatch(err, 0, 1, error_text, result);
}

// Tests a match between game's and save's data count, handles mismatch using SaveRestorationFlags
inline bool AssertGameContent(HSaveError &err, const uint32_t new_val, const uint32_t original_val,
                              const char *content_name, SaveRestoreResult &result, uint32_t &record_count)
{
    record_count = new_val;
    if (new_val == original_val)
        return true; // numbers match, success

    const String error_text =
        String::FromFormat("Mismatching number of %s (game: %u, save: %u).", content_name, original_val, new_val);
    return HandleGameContentMismatch(err, new_val, original_val, error_text, result);
}

// Tests a match between game's and save's data count, uses default mismatch handling (always error)
inline bool AssertGameContent(HSaveError &err, const uint32_t new_val, const uint32_t original_val,
                              const char *content_name)
{
    SaveRestoreResult dummy_result;
    uint32_t dummy_count;
    return AssertGameContent(err, new_val, original_val, content_name, dummy_result, dummy_count);
}

// Tests a match between game's and save's data count, handles mismatch using SaveRestorationFlags
inline bool AssertGameObjectContent(HSaveError &err, const uint32_t new_val, const uint32_t original_val,
                                    const char *content_name, const char *obj_type, const uint32_t obj_id,
                                    SaveRestoreResult &result, uint32_t &record_count)
{
    record_count = new_val;
    if (new_val == original_val)
        return true; // numbers match, success

    const String error_text = String::FromFormat("Mismatching number of %s, %s #%u (game: %u, save: %u).", content_name,
                                                 obj_type, obj_id, original_val, new_val);
    return HandleGameContentMismatch(err, new_val, original_val, error_text, result);
}

// Tests a match between game's and save's data count, handles mismatch using SaveRestorationFlags
inline bool AssertGameObjectContent2(HSaveError &err, const uint32_t new_val, const uint32_t original_val,
                                     const char *content_name, const char *obj1_type, const uint32_t obj1_id,
                                     const char *obj2_type, const uint32_t obj2_id, SaveRestoreResult &result,
                                     uint32_t &record_count)
{
    record_count = new_val;
    if (new_val == original_val)
        return true; // numbers match, success

    const String error_text =
        String::FromFormat("Mismatching number of %s, %s #%u, %s #%u (game: %u, save: %u).", content_name, obj1_type,
                           obj1_id, obj2_type, obj2_id, original_val, new_val);
    return HandleGameContentMismatch(err, new_val, original_val, error_text, result);
}

//-----------------------------------------------------------------------------
//
// Save components writers and readers.
//
//-----------------------------------------------------------------------------

HSaveError WriteGameInfo(Stream *out)
{
    StrUtil::WriteStringMap(Game::Current().GameInfo, out);
    return HSaveError::None();
}

HSaveError ReadGameInfo(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                        RestoredData &r_data)
{
    // We don't overwrite current game's info, but read this into RestoredData,
    // in case they want to check it in "validate_restored_save"
    StrUtil::ReadStringMap(r_data.DataCounts.GameInfo, in);
    return HSaveError::None();
}

HSaveError PrescanGameInfo(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                           RestoredData &r_data)
{
    StrUtil::ReadStringMap(r_data.DataCounts.GameInfo, in);
    return HSaveError::None();
}

void WriteCameraState(const Camera &cam, Stream *out)
{
    int flags = 0;
    if (cam.IsLocked())
        flags |= kSvgCamPosLocked;
    out->WriteInt32(flags);
    const Rect &rc = cam.GetRect();
    out->WriteInt32(rc.Left);
    out->WriteInt32(rc.Top);
    out->WriteInt32(rc.GetWidth());
    out->WriteInt32(rc.GetHeight());
    // kGSSvgVersion_400_18
    out->WriteInt32(cam.GetShaderID());
    out->WriteInt32(cam.GetShaderHandle());
    out->WriteFloat32(cam.GetRotation());
    out->WriteInt32(0); // reserved
    // kGSSvgVersion_400_29
    const auto &pivot = cam.GetPivot();
    const auto &pivot_off = cam.GetPivotOffset();
    out->WriteFloat32(pivot.X);
    out->WriteFloat32(pivot.Y);
    out->WriteInt32(pivot_off.X);
    out->WriteInt32(pivot_off.Y);
}

void WriteViewportState(const Viewport &view, Stream *out)
{
    int flags = 0;
    if (view.IsVisible())
        flags |= kSvgViewportVisible;
    out->WriteInt32(flags);
    const Rect &rc = view.GetRect();
    out->WriteInt32(rc.Left);
    out->WriteInt32(rc.Top);
    out->WriteInt32(rc.GetWidth());
    out->WriteInt32(rc.GetHeight());
    out->WriteInt32(view.GetZOrder());
    auto cam = view.GetCamera();
    if (cam)
        out->WriteInt32(cam->GetID());
    else
        out->WriteInt32(-1);
    // kGSSvgVersion_400_18
    out->WriteInt32(view.GetShaderID());
    out->WriteInt32(view.GetShaderHandle());
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
}

HSaveError WriteGameState(Stream *out)
{
    Game &game = Game::Current();
    // Game base
    WriteGameSetupStruct(game, out);
    // Game palette
    // TODO: probably no need to save this for hi/true-res game
    out->WriteArray(game.Palette, sizeof(RGB), 256);

    // Game state
    game.Play.WriteForSavegame(out);
    // Other dynamic values
    out->WriteInt32(game.FramesPerSecond);
    out->WriteInt32(get_loop_counter());
    out->WriteInt32(game.IfacePopped);
    out->WriteInt32(game.Paused);
    // Mouse cursor
    out->WriteInt32(game.Cursor.Mode);
    out->WriteInt32(game.Cursor.Look);
    out->WriteInt32(game.MouseOnIface);

    // Viewports and cameras
    int viewcam_flags = 0;
    if (game.Viewports.IsAutoRoomViewport())
        viewcam_flags |= kSvgGameAutoRoomView;
    out->WriteInt32(viewcam_flags);
    out->WriteInt32(game.Viewports.GetRoomCameraCount());
    for (int i = 0; i < game.Viewports.GetRoomCameraCount(); ++i)
        WriteCameraState(*game.Viewports.GetRoomCamera(i), out);
    out->WriteInt32(game.Viewports.GetRoomViewportCount());
    for (int i = 0; i < game.Viewports.GetRoomViewportCount(); ++i)
        WriteViewportState(*game.Viewports.GetRoomViewport(i), out);

    return HSaveError::None();
}

void ReadCameraState(GameStateSvgVersion svg_ver, RestoredData &r_data, Stream *in)
{
    RestoredData::CameraData cam;
    cam.ID = r_data.Cameras.size();
    cam.Flags = in->ReadInt32();
    cam.Left = in->ReadInt32();
    cam.Top = in->ReadInt32();
    cam.Width = in->ReadInt32();
    cam.Height = in->ReadInt32();
    if (svg_ver >= kGSSvgVersion_400_18)
    {
        cam.ShaderID = in->ReadInt32();
        cam.ShaderHandle = in->ReadInt32();
        cam.Rotation = in->ReadFloat32();
        in->ReadInt32(); // reserved
    }
    if (svg_ver >= kGSSvgVersion_400_29)
    {
        float pivotx = in->ReadFloat32();
        float pivoty = in->ReadFloat32();
        int pivot_offx = in->ReadInt32();
        int pivot_offy = in->ReadInt32();
        cam.Pivot = Pointf(pivotx, pivoty);
        cam.PivotOffset = Point(pivot_offx, pivot_offy);
    }
    r_data.Cameras.push_back(cam);
}

void ReadViewportState(GameStateSvgVersion svg_ver, RestoredData &r_data, Stream *in)
{
    RestoredData::ViewportData view;
    view.ID = r_data.Viewports.size();
    view.Flags = in->ReadInt32();
    view.Left = in->ReadInt32();
    view.Top = in->ReadInt32();
    view.Width = in->ReadInt32();
    view.Height = in->ReadInt32();
    view.ZOrder = in->ReadInt32();
    view.CamID = in->ReadInt32();
    if (svg_ver >= kGSSvgVersion_400_18)
    {
        view.ShaderID = in->ReadInt32();
        view.ShaderHandle = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
    }
    r_data.Viewports.push_back(view);
}

HSaveError ReadGameState(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                         RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    GameStateSvgVersion svg_ver = (GameStateSvgVersion)cmp_ver;
    // Game base
    ReadGameSetupStruct(game, in, svg_ver);
    // Game palette
    in->ReadArray(game.Palette, sizeof(RGB), 256);

    // Game state
    game.Play.ReadFromSavegame(in, game.LoadedDataVersion, svg_ver, r_data);

    // Other dynamic values
    r_data.FPS = in->ReadInt32();
    set_loop_counter(in->ReadInt32());
    game.IfacePopped = in->ReadInt32();
    game.Paused = in->ReadInt32();
    // Mouse cursor state
    r_data.CursorMode = in->ReadInt32();
    r_data.CursorID = in->ReadInt32();
    game.MouseOnIface = in->ReadInt32();

    // Viewports and cameras
    {
        int viewcam_flags = in->ReadInt32();
        game.Viewports.SetAutoRoomViewport((viewcam_flags & kSvgGameAutoRoomView) != 0);
        // TODO: we create viewport and camera objects here because they are
        // required for the managed pool deserialization, but read actual
        // data into temp structs because we need to apply it after active
        // room is loaded.
        // See comments to RestoredData struct for further details.
        int cam_count = in->ReadInt32();
        for (int i = 0; i < cam_count; ++i)
        {
            game.Viewports.CreateRoomCamera();
            ReadCameraState(svg_ver, r_data, in);
        }
        int view_count = in->ReadInt32();
        for (int i = 0; i < view_count; ++i)
        {
            game.Viewports.CreateRoomViewport();
            ReadViewportState(svg_ver, r_data, in);
        }
    }
    return err;
}

// Savegame data format for Audio state
enum AudioSvgVersion
{
    // 0 initial, 1 (3.5.0.26) source position settings: no longer read
    kAudioSvgVersion_36009 = 2,       // up number of channels
    kAudioSvgVersion_36130 = 3060130, // playback state
    kAudioSvgVersion_36215 = 3060215, // clips referenced by filename
    kAudioSvgVersion_36303 = 3060303, // explicit audio type
};

HSaveError WriteAudio(Stream *out)
{
    Game &game = Game::Current();
    // Game content assertion
    out->WriteInt32(game.AudioClipTypes.size());
    out->WriteInt8(TOTAL_AUDIO_CHANNELS);
    out->WriteInt8(game.GameChannelCount);
    out->WriteInt16(0); // reserved 2 bytes (remains of int32)
    // Audio types
    for (size_t i = 0; i < game.AudioClipTypes.size(); ++i)
    {
        WriteAudioClipType(game.AudioClipTypes[i], out);
        out->WriteInt32(game.Play.default_audio_type_volumes[i]);
    }

    // Audio playbacks (from channels)
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; i++)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(i);
        if (playback)
        {
            out->WriteInt32(playback->sourceClipID);
            // kAudioSvgVersion_36215
            StrUtil::WriteString(playback->fileName, out);
            out->WriteInt8(playback->bundlingType);
            //
            out->WriteInt32(playback->get_pos());
            out->WriteInt32(playback->priority);
            out->WriteInt32(playback->repeat ? 1 : 0);
            out->WriteInt32(playback->get_volume255());
            out->WriteInt32(0); // was redundant data
            out->WriteInt32(playback->get_volume100());
            out->WriteInt32(playback->get_panning());
            out->WriteInt32(playback->get_speed());
            // since 3.5.0.26
            out->WriteInt32(playback->xSource);
            out->WriteInt32(playback->ySource);
            out->WriteInt32(playback->maximumPossibleDistanceAway);
            // since version kAudioSvgVersion_36130
            int playback_flags = 0;
            if (playback->is_paused())
            {
                playback_flags |= kSvgAudioPaused;
            }
            out->WriteInt32(playback_flags);
            out->WriteInt32(playback->sourceClipType);
            out->WriteInt32(0); // reserved
            out->WriteInt32(0);
        }
        else
        {
            out->WriteInt32(-1); // no clip id
            out->WriteInt32(0);  // empty string (fileName)
            out->WriteInt8(kAudioBundle_Undefined);
        }
    }

    // DEPRECATED: legacy crossfade params
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0); // DEPRECATED current_music_type

    // Skip legacy Ambient sounds
    // NOTE: six ints per game channel, kept in the save format and ignored when reading
    for (int i = 0; i < game.GameChannelCount; ++i)
    {
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
    return HSaveError::None();
}

HSaveError ReadAudio(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/, RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    // Game content assertion
    const uint32_t audiocliptype_read = in->ReadInt32();
    if (!AssertGameContent(err, audiocliptype_read, game.AudioClipTypes.size(), "Audio Clip Types", r_data.Result,
                           r_data.DataCounts.AudioClipTypes))
        return err;

    const int total_channels = in->ReadInt8();
    const int max_game_channels = in->ReadInt8();
    in->ReadInt16(); // reserved 2 bytes
    if (!AssertCompatLimit(err, total_channels, TOTAL_AUDIO_CHANNELS, "System Audio Channels") ||
        !AssertCompatLimit(err, max_game_channels, MAX_GAME_CHANNELS, "Game Audio Channels"))
    {
        return err;
    }

    // Audio types
    for (uint32_t i = 0; i < audiocliptype_read; ++i)
    {
        ReadAudioClipType(game.AudioClipTypes[i], in);
        game.Play.default_audio_type_volumes[i] = in->ReadInt32();
    }

    // Active playbacks and crossfade
    for (int i = 0; i < total_channels; ++i)
    {
        RestoredData::ChannelInfo &chan_info = r_data.AudioChans[i];
        chan_info.Pos = 0;
        chan_info.ClipID = in->ReadInt32();

        if (cmp_ver >= kAudioSvgVersion_36215)
        {
            chan_info.FileName = StrUtil::ReadString(in);
            chan_info.BundlingType = in->ReadInt8();
        }

        if (chan_info.ClipID >= 0 || !chan_info.FileName.IsEmpty())
        {
            chan_info.Pos = in->ReadInt32();
            if (chan_info.Pos < 0)
                chan_info.Pos = 0;
            chan_info.Priority = in->ReadInt32();
            chan_info.Repeat = in->ReadInt32();
            chan_info.Vol = in->ReadInt32();
            in->ReadInt32(); // was redundant data
            chan_info.VolAsPercent = in->ReadInt32();
            chan_info.Pan = in->ReadInt32();
            chan_info.Speed = in->ReadInt32();
            chan_info.XSource = in->ReadInt32();
            chan_info.YSource = in->ReadInt32();
            chan_info.MaxDist = in->ReadInt32();
            if (cmp_ver >= kAudioSvgVersion_36130)
            {
                chan_info.Flags = in->ReadInt32();
                int audio_type = in->ReadInt32();
                in->ReadInt32(); // reserved
                in->ReadInt32();

                // Because 0 is also a valid audiotype index, check the save version explicitly
                chan_info.AudioType = (cmp_ver >= kAudioSvgVersion_36303) ? audio_type : AUDIOTYPE_UNDEFINED;
            }
        }
    }
    in->ReadInt32(); // DEPRECATED: legacy crossfade params
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32();
    in->ReadInt32(); // DEPRECATED current_music_type

    // Skip legacy Ambient sounds
    // NOTE: six ints per game channel, kept in the save format and ignored when reading
    for (int i = 0; i < max_game_channels; ++i)
    {
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
    }
    return err;
}

HSaveError PrescanAudio(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                        RestoredData &r_data)
{
    HSaveError err;
    const uint32_t audiocliptype_read = in->ReadInt32();
    if (!AssertGameContent(err, audiocliptype_read, Game::Current().AudioClipTypes.size(), "Audio Clip Types",
                           r_data.Result, r_data.DataCounts.AudioClipTypes))
        return err;

    const int total_channels = in->ReadInt8();
    const int max_game_channels = in->ReadInt8();
    if (!AssertCompatLimit(err, total_channels, TOTAL_AUDIO_CHANNELS, "System Audio Channels") ||
        !AssertCompatLimit(err, max_game_channels, MAX_GAME_CHANNELS, "Game Audio Channels"))
    {
        return err;
    }
    return HSaveError::None();
}

HSaveError WriteCharacters(Stream *out)
{
    Game &game = Game::Current();
    out->WriteInt32(game.numcharacters);
    for (int i = 0; i < game.numcharacters; ++i)
    {
        WriteCharacterDef(game.Characters[i], out);
        game.Characters[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.charProps[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadCharacters(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                          RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const uint32_t characters_read = in->ReadInt32();
    if (!AssertGameContent(err, characters_read, game.numcharacters, "Characters", r_data.Result,
                           r_data.DataCounts.Characters))
        return err;

    const CharacterSvgVersion svg_ver = static_cast<CharacterSvgVersion>(cmp_ver);
    for (uint32_t i = 0; i < characters_read; ++i)
    {
        auto &ch = game.Characters[i];
        // TODO: this is ugly reading old fields like that, might either join Read methods in one, or drop old format at some point
        CharacterLegacyFields old_fields;
        ReadCharacterDef(ch, in, old_fields, svg_ver);
        ch.ViewOffset = old_fields.ViewOffset;
        ch.ReadFromSavegame(in, svg_ver);
        Properties::ReadValues(game.Play.charProps[i], in);

        // Upgrade legacy turning mask
        if (svg_ver < kCharSvgVersion_400_16)
        {
            int turns = (ch.MoveListID % LEGACY_CHAR_TURNING_BACKWARDS) / LEGACY_CHAR_TURNING_AROUND;
            bool turn_ccw = ch.MoveListID % LEGACY_CHAR_TURNING_BACKWARDS;
            if (turns > 0)
            {
                ch.SetTurning(true, turn_ccw, turns);
            }
            ch.MoveListID = ch.MoveListID % LEGACY_CHAR_TURNING_AROUND;
        }

        // Upgrade legacy transparency, tint and light ranges
        if (svg_ver < kCharSvgVersion_400_34)
        {
            ch.Opacity = static_cast<int16_t>(GfxDef::LegacyTrans255ToAlpha255(ch.Opacity));
            if (ch.has_explicit_tint())
            {
                ch.TintLevel = static_cast<short>(GfxDef::Value100ToValue255(ch.TintLevel));
                ch.TintLight = static_cast<short>(GfxDef::Value250ToValue255(ch.TintLight));
            }
            else if (ch.has_explicit_light())
            {
                ch.TintLight = static_cast<short>(GfxDef::Value100ToValue255(ch.TintLight));
            }
        }
    }
    return err;
}

HSaveError PrescanCharacters(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                             RestoredData &r_data)
{
    HSaveError err;
    const uint32_t characters_read = in->ReadInt32();
    if (!AssertGameContent(err, characters_read, Game::Current().numcharacters, "Characters", r_data.Result,
                           r_data.DataCounts.Characters))
        return err;
    return HSaveError::None();
}

HSaveError WriteDialogs(Stream *out)
{
    Game &game = Game::Current();
    out->WriteInt32(game.numdialog);
    for (int i = 0; i < game.numdialog; ++i)
    {
        WriteDialogTopic(game.Dialogs[i], out);
        Properties::WriteValues(game.Play.dialogProps[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadDialogs(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                       RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const uint32_t dialogs_read = in->ReadInt32();
    if (!AssertGameContent(err, dialogs_read, game.numdialog, "Dialogs", r_data.Result, r_data.DataCounts.Dialogs))
        return err;

    const DialogTopicSvgVersion svg_ver = static_cast<DialogTopicSvgVersion>(cmp_ver);
    r_data.DataCounts.DialogOptions.resize(dialogs_read);
    for (uint32_t i = 0; i < dialogs_read; ++i)
    {
        uint32_t read_opt_count = 0;
        ReadDialogTopic(game.Dialogs[i], in, svg_ver, &read_opt_count);
        if (!AssertGameObjectContent(err, read_opt_count, game.Dialogs[i].Options.size(), "Options", "Dialog", i,
                                     r_data.Result, r_data.DataCounts.DialogOptions[i]))
            return err;
        if (cmp_ver >= kDialogTopicSvgVer_40008)
            Properties::ReadValues(game.Play.dialogProps[i], in);
    }
    return err;
}

HSaveError PrescanDialogs(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                          RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const uint32_t dialogs_read = in->ReadInt32();
    if (!AssertGameContent(err, dialogs_read, game.numdialog, "Dialogs", r_data.Result, r_data.DataCounts.Dialogs))
        return err;

    r_data.DataCounts.DialogOptions.resize(dialogs_read);
    Dialog dummy;
    for (uint32_t i = 0; i < dialogs_read; ++i)
    {
        uint32_t read_opt_count = 0;
        ReadDialogTopic(dummy, in, static_cast<DialogTopicSvgVersion>(cmp_ver), &read_opt_count);
        if (!AssertGameObjectContent(err, read_opt_count, game.Dialogs[i].Options.size(), "Options", "Dialog", i,
                                     r_data.Result, r_data.DataCounts.DialogOptions[i]))
            return err;
    }
    return HSaveError::None();
}

HSaveError WriteGUI(Stream *out)
{
    Game &game = Game::Current();
    // GUI state
    WriteFormatTag(out, "GUIs");
    out->WriteInt32(game.numgui);
    for (int i = 0; i < game.numgui; ++i)
    {
        game.Guis[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guiProps[i], out);
    }

    WriteFormatTag(out, "GUIButtons");
    out->WriteInt32(static_cast<int32_t>(game.GuiButtons.size()));
    for (size_t i = 0; i < game.GuiButtons.size(); ++i)
    {
        game.GuiButtons[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guicontrolProps[kGUIButton][i], out);
    }

    WriteFormatTag(out, "GUILabels");
    out->WriteInt32(static_cast<int32_t>(game.GuiLabels.size()));
    for (size_t i = 0; i < game.GuiLabels.size(); ++i)
    {
        game.GuiLabels[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guicontrolProps[kGUILabel][i], out);
    }

    WriteFormatTag(out, "GUIInvWindows");
    out->WriteInt32(static_cast<int32_t>(game.GuiInvWindows.size()));
    for (size_t i = 0; i < game.GuiInvWindows.size(); ++i)
    {
        game.GuiInvWindows[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guicontrolProps[kGUIInvWindow][i], out);
    }

    WriteFormatTag(out, "GUISliders");
    out->WriteInt32(static_cast<int32_t>(game.GuiSliders.size()));
    for (size_t i = 0; i < game.GuiSliders.size(); ++i)
    {
        game.GuiSliders[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guicontrolProps[kGUISlider][i], out);
    }

    WriteFormatTag(out, "GUITextBoxes");
    out->WriteInt32(static_cast<int32_t>(game.GuiTextBoxes.size()));
    for (size_t i = 0; i < game.GuiTextBoxes.size(); ++i)
    {
        game.GuiTextBoxes[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guicontrolProps[kGUITextBox][i], out);
    }

    WriteFormatTag(out, "GUIListBoxes");
    out->WriteInt32(static_cast<int32_t>(game.GuiListBoxes.size()));
    for (size_t i = 0; i < game.GuiListBoxes.size(); ++i)
    {
        game.GuiListBoxes[i].WriteToSavegame(out);
        Properties::WriteValues(game.Play.guicontrolProps[kGUIListBox][i], out);
    }

    // Animated buttons
    WriteFormatTag(out, "AnimatedButtons");
    size_t anim_button_count = GetAnimatingButtonCount();
    out->WriteInt32(anim_button_count);
    for (size_t i = 0; i < anim_button_count; ++i)
        GetAnimatingButtonByIndex(i)->WriteToSavegame(out);
    return HSaveError::None();
}

// Builds a list of ranges of control indexes per type per each GUI;
// this lets to know which ranges in global flat control arrays belong to which GUI.
//
// As input this takes a list of control references from GUIMain,
//   where for each GUIMain it lists pairs of ControlType : index in its type's global array.
//
// As output this gives an array of per-Control-Type vectors,
//   where each vector holds a series of ranges [first; last),
//   telling which controls in global array belong to 0th, 1st, 2nd etc GUIMains.
void BuildGUIControlRangeReference(
    const std::vector<std::vector<GUIMain::ControlRef>> &gui_ctrl_refs,
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, kGUIControlTypeNum> &ctrlidx_per_gui)
{
    // Prepare index arrays for each control type
    for (int type = kGUIButton; type < kGUIControlTypeNum; ++type)
    {
        ctrlidx_per_gui[type].resize(gui_ctrl_refs.size());
    }

    // For each GUI...
    for (size_t gui_index = 0; gui_index < gui_ctrl_refs.size(); ++gui_index)
    {
        // Assign the "empty" index range starting at the previous GUI controls range
        if (gui_index > 0)
        {
            for (int type = kGUIButton; type < kGUIControlTypeNum; ++type)
                ctrlidx_per_gui[type][gui_index] = std::make_pair(ctrlidx_per_gui[type][gui_index - 1].second,
                                                                  ctrlidx_per_gui[type][gui_index - 1].second);
        }

        // For each child control reference of this GUI...
        for (const auto &ctrl_ref : gui_ctrl_refs[gui_index])
        {
            GUIControlType type = ctrl_ref.first;
            uint32_t index = static_cast<uint32_t>(ctrl_ref.second);
            ctrlidx_per_gui[type][gui_index].second = std::max(index + 1, ctrlidx_per_gui[type][gui_index].second);
        }
    }
}

// Moves gui controls from source array to dest array, in portions, belonging to matching GUIMains,
// using their ranges per GUI as a reference. The copied amount per GUI is min of controls in old GUI and new GUI.
template <typename TGUIControl>
void MoveGUIControlArray(const std::vector<TGUIControl> &src_arr, std::vector<TGUIControl> &dest_arr,
                         const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_src,
                         const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_dst)
{
    for (size_t cur_gui = 0; cur_gui < ctrlidx_per_gui_src.size() && cur_gui < ctrlidx_per_gui_dst.size(); ++cur_gui)
    {
        for (size_t src_idx = ctrlidx_per_gui_src[cur_gui].first, dst_idx = ctrlidx_per_gui_dst[cur_gui].first;
             src_idx < ctrlidx_per_gui_src[cur_gui].second && src_idx < src_arr.size() &&
             dst_idx < ctrlidx_per_gui_dst[cur_gui].second && dst_idx < dest_arr.size();
             ++src_idx, ++dst_idx)
        {
            dest_arr[dst_idx] = std::move(src_arr[src_idx]);
        }
    }
}

template <typename TGUIControl, GUIControlType control_type>
bool ReadGUIControlArray(std::vector<TGUIControl> &obj_arr, Stream *in, GuiSvgVersion svg_ver, HSaveError &err,
                         const char *tag, const char *friendly_name, RestoredData &r_data,
                         const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_old,
                         const std::vector<std::pair<uint32_t, uint32_t>> &ctrlidx_per_gui_new)
{
    Game &game = Game::Current();
    if (!AssertFormatTagStrict(in, err, tag))
        return false;
    const uint32_t guiobj_read_count = in->ReadInt32();
    if (!AssertGameContent(err, guiobj_read_count, obj_arr.size(), friendly_name, r_data.Result,
                           r_data.DataCounts.Dummy))
        return false;

    // We read and apply controls in a 3-step way:
    // 1. Copy game's controls into the temporary array, size equal to number of elements in save.
    // 2. Read save data into controls in temporary array.
    // 3. Copy controls from temporary array back to game's array.
    //
    // Why: because not all fields are written in saves, so we first copy full fields to temp array,
    // then read dynamic fields only from a save into temp array; keep persistent fields this way.
    std::vector<TGUIControl> guiobj_read(guiobj_read_count);
    MoveGUIControlArray<TGUIControl>(obj_arr, guiobj_read, ctrlidx_per_gui_new, ctrlidx_per_gui_old);
    // Read actual saved GUI control data
    for (uint32_t i = 0; i < guiobj_read_count; ++i)
    {
        guiobj_read[i].ReadFromSavegame(in, svg_ver);
        if (svg_ver >= kGuiSvgVersion_40008)
            Properties::ReadValues(game.Play.guicontrolProps[control_type][i], in);
    }
    MoveGUIControlArray<TGUIControl>(guiobj_read, obj_arr, ctrlidx_per_gui_old, ctrlidx_per_gui_new);
    return true;
}

HSaveError ReadGUI(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/, RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const GuiSvgVersion svg_ver = (GuiSvgVersion)cmp_ver;
    // GUI state
    if (!AssertFormatTagStrict(in, err, "GUIs"))
        return err;
    const uint32_t guis_read = in->ReadInt32();
    if (!AssertGameContent(err, guis_read, game.numgui, "GUIs", r_data.Result, r_data.DataCounts.GUIs))
        return err;
    std::vector<std::vector<GUIMain::ControlRef>> guictrl_refs_old(guis_read);
    for (uint32_t i = 0; i < guis_read; ++i)
    {
        game.Guis[i].ReadFromSavegame(in, svg_ver, guictrl_refs_old[i]);
        if (svg_ver >= kGuiSvgVersion_40008)
            Properties::ReadValues(game.Play.guiProps[i], in);
    }

    r_data.DataCounts.GUIControls.resize(guis_read);

    // Build a reference of the range of control indexes per type per each GUI;
    // this lets us know which range of elements in control arrays belong to which GUI.
    // old refs are read from the save, new refs are from the current game data
    std::vector<std::vector<GUIMain::ControlRef>> guictrl_refs_new;
    for (const auto &gui : game.Guis)
        guictrl_refs_new.push_back(gui.GetControlRefs());
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, kGUIControlTypeNum> ctrlidx_per_gui_new;
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, kGUIControlTypeNum> ctrlidx_per_gui_old;
    BuildGUIControlRangeReference(guictrl_refs_new, ctrlidx_per_gui_new);
    BuildGUIControlRangeReference(guictrl_refs_old, ctrlidx_per_gui_old);

    if (!ReadGUIControlArray<GUIButton, kGUIButton>(game.GuiButtons, in, svg_ver, err, "GUIButtons", "GUI Buttons",
                                                    r_data, ctrlidx_per_gui_old[kGUIButton],
                                                    ctrlidx_per_gui_new[kGUIButton]))
        return err;

    if (!ReadGUIControlArray<GUILabel, kGUILabel>(game.GuiLabels, in, svg_ver, err, "GUILabels", "GUI Labels", r_data,
                                                  ctrlidx_per_gui_old[kGUILabel], ctrlidx_per_gui_new[kGUILabel]))
        return err;

    if (!ReadGUIControlArray<GUIInvWindow, kGUIInvWindow>(game.GuiInvWindows, in, svg_ver, err, "GUIInvWindows",
                                                          "GUI InvWindows", r_data, ctrlidx_per_gui_old[kGUIInvWindow],
                                                          ctrlidx_per_gui_new[kGUIInvWindow]))
        return err;

    if (!ReadGUIControlArray<GUISlider, kGUISlider>(game.GuiSliders, in, svg_ver, err, "GUISliders", "GUI Sliders",
                                                    r_data, ctrlidx_per_gui_old[kGUISlider],
                                                    ctrlidx_per_gui_new[kGUISlider]))
        return err;

    if (!ReadGUIControlArray<GUITextBox, kGUITextBox>(game.GuiTextBoxes, in, svg_ver, err, "GUITextBoxes",
                                                      "GUI TextBoxes", r_data, ctrlidx_per_gui_old[kGUITextBox],
                                                      ctrlidx_per_gui_new[kGUITextBox]))
        return err;

    if (!ReadGUIControlArray<GUIListBox, kGUIListBox>(game.GuiListBoxes, in, svg_ver, err, "GUIListBoxes",
                                                      "GUI ListBoxes", r_data, ctrlidx_per_gui_old[kGUIListBox],
                                                      ctrlidx_per_gui_new[kGUIListBox]))
        return err;

    // Animated buttons
    if (!AssertFormatTagStrict(in, err, "AnimatedButtons"))
        return err;
    int anim_count = in->ReadInt32();
    for (int i = 0; i < anim_count; ++i)
    {
        AnimatingGUIButton anim_button;
        anim_button.ReadFromSavegame(in, cmp_ver);
        AddButtonAnimation(anim_button);
    }
    return err;
}

HSaveError PrescanGUI(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                      RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    // GUI state
    if (!AssertFormatTagStrict(in, err, "GUIs"))
        return err;
    const uint32_t guis_read = in->ReadInt32();
    if (!AssertGameContent(err, guis_read, game.numgui, "GUIs", r_data.Result, r_data.DataCounts.GUIs))
        return err;

    r_data.DataCounts.GUIControls.resize(guis_read);

    const GuiSvgVersion svg_ver = (GuiSvgVersion)cmp_ver;
    std::vector<std::vector<GUIMain::ControlRef>> guictrl_refs_old(guis_read);
    String assert_buf;
    for (uint32_t i = 0; i < guis_read; ++i)
    {
        GUIMain::SkipSavestate(in, svg_ver, &guictrl_refs_old[i]);
        if (svg_ver >= kGuiSvgVersion_40008)
            Properties::SkipValues(in);

        assert_buf.Format("GUI %u Controls", i);
        if (i < game.Guis.size() &&
            !AssertGameContent(err, guictrl_refs_old[i].size(), game.Guis[i].GetControlCount(), assert_buf.GetCStr(),
                               r_data.Result, r_data.DataCounts.GUIControls[i]))
            return err;
    }

    std::array<size_t, kGUIControlTypeNum> ctrl_counts_old = {}, ctrl_counts_new = {};
    for (const auto &refs_arr : guictrl_refs_old)
        for (const auto &ref : refs_arr)
            ctrl_counts_old[ref.first]++;
    for (const auto &gui : game.Guis)
        for (const auto &ref : gui.GetControlRefs())
            ctrl_counts_new[ref.first]++;

    // Overall control objects check
    std::array<const char *, kGUIControlTypeNum> ctrl_name = {
        {"", "GUI Buttons", "GUI Labels", "GUI InvWindows", "GUI Sliders", "GUI TextBoxes", "GUI ListBoxes"}};
    for (int type = kGUIButton; type < kGUIControlTypeNum; ++type)
    {
        if (!AssertGameContent(err, ctrl_counts_old[type], ctrl_counts_new[type], ctrl_name[type], r_data.Result,
                               r_data.DataCounts.Dummy))
            return err;
    }

    return HSaveError::None();
}

HSaveError WriteInventory(Stream *out)
{
    Game &game = Game::Current();
    out->WriteInt32(game.numinvitems);
    for (int i = 0; i < game.numinvitems; ++i)
    {
        WriteInventoryItemDef(game.InventoryItems[i], out);
        Properties::WriteValues(game.Play.invProps[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadInventory(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                         RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const uint32_t invitems_read = in->ReadInt32();
    if (!AssertGameContent(err, invitems_read, game.numinvitems, "Inventory Items", r_data.Result,
                           r_data.DataCounts.InventoryItems))
        return err;
    for (uint32_t i = 0; i < invitems_read; ++i)
    {
        ReadInventoryItemDef(game.InventoryItems[i], in, static_cast<InvitemSvgVersion>(cmp_ver));
        Properties::ReadValues(game.Play.invProps[i], in);
    }
    return err;
}

HSaveError PrescanInventory(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                            RestoredData &r_data)
{
    HSaveError err;
    const uint32_t invitems_read = in->ReadInt32();
    if (!AssertGameContent(err, invitems_read, Game::Current().numinvitems, "Inventory Items", r_data.Result,
                           r_data.DataCounts.InventoryItems))
        return err;
    return HSaveError::None();
}

HSaveError WriteMouseCursors(Stream *out)
{
    Game &game = Game::Current();
    out->WriteInt32(game.numcursors);
    for (int i = 0; i < game.numcursors; ++i)
    {
        WriteMouseCursor(game.MouseCursors[i], out);
    }
    return HSaveError::None();
}

HSaveError ReadMouseCursors(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                            RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const uint32_t cursors_read = in->ReadInt32();
    if (!AssertGameContent(err, cursors_read, game.numcursors, "Mouse Cursors", r_data.Result,
                           r_data.DataCounts.Cursors))
        return err;
    for (uint32_t i = 0; i < cursors_read; ++i)
    {
        ReadMouseCursor(game.MouseCursors[i], in);
    }
    return err;
}

HSaveError PrescanMouseCursors(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                               RestoredData &r_data)
{
    HSaveError err;
    const uint32_t cursors_read = in->ReadInt32();
    if (!AssertGameContent(err, cursors_read, Game::Current().numcursors, "Mouse Cursors", r_data.Result,
                           r_data.DataCounts.Cursors))
        return err;
    return HSaveError::None();
}

enum ViewSvgVersion
{
    kViewSvgVersion_Initial = 0,
    kViewSvgVersion_4000018 = 4000018, // view frame offsets
    kViewSvgVersion_4000024 = 4000024, // view frame events
};

HSaveError WriteViews(Stream *out)
{
    Game &game = Game::Current();
    out->WriteInt32(game.numviews);
    for (int view = 0; view < game.numviews; ++view)
    {
        out->WriteInt32(game.Views[view].numLoops);
        for (int loop = 0; loop < game.Views[view].numLoops; ++loop)
        {
            out->WriteInt32(game.Views[view].loops[loop].numFrames);
            for (int frame = 0; frame < game.Views[view].loops[loop].numFrames; ++frame)
            {
                auto &vf = game.Views[view].loops[loop].frames[frame];
                out->WriteInt32(vf.sound);
                out->WriteInt32(vf.pic);
                // ---- kViewSvgVersion_4000018
                out->WriteInt32(vf.flags);
                out->WriteInt16(vf.speed);
                out->WriteInt16(vf.xoffs);
                out->WriteInt16(vf.yoffs);
            }

            // ---- kViewSvgVersion_4000024
            for (int frame = 0; frame < game.Views[view].loops[loop].numFrames; ++frame)
            {
                StrUtil::WriteCStr(game.Views[view].loops[loop].frames[frame].event_name, out);
            }
        }
    }
    return HSaveError::None();
}

HSaveError ReadViews(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                     RestoredData &r_data)
{
    Game &game = Game::Current();
    HSaveError err;
    const uint32_t views_read = in->ReadInt32();
    if (!AssertGameContent(err, views_read, game.numviews, "Views", r_data.Result, r_data.DataCounts.Views))
        return err;

    r_data.DataCounts.ViewLoops.resize(views_read);
    r_data.DataCounts.ViewFrames.resize(views_read);
    for (uint32_t view = 0; view < views_read; ++view)
    {
        const uint32_t loops_read = in->ReadInt32();
        if (!AssertGameObjectContent(err, loops_read, game.Views[view].numLoops, "Loops", "View", view, r_data.Result,
                                     r_data.DataCounts.ViewLoops[view]))
            return err;

        for (uint32_t loop = 0; loop < loops_read; ++loop)
        {
            const uint32_t frames_read = in->ReadInt32();
            if (!AssertGameObjectContent2(err, frames_read, game.Views[view].loops[loop].numFrames, "Frame", "View",
                                          view, "Loop", loop, r_data.Result, r_data.DataCounts.Dummy))
                return err;

            r_data.DataCounts.ViewFrames[view] += frames_read;
            for (uint32_t frame = 0; frame < frames_read; ++frame)
            {
                auto &vf = game.Views[view].loops[loop].frames[frame];
                vf.sound = in->ReadInt32();
                vf.pic = in->ReadInt32();

                if (cmp_ver >= kViewSvgVersion_4000018)
                {
                    vf.flags = (SpriteTransformFlags)in->ReadInt32();
                    vf.speed = in->ReadInt16();
                    vf.xoffs = in->ReadInt16();
                    vf.yoffs = in->ReadInt16();
                }
            }

            if (cmp_ver >= kViewSvgVersion_4000024)
            {
                for (uint32_t frame = 0; frame < frames_read; ++frame)
                {
                    game.Views[view].loops[loop].frames[frame].event_name = StrUtil::ReadCStr(in);
                }
            }
        }
    }
    return err;
}

HSaveError PrescanViews(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                        RestoredData &r_data)
{
    Game &game = Game::Current();
    // a frame as ReadViews reads it: sound and pic, then flags, speed and offsets since kViewSvgVersion_4000018
    const soff_t frame_size = (cmp_ver >= kViewSvgVersion_4000018)
                                  ? sizeof(int32_t) * 2 + sizeof(int32_t) + sizeof(int16_t) * 3
                                  : sizeof(int32_t) * 2;
    HSaveError err;
    const uint32_t views_read = in->ReadInt32();
    if (!AssertGameContent(err, views_read, game.numviews, "Views", r_data.Result, r_data.DataCounts.Views))
        return err;

    r_data.DataCounts.ViewLoops.resize(views_read);
    r_data.DataCounts.ViewFrames.resize(views_read);
    for (uint32_t view = 0; view < views_read; ++view)
    {
        const uint32_t loops_read = in->ReadInt32();
        if (!AssertGameObjectContent(err, loops_read, game.Views[view].numLoops, "Loops", "View", view, r_data.Result,
                                     r_data.DataCounts.ViewLoops[view]))
            return err;

        for (uint32_t loop = 0; loop < loops_read; ++loop)
        {
            const uint32_t frames_read = in->ReadInt32();
            if (!AssertGameObjectContent2(err, frames_read, game.Views[view].loops[loop].numFrames, "Frame", "View",
                                          view, "Loop", loop, r_data.Result, r_data.DataCounts.Dummy))
                return err;

            r_data.DataCounts.ViewFrames[view] += frames_read;
            in->Seek(frames_read * frame_size);
            // the frames' event names follow each loop's frames since kViewSvgVersion_4000024
            if (cmp_ver >= kViewSvgVersion_4000024)
            {
                for (uint32_t frame = 0; frame < frames_read; ++frame)
                    StrUtil::ReadCStr(in);
            }
        }
    }
    return HSaveError::None();
}

HSaveError WriteDynamicSpritesImpl(Stream *out, int match_flags)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    int count = 0;
    int top_index = 1;
    for (size_t i = 1; i < services.Sprites.GetSpriteSlotCount(); ++i)
    {
        if ((game.SpriteInfos[i].Flags & match_flags) == match_flags)
        {
            count++;
            top_index = i;
        }
    }
    out->WriteInt32(count);
    out->WriteInt32(top_index);

    for (size_t i = 1; i < services.Sprites.GetSpriteSlotCount(); ++i)
    {
        if ((game.SpriteInfos[i].Flags & match_flags) == match_flags)
        {
            out->WriteInt32(i);
            out->WriteInt32(game.SpriteInfos[i].Flags);
            WriteBitmap(services.Sprites[i], out, false /* not compressed (expect component is compressed) */);
        }
    }
    return HSaveError::None();
}

HSaveError ReadDynamicSpritesImpl(Stream *in, int match_flags)
{
    HSaveError err;
    const int spr_count = in->ReadInt32();
    // ensure the sprite set is at least large enough
    // to accommodate top dynamic sprite index
    const int top_index = in->ReadInt32();
    EngineServices::Current().Sprites.EnlargeTo(top_index);
    for (int i = 0; i < spr_count; ++i)
    {
        int id = in->ReadInt32();
        int flags = in->ReadInt32();
        if ((flags & match_flags) == match_flags)
        {
            std::unique_ptr<Bitmap> image(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
            add_dynamic_sprite(id, std::move(image), flags);
        }
        else
        {
            SkipBitmap(in, false /* not compressed */);
        }
    }
    return err;
}

HSaveError WriteDynamicSprites(Stream *out)
{
    return WriteDynamicSpritesImpl(out, SPF_DYNAMICALLOC);
}

HSaveError WriteObjectSprites(Stream *out)
{
    return WriteDynamicSpritesImpl(out, SPF_DYNAMICALLOC | SPF_OBJECTOWNED);
}

HSaveError ReadDynamicSprites(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                              RestoredData & /*r_data*/)
{
    return ReadDynamicSpritesImpl(in, SPF_DYNAMICALLOC);
}

HSaveError ReadObjectSprites(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                             RestoredData & /*r_data*/)
{
    return ReadDynamicSpritesImpl(in, SPF_DYNAMICALLOC | SPF_OBJECTOWNED);
}

HSaveError WriteOverlays(Stream *out)
{
    const auto &overs = get_overlays();
    // Calculate and save valid overlays only
    uint32_t valid_count = 0;
    for (const auto &over : overs)
    {
        if (over.GetID() >= 0)
            valid_count++;
    }
    out->WriteInt32(valid_count);

    for (const auto &over : overs)
    {
        if (over.GetID() >= 0)
            over.WriteToSavegame(out);
    }
    return HSaveError::None();
}

HSaveError ReadOverlays(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                        RestoredData & /*r_data*/)
{
    // Remember that overlay indexes may be non-sequential
    // the vector may be resized during read
    uint32_t over_count = static_cast<uint32_t>(in->ReadInt32());
    std::vector<ScreenOverlay> overs;
    std::vector<bool> used_ids;
    overs.reserve(over_count); // reserve minimal size
    used_ids.reserve(over_count);
    for (size_t i = 0; i < over_count; ++i)
    {
        ScreenOverlay over;
        over.ReadFromSavegame(in, cmp_ver);
        if (over.GetID() < 0)
            continue; // safety abort
        auto type = over.GetID();
        if (static_cast<size_t>(type) >= overs.size())
        {
            overs.resize(type + 1);
            used_ids.resize(type + 1);
        }
        overs[type] = std::move(over);
        used_ids[type] = true;
    }

    auto &overlays = get_overlays();
    overlays.Set(std::move(overs), used_ids);
    return HSaveError::None();
}

HSaveError WriteAnimOverlays(Stream *out)
{
    const auto &aovers = GetAnimatedOverlays();
    // Calculate and save valid overlays only
    uint32_t valid_count = 0;
    for (const auto &over : aovers)
    {
        if (over.GetOverID() >= 0)
            valid_count++;
    }
    out->WriteInt32(valid_count);

    for (const auto &over : aovers)
    {
        if (over.GetOverID() >= 0)
            over.WriteToSavegame(out);
    }
    return HSaveError::None();
}

HSaveError ReadAnimOverlays(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                            RestoredData &r_data)
{
    uint32_t over_count = static_cast<uint32_t>(in->ReadInt32());
    for (size_t i = 0; i < over_count; ++i)
    {
        AnimatedOverlay over;
        over.ReadFromSavegame(in, cmp_ver);
        if (over.GetOverID() < 0)
            continue; // safety abort
        AddAnimatedOverlay(std::move(over));
    }
    return HSaveError::None();
}

HSaveError WriteDynamicSurfaces(Stream *out)
{
    Game &game = Game::Current();
    out->WriteInt32(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (game.DynamicSurfaces[i] == nullptr)
        {
            out->WriteInt8(0);
        }
        else
        {
            out->WriteInt8(1);
            WriteBitmap(game.DynamicSurfaces[i].get(), out,
                        false /* not compressed (expect component is compressed) */);
        }
    }
    return HSaveError::None();
}

HSaveError ReadDynamicSurfaces(Stream *in, int32_t /*cmp_ver*/, soff_t cmp_size, const PreservedParams & /*pp*/,
                               RestoredData &r_data)
{
    HSaveError err;
    if (!AssertCompatLimit(err, in->ReadInt32(), MAX_DYNAMIC_SURFACES, "Dynamic Surfaces"))
        return err;
    // Load the surfaces into a temporary array since ccUnserializeAllObjects will destroy them otherwise
    r_data.DynamicSurfaces.resize(MAX_DYNAMIC_SURFACES);
    for (int i = 0; i < MAX_DYNAMIC_SURFACES; ++i)
    {
        if (in->ReadInt8() == 0)
            r_data.DynamicSurfaces[i] = nullptr;
        else
            r_data.DynamicSurfaces[i].reset(
                ReadBitmap(in, false /* not compressed (expect component is compressed) */));
    }
    return err;
}

enum ScriptModulesSvgVersion
{
    kScriptModulesSvgVersion_Initial = 0,
    kScriptModulesSvgVersion_36200 = 3060200, // module names
};

HSaveError WriteScriptModules(Stream *out)
{
    Game &game = Game::Current();
    // write the data segment of the global script
    int data_len = game.Scripts.Global->GetGlobalData().size();
    out->WriteInt32(data_len);
    if (data_len > 0)
        out->Write(game.Scripts.Global->GetGlobalData().data(), data_len);
    // write the script modules data segments
    out->WriteInt32(game.Scripts.NumModules);
    for (size_t i = 0; i < game.Scripts.NumModules; ++i)
    {
        StrUtil::WriteString(game.Scripts.Modules[i]->GetScriptName(), out);
        data_len = game.Scripts.Modules[i]->GetGlobalData().size();
        out->WriteInt32(data_len);
        if (data_len > 0)
            out->Write(game.Scripts.Modules[i]->GetGlobalData().data(), data_len);
    }
    return HSaveError::None();
}

HSaveError ReadScriptModules(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams &pp,
                             RestoredData &r_data)
{
    HSaveError err;
    // read the global script data segment
    uint32_t data_len = in->ReadInt32();
    if (!AssertGameContent(err, data_len, pp.GlScDataSize, "global script data", r_data.Result,
                           r_data.DataCounts.GlobalScriptDataSz))
        return err;
    r_data.GlobalScript.Data.resize(data_len);
    if (data_len > 0u)
        in->Read(r_data.GlobalScript.Data.data(), data_len);

    std::vector<bool> modules_match(pp.ScriptModuleNames.size());
    const uint32_t modules_read = in->ReadInt32();
    r_data.DataCounts.ScriptModules = modules_read;
    r_data.DataCounts.ScriptModuleDataSz.resize(modules_read);
    r_data.DataCounts.ScriptModuleNames.resize(modules_read);
    for (size_t read_module_index = 0; read_module_index < modules_read; ++read_module_index)
    {
        const String module_name =
            (cmp_ver < kScriptModulesSvgVersion_36200)
                ? (read_module_index < pp.ScriptModuleNames.size() ? pp.ScriptModuleNames[read_module_index] : "")
                : StrUtil::ReadString(in);
        r_data.DataCounts.ScriptModuleNames[read_module_index] = module_name;

        data_len = in->ReadInt32();

        // Try to find existing module length and assert its presence and matching size
        uint32_t game_module_index = UINT32_MAX;
        for (size_t i = 0; i < pp.ScriptModuleNames.size(); ++i)
        {
            if (module_name.Compare(pp.ScriptModuleNames[i]) == 0)
            {
                game_module_index = i;
                break;
            }
        }

        if (game_module_index < UINT32_MAX)
        {
            // Found matching module in the game
            if (!AssertGameObjectContent(err, data_len, pp.ScMdDataSize[game_module_index], "script module data",
                                         module_name.GetCStr(), game_module_index, r_data.Result,
                                         r_data.DataCounts.ScriptModuleDataSz[read_module_index]))
                return err;
            modules_match[game_module_index] = true;
        }
        else
        {
            // No such module in the game: treat this as "more data"
            if (!HandleExtraGameComponent(err, "script module", module_name, r_data.Result))
                return err;
        }

        RestoredData::ScriptData script_data;
        script_data.Data.resize(data_len);
        if (data_len > 0u)
            in->Read(script_data.Data.data(), data_len);
        r_data.ScriptModules[module_name] = std::move(script_data);
    }

    // Assert that all game's script modules were read from the save
    for (size_t i = 0; i < modules_match.size(); ++i)
    {
        if (!modules_match[i] &&
            !HandleMissingGameComponent(err, "script module", pp.ScriptModuleNames[i], r_data.Result))
            return err;
    }

    return HSaveError::None();
}

HSaveError PrescanScriptModules(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams &pp,
                                RestoredData &r_data)
{
    HSaveError err;
    uint32_t data_len = in->ReadInt32();
    if (!AssertGameContent(err, data_len, pp.GlScDataSize, "global script data", r_data.Result,
                           r_data.DataCounts.GlobalScriptDataSz))
        return err;
    in->Seek(data_len);

    std::vector<bool> modules_match(pp.ScriptModuleNames.size());
    const uint32_t modules_read = in->ReadInt32();
    r_data.DataCounts.ScriptModules = modules_read;
    r_data.DataCounts.ScriptModuleDataSz.resize(modules_read);
    r_data.DataCounts.ScriptModuleNames.resize(modules_read);
    for (size_t read_module_index = 0; read_module_index < modules_read; ++read_module_index)
    {
        const String module_name =
            (cmp_ver < kScriptModulesSvgVersion_36200)
                ? (read_module_index < pp.ScriptModuleNames.size() ? pp.ScriptModuleNames[read_module_index] : "")
                : StrUtil::ReadString(in);
        r_data.DataCounts.ScriptModuleNames[read_module_index] = module_name;

        data_len = in->ReadInt32();

        // Try to find existing module length and assert its presence and matching size
        uint32_t game_module_index = UINT32_MAX;
        for (size_t i = 0; i < pp.ScriptModuleNames.size(); ++i)
        {
            if (module_name.Compare(pp.ScriptModuleNames[i]) == 0)
            {
                game_module_index = i;
                break;
            }
        }

        if (game_module_index < UINT32_MAX)
        {
            // Found matching module in the game
            if (!AssertGameObjectContent(err, data_len, pp.ScMdDataSize[game_module_index], "script module data",
                                         module_name.GetCStr(), game_module_index, r_data.Result,
                                         r_data.DataCounts.ScriptModuleDataSz[read_module_index]))
                return err;
            modules_match[game_module_index] = true;
        }
        else
        {
            // No such module in the game: treat this as "more data"
            if (!HandleExtraGameComponent(err, "script module", module_name, r_data.Result))
                return err;
        }

        in->Seek(data_len);
    }

    // Assert that all game's script modules were read from the save
    for (size_t i = 0; i < modules_match.size(); ++i)
    {
        if (!modules_match[i] &&
            !HandleMissingGameComponent(err, "script module", pp.ScriptModuleNames[i], r_data.Result))
            return err;
    }

    return HSaveError::None();
}

HSaveError WriteRoomStates(Stream *out)
{
    // write the room state for all the rooms the player has been in
    out->WriteInt32(MAX_ROOMS);
    for (int i = 0; i < MAX_ROOMS; ++i)
    {
        RoomState *roomstat = GetRoomStateIfExists(i);
        if (roomstat)
        {
            if (roomstat->IsVisited)
            {
                out->WriteInt32(i);
                WriteFormatTag(out, "RoomState", true);
                roomstat->WriteToSavegame(out);
                WriteFormatTag(out, "RoomState", false);
            }
            else
            {
                out->WriteInt32(-1);
            }
        }
        else
        {
            out->WriteInt32(-1);
        }
    }
    return HSaveError::None();
}

HSaveError ReadRoomStates(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                          RestoredData & /*r_data*/)
{
    HSaveError err;
    int roomstat_count = in->ReadInt32();
    for (; roomstat_count > 0; --roomstat_count)
    {
        int id = in->ReadInt32();
        // If id == -1, then the player has not been there yet (or room state was reset)
        if (id != -1)
        {
            if (!AssertCompatRange(err, id, 0, MAX_ROOMS - 1, "room index"))
            {
                return err;
            }
            if (!AssertFormatTagStrict(in, err, "RoomState", String::FromFormat("Room %d", id), true))
            {
                return err;
            }
            RoomState *roomstat = GetRoomState(id);
            roomstat->ReadFromSavegame(in, (RoomStatSvgVersion)cmp_ver);
            if (!AssertFormatTagStrict(in, err, "RoomState", String::FromFormat("Room %d", id), false))
            {
                return err;
            }
        }
    }
    return HSaveError::None();
}

HSaveError WriteThisRoom(Stream *out)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    out->WriteInt32(game.DisplayedRoom);
    if (game.DisplayedRoom < 0)
        return HSaveError::None();

    // modified room backgrounds
    // -- varied count since kRoomStatSvgVersion_40033
    out->WriteInt8(static_cast<uint8_t>(room.BgImages.size()));
    for (size_t i = 0; i < room.BgImages.size(); ++i)
    {
        out->WriteBool(room.BgModified[i]);
        if (room.BgModified[i])
            WriteBitmap(room.BgImages[i].get(), out, false /* not compressed (expect component is compressed) */);
    }
    out->WriteBool(false); //[DEPRECATED]

    // modified room masks
    // -- kRoomStatSvgVersion_40024
    for (int i = 0; i < kNumRoomAreaTypes; ++i)
    {
        out->WriteBool(room.MaskModified[i]);
        if (room.MaskModified[i])
            WriteBitmap(room.GetMask(static_cast<RoomAreaMask>(i)), out,
                        false /* not compressed (expect component is compressed) */);
    }

    // room region state
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        out->WriteInt32(room.Regions[i].Light);
        out->WriteInt32(room.Regions[i].Tint);
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        out->WriteInt32(room.WalkAreas[i].ScalingFar);
        out->WriteInt32(room.WalkAreas[i].ScalingNear);
    }

    out->WriteInt32(0); // [DEPRECATED]

    // persistent room's indicator
    const bool persist = game.DisplayedRoom < MAX_ROOMS;
    out->WriteBool(persist);
    // write room.TempState, in case they save in temporary room
    if (!persist)
        room.TempState.WriteToSavegame(out);
    return HSaveError::None();
}

HSaveError ReadThisRoom(Stream *in, int32_t cmp_ver, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                        RestoredData &r_data)
{
    HSaveError err;
    r_data.Room = in->ReadInt32();
    r_data.DataCounts.Room = r_data.Room;
    if (r_data.Room < 0)
        return HSaveError::None();

    // modified room backgrounds
    uint8_t bg_count = MAX_ROOM_BGFRAMES_321;
    if (cmp_ver >= kRoomStatSvgVersion_40033)
    {
        bg_count = static_cast<uint8_t>(in->ReadInt8());
    }
    r_data.RoomBkgScene.resize(bg_count);
    for (uint8_t i = 0; i < bg_count; ++i)
    {
        bool is_modified = in->ReadBool();
        if (is_modified)
            r_data.RoomBkgScene[i].reset(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
    }
    if (in->ReadBool())
        SkipBitmap(in, false /* not compressed (expect component is compressed) */); //[DEPRECATED]

    // modified room masks
    if (cmp_ver >= kRoomStatSvgVersion_40024)
    {
        for (int i = 0; i < kNumRoomAreaTypes; ++i)
        {
            bool is_modified = in->ReadBool();
            if (is_modified)
                r_data.RoomMask[i].reset(ReadBitmap(in, false /* not compressed (expect component is compressed) */));
        }
    }

    // room region state
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        r_data.RoomLightLevels[i] = in->ReadInt32();
        r_data.RoomTintLevels[i] = in->ReadInt32();
        if (cmp_ver < kRoomStatSvgVersion_40034)
        {
            int light = r_data.RoomLightLevels[i];
            RoomRegionDef::UpgradeLegacyLight(light, r_data.RoomTintLevels[i]);
            r_data.RoomLightLevels[i] = static_cast<short>(light);
        }
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        r_data.RoomZoomLevels1[i] = in->ReadInt32();
        r_data.RoomZoomLevels2[i] = in->ReadInt32();
    }

    in->ReadInt32(); // [DEPRECATED]

    // read room.TempState, in case they saved in temporary room
    if (!in->ReadBool())
        Room::Current().TempState.ReadFromSavegame(in, (RoomStatSvgVersion)cmp_ver);

    return HSaveError::None();
}

HSaveError PrescanThisRoom(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                           RestoredData &r_data)
{
    r_data.DataCounts.Room = in->ReadInt32();
    return HSaveError::None();
}

HSaveError WriteMoveLists(Stream *out)
{
    const auto &movelists = get_movelists();
    out->WriteInt32(static_cast<int32_t>(movelists.size()));
    for (const auto &movelist : movelists)
    {
        movelist.WriteToSavegame(out);
    }
    return HSaveError::None();
}

HSaveError ReadMoveLists(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                         RestoredData &r_data)
{
    HSaveError err;
    uint32_t movelist_count = in->ReadInt32();
    for (uint32_t i = 0; i < movelist_count; ++i)
    {
        MoveList move_list;
        err = move_list.ReadFromSavegame(in, cmp_ver);
        if (!err)
            return err;
        add_movelist(std::move(move_list), i);
    }
    return err;
}

HSaveError PrescanMoveLists(Stream *in, int32_t /*cmp_ver*/, soff_t /*cmp_size*/, const PreservedParams & /*pp*/,
                            RestoredData &r_data)
{
    return HSaveError::None();
}

HSaveError WriteManagedPool(Stream *out)
{
    ccSerializeAllObjects(out);
    return HSaveError::None();
}

HSaveError ReadManagedPool(Stream *in, int32_t /*cmp_ver*/, soff_t cmp_size, const PreservedParams & /*pp*/,
                           RestoredData &r_data)
{
    if (ccUnserializeAllObjects(in, &r_data.ManObjReader))
    {
        return new SavegameError(kSvgErr_GameObjectInitFailed, "Managed pool deserialization failed: %s",
                                 cc_get_error().ErrorString.GetCStr());
    }
    return HSaveError::None();
}

enum RTTISaveVersion
{
    kRTTISvgVer_Initial = 4000000,
    kRTTISvgVer_400_22 = 4000022,
};

HSaveError WriteRTTI(Stream *out)
{
    // Write the minimal necessary RTTI data, enough to resolve types when restoring a save;
    // NOTE: we might just dump whole RTTI here, if it's necessary to keep all field descs and names
    const auto &rtti = RuntimeScript::GetJointRTTI()->AsConstRTTI();
    const auto &helper = RuntimeScript::GetRTTIHelper();
    const auto &locs = rtti.GetLocations();
    const auto &types = rtti.GetTypes();
    // NOTE: we don't write IDs here, as the Joint RTTI assumes to have them strictly sequential
    out->WriteInt32(locs.size());
    for (const auto &loc : locs)
    {
        StrUtil::WriteCStr(loc.name, out);
        out->WriteInt32(loc.flags);
    }
    out->WriteInt32(types.size());
    for (const auto &type : types)
    {
        StrUtil::WriteCStr(type.name, out);
        out->WriteInt32(type.loc_id);
        out->WriteInt32(type.parent_id);
        out->WriteInt32(type.flags);
        out->WriteInt32(type.size);
        // kRTTISvgVer_400_22
        out->WriteInt32(type.base_id);
        out->WriteInt32(type.dim_num);
        // Managed ptrs offsets
        const auto man_offs = helper->GetManagedOffsetsForType(type.this_id);
        out->WriteInt32(man_offs.second - man_offs.first);
        for (auto off = man_offs.first; off < man_offs.second; ++off)
            out->WriteInt32(*off);
    }
    return HSaveError::None();
}

HSaveError ReadRTTI(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/, RestoredData &r_data)
{
    // Restore the minimal necessary RTTI data, enough to resolve types when restoring a save
    // Create a pseudo-RTTI with "generated" types, and fields only for managed pointers
    RTTIBuilder rtti_builder;
    char buf[256];
    uint32_t num_locs = in->ReadInt32();
    for (uint32_t i = 0; i < num_locs; ++i)
    {
        StrUtil::ReadCStr(buf, in, sizeof(buf));
        uint32_t flags = in->ReadInt32();
        rtti_builder.AddLocation(buf, i, flags | RTTI::kLoc_Generated);
    }
    uint32_t num_types = in->ReadInt32();
    for (uint32_t i = 0; i < num_types; ++i)
    {
        StrUtil::ReadCStr(buf, in, sizeof(buf));
        uint32_t loc_id = in->ReadInt32();
        uint32_t parent_id = in->ReadInt32();
        uint32_t flags = in->ReadInt32();
        uint32_t size = in->ReadInt32();
        uint32_t base_id = 0u;
        uint32_t dim_num = 0u;
        if (cmp_ver >= kRTTISvgVer_400_22)
        {
            base_id = in->ReadInt32();
            dim_num = in->ReadInt32();
        }
        rtti_builder.AddType(buf, i, loc_id, base_id, parent_id, flags | RTTI::kType_Generated, size, dim_num);
        // Read managed ptrs offsets and generate pseudo-fields for these
        uint32_t num_offs = in->ReadInt32();
        for (uint32_t off_index = 0; off_index < num_offs; ++off_index)
        {
            uint32_t off = in->ReadInt32();
            rtti_builder.AddField(i, "", off, 0u, RTTI::kField_ManagedPtr | RTTI::kField_Generated, 0u);
        }
    }

    r_data.GenRTTI = std::move(rtti_builder.Finalize());
    return HSaveError::None();
}

HSaveError WritePluginData(Stream *out)
{
    return WritePluginSaveData(out);
}

HSaveError ReadPluginData(Stream *in, int32_t cmp_ver, soff_t cmp_size, const PreservedParams & /*pp*/,
                          RestoredData & /*r_data*/)
{
    return ReadPluginSaveData(in, static_cast<PluginSvgVersion>(cmp_ver), cmp_size);
}


// TODO: move elsewhere later
// FIXME: ReadManagedPool does not pass the component version to ccUnserializeAllObjects
enum ManagedPoolSavegameVersion
{
    kManagedPoolSvgVersion_400 = 4000000 // typeinfo for dynamic objects
};

// Description of a supported game state serialization component
struct ComponentHandler
{
    String Name;                // internal component's ID
    int32_t Version;            // current version to write and the highest supported version
    int32_t LowestVersion;      // lowest supported version that the engine can read
    SaveCmpSelection Selection; // flag mask corresponding to this component
    HSaveError (*Serialize)(Stream *);
    HSaveError (*Unserialize)(Stream *, int32_t cmp_ver, soff_t cmp_size, const PreservedParams &, RestoredData &);
    HSaveError (*Prescan)(Stream *, int32_t cmp_ver, soff_t cmp_size, const PreservedParams &, RestoredData &);
};

// Array of supported components
ComponentHandler ComponentHandlers[] = {
    // NOTE: the new format values should now be defined as AGS version
    // at which a change was introduced, represented as NN,NN,NN,NN.
    // The lowest version is the one written by the first 4.0 format (kSvgVersion_400) or a later one.
    {"Game Info", 0, 0,
     kSaveCmp_GameState, // consider this is a part of game state
     WriteGameInfo, ReadGameInfo, PrescanGameInfo},
    {"Game State", kGSSvgVersion_Current, kGSSvgVersion_400, kSaveCmp_GameState, WriteGameState, ReadGameState,
     nullptr},
    {"Audio", kAudioSvgVersion_36303, kAudioSvgVersion_36009, kSaveCmp_Audio, WriteAudio, ReadAudio, PrescanAudio},
    {"Characters", kCharSvgVersion_Current, kCharSvgVersion_400, kSaveCmp_Characters, WriteCharacters, ReadCharacters,
     PrescanCharacters},
    {"Dialogs", kDialogTopicSvgVer_40026, kDialogTopicSvgVer_Initial, kSaveCmp_Dialogs, WriteDialogs, ReadDialogs,
     PrescanDialogs},
    {"GUI", kGuiSvgVersion_Current, kGuiSvgVersion_400, kSaveCmp_GUI, WriteGUI, ReadGUI, PrescanGUI},
    {"Inventory Items", kInvitemSvgVersion_Current, kInvitemSvgVersion_Initial, kSaveCmp_InvItems, WriteInventory,
     ReadInventory, PrescanInventory},
    {"Mouse Cursors", kCursorSvgVersion_36016, kCursorSvgVersion_36016, kSaveCmp_Cursors, WriteMouseCursors,
     ReadMouseCursors, PrescanMouseCursors},
    {"Views", kViewSvgVersion_4000024, kViewSvgVersion_Initial, kSaveCmp_Views, WriteViews, ReadViews, PrescanViews},
    {"Dynamic Sprites", 0, 0, kSaveCmp_DynamicSprites, WriteDynamicSprites, ReadDynamicSprites, nullptr},
    // Alternate "Dynamic Sprites" handler in case only object-owned sprites are serialized
    {"Dynamic Sprites", 0, 0, kSaveCmp_ObjectSprites, WriteObjectSprites, ReadObjectSprites, nullptr},
    {"Overlays", kOverSvgVersion_Current, kOverSvgVersion_400, kSaveCmp_Overlays, WriteOverlays, ReadOverlays, nullptr},
    {"AnimatedOverlays", kAnimOverSvgVersion_40024, kAnimOverSvgVersion_40024, kSaveCmp_Overlays, WriteAnimOverlays,
     ReadAnimOverlays, nullptr},
    {"Dynamic Surfaces", 0, 0,
     kSaveCmp_DynamicSprites, // share flag with "Dynamic Sprites"
     WriteDynamicSurfaces, ReadDynamicSurfaces, nullptr},
    {"Script Modules", kScriptModulesSvgVersion_36200, kScriptModulesSvgVersion_Initial, kSaveCmp_Scripts,
     WriteScriptModules, ReadScriptModules, PrescanScriptModules},
    {"Room States", kRoomStatSvgVersion_Current, kRoomStatSvgVersion_40003, kSaveCmp_Rooms, WriteRoomStates,
     ReadRoomStates, nullptr},
    {"Loaded Room State",
     kRoomStatSvgVersion_Current, // must correspond to "Room States"
     kRoomStatSvgVersion_40003, kSaveCmp_ThisRoom, WriteThisRoom, ReadThisRoom, PrescanThisRoom},
    {"Move Lists", kMoveSvgVersion_40020, kMoveSvgVersion_400,
     (SaveCmpSelection)(kSaveCmp_Characters | kSaveCmp_ThisRoom), // must go along with characters and room objects
     WriteMoveLists, ReadMoveLists, PrescanMoveLists},
    {"Managed Pool", kManagedPoolSvgVersion_400, kManagedPoolSvgVersion_400,
     kSaveCmp_Scripts, // must go along with scripts
     WriteManagedPool, ReadManagedPool, nullptr},
    {"RTTI", kRTTISvgVer_400_22, kRTTISvgVer_Initial,
     kSaveCmp_Scripts, // must go along with scripts
     WriteRTTI, ReadRTTI, nullptr},
    {"Plugin Data", kPluginSvgVersion_36115, kPluginSvgVersion_Initial, kSaveCmp_Plugins, WritePluginData,
     ReadPluginData, nullptr},
    {nullptr, 0, 0, kSaveCmp_None, nullptr, nullptr, nullptr} // end of array
};


typedef std::multimap<String, ComponentHandler> HandlersMap;
void GenerateHandlersMap(HandlersMap &map)
{
    map.clear();
    for (int i = 0; !ComponentHandlers[i].Name.IsEmpty(); ++i)
        map.insert(std::make_pair(ComponentHandlers[i].Name, ComponentHandlers[i]));
}

// A helper struct to pass to (de)serialization handlers
struct SvgCmpReadHelper
{
    SavegameVersion Version; // general savegame version
    // Flag mask, instructing which components to read (others shall be skipped)
    SaveCmpSelection ComponentSelection = kSaveCmp_All;
    const PreservedParams &PP; // previous game state kept for reference
    RestoredData &RData;       // temporary storage for loaded data, that
                               // will be applied after loading is done
    // The map of serialization handlers, one per supported component type ID
    HandlersMap Handlers;

    SvgCmpReadHelper(SavegameVersion svg_version, SaveCmpSelection select_cmp, const PreservedParams &pp,
                     RestoredData &r_data)
        : Version(svg_version),
          ComponentSelection(select_cmp),
          PP(pp),
          RData(r_data)
    {
    }
};

enum ComponentFlags
{
    kSvgCmp_Deflate = 0x0001 // compress using Deflate algorithm
};

// The basic information about deserialized component, used for debugging purposes
struct ComponentInfo
{
    String Name;                        // internal component's ID
    uint32_t TagOffset = 0;             // offset at which an opening tag is located [not serialized]
    uint32_t HeaderSize = 0;            // component header size (meta data)
    uint32_t Flags = 0;                 // ComponentFlags
    int32_t Version = -1;               // data format version
    uint32_t DataOffset = 0;            // offset at which component data begins [not serialized]
    uint32_t DataSize = 0u;             // expected size of component data
    uint32_t UncompressedDataSize = 0u; // uncompressed data size
    uint32_t Checksum = 0u;             // checksum of the uncompressed data (only if compressed)

    ComponentInfo() = default;
};

HSaveError ReadComponent(Stream *in, SvgCmpReadHelper &hlp, ComponentInfo &info)
{
    const bool has_fileformat = hlp.Version >= kSvgVersion_400_18;

    // Read component info
    info = ComponentInfo();
    info.TagOffset = in->GetPosition();
    if (!ReadFormatTag(in, info.Name, true))
        return new SavegameError(kSvgErr_ComponentOpeningTagFormat);
    if (has_fileformat)
    {
        info.HeaderSize = in->ReadInt32();
        info.Flags = in->ReadInt32();
        info.Version = in->ReadInt32();
        info.DataSize = in->ReadInt32();
        info.UncompressedDataSize = in->ReadInt32();
        info.Checksum = in->ReadInt32();
    }
    else
    {
        info.Version = in->ReadInt32();
        info.DataSize = in->ReadInt32();
        in->ReadInt32(); // reserved
    }
    // Assume that component data begins right after the header
    info.DataOffset = in->GetPosition();

    // Find component's handler(s)
    const ComponentHandler *handler = nullptr;
    auto it_hdr = hlp.Handlers.equal_range(info.Name);
    const bool found_any = it_hdr.first != it_hdr.second;
    if (!found_any)
        return new SavegameError(kSvgErr_UnsupportedComponent);

    // Find any first handler, that is not disabled by ComponentSelection
    for (auto it = it_hdr.first; handler == nullptr && it != it_hdr.second; ++it)
    {
        if ((it->second.Selection & hlp.ComponentSelection) != 0)
        {
            handler = &it->second;
        }
    }

    const bool prescan = (hlp.RData.Result.RestoreFlags & kSaveRestore_Prescan) != 0;
    // no handler when the component is left out of the selection: its data is skipped below
    auto pfn_read = handler ? (prescan ? handler->Prescan : handler->Unserialize) : nullptr;

    // If a handler is chosen, and has Unserialize method, then try reading the data
    if (handler && pfn_read)
    {
#if (DEBUG_SAVECOMPONENTS)
        Debug::Printf("Save: ReadComponent: %s: data begins at: %u", handler->Name.GetCStr(), info.DataOffset);
        Debug::Printf("Save: ReadComponent: %s: data size: %zu", handler->Name.GetCStr(), info.DataSize);
        Debug::Printf("Save: ReadComponent: %s: uncompressed size: %zu", handler->Name.GetCStr(),
                      info.UncompressedDataSize);
#endif

        if (info.Version > handler->Version || info.Version < handler->LowestVersion)
            return new SavegameError(kSvgErr_UnsupportedComponentVersion, "Saved version: %d, supported: %d - %d",
                                     info.Version, handler->LowestVersion, handler->Version);

        if ((info.Flags & kSvgCmp_Deflate) != 0)
        {
            auto deflate_s = std::make_unique<DeflateStream>(in->ReleaseStreamBase(), info.DataOffset,
                                                             info.DataOffset + info.DataSize);
            auto deflate_in = std::make_unique<Stream>(std::move(deflate_s));

            HSaveError err = pfn_read(deflate_in.get(), info.Version, info.UncompressedDataSize, hlp.PP, hlp.RData);
            if (!err)
                return err;

            if (prescan)
                deflate_in->Seek(info.UncompressedDataSize, kSeekBegin);

            // FIXME: this is very ugly, maybe may be fixed by changing stream base storage to shared ptr?
            deflate_s.reset(dynamic_cast<DeflateStream *>(deflate_in->ReleaseStreamBase().release()));
            uint32_t uncomp_data_sz = deflate_s->GetProcessedInput();
            if (uncomp_data_sz != info.UncompressedDataSize)
                return new SavegameError(kSvgErr_ComponentUncompressedSizeMismatch, "Expected: %u, actual: %u",
                                         info.UncompressedDataSize, uncomp_data_sz);
            // TODO: test checksum too?

            in->AttachStreamBase(deflate_s->ReleaseStreamBase());
        }
        else
        {
            HSaveError err = pfn_read(in, info.Version, info.DataSize, hlp.PP, hlp.RData);
            if (!err)
                return err;

            if (prescan)
                in->Seek(info.DataOffset + info.DataSize, kSeekBegin);
        }
    }
    // Else, skip the data
    else
    {
        in->Seek(info.DataSize);
    }

    // Test that we have reached an expected position in stream
    if (in->GetPosition() - info.DataOffset != info.DataSize)
        return new SavegameError(kSvgErr_ComponentSizeMismatch, "Expected: %jd, actual: %jd",
                                 static_cast<intmax_t>(info.DataSize),
                                 static_cast<intmax_t>(in->GetPosition() - info.DataOffset));
    if (!AssertFormatTag(in, info.Name, false))
        return new SavegameError(kSvgErr_ComponentClosingTagFormat);
    return HSaveError::None();
}

HSaveError ReadAllImpl(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp, const PreservedParams &pp,
                       RestoredData &r_data)
{
    // Prepare a helper struct we will be passing to the block reading proc
    SvgCmpReadHelper hlp(svg_version, select_cmp, pp, r_data);
    GenerateHandlersMap(hlp.Handlers);

    size_t idx = 0;
    if (!AssertFormatTag(in, ComponentListTag, true))
        return new SavegameError(kSvgErr_ComponentListOpeningTagFormat);
    do
    {
        // Look out for the end of the component list:
        // this is the only way how this function ends with success
        soff_t off = in->GetPosition();
        if (AssertFormatTag(in, ComponentListTag, false))
            return HSaveError::None();
        // If the list's end was not detected, then seek back and continue reading
        in->Seek(off, kSeekBegin);

        ComponentInfo info;
        HSaveError err = ReadComponent(in, hlp, info);
        if (!err)
        {
            return new SavegameError(err, kSvgErr_ComponentUnserialization, "(#%zu) %s, version %i, at offset %u.", idx,
                                     info.Name.IsEmpty() ? "unknown" : info.Name.GetCStr(), info.Version,
                                     info.TagOffset);
        }
        idx++;
    } while (!in->EOS());
    return new SavegameError(kSvgErr_ComponentListClosingTagMissing);
}

HSaveError ReadAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp, const PreservedParams &pp,
                   RestoredData &r_data)
{
    return ReadAllImpl(in, svg_version, select_cmp, pp, r_data);
}

HSaveError PrescanAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp, const PreservedParams &pp,
                      RestoredData &r_data)
{
    r_data.Result.RestoreFlags = (SaveRestorationFlags)(r_data.Result.RestoreFlags | kSaveRestore_Prescan);
    return ReadAllImpl(in, svg_version, select_cmp, pp, r_data);
}

HSaveError WriteComponent(Stream *out, ComponentHandler &handler, bool compress)
{
    uint32_t flags = kSvgCmp_Deflate * compress;

    WriteFormatTag(out, handler.Name, true);
    soff_t header_pos = out->GetPosition();
    out->WriteInt32(0);     // header size placeholder
    out->WriteInt32(flags); // flags
    out->WriteInt32(handler.Version);
    soff_t data_sz_pos = out->GetPosition();
    out->WriteInt32(0); // component size placeholder
    out->WriteInt32(0); // uncompressed size
    out->WriteInt32(0); // checksum

    soff_t data_begin_pos = out->GetPosition();

#if (DEBUG_SAVECOMPONENTS)
    Debug::Printf("Save: WriteComponent: %s: data begins at: %lld", handler.Name.GetCStr(), data_begin_pos);
#endif

    uint32_t uncomp_data_sz = 0u;
    if (compress)
    {
        auto deflate_s = std::make_unique<DeflateStream>(out->ReleaseStreamBase(), kStream_Write);
        auto deflate_out = std::make_unique<Stream>(std::move(deflate_s));
        HSaveError err = handler.Serialize(deflate_out.get());
        if (!err)
            return err;

        // FIXME: this is very ugly, maybe may be fixed by changing stream base storage to shared ptr?
        deflate_s.reset(dynamic_cast<DeflateStream *>(deflate_out->ReleaseStreamBase().release()));
        deflate_s->Finalize();
        uncomp_data_sz = deflate_s->GetProcessedInput();
        out->AttachStreamBase(deflate_s->ReleaseStreamBase());
    }
    else
    {
        HSaveError err = handler.Serialize(out);
        if (!err)
            return err;
    }

    soff_t data_end_pos = out->GetPosition();

#if (DEBUG_SAVECOMPONENTS)
    Debug::Printf("Save: WriteComponent: %s: data ends at: %lld", handler.Name.GetCStr(), data_end_pos);
    Debug::Printf("Save: WriteComponent: %s: data size: %u", handler.Name.GetCStr(),
                  static_cast<uint32_t>(data_end_pos - data_begin_pos));
    Debug::Printf("Save: WriteComponent: %s: uncompressed size: %u", handler.Name.GetCStr(),
                  static_cast<uint32_t>(compress ? uncomp_data_sz : (data_end_pos - data_begin_pos)));
#endif

    out->Seek(header_pos, kSeekBegin);
    out->WriteInt32(data_begin_pos - header_pos);
    out->Seek(data_sz_pos, kSeekBegin);
    out->WriteInt32(data_end_pos - data_begin_pos);                               // size of serialized component data
    out->WriteInt32(compress ? uncomp_data_sz : (data_end_pos - data_begin_pos)); // uncompressed size
    out->WriteInt32(0); // checksum: never computed, ReadComponent does not check it
    out->Seek(data_end_pos, kSeekBegin);
    WriteFormatTag(out, handler.Name, false);
    return HSaveError::None();
}

HSaveError WriteAllCommon(Stream *out, SaveCmpSelection select_cmp, bool compress)
{
    WriteFormatTag(out, ComponentListTag, true);
    for (int type = 0; !ComponentHandlers[type].Name.IsEmpty(); ++type)
    {
        if ((ComponentHandlers[type].Selection & select_cmp) == 0)
            continue; // skip this component

        HSaveError err = WriteComponent(out, ComponentHandlers[type], compress);
        if (!err)
        {
            return new SavegameError(err, kSvgErr_ComponentSerialization, "Component: (#%d) %s", type,
                                     ComponentHandlers[type].Name.GetCStr());
        }
    }
    WriteFormatTag(out, ComponentListTag, false);
    return HSaveError::None();
}

} // namespace SavegameComponents
} // namespace Engine
} // namespace AGS
