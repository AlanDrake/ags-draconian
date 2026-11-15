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
#include "game/save/savegame.h"

#include "debug/debugger.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "font/fonts.h"
#include "game/audio/audio.h"
#include "game/dialog/dialog.h"
#include "game/dialog/speech.h"
#include "game/dynobj/cc_audio_channel.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/dynobj/cc_character.h"
#include "game/dynobj/cc_dialog.h"
#include "game/dynobj/cc_gui.h"
#include "game/dynobj/cc_serializer.h"
#include "game/dynobj/script_restored_save_info.h"
#include "game/dynobj/script_shader.h"
#include "game/event.h"
#include "game/file.h"
#include "game/game.h"
#include "game/game_setup_struct.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/render/dynamic_sprite.h"
#include "game/render/overlay.h"
#include "game/render/shader_script.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
#include "game/script.h"
#include "game/system.h"
#include "game/ui/button.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/world/character.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/room_status.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "platform/base/sys_main.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/managed_object_pool.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"
#include "util/common.h"
#include "util/compress.h"
#include "util/file.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"
#include "util/stream.h"
#include "util/string_utils.h"
#include "util/timer.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;


namespace AGS
{
namespace Engine
{

const String SavegameSource::Signature = "Adventure Game Studio saved game v2";

SavegameSource::SavegameSource() : Version(kSvgVersion_Undefined) {}

SavegameDescription::SavegameDescription(const SavegameDescription &desc)
{
    Slot = desc.Slot;
    EngineName = (desc.EngineName);
    EngineVersion = desc.EngineVersion;
    GameGuid = desc.GameGuid;
    LegacyID = desc.LegacyID;
    GameTitle = desc.GameTitle;
    MainDataFilename = desc.MainDataFilename;
    MainDataVersion = desc.MainDataVersion;
    ColorDepth = desc.ColorDepth;
    UserText = desc.UserText;
    if (desc.UserImage)
        UserImage.reset(BitmapHelper::CreateBitmapCopy(desc.UserImage.get()));
}

RestoredData::RestoredData() : FPS(0), CursorID(0), CursorMode(0)
{
    memset(RoomLightLevels, 0, sizeof(RoomLightLevels));
    memset(RoomTintLevels, 0, sizeof(RoomTintLevels));
    memset(RoomZoomLevels1, 0, sizeof(RoomZoomLevels1));
    memset(RoomZoomLevels2, 0, sizeof(RoomZoomLevels2));
}

String GetSavegameErrorText(SavegameErrorType err)
{
    switch (err)
    {
    case kSvgErr_NoError: return "No error.";
    case kSvgErr_FileOpenFailed: return "File not found or could not be opened.";
    case kSvgErr_SignatureFailed: return "Not an AGS saved game or unsupported format.";
    case kSvgErr_FormatVersionNotSupported: return "Save format version not supported.";
    case kSvgErr_IncompatibleEngine: return "Save was written by incompatible engine, or file is corrupted.";
    case kSvgErr_GameGuidMismatch: return "Game GUID does not match, saved by a different game.";
    case kSvgErr_ComponentListOpeningTagFormat: return "Failed to parse opening tag of the components list.";
    case kSvgErr_ComponentListClosingTagMissing: return "Closing tag of the components list was not met.";
    case kSvgErr_ComponentOpeningTagFormat: return "Failed to parse opening component tag.";
    case kSvgErr_ComponentClosingTagFormat: return "Failed to parse closing component tag.";
    case kSvgErr_ComponentSizeMismatch: return "Component data size mismatch.";
    case kSvgErr_UnsupportedComponent: return "Unknown and/or unsupported component.";
    case kSvgErr_ComponentSerialization: return "Failed to write the savegame component.";
    case kSvgErr_ComponentUnserialization: return "Failed to restore the savegame component.";
    case kSvgErr_InconsistentFormat: return "Inconsistent format, or file is corrupted.";
    case kSvgErr_UnsupportedComponentVersion: return "Component data version not supported.";
    case kSvgErr_GameContentAssertion: return "Saved content does not match current game.";
    case kSvgErr_InconsistentData: return "Inconsistent save data, or file is corrupted.";
    case kSvgErr_InconsistentPlugin: return "One of the game plugins did not restore its game data correctly.";
    case kSvgErr_DifferentColorDepth: return "Saved with the engine running at a different colour depth.";
    case kSvgErr_GameObjectInitFailed: return "Game object initialization failed after save restoration.";
    case kSvgErr_ComponentUncompressedSizeMismatch: return "Uncompressed component data size mismatch.";
    case kSvgErr_InternalError: return "Internal program error.";
    default: return "Unknown error.";
    }
}

// A bitmap in a save is stored as width, height and color depth (three int32), followed by either
// the pixel data as they are (not compressed), or by a reserved int32, the size of the deflated data (int32)
// and the deflated pixel data (compressed). The reader must be told which one it is: components are
// stored uncompressed, because the whole component is compressed on its own, while the user's screenshot
// has a flag of its own (kSvgImage_Deflate).
// Returns null if a bitmap of such size cannot be created; the pixel data is not skipped then.
Bitmap *ReadBitmap(Stream *in, bool compressed)
{
    const int width = in->ReadInt32();
    const int height = in->ReadInt32();
    const int color_depth = in->ReadInt32();
    Bitmap *bmp = BitmapHelper::CreateBitmap(width, height, color_depth);
    if (bmp == nullptr)
        return nullptr;

    if (compressed)
    {
        in->ReadInt32(); // reserved
        size_t compressed_size = in->ReadInt32();
        inflate_decompress(bmp->GetDataForWriting(), bmp->GetWidth() * bmp->GetHeight() * bmp->GetBPP(), bmp->GetBPP(),
                           in, compressed_size);
    }
    else
    {
        PixelOp::ReadPixelData(in, bmp->GetDataForWriting(), bmp->GetWidth(), bmp->GetHeight(), bmp->GetBPP(),
                               bmp->GetPitch());
    }

    return bmp;
}

// Skips a bitmap block written by WriteBitmap (see ReadBitmap for the layout)
void SkipBitmap(Stream *in, bool compressed)
{
    int width = in->ReadInt32();
    int height = in->ReadInt32();
    int color_depth = in->ReadInt32();

    if (compressed)
    {
        in->ReadInt32(); // reserved
        size_t compressed_size = in->ReadInt32();
        in->Seek(compressed_size);
    }
    else
    {
        int bpp = (color_depth + 7) / 8;
        in->Seek(width * height * bpp);
    }
}

// Writes a bitmap block (see ReadBitmap for the layout). A compressed bitmap needs a seekable stream,
// because its data size is written back after the data.
void WriteBitmap(const Common::Bitmap *bmp, Stream *out, bool compressed)
{
    assert(bmp);
    if (!bmp)
        return;

    out->WriteInt32(bmp->GetWidth());
    out->WriteInt32(bmp->GetHeight());
    out->WriteInt32(bmp->GetColorDepth());

    if (compressed)
    {
        out->WriteInt32(0); // reserved
        soff_t size_pos = out->GetPosition();
        out->WriteInt32(0); // size placeholder
        deflate_compress(bmp->GetData(), bmp->GetWidth() * bmp->GetHeight() * bmp->GetBPP(), bmp->GetBPP(), out);
        soff_t end_pos = out->GetPosition();
        out->Seek(size_pos, kSeekBegin);
        out->WriteInt32(end_pos - size_pos - sizeof(int32_t));
        out->Seek(end_pos, kSeekEnd);
    }
    else
    {
        PixelOp::WritePixelData(bmp->GetBitmapData(), out);
    }
}

// The user image (screenshot) is stored as a flags int32 (kSvgImage_*), followed by a bitmap block if
// the "present" flag is set. Returns null if there is none.
Bitmap *RestoreUserImage(Stream *in)
{
    uint32_t flags = in->ReadInt32();
    if ((flags & kSvgImage_Present) != 0)
        return ReadBitmap(in, (flags & kSvgImage_Deflate) != 0);
    return nullptr;
}

void SkipUserImage(Stream *in)
{
    uint32_t flags = in->ReadInt32();
    if ((flags & kSvgImage_Present) != 0)
        SkipBitmap(in, (flags & kSvgImage_Deflate) != 0);
}

// Reads the save header, which follows the signature: the format version (int32), the file format block
// (since kSvgVersion_400_18), the environment info (engine name and version, game GUID and title, name and
// version of the main data file, color depth, legacy game id), and the user description (text and image).
// The parts not selected by 'elems' are skipped. Leaves the stream at the start of the game data.
// The offsets of the file format block, where it is present, let a reader skip what it does not know.
HSaveError ReadDescription(Stream *in, SavegameVersion &svg_ver, SavegameDescription &desc, SavegameDescElem elems)
{
    svg_ver = (SavegameVersion)in->ReadInt32();
    if (svg_ver < kSvgVersion_LowestSupported || svg_ver > kSvgVersion_Current)
        return new SavegameError(kSvgErr_FormatVersionNotSupported, "Required: %d, supported: %d - %d.", svg_ver,
                                 kSvgVersion_LowestSupported, kSvgVersion_Current);

    const bool has_fileformat = svg_ver >= kSvgVersion_400_18;

    // File format info
    if (has_fileformat)
    {
        desc.Format.FileFormatOffset = in->GetPosition();
        desc.Format.FileFormatSize = in->ReadInt32();
        desc.Format.Flags = in->ReadInt32();
        desc.Format.EnvInfoOffset = in->ReadInt32();
        desc.Format.UserDescOffset = in->ReadInt32();
        desc.Format.GameDataOffset = in->ReadInt32();
    }

    // If env info offset is valid, then skip right to env info
    if (desc.Format.EnvInfoOffset > 0)
        in->Seek(desc.Format.EnvInfoOffset, kSeekBegin);

    // Environment information
    const soff_t env_info_pos = in->GetPosition();
    const uint32_t env_info_size = in->ReadInt32(); // size of the whole environment info block
    desc.EngineName = StrUtil::ReadString(in);
    desc.EngineVersion.SetFromString(StrUtil::ReadString(in));
    desc.GameGuid = StrUtil::ReadString(in);
    desc.GameTitle = StrUtil::ReadString(in);
    desc.MainDataFilename = StrUtil::ReadString(in);
    desc.MainDataVersion = (GameDataVersion)in->ReadInt32();
    desc.ColorDepth = in->ReadInt32();
    desc.LegacyID = in->ReadInt32();

    // If user desc offset is valid, then skip any remaining part
    // (this is in case there is some data that we do not support)
    const soff_t user_desc_pos =
        (desc.Format.UserDescOffset > 0u) ? desc.Format.UserDescOffset : (env_info_pos + env_info_size);
    in->Seek(user_desc_pos, kSeekBegin);

    // User description
    if (elems & kSvgDesc_UserText)
        desc.UserText = StrUtil::ReadString(in);
    else
        StrUtil::SkipString(in);

    if (elems & kSvgDesc_UserImage)
        desc.UserImage.reset(RestoreUserImage(in));
    else
        SkipUserImage(in);

    // Saves before kSvgVersion_400_18 have no file format block: derive its values from what was read
    if (!has_fileformat)
    {
        desc.Format.Flags = 0u;
        desc.Format.EnvInfoOffset = env_info_pos;
        desc.Format.UserDescOffset = user_desc_pos;
        desc.Format.GameDataOffset = in->GetPosition();
    }

    // Skip directly to the game state data
    // (this is in case there is some data that we do not support)
    if (has_fileformat)
    {
        in->Seek(desc.Format.GameDataOffset, kSeekBegin);
    }

    return HSaveError::None();
}

// Tests for the save signature at the start of the stream
bool CheckSaveSignature(Stream *in)
{
    String svg_sig = String::FromStreamCount(in, SavegameSource::Signature.GetLength());
    return svg_sig.Compare(SavegameSource::Signature) == 0;
}

// Opens the save file, checks the signature and reads the header. 'src', if given, takes the open stream;
// otherwise the file is closed on return. 'desc', if given, receives the parts selected by 'elems'.
HSaveError OpenSavegameBase(const String &filename, SavegameSource *src, SavegameDescription *desc,
                            SavegameDescElem elems)
{
    UStream in(File::OpenFileRead(filename));
    if (!in.get())
        return new SavegameError(kSvgErr_FileOpenFailed, "Requested filename: %s.", filename.GetCStr());

    // Check saved game signature
    if (!CheckSaveSignature(in.get()))
    {
        return new SavegameError(kSvgErr_SignatureFailed);
    }

    SavegameVersion svg_ver;
    SavegameDescription temp_desc;
    HSaveError err = ReadDescription(in.get(), svg_ver, temp_desc, desc ? elems : kSvgDesc_None);
    if (!err)
        return err;

    if (src)
    {
        src->Filename = filename;
        src->Version = svg_ver;
        src->InputStream.reset(in.release()); // give the stream away to the caller
        src->Format = temp_desc.Format;
    }
    if (desc)
    {
        if (elems & kSvgDesc_EnvInfo)
        {
            desc->EngineName = temp_desc.EngineName;
            desc->EngineVersion = temp_desc.EngineVersion;
            desc->GameGuid = temp_desc.GameGuid;
            desc->LegacyID = temp_desc.LegacyID;
            desc->GameTitle = temp_desc.GameTitle;
            desc->MainDataFilename = temp_desc.MainDataFilename;
            desc->MainDataVersion = temp_desc.MainDataVersion;
            desc->ColorDepth = temp_desc.ColorDepth;
        }
        if (elems & kSvgDesc_FileFormat)
            desc->Format = temp_desc.Format;
        if (elems & kSvgDesc_UserText)
            desc->UserText = temp_desc.UserText;
        if (elems & kSvgDesc_UserImage)
            desc->UserImage = std::move(temp_desc.UserImage);
    }
    return err;
}

bool DoesSavegameExist(const String &filename)
{
    return File::IsFile(filename);
}

HSaveError OpenSavegame(const String &filename, SavegameSource &src, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, &src, &desc, elems);
}

HSaveError OpenSavegame(const String &filename, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegameBase(filename, nullptr, &desc, elems);
}

// Prepares engine for actual save restore (stops processes, cleans up memory)
void DoBeforeRestore(PreservedParams &pp, SaveCmpSelection select_cmp)
{
    pp.SpeechVOX = play.voice_avail;
    pp.MusicVOX = play.separate_music_lib;
    memcpy(pp.GameOptions, game.options, GameSetupStruct::MAX_OPTIONS * sizeof(int));

    unload_old_room();
    remove_all_overlays();
    play.complete_overlay_on = 0;
    play.text_overlay_on = 0;

    // cleanup dynamic sprites
    // NOTE: sprite 0 is reserved for the "empty sprite" and is never allocated as a dynamic one
    // (SpriteCache::MIN_SPRITE_INDEX)
    for (size_t i = 1; i < spriteset.GetSpriteSlotCount(); ++i)
    {
        if (game.SpriteInfos[i].Flags & SPF_DYNAMICALLOC)
        {
            free_dynamic_sprite(i);
        }
    }

    // Cleanup draw caches
    clear_drawobj_cache(false /* reset everything */);

    // preserve script data sizes and cleanup scripts
    pp.GlScDataSize = gamescript->GetGlobalData().size();
    pp.ScriptModuleNames.resize(numScriptModules);
    pp.ScMdDataSize.resize(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        pp.ScriptModuleNames[i] = scriptModules[i]->GetScriptName();
        pp.ScMdDataSize[i] = scriptModules[i]->GetGlobalData().size();
    }

    // TODO: investigate if we actually have to do this when restoring a save
    UnlinkAllScripts();

    // reset saved room states
    ResetRoomStates();
    // reset temp room state
    troom = RoomStatus();
    // Free what GameState owns outside of its plain fields (properties, viewports and cameras, do-once tokens);
    // the plain fields are overwritten by the GameState component
    // TODO: reset the whole GameState object here instead, once it is clear nothing in it must survive
    play.FreeProperties();
    play.FreeViewportsAndCameras();
    free_do_once_tokens();

    RemoveAllButtonAnimations();
    // Clear the managed object pool; the saved objects are read back from the save
    ccUnregisterAllObjects();

    if ((select_cmp & kSaveCmp_Audio) != 0)
    {
        for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
        {
            stop_and_destroy_channel(i);
        }
    }
}

void FillPreservedParams(PreservedParams &pp)
{
    // preserve script data sizes
    pp.GlScDataSize = gamescript->GetGlobalData().size();
    pp.ScriptModuleNames.resize(numScriptModules);
    pp.ScMdDataSize.resize(numScriptModules);
    for (size_t i = 0; i < numScriptModules; ++i)
    {
        pp.ScriptModuleNames[i] = scriptModules[i]->GetScriptName();
        pp.ScMdDataSize[i] = scriptModules[i]->GetGlobalData().size();
    }
}

static HSaveError RestoreAudio(const RestoredData &r_data)
{
    // Drop the preloaded clips of the music queue (they are unique_ptr, so this frees them);
    // the queue itself comes from the save, without the loaded clips
    // TODO: perhaps this should be done in DoBeforeRestore instead, with the rest of the cleanup
    for (int i = 0; i < play.new_music_queue_size; ++i)
    {
        play.new_music_queue[i].cachedClip = nullptr;
    }

    if (play.audio_master_volume >= 0)
    {
        int temp_vol = play.audio_master_volume;
        play.audio_master_volume = -1; // reset to invalid state before re-applying
        System_SetVolume(temp_vol);
    }

    // Run audio clips on channels
    // these two crossfading parameters have to be temporarily reset
    const int cf_in_chan = play.crossfading_in_channel;
    const int cf_out_chan = play.crossfading_out_channel;
    play.crossfading_in_channel = 0;
    play.crossfading_out_channel = 0;

    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
    {
        const RestoredData::ChannelInfo &chan_info = r_data.AudioChans[i];
        if (chan_info.ClipID < 0 && chan_info.FileName.IsEmpty())
            continue;

        if (chan_info.ClipID >= 0)
        {
            if ((size_t)chan_info.ClipID >= game.audioClips.size())
            {
                return new SavegameError(kSvgErr_GameObjectInitFailed,
                                         "Invalid audio clip index: %d (clip count: %zu).", chan_info.ClipID,
                                         game.audioClips.size());
            }

            int audio_type = chan_info.AudioType != AUDIOTYPE_UNDEFINED ? chan_info.AudioType
                                                                        : game.audioClips[chan_info.ClipID].type;
            // TODO: queueIfNoChannel is set here only because play_audio_clip then does not remove the clips of
            // this audio type from the queue, which was just restored; a parameter of its own would say it
            play_audio_clip(AudioPlayback(&game.audioClips[chan_info.ClipID], audio_type), i, chan_info.Priority,
                            chan_info.Repeat, chan_info.Pos, true /* queueIfNoChannel */);
        }
        else
        {
            auto sound = load_sound_clip(chan_info.FileName, chan_info.BundlingType, chan_info.Repeat);
            if (sound)
                play_sound_on_channel(std::move(sound), i, chan_info.Priority, chan_info.Repeat, chan_info.Pos);
        }

        auto *playback = AudioChans::GetChannel(i);
        if (playback != nullptr)
        {
            playback->set_volume_direct(chan_info.VolAsPercent, chan_info.Vol);
            playback->set_speed(chan_info.Speed);
            playback->set_panning(chan_info.Pan);
            playback->xSource = chan_info.XSource;
            playback->ySource = chan_info.YSource;
            playback->maximumPossibleDistanceAway = chan_info.MaxDist;

            if ((chan_info.Flags & kSvgAudioPaused) != 0)
                playback->pause();
        }
    }
    if ((cf_in_chan > 0) && (AudioChans::GetChannel(cf_in_chan) != nullptr))
        play.crossfading_in_channel = cf_in_chan;
    if ((cf_out_chan > 0) && (AudioChans::GetChannel(cf_out_chan) != nullptr))
        play.crossfading_out_channel = cf_out_chan;

    // If there were synced audio tracks, the time taken to load in the
    // different channels will have thrown them out of sync, so re-time it
    for (int i = 0; i < TOTAL_AUDIO_CHANNELS; ++i)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(i);
        int pos = r_data.AudioChans[i].Pos;
        if ((pos > 0) && (playback != nullptr))
        {
            playback->seek(pos);
        }
    }

