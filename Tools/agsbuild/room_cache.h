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
// Incremental room builds: a room built from its sources (Rooms/<n>/) is kept
// in the intermediate folder with a manifest of everything its build read, and
// the next build keeps it while the manifest stays the same. Inputs are
// compared by their contents, never by file times.
//
// A room's entry in the intermediate folder: room<n>.crm, and the folder
// room<n>/ with the room's script header, its compiled script and manifest.txt.
//
//=============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
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

// 64-bit FNV-1a: not for security; it tells files apart, and is the same on every platform
uint64_t HashBytes(const void *data, size_t size, uint64_t hash = 0xcbf29ce484222325ull);

// Hashes files by their contents, each once
class FileHasher
{
public:
    // The hash of the file's contents; an error if it cannot be read
    HError Hash(const String &path, uint64_t &hash);

private:
    std::map<String, uint64_t> _hashes;
};

// What a room's build reads, besides its own sources
struct RoomBuildInputs
{
    // The programs that build the room (agsbuild itself, agscc, crmbuild)
    std::vector<String> Tools;
    // What crmbuild --game-key wrote: the settings that building a room reads from Game.agf
    String GameKeyFile;
    // The command lines that build the room, without what does not change the result (-v)
    std::vector<String> Commands;
    // Every header the room script is compiled with, in the order given to the compiler; the
    // room's own header is made from the first one listed here and the room's sources
    std::vector<String> Headers;
    // Rooms/<n>/: every file in it is an input (the room data, images, script)
    String SourceDir;
};

// Makes the text of the room's manifest: a line for each input, with the hash of its contents
HError MakeRoomManifest(const RoomBuildInputs &inputs, FileHasher &hasher, String &manifest);

// A room's entry in the intermediate folder
struct RoomCacheEntry
{
    String CrmFile;
    String ManifestFile;
    // The manifest of this build's inputs
    String Manifest;
    // Whether the built room is kept: set by CheckRoomCache
    bool UpToDate = false;
};

// Decides whether the room built earlier is up to date: its file exists and its manifest is
// the one given. If not (or if rebuild), deletes the room file and the manifest, so that a
// build which fails later leaves nothing that a next build would take as up to date.
HError CheckRoomCache(RoomCacheEntry &entry, bool rebuild);

// Records the room as built from the manifest's inputs
HError WriteRoomManifest(const RoomCacheEntry &entry);

// Clears the intermediate folder for a build, but for the rooms kept from earlier builds: the
// folders room<n> that have a manifest, and the room<n>.crm next to them
HError ClearTempDirKeepingRooms(const String &dir);

// A room of the build, which the tasks that check, build and record it share
struct CachedRoom
{
    RoomBuildInputs Inputs;
    RoomCacheEntry Entry;
};

// Decides for each room whether it is up to date (CheckRoomCache); runs after the tasks that make
// the generated headers. The tasks that build a room skip their work when it is.
class TaskCheckRooms : public Task
{
public:
    TaskCheckRooms(const String &name, ITaskManager *mgr, const std::vector<std::shared_ptr<CachedRoom>> &rooms,
                   bool rebuild, const std::vector<TaskInput> &input_tasks);

private:
    HError RunImpl() override;

    std::vector<std::shared_ptr<CachedRoom>> _rooms;
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

} // namespace AGSBuild
