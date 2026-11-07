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
#include "room_cache.h"

#include <algorithm>
#include <set>

#include "diagnostics.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

namespace Directory = AGS::Common::Directory;
namespace File = AGS::Common::File;
namespace Path = AGS::Common::Path;
using AGS::Common::Error;

namespace AGSBuild
{

// The first line of a manifest: a manifest written by another version of it never matches
static const char *kManifestHeader = "agsbuild room manifest 1\n";

uint64_t HashBytes(const void *data, size_t size, uint64_t hash)
{
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    for (size_t i = 0; i < size; ++i)
    {
        hash = (hash ^ bytes[i]) * 0x100000001b3ull;
    }
    return hash;
}

HError FileHasher::Hash(const String &path, uint64_t &hash)
{
    const auto found = _hashes.find(path);
    if (found != _hashes.end())
    {
        hash = found->second;
        return HError::None();
    }
    auto in = File::OpenFileRead(path);
    if (!in)
    {
        return new Error(String::FromFormat("Failed to open %s", path.GetCStr()));
    }
    hash = HashBytes(nullptr, 0);
    std::vector<uint8_t> buf(64 * 1024);
    for (size_t read = in->Read(buf.data(), buf.size()); read > 0; read = in->Read(buf.data(), buf.size()))
    {
        hash = HashBytes(buf.data(), read, hash);
    }
    if (in->GetError())
    {
        return new Error(String::FromFormat("Failed to read %s", path.GetCStr()));
    }
    _hashes[path] = hash;
    return HError::None();
}

static void AppendLine(String &manifest, const char *kind, uint64_t hash, const String &name)
{
    manifest.AppendFmt("%s %016llx %s\n", kind, static_cast<unsigned long long>(hash), name.GetCStr());
}

static HError AppendFile(String &manifest, FileHasher &hasher, const char *kind, const String &path, const String &name)
{
    uint64_t hash = 0;
    HError err = hasher.Hash(path, hash);
    if (!err)
    {
        return err;
    }
    AppendLine(manifest, kind, hash, name);
    return HError::None();
}

HError MakeRoomManifest(const RoomBuildInputs &inputs, FileHasher &hasher, String &manifest)
{
    manifest = kManifestHeader;
    for (const auto &tool : inputs.Tools)
    {
        HError err = AppendFile(manifest, hasher, "tool", tool, Path::GetFilename(tool));
        if (!err)
        {
            return err;
        }
    }
    HError err = AppendFile(manifest, hasher, "game", inputs.GameKeyFile, Path::GetFilename(inputs.GameKeyFile));
    if (!err)
    {
        return err;
    }
    for (const auto &command : inputs.Commands)
    {
        AppendLine(manifest, "command", HashBytes(command.GetCStr(), command.GetLength()), "");
    }
    for (const auto &header : inputs.Headers)
    {
        err = AppendFile(manifest, hasher, "header", header, Path::GetFilename(header));
        if (!err)
        {
            return err;
        }
    }
    // Every file of the room's folder, so that a file added there (an extra background) counts too.
    // The room build reads none of the folders in it.
    std::vector<String> files;
    Directory::GetFiles(inputs.SourceDir, files);
    std::sort(files.begin(), files.end());
    for (const auto &file : files)
    {
        err = AppendFile(manifest, hasher, "source", Path::ConcatPaths(inputs.SourceDir, file), file);
        if (!err)
        {
            return err;
        }
    }
    return HError::None();
}

// Deletes the file if it is there; false if it is still there
static bool DeleteIfExists(const String &file)
{
    if (File::IsFile(file))
    {
        // File::DeleteFile does not tell every failure on Windows: look again
        File::DeleteFile(file);
    }
    return !File::IsFile(file);
}

HError CheckRoomCache(RoomCacheEntry &entry, bool rebuild)
{
    entry.UpToDate = false;
    if (!rebuild && File::IsFile(entry.CrmFile))
    {
        auto in = File::OpenFileRead(entry.ManifestFile);
        if (in && String::FromStream(in.get(), SIZE_MAX) == entry.Manifest)
        {
            entry.UpToDate = true;
            return HError::None();
        }
    }
    for (const auto &file : {entry.CrmFile, entry.ManifestFile})
    {
        if (!DeleteIfExists(file))
        {
            return new Error(String::FromFormat("Failed to delete %s", file.GetCStr()));
        }
    }
    return HError::None();
}

HError WriteRoomManifest(const RoomCacheEntry &entry)
{
    auto out = File::CreateFile(entry.ManifestFile);
    if (!out)
    {
        return new Error(String::FromFormat("Failed to open %s for writing", entry.ManifestFile.GetCStr()));
    }
    out->Write(entry.Manifest.GetCStr(), entry.Manifest.GetLength());
    if (out->GetError())
    {
        out.reset();
        File::DeleteFile(entry.ManifestFile);
        return new Error(String::FromFormat("Failed to write %s", entry.ManifestFile.GetCStr()));
    }
    return HError::None();
}

// Whether the name is "room" and a number
static bool IsRoomEntryName(const String &name)
{
    if (name.GetLength() <= 4 || !name.StartsWith("room"))
    {
        return false;
    }
    for (size_t i = 4; i < name.GetLength(); ++i)
    {
        if (name[i] < '0' || name[i] > '9')
        {
            return false;
        }
    }
    return true;
}

HError ClearTempDirKeepingRooms(const String &dir)
{
    std::vector<String> dirs;
    Directory::GetDirs(dir, dirs);
    std::set<String> kept;
    for (const auto &name : dirs)
    {
        const String path = Path::ConcatPaths(dir, name);
        if (IsRoomEntryName(name) && File::IsFile(Path::ConcatPaths(path, kRoomManifestFileName)))
        {
            kept.insert(name);
            continue;
        }
        String failed_path;
        if (!Directory::DeleteDirectoryTree(path, &failed_path))
        {
            return new Error(String::FromFormat("could not delete '%s'", failed_path.GetCStr()));
        }
    }
    std::vector<String> files;
    Directory::GetFiles(dir, files);
    for (const auto &name : files)
    {
        if (name.EndsWith(".crm") && kept.count(name.Left(name.GetLength() - 4)) > 0)
        {
            continue;
        }
        const String path = Path::ConcatPaths(dir, name);
        if (!DeleteIfExists(path))
        {
            return new Error(String::FromFormat("could not delete '%s'", path.GetCStr()));
        }
    }
    return HError::None();
}

//-----------------------------------------------------------------------------
// Tasks
//-----------------------------------------------------------------------------

TaskCheckRooms::TaskCheckRooms(const String &name, ITaskManager *mgr,
                               const std::vector<std::shared_ptr<CachedRoom>> &rooms, bool rebuild,
                               const std::vector<TaskInput> &input_tasks)
    : Task(name, mgr, input_tasks),
      _rooms(rooms),
      _rebuild(rebuild)
{
    // The strings are used on the task's thread: none may share its buffer with a string of
    // another thread (String's reference count is not thread-safe)
    for (auto &room : _rooms)
    {
        RoomBuildInputs &in = room->Inputs;
        in.Tools = Clone(in.Tools);
        in.GameKeyFile = in.GameKeyFile.Clone();
        in.Commands = Clone(in.Commands);
        in.Headers = Clone(in.Headers);
        in.SourceDir = in.SourceDir.Clone();
        room->Entry.CrmFile = room->Entry.CrmFile.Clone();
        room->Entry.ManifestFile = room->Entry.ManifestFile.Clone();
    }
}

HError TaskCheckRooms::RunImpl()
{
    FileHasher hasher;
    size_t up_to_date = 0;
    for (auto &room : _rooms)
    {
        // An input that cannot be read is left for the tools to report, as they would without
        // the check: the room is built, and not recorded
        HError err = MakeRoomManifest(room->Inputs, hasher, room->Entry.Manifest);
        if (!err)
        {
            _logWriter->WriteFormat("%s\n", err->FullMessage().GetCStr());
            room->Entry.Manifest = "";
        }
        err = CheckRoomCache(room->Entry, _rebuild || room->Entry.Manifest.IsEmpty());
        if (!err)
        {
            return err;
        }
        if (room->Entry.UpToDate)
        {
            up_to_date++;
        }
        _logWriter->WriteFormat("%s: %s\n", Path::GetFilename(room->Entry.CrmFile).GetCStr(),
                                room->Entry.UpToDate ? "up to date" : "to build");
    }
    Diag::Log("Rooms up to date: %zu of %zu\n", up_to_date, _rooms.size());
    return HError::None();
}

HError TaskWriteRoomManifest::RunImpl()
{
    if (_room->Entry.UpToDate || _room->Entry.Manifest.IsEmpty())
    {
        return HError::None();
    }
    return WriteRoomManifest(_room->Entry);
}

} // namespace AGSBuild