    update_directional_sound_vol();
    return HSaveError::None();
}

static void RestoreViewportsAndCameras(const RestoredData &r_data)
{
    for (size_t i = 0; i < r_data.Cameras.size(); ++i)
    {
        const auto &cam_dat = r_data.Cameras[i];
        auto cam = play.GetRoomCamera(i);
        cam->SetID(cam_dat.ID);
        if ((cam_dat.Flags & kSvgCamPosLocked) != 0)
            cam->Lock();
        else
            cam->Release();
        // Set size first, or offset position may clamp to the room
        cam->SetSize(Size(cam_dat.Width, cam_dat.Height));
        cam->SetAt(cam_dat.Left, cam_dat.Top);
        cam->SetRotation(cam_dat.Rotation);
        cam->SetPivot(cam_dat.Pivot);
        cam->SetPivotOffset(cam_dat.PivotOffset);
    }
    for (size_t i = 0; i < r_data.Viewports.size(); ++i)
    {
        const auto &view_dat = r_data.Viewports[i];
        auto view = play.GetRoomViewport(i);
        view->SetID(view_dat.ID);
        view->SetVisible((view_dat.Flags & kSvgViewportVisible) != 0);
        view->SetRect(RectWH(view_dat.Left, view_dat.Top, view_dat.Width, view_dat.Height));
        view->SetZOrder(view_dat.ZOrder);
        view->SetShader(view_dat.ShaderID, view_dat.ShaderHandle);
        // Restore camera link
        int cam_index = view_dat.CamID;
        if (cam_index < 0)
            continue;
        auto cam = play.GetRoomCamera(cam_index);
        view->LinkCamera(cam);
        cam->LinkToViewport(view);
    }
    play.InvalidateViewportZOrder();
}

