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
#include "build_cache.h"

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
static const char *kManifestHeader = "agsbuild manifest 2\n";
// The line after the inputs, before what the tool printed
static const char *kToolOutputLine = "output\n";
static const char *kScriptManifestExtension = ".manifest";

String MakeScriptManifestName(const String &script_obj)
{
    return String::FromFormat("%s/%s%s", kScriptManifestDir, script_obj.GetCStr(), kScriptManifestExtension);
}

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
    const std::string key = path.GetCStr();
    {
        std::lock_guard<std::mutex> lk(_mutex);
        const auto found = _hashes.find(key);
        if (found != _hashes.end())
        {
            hash = found->second;
            return HError::None();
        }
    }
    // Hashed without the lock: two threads may hash the same file at once, and get the same
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
    std::lock_guard<std::mutex> lk(_mutex);
    _hashes[key] = hash;
    return HError::None();
}

BuildInputs BuildInputs::Clone() const
{
    BuildInputs copy;
    copy.Tools = AGSBuild::Clone(Tools);
    copy.GameKeyFile = GameKeyFile.Clone();
    copy.Commands = AGSBuild::Clone(Commands);
    copy.Headers = AGSBuild::Clone(Headers);
    copy.Sources = AGSBuild::Clone(Sources);
    copy.SourceDir = SourceDir.Clone();
    return copy;
}

