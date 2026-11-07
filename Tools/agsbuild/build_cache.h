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
// Incremental builds: a room built from its sources (Rooms/<n>/) and a
// compiled script are kept in the intermediate folder with a manifest of
// everything their build read, and the next build keeps them while the
// manifest stays the same. Inputs are compared by their contents, never by
// file times.
//
// A room's entry in the intermediate folder: room<n>.crm, and the folder
// room<n>/ with the room's script header, its compiled script and manifest.txt.
// A script's entry: <script>.o, and scripts/<script>.o.manifest, which also
// holds what the compiler printed (its warnings), to be shown again by a
// build that keeps the script.
//
//=============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "taskmanager.h"
#include "util/error.h"
#include "util/string.h"

namespace AGSBuild
{

using String = AGS::Common::String;
using HError = AGS::Common::HError;

// The manifest's file name in the room's folder
constexpr const char *kRoomManifestFileName = "manifest.txt";
// The folder of the compiled scripts' manifests, in the intermediate folder
constexpr const char *kScriptManifestDir = "scripts";

// The manifest of a compiled script (scripts/<script>.o.manifest), relative to the intermediate folder
String MakeScriptManifestName(const String &script_obj);

// 64-bit FNV-1a: not for security; it tells files apart, and is the same on every platform
uint64_t HashBytes(const void *data, size_t size, uint64_t hash = 0xcbf29ce484222325ull);

// Hashes files by their contents, each once; may be shared by tasks on several threads
class FileHasher
{
public:
    // The hash of the file's contents; an error if it cannot be read
    HError Hash(const String &path, uint64_t &hash);

private:
    std::mutex _mutex;
    std::map<std::string, uint64_t> _hashes;
};

// What a build step reads
struct BuildInputs
{
    // The programs that make the output (agsbuild itself, agscc, crmbuild)
    std::vector<String> Tools;
    // For a room: what crmbuild --game-key wrote, the settings that building a room reads from
    // Game.agf; empty if none
    String GameKeyFile;
    // The command lines that make the output, without what does not change the result (-v)
    std::vector<String> Commands;
    // Every header the script is compiled with, in the order given to the compiler
    std::vector<String> Headers;
    // Source files, each an input by its name
    std::vector<String> Sources;
    // A folder of which every file is an input (a room's Rooms/<n>/); empty if none
    String SourceDir;

    // A copy that shares no string buffer with this one, for another thread (String's reference
    // count is not thread-safe)
    BuildInputs Clone() const;
};

// Makes the text of a manifest: a line for each input, with the hash of its contents
HError MakeManifest(const BuildInputs &inputs, FileHasher &hasher, String &manifest);

// An entry in the intermediate folder: the output of a build step, and its manifest
struct CacheEntry
{
    String OutputFile;
    String ManifestFile;
    // The manifest of this build's inputs
    String Manifest;
    // Whether the output is kept: set by CheckCache
    bool UpToDate = false;
    // What the tool printed when it made the kept output, as recorded with the manifest
    String ToolOutput;

    // A copy that shares no string buffer with this one
    CacheEntry Clone() const;
};

// Decides whether the output built earlier is up to date: its file exists and its manifest is
// the one given. If not (or if rebuild), deletes the output and the manifest, so that a build
// which fails later leaves nothing that a next build would take as up to date.
HError CheckCache(CacheEntry &entry, bool rebuild);

// Records the output as made from the manifest's inputs, with what the tool printed
HError WriteManifest(const CacheEntry &entry, const String &tool_output = String());

// Clears the intermediate folder for a build, but for what earlier builds left to keep: the
// folders room<n> that have a manifest and the room<n>.crm next to them, and the files that
// have a manifest in scripts/ (the compiled scripts) with their manifests
HError ClearTempDirKeepingCache(const String &dir);

// A room of the build, which the tasks that check, build and record it share
struct CachedRoom
{
    BuildInputs Inputs;
    CacheEntry Entry;
};

// Decides for each room whether it is up to date (CheckCache); runs after the tasks that make
// the generated headers. The tasks that build a room skip their work when it is.
class TaskCheckRooms : public Task
{
public:
    TaskCheckRooms(const String &name, ITaskManager *mgr, const std::vector<std::shared_ptr<CachedRoom>> &rooms,
                   const std::shared_ptr<FileHasher> &hasher, bool rebuild, const std::vector<TaskInput> &input_tasks);

private:
    HError RunImpl() override;

    std::vector<std::shared_ptr<CachedRoom>> _rooms;
    std::shared_ptr<FileHasher> _hasher;
    const bool _rebuild;
};

// Writes the manifest of a room that was built, after it was
class TaskWriteRoomManifest : public Task
{
public:
    TaskWriteRoomManifest(const String &name, ITaskManager *mgr, const std::shared_ptr<CachedRoom> &room,
                          const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks),
          _room(room)
    {
    }

private:
    HError RunImpl() override;

    std::shared_ptr<CachedRoom> _room;
};

// Compiles a script unless the compiled script kept from an earlier build is up to date: then it
// prints what the compiler printed then, so that its warnings are reported again. Runs after the
// tasks that make the generated script and headers, which are inputs. A script that is compiled
// is recorded after the compiler succeeds; a stale one is deleted before it runs.
class TaskCompileScript : public TaskPipedProcess
{
public:
    TaskCompileScript(const String &name, ITaskManager *mgr, const PipedProcessParams &params,
                      const BuildInputs &inputs, const CacheEntry &entry, const std::shared_ptr<FileHasher> &hasher,
                      bool rebuild, const std::vector<TaskInput> &input_tasks);

private:
    HError RunImpl() override;

    const BuildInputs _inputs;
    CacheEntry _entry;
    std::shared_ptr<FileHasher> _hasher;
    const bool _rebuild;
};

} // namespace AGSBuild