// Resets the options that are not supposed to be changed at runtime (the restricted and the preserved ones)
// to the values the running game had before the restoration (see DoBeforeRestore)
static void CopyPreservedGameOptions(GameSetupStructBase &gs, const PreservedParams &pp)
{
    const auto restricted_opts = GameSetupStructBase::GetRestrictedOptions();
    for (auto opt : restricted_opts)
        gs.options[opt] = pp.GameOptions[opt];
    const auto preserved_opts = GameSetupStructBase::GetPreservedOptions();
    for (auto opt : preserved_opts)
        gs.options[opt] = pp.GameOptions[opt];
}

// A callback that tests if DynamicSprite refers a valid dynamic sprite in cache, and resets it to
// sprite 0 if it does not. Used in a call to ccTraverseManagedObjects.
static void ValidateDynamicSprite(int handle, IScriptObject *obj)
{
    ScriptDynamicSprite *dynsprite = static_cast<ScriptDynamicSprite *>(obj);
    if (dynsprite->slot < 0 || static_cast<uint32_t>(dynsprite->slot) >= game.SpriteInfos.size() ||
        !game.SpriteInfos[dynsprite->slot].IsDynamicSprite())
    {
        dynsprite->slot = 0; // 0 is a "safety placeholder" sprite id
    }
}