CacheEntry CacheEntry::Clone() const
{
    CacheEntry copy;
    copy.OutputFile = OutputFile.Clone();
    copy.ManifestFile = ManifestFile.Clone();
    copy.Manifest = Manifest.Clone();
    copy.UpToDate = UpToDate;
    copy.ToolOutput = ToolOutput.Clone();
    return copy;
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

static HError AppendFiles(String &manifest, FileHasher &hasher, const char *kind, const std::vector<String> &paths)
{
    for (const auto &path : paths)
    {
        HError err = AppendFile(manifest, hasher, kind, path, Path::GetFilename(path));
        if (!err)
        {
            return err;
        }
    }
    return HError::None();
}

HError MakeManifest(const BuildInputs &inputs, FileHasher &hasher, String &manifest)
{
    manifest = kManifestHeader;
    HError err = AppendFiles(manifest, hasher, "tool", inputs.Tools);
    if (!err)
    {
        return err;
    }
    if (!inputs.GameKeyFile.IsEmpty())
    {
        err = AppendFile(manifest, hasher, "game", inputs.GameKeyFile, Path::GetFilename(inputs.GameKeyFile));
        if (!err)
        {
            return err;
        }
    }
    for (const auto &command : inputs.Commands)
    {
        AppendLine(manifest, "command", HashBytes(command.GetCStr(), command.GetLength()), "");
    }
    err = AppendFiles(manifest, hasher, "header", inputs.Headers);
    if (!err)
    {
        return err;
    }
    err = AppendFiles(manifest, hasher, "source", inputs.Sources);
    if (!err || inputs.SourceDir.IsEmpty())
    {
        return err;
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

HError CheckCache(CacheEntry &entry, bool rebuild)
{
    entry.UpToDate = false;
    entry.ToolOutput = "";
    if (!rebuild && File::IsFile(entry.OutputFile))
    {
        auto in = File::OpenFileRead(entry.ManifestFile);
        const String recorded = in ? String::FromStream(in.get(), SIZE_MAX) : String();
        const String inputs = String::FromFormat("%s%s", entry.Manifest.GetCStr(), kToolOutputLine);
        if (recorded.StartsWith(inputs))
        {
            entry.UpToDate = true;
            entry.ToolOutput = recorded.Mid(inputs.GetLength());
            return HError::None();
        }
    }
    for (const auto &file : {entry.OutputFile, entry.ManifestFile})
    {
        if (!DeleteIfExists(file))
        {
            return new Error(String::FromFormat("Failed to delete %s", file.GetCStr()));
        }
    }
    return HError::None();
}

HError WriteManifest(const CacheEntry &entry, const String &tool_output)
{
    auto out = File::CreateFile(entry.ManifestFile);
    if (!out)
    {
        return new Error(String::FromFormat("Failed to open %s for writing", entry.ManifestFile.GetCStr()));
    }
    const String text = String::FromFormat("%s%s%s", entry.Manifest.GetCStr(), kToolOutputLine, tool_output.GetCStr());
    out->Write(text.GetCStr(), text.GetLength());
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

// Clears the folder of the scripts' manifests, but for those of the scripts in dir;
// adds the names of the scripts kept
static HError ClearScriptManifests(const String &dir, std::set<String> &kept_files)
{
    const String manifest_dir = Path::ConcatPaths(dir, kScriptManifestDir);
    std::vector<String> dirs;
    Directory::GetDirs(manifest_dir, dirs);
    for (const auto &name : dirs)
    {
        String failed_path;
        if (!Directory::DeleteDirectoryTree(Path::ConcatPaths(manifest_dir, name), &failed_path))
        {
            return new Error(String::FromFormat("could not delete '%s'", failed_path.GetCStr()));
        }
    }
    const String extension = kScriptManifestExtension;
    std::vector<String> files;
    Directory::GetFiles(manifest_dir, files);
    for (const auto &name : files)
    {
        const String script_obj =
            name.EndsWith(extension) ? name.Left(name.GetLength() - extension.GetLength()) : String();
        if (!script_obj.IsEmpty() && File::IsFile(Path::ConcatPaths(dir, script_obj)))
        {
            kept_files.insert(script_obj);
            continue;
        }
        const String path = Path::ConcatPaths(manifest_dir, name);
        if (!DeleteIfExists(path))
        {
            return new Error(String::FromFormat("could not delete '%s'", path.GetCStr()));
        }
    }
    return HError::None();
}

HError ClearTempDirKeepingCache(const String &dir)
{
    std::set<String> kept_files;
    HError err = ClearScriptManifests(dir, kept_files);
    if (!err)
    {
        return err;
    }
    std::vector<String> dirs;
    Directory::GetDirs(dir, dirs);
    for (const auto &name : dirs)
    {
        const String path = Path::ConcatPaths(dir, name);
        if (name == kScriptManifestDir)
        {
            continue;
        }
        if (IsRoomEntryName(name) && File::IsFile(Path::ConcatPaths(path, kRoomManifestFileName)))
        {
            kept_files.insert(String::FromFormat("%s.crm", name.GetCStr()));
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
        if (kept_files.count(name) > 0)
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
                               const std::vector<std::shared_ptr<CachedRoom>> &rooms,
                               const std::shared_ptr<FileHasher> &hasher, bool rebuild,
                               const std::vector<TaskInput> &input_tasks)
    : Task(name, mgr, input_tasks),
      _rooms(rooms),
      _hasher(hasher),
      _rebuild(rebuild)
{
    // The strings are used on the task's thread: none may share its buffer with a string of
    // another thread (String's reference count is not thread-safe)
    for (auto &room : _rooms)
    {
        room->Inputs = room->Inputs.Clone();
        room->Entry = room->Entry.Clone();
    }
}

HError TaskCheckRooms::RunImpl()
{
    size_t up_to_date = 0;
    for (auto &room : _rooms)
    {
        // An input that cannot be read is left for the tools to report, as they would without
        // the check: the room is built, and not recorded
        HError err = MakeManifest(room->Inputs, *_hasher, room->Entry.Manifest);
        if (!err)
        {
            _logWriter->WriteFormat("%s\n", err->FullMessage().GetCStr());
            room->Entry.Manifest = "";
        }
        err = CheckCache(room->Entry, _rebuild || room->Entry.Manifest.IsEmpty());
        if (!err)
        {
            return err;
        }
        if (room->Entry.UpToDate)
        {
            up_to_date++;
        }
        _logWriter->WriteFormat("%s: %s\n", Path::GetFilename(room->Entry.OutputFile).GetCStr(),
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
    return WriteManifest(_room->Entry);
}

TaskCompileScript::TaskCompileScript(const String &name, ITaskManager *mgr, const PipedProcessParams &params,
                                     const BuildInputs &inputs, const CacheEntry &entry,
                                     const std::shared_ptr<FileHasher> &hasher, bool rebuild,
                                     const std::vector<TaskInput> &input_tasks)
    : TaskPipedProcess(name, mgr, params, input_tasks),
      _inputs(inputs.Clone()),
      _entry(entry.Clone()),
      _hasher(hasher),
      _rebuild(rebuild)
{
}

HError TaskCompileScript::RunImpl()
{
    // An input that cannot be read is left for the compiler to report: the script is compiled,
    // and not recorded
    HError err = MakeManifest(_inputs, *_hasher, _entry.Manifest);
    if (!err)
    {
        _logWriter->WriteFormat("%s\n", err->FullMessage().GetCStr());
        _entry.Manifest = "";
    }
    err = CheckCache(_entry, _rebuild || _entry.Manifest.IsEmpty());
    if (!err)
    {
        return err;
    }
    if (_entry.UpToDate)
    {
        _logWriter->WriteFormat("Up to date\n");
        _logWriter->WriteString(_entry.ToolOutput);
        if (!_entry.ToolOutput.IsEmpty() && !_entry.ToolOutput.EndsWith("\n"))
        {
            _logWriter->WriteString("\n");
        }
        return HError::None();
    }
    size_t output_start = 0;
    err = RunProcess(&output_start);
    if (!err || _entry.Manifest.IsEmpty())
    {
        return err;
    }
    const String tool_output(reinterpret_cast<const char *>(_logBuffer.data() + output_start),
                             _logBuffer.size() - output_start);
    return WriteManifest(_entry, tool_output);
}

} // namespace AGSBuild