// Call a scripting event to let user validate the restored save: returns an error if the script cancels
// the restoration, and asks for a retry through 'feedback' if the script sets components to skip
static HSaveError ValidateRestoredSave(const SavegameDescription &save_desc, const RestoredData &r_data,
                                       SaveRestoreFeedback &feedback)
{
    auto *save_info = new ScriptRestoredSaveInfo(
        (SaveRestorationFlags)(r_data.Result.RestoreFlags & kSaveRestore_ResultMask), save_desc, r_data.DataCounts,
        (r_data.Result.RestoreFlags & kSaveRestore_MismatchMask) != 0);
    int handle = ccRegisterManagedObject(save_info, save_info);
    ccAddObjectReference(handle); // add internal ref

    RuntimeScriptValue params[1] = {RuntimeScriptValue().SetScriptObject(save_info, save_info)};
    RunScriptFunctionInModules("validate_restored_save", 1, params); // TODO: run in room script too?

    const bool do_cancel = save_info->GetCancel();
    const SaveCmpSelection retry_ignore_cmp = save_info->GetRetryWithoutComponents();
    ccReleaseObjectReference(handle); // rem internal ref

    if (do_cancel)
    {
        return new SavegameError(kSvgErr_GameContentAssertion, r_data.Result.FirstMismatchError);
    }

    if (retry_ignore_cmp != 0)
    {
        feedback.RetryWithClearGame = true;
        feedback.RetryWithoutComponents = retry_ignore_cmp;
        return new SavegameError(kSvgErr_GameContentAssertion);
    }

    return HSaveError::None();
}

// Final processing after the save components were read: applies the restored data to the engine
// (scripts, room, audio, GUI, viewports), runs the validation callback and the post-restore plugin event
HSaveError DoAfterRestore(const PreservedParams &pp, RestoredData &r_data, SaveCmpSelection select_cmp)
{
    EngineServices &services = EngineServices::Current();
    // Put back what the running game had before the restoration: whether the speech pack and the
    // separate music library are available
    play.voice_avail = pp.SpeechVOX;
    play.separate_music_lib = pp.MusicVOX;

    // Put back the game options that must not change at runtime, whatever the save had
    CopyPreservedGameOptions(game, pp);

    // Keep the debug mode on if the engine was started in it, whatever the save had
    if (debug_flags & DBG_DEBUGMODE)
        play.debug_mode = 1;

    // Move the restored dynamic surfaces to where the engine keeps them
    const size_t dynamic_surface_count = std::min((size_t)MAX_DYNAMIC_SURFACES, r_data.DynamicSurfaces.size());
    for (size_t i = 0; i < dynamic_surface_count; ++i)
    {
        dynamicallyCreatedSurfaces[i] = std::move(r_data.DynamicSurfaces[i]);
    }

    // Rebuild GUI links to child controls
    GUIRefCollection gui_control_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
    GUI::RebuildGUI(guis, gui_control_refs);

    // Register any static script objects missing from the restored managed pool
    RegisterMissingManagedObjects(game);

    // Link the scripts again, DoBeforeRestore unlinked them
    HError err = LinkGlobalScripts();
    if (!err)
    {
        return new SavegameError(err, kSvgErr_GameObjectInitFailed, "Failed to link script modules");
    }

    // copy the saved global data into the global script
    if (!r_data.GlobalScript.Data.empty())
    {
        gamescript->CopyGlobalData(r_data.GlobalScript.Data);
    }

    // copy the saved data into the script modules, which are found by name
    for (auto &script_entry : r_data.ScriptModules)
    {
        const String &name = script_entry.first;
        auto &script_data = script_entry.second;
        if (script_data.Data.empty())
            continue;
        for (auto &script_module : scriptModules)
        {
            if (name.Compare(script_module->GetScriptName()) == 0)
            {
                script_module->CopyGlobalData(script_data.Data);
                break;
            }
        }
    }

    setup_player_character(game.playercharacter);

    // Save some parameters to restore them after room load
    const int gscript_timer = play.gscript_timer;
    const Rect mouse_bounds = play.mbounds;
    // load the room the game was saved in
    displayed_room = r_data.Room;
    if (displayed_room >= 0)
    {
        load_new_room(displayed_room, nullptr);
    }
    else
    {
        set_room_placeholder();
    }

    // Apply restored RTTI placeholder, after current room was loaded;
    // remap typeids in the deserialized managed objects, where necessary
    std::unordered_map<uint32_t, uint32_t> loc_l2g, type_l2g;
    RuntimeScript::JoinRTTI(r_data.GenRTTI, loc_l2g, type_l2g);
    pool.RemapTypeids(type_l2g);

    // Reapply few parameters after room load
    play.gscript_timer = gscript_timer;
    play.mbounds = mouse_bounds;


    set_cursor_mode(r_data.CursorMode);
    set_cursor_look(r_data.CursorID, true);
    if (game.HasCursorRole(r_data.CursorID, kCursorRole_UseInv))
        SetActiveInventory(playerchar->activeinv);
    // precache current cursor
    spriteset.PrecacheSprite(game.mcurs[r_data.CursorID].pic);

    services.Platform->SetWindowTitle(play.game_name);

    if (displayed_room >= 0)
    {
        // Fixup the frame index, in case the restored room does not have enough background frames
        if (play.bg_frame < 0 || static_cast<size_t>(play.bg_frame) >= thisroom.BgFrameCount)
            play.bg_frame = 0;

        for (size_t i = 0; i < std::min(thisroom.BgImages.size(), r_data.RoomBkgScene.size()); ++i)
        {
            if (r_data.RoomBkgScene[i])
            {
                play.room_bg_modified[i] = true;
                // Blit, don't replace image, in case we restored a image of different size
                thisroom.BgImages[i]->Clear(0);
                thisroom.BgImages[i]->Blit(r_data.RoomBkgScene[i].get());
            }
        }

        for (int i = 0; i < kNumRoomAreaTypes; ++i)
        {
            if (r_data.RoomMask[i])
            {
                play.room_mask_modified[i] = true;
                // Blit, don't replace mask, in case we restored a mask of different size
                thisroom.CopyMask(static_cast<RoomAreaMask>(i), r_data.RoomMask[i].get());
            }
        }

        in_new_room = kEnterRoom_RestoredSave; // don't run "enters screen" events
        // now that room has loaded, copy saved region light and tint levels in
        for (size_t i = 0; i < MAX_ROOM_REGIONS; ++i)
        {
            thisroom.Regions[i].Light = r_data.RoomLightLevels[i];
            thisroom.Regions[i].Tint = r_data.RoomTintLevels[i];
        }
        generate_light_table();

        // and the walkable areas' scaling (the far and the near zoom levels)
        for (size_t i = 0; i < MAX_WALK_AREAS; ++i)
        {
            thisroom.WalkAreas[i].ScalingFar = r_data.RoomZoomLevels1[i];
            thisroom.WalkAreas[i].ScalingNear = r_data.RoomZoomLevels2[i];
        }

        on_background_frame_change();
    }

    if ((select_cmp & kSaveCmp_Audio) != 0)
    {
        HSaveError err = RestoreAudio(r_data);
        if (!err)
            return err;
    }

    RestoreShaders();

    adjust_fonts_for_render_mode(game.options[OPT_ANTIALIASFONTS] != 0);

    restore_characters();
    restore_movelists();
    restore_overlays();

    prepare_gui_runtime(false /* not startup */);

    RestoreViewportsAndCameras(r_data);
    set_game_speed(r_data.FPS);

    // Run fixups over managed objects if necessary
    if ((select_cmp & kSaveCmp_DynamicSprites) == 0)
    {
        // If dynamic sprite images were not restored from this save, then invalidate all
        // DynamicSprite objects in the managed pool
        ccTraverseManagedObjects(ScriptDynamicSprite::TypeID, ValidateDynamicSprite);
    }

    // After all of the game logical state is initialized and reapplied values from save,
    // call a "validate" script callback, to let user check the restored save
    // and make final decision: whether to continue with the game, or cancel the restoration.
    HSaveError validate_err = ValidateRestoredSave(pp.Desc, r_data, r_data.Result.Feedback);
    if (!validate_err)
        return validate_err;

    // Run optional plugin event, reporting game restored
    pl_run_plugin_hooks(kPluginEvt_PostRestoreGame, 0);

    // Next load up any immediately required resources
    // If this is a restart point and no room was loaded, then load startup room
    if (displayed_room < 0)
    {
        load_new_room(playerchar->room, playerchar);
        first_room_initialization();
    }

    Mouse::SetMoveLimit(play.mbounds); // apply mouse bounds

    // Apply accessibility options, must be done last, because some
    // may override restored game settings
    ApplyAccessibilityOptions(play, services.Config);
    // Applies behavior options, also hacks and backwards compatibility fixes
    ApplyBehaviorOptions(game, play, services.Config);

    play.ClearIgnoreInput(); // don't keep ignored input after save restore
    update_polled_stuff();

    return HSaveError::None();
}

// Fixes up a requested component selection, in case we must override or
// substitute something internally.
static SaveCmpSelection FixupCmpSelection(SaveCmpSelection select_cmp)
{
    // If kSaveCmp_DynamicSprites is not set, then set kSaveCmp_ObjectSprites,
    // to ensure that object-owned images are still serialized.
    return (SaveCmpSelection)(select_cmp | kSaveCmp_ObjectSprites * ((select_cmp & kSaveCmp_DynamicSprites) == 0));
}

HSaveError RestoreGameState(Stream *in, SavegameVersion save_ver, const SavegameDescription &desc,
                            const RestoreGameStateOptions &options, SaveRestoreFeedback &feedback)
{
    SaveCmpSelection select_cmp = FixupCmpSelection(options.SelectedComponents);
    const bool has_validate_cb = DoesScriptFunctionExistInModules("validate_restored_save");

    PreservedParams pp(desc);
    RestoredData r_data;
    DoBeforeRestore(pp, select_cmp); // WARNING: this frees scripts and some other data

    // Mark the clear game data state for restoration process
    r_data.Result.RestoreFlags =
        (SaveRestorationFlags)((kSaveRestore_ClearData * options.IsGameClear)       // tell that the game data is reset
                               | (kSaveRestore_AllowMismatchLess * has_validate_cb) // allow less data in saves
        );

    HSaveError err = SavegameComponents::ReadAll(in, save_ver, select_cmp, pp, r_data);
    feedback = r_data.Result.Feedback;
    if (!err)
        return err;
    return DoAfterRestore(pp, r_data, select_cmp);
}

HSaveError PrescanSaveState(Stream *in, SavegameVersion save_ver, const SavegameDescription &desc,
                            const RestoreGameStateOptions &options)
{
    SaveCmpSelection select_cmp = FixupCmpSelection(options.SelectedComponents);
    const bool has_validate_cb = DoesScriptFunctionExistInModules("validate_restored_save");

    PreservedParams pp(desc);
    RestoredData r_data;
    FillPreservedParams(pp);

    // Mark the clear game data state for restoration process
    r_data.Result.RestoreFlags =
        (SaveRestorationFlags)((kSaveRestore_ClearData) // always tell that the game data is reset for prescanning
                               | (kSaveRestore_AllowMismatchLess * has_validate_cb) // allow less data in saves
        );

    HSaveError err = SavegameComponents::PrescanAll(in, save_ver, select_cmp, pp, r_data);
    if (!err)
    {
        return err;
    }

    if (has_validate_cb)
    {
        // After we have prescanned and gathered save info,
        // call a "validate" script callback, to let user check the restored save
        // and make final decision: whether it is considered compatible or not.
        err = ValidateRestoredSave(pp.Desc, r_data, r_data.Result.Feedback);
    }
    return err;
}

HSaveError ReadSaveDescription(const String &filename, SavegameDescription &desc, SavegameDescElem elems)
{
    return OpenSavegame(filename, desc, elems);
}

HSaveError RestoreSavegame(const String &filename, const RestoreGameStateOptions &options,
                           SaveRestoreFeedback &feedback)
{
    SavegameSource src;
    SavegameDescription desc;
    HSaveError err = OpenSavegame(filename, src, desc, (SavegameDescElem)(kSvgDesc_FileFormat | kSvgDesc_EnvInfo));
    if (!err)
        return err;

    err = RestoreGameState(src.InputStream.get(), src.Version, desc, options, feedback);
    return err;
}

// Writes the user image block (see RestoreUserImage): the flags, and the bitmap if there is one
void WriteUserImage(Stream *out, const Bitmap *screenshot, bool compress)
{
    uint32_t flags = (screenshot != nullptr) * kSvgImage_Present | compress * kSvgImage_Deflate;
    out->WriteInt32(flags);

    if (screenshot)
        WriteBitmap(screenshot, out, compress);
}

// Writes the file format block (see SavegameFileFormat): at FileFormatOffset if it is set (which seeks there),
// at the current position otherwise. The stream is left after the block.
void WriteFileFormat(Stream *out, const SavegameFileFormat &file_format)
{
    if (file_format.FileFormatOffset > 0)
        out->Seek(file_format.FileFormatOffset, kSeekBegin);

    out->WriteInt32(file_format.FileFormatSize);
    out->WriteInt32(file_format.Flags);
    out->WriteInt32(file_format.EnvInfoOffset);
    out->WriteInt32(file_format.UserDescOffset);
    out->WriteInt32(file_format.GameDataOffset);
}

// Writes the save header that follows the signature (see ReadDescription for the layout). The file format
// block is only a placeholder here, since the offset of the game data is not known yet: 'file_format'
// receives the offsets known so far, and the caller writes the block again when the save is finished.
// The screenshot is always compressed.
void WriteDescription(Stream *out, const String &user_text, const Bitmap *user_image, SavegameFileFormat &file_format)
{
    EngineServices &services = EngineServices::Current();
    // Data format version
    out->WriteInt32(kSvgVersion_Current);
    soff_t file_format_pos = out->GetPosition();
    WriteFileFormat(out, SavegameFileFormat()); // write placeholder
    // Environment information
    soff_t env_info_pos = out->GetPosition();
    out->WriteInt32(0); // size placeholder
    // TODO: the environment info is read from global objects (game, engine version, resource paths);
    // pass it as arguments instead
    StrUtil::WriteString(get_engine_name(), out);
    StrUtil::WriteString(services.Version.LongString, out);
    StrUtil::WriteString(game.guid, out);
    StrUtil::WriteString(game.gamename, out);
    StrUtil::WriteString(services.ResPaths.GamePak.Name, out);
    out->WriteInt32(loaded_game_file_version);
    out->WriteInt32(game.GetColorDepth());
    out->WriteInt32(game.uniqueid);
    // Write the size of the environment info block into its placeholder
    soff_t env_info_end_pos = out->GetPosition();
    out->Seek(env_info_pos, kSeekBegin);
    out->WriteInt32(env_info_end_pos - env_info_pos);
    out->Seek(env_info_end_pos, kSeekBegin);
    // User description
    soff_t user_desc_pos = out->GetPosition();
    StrUtil::WriteString(user_text, out);
    WriteUserImage(out, user_image, true /* always compress screenshots */);

    // Fill few known fields for SavegameFileFormat
    file_format.FileFormatOffset = file_format_pos;
    file_format.FileFormatSize = env_info_pos - file_format_pos;
    file_format.EnvInfoOffset = env_info_pos;
    file_format.UserDescOffset = user_desc_pos;
}

std::unique_ptr<Stream> StartSavegame(const String &filename, const String &user_text, const Bitmap *user_image,
                                      SavegameFileFormat &file_format)
{
    auto out = File::CreateFile(filename);
    if (!out)
        return nullptr;

    // Savegame signature
    out->Write(SavegameSource::Signature.GetCStr(), SavegameSource::Signature.GetLength());
    // Write description block
    WriteDescription(out.get(), user_text, user_image, file_format);
    return out;
}

void DoBeforeSave()
{
    if (displayed_room >= 0)
    {
        // update the current room script's data segment copy
        if (roomscript)
            save_room_data_segment();
    }
}

void SaveGameState(Stream *out, SaveCmpSelection select_cmp, bool compress)
{
    select_cmp = FixupCmpSelection(select_cmp);

    DoBeforeSave();
    SavegameComponents::WriteAllCommon(out, select_cmp, compress);
}

// Reads the layout written by WritePluginSaveData (kPluginSvgVersion_36115 and later), giving each
// plugin its own data by name. Before that version a plugin's data was not delimited, so every plugin
// subscribed to the restore event is given a stream section that runs to the end of the component.
HSaveError ReadPluginSaveData(Stream *in, PluginSvgVersion svg_ver, soff_t max_size)
{
    const soff_t start_pos = in->GetPosition();
    const soff_t end_pos = start_pos + max_size;

    if (svg_ver >= kPluginSvgVersion_36115)
    {
        // IMPORTANT: we have to use an intermediate vector here for plugin data,
        // because we need to safeguard a stream section, but the savegame stream is
        // restricted from Seeking, as it may be doing decompression right in memory.
        std::vector<uint8_t> plugin_data;
        int num_plugins_read = in->ReadInt32();
        soff_t cur_pos = start_pos;
        while ((num_plugins_read--) > 0 && (cur_pos < end_pos))
        {
            String pl_name = StrUtil::ReadString(in);
            size_t data_size = in->ReadInt32();
            soff_t data_start = in->GetPosition();

            try
            {
                // Allocate enough to read data_size, but then trim if read less
                plugin_data.resize(data_size);
                plugin_data.resize(in->Read(plugin_data.data(), data_size));
                auto guard_stream = std::make_unique<Stream>(std::make_unique<VectorStream>(plugin_data, kStream_Read));
                int32_t file_handle = add_file_stream(std::move(guard_stream), "RestoreGame");
                pl_run_plugin_hook_by_name(pl_name, kPluginEvt_RestoreGame, file_handle);
                close_file_stream(file_handle, "RestoreGame");
                guard_stream = nullptr;
            }
            catch (std::runtime_error ex)
            {
                return new SavegameError(kSvgErr_InternalError, String(ex.what()));
            }

            // Read-out until the end of plugin data, in case it ended up reading not in the end
            cur_pos = data_start + data_size;
            in->ReadByteCount(cur_pos - in->GetPosition());
        }
    }
    else
    {
        // NOTE: we assume that the old save format won't be found in a compressed save,
        // so we can use StreamSection here. Fix the code below if this assumption becomes incorrect.
        String pl_name;
        for (uint32_t pl_index = 0; pl_query_next_plugin_for_event(kPluginEvt_RestoreGame, pl_index, pl_name);
             ++pl_index)
        {
            try
            {
                auto guard_stream = std::make_unique<Stream>(
                    std::make_unique<StreamSection>(in->GetStreamBase(), in->GetPosition(), end_pos));
                int32_t file_handle = add_file_stream(std::move(guard_stream), "RestoreGame");
                pl_run_plugin_hook_by_index(pl_index, kPluginEvt_RestoreGame, file_handle);
                close_file_stream(file_handle, "RestoreGame");
            }
            catch (std::runtime_error ex)
            {
                return new SavegameError(kSvgErr_InternalError, String(ex.what()));
            }
        }
    }

    return HSaveError::None();
}

// Layout: the number of plugins (int32), then for each of them its name (string), the data size (int32)
// and the data. The plugins are the ones subscribed to the save event, in the order the plugin engine lists them.
HSaveError WritePluginSaveData(Stream *out)
{
    uint32_t num_plugins_wrote = 0;
    String pl_name;
    for (uint32_t pl_index = 0; pl_query_next_plugin_for_event(kPluginEvt_SaveGame, pl_index, pl_name); ++pl_index)
    {
        // NOTE: we don't care if they really write anything,
        // but count them so long as they subscribed to AGSE_SAVEGAME
        num_plugins_wrote++;
    }
    out->WriteInt32(num_plugins_wrote); // number of plugins subscribed to the save event

    // IMPORTANT: we have to use an intermediate vector here for plugin data,
    // because we need to prepend its size, but the savegame stream is
    // restricted from Seeking, as it may be doing compression right in memory.
    std::vector<uint8_t> plugin_data;
    for (uint32_t pl_index = 0; pl_query_next_plugin_for_event(kPluginEvt_SaveGame, pl_index, pl_name); ++pl_index)
    {
        // Let the plugin write its data into a memory stream
        // TODO: implement a size limit for plugin data (now it is not limited, a plugin may write any amount):
        // * support a limit in VectorStream
        // * define a reasonable default limit (e.g. 16-32 MB)
        // * compare with system memory?
        plugin_data.clear();

        try
        {
            auto guard_stream = std::make_unique<Stream>(std::make_unique<VectorStream>(plugin_data, kStream_Write));
            int32_t file_handle = add_file_stream(std::move(guard_stream), "SaveGame");
            pl_run_plugin_hook_by_index(pl_index, kPluginEvt_SaveGame, file_handle);
            close_file_stream(file_handle, "SaveGame");
            guard_stream = nullptr;
        }
        catch (std::runtime_error ex)
        {
            return new SavegameError(kSvgErr_InternalError, String(ex.what()));
        }

        // Write a header for plugin data
        StrUtil::WriteString(pl_name, out);
        out->WriteInt32(plugin_data.size()); // data size
        out->Write(plugin_data.data(), plugin_data.size());
    }

    return HSaveError::None();
}

HSaveError SaveGame(const String &filename, const String &user_text, const Bitmap *user_image,
                    SaveCmpSelection select_cmp, bool compress_data)
{
    SavegameFileFormat file_format;
    file_format.Flags = kSvgFmt_DeflateComponents * compress_data;
    std::unique_ptr<Stream> out(StartSavegame(filename, user_text, user_image, file_format));
    if (!out)
        return new SavegameError(kSvgErr_FileOpenFailed, "Requested filename: %s.", filename.GetCStr());

    file_format.GameDataOffset = out->GetPosition();
    SaveGameState(out.get(), select_cmp, compress_data);

    // Finalize the save file, write the composed file format block over its placeholder
    WriteFileFormat(out.get(), file_format);
    return HSaveError::None();
}

//=============================================================================
//
// RestoredSaveInfo API
//
//=============================================================================

} // namespace Engine
} // namespace AGS

#include "debug/debug_log.h"

bool SaveInfo_GetCancel(ScriptRestoredSaveInfo *info)
{
    return info->GetCancel();
}

void SaveInfo_SetCancel(ScriptRestoredSaveInfo *info, bool cancel)
{
    info->SetCancel(cancel);
}

int SaveInfo_GetRetryWithoutComponents(ScriptRestoredSaveInfo *info)
{
    return info->GetRetryWithoutComponents();
}

void SaveInfo_SetRetryWithoutComponents(ScriptRestoredSaveInfo *info, int cmp_selection)
{
    info->SetRetryWithoutComponents(static_cast<SaveCmpSelection>(cmp_selection));
}

bool SaveInfo_HasExtraData(ScriptRestoredSaveInfo *info)
{
    return (info->GetResult() & kSaveRestore_ExtraDataInSave) != 0;
}

bool SaveInfo_HasMissingData(ScriptRestoredSaveInfo *info)
{
    return (info->GetResult() & kSaveRestore_MissingDataInSave) != 0;
}

bool SaveInfo_IsPrescan(ScriptRestoredSaveInfo *info)
{
    return (info->GetResult() & kSaveRestore_Prescan) != 0;
}

int SaveInfo_GetSlot(ScriptRestoredSaveInfo *info)
{
    return info->GetDesc().Slot;
}

const char *SaveInfo_GetDescription(ScriptRestoredSaveInfo *info)
{
    return CreateNewScriptString(info->GetDesc().UserText.GetCStr());
}

const char *SaveInfo_GetEngineVersion(ScriptRestoredSaveInfo *info)
{
    return CreateNewScriptString(info->GetDesc().EngineVersion.LongString.GetCStr());
}

const char *SaveInfo_GetGameVersion(ScriptRestoredSaveInfo *info)
{
    return CreateNewScriptString(
        (info->GetCounts().GameInfo.count("version") > 0) ? info->GetCounts().GameInfo.at("version").GetCStr() : "");
}

int SaveInfo_GetAudioClipTypeCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().AudioClipTypes;
}

int SaveInfo_GetCharacterCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Characters;
}

int SaveInfo_GetDialogCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Dialogs;
}

int SaveInfo_GetGUICount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().GUIs;
}

int SaveInfo_GetGUIControlCount(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().GUIControls.size())
    {
        debug_script_error("RestoredSaveInfo::GUIControlCount: index %d out of bounds (%d..%d)", index, 0,
                           info->GetCounts().GUIs);
        return 0;
    }
    return info->GetCounts().GUIControls[index];
}

int SaveInfo_GetInventoryItemCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().InventoryItems;
}

int SaveInfo_GetCursorCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Cursors;
}

int SaveInfo_GetViewCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Views;
}

int SaveInfo_GetViewLoopCount(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ViewLoops.size())
    {
        debug_script_error("RestoredSaveInfo::ViewLoopCount: index %d out of bounds (%d..%d)", index, 0,
                           info->GetCounts().Views);
        return 0;
    }
    return info->GetCounts().ViewLoops[index];
}

int SaveInfo_GetViewFrameCount(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ViewFrames.size())
    {
        debug_script_error("RestoredSaveInfo::ViewFrameCount: index %d out of bounds (%d..%d)", index, 0,
                           info->GetCounts().Views);
        return 0;
    }
    return info->GetCounts().ViewFrames[index];
}

int SaveInfo_GetGlobalScriptDataSize(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().GlobalScriptDataSz;
}

int SaveInfo_GetScriptModuleCount(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().ScriptModules;
}

const char *SaveInfo_GetScriptModuleNames(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ScriptModuleDataSz.size())
    {
        debug_script_error("RestoredSaveInfo::ScriptModuleNames: index %d out of bounds (%d..%d)", index, 0,
                           info->GetCounts().ScriptModules);
        return nullptr;
    }
    return CreateNewScriptString(info->GetCounts().ScriptModuleNames[index]);
}

int SaveInfo_GetScriptModuleDataSizes(ScriptRestoredSaveInfo *info, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= info->GetCounts().ScriptModuleDataSz.size())
    {
        debug_script_error("RestoredSaveInfo::ScriptModuleDataSize: index %d out of bounds (%d..%d)", index, 0,
                           info->GetCounts().ScriptModules);
        return 0;
    }
    return info->GetCounts().ScriptModuleDataSz[index];
}

int SaveInfo_GetRoom(ScriptRestoredSaveInfo *info)
{
    return info->GetCounts().Room;
}
