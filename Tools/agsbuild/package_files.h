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
// Which files of a game project go in its packages, and under which names:
// the rules of the editor's build (BuildTargetDataFile.cs, SpeechComponent.cs),
// and the tasks which stage files for agspak and deploy a package in parts.
//
//=============================================================================
#pragma once

#include <utility>
#include <vector>

#include "taskmanager.h"
#include "util/error.h"
#include "util/string.h"

namespace AGSBuild
{

//-----------------------------------------------------------------------------
// Files of the project folder
//-----------------------------------------------------------------------------

// The files in the project folder that go in the main package because of their names:
// preload.pcx, flic*.fl? and *.ogv, in any letter case. Returns them sorted.
// The editor finds them with Windows wildcards, which also match a file's short (8.3) name;
// that is not followed here.
std::vector<String> SelectPackedProjectFiles(const std::vector<String> &project_files);

//-----------------------------------------------------------------------------
// Custom data folders (Settings.CustomDataDir)
//-----------------------------------------------------------------------------

struct CustomDataDirs
{
    // The folders to pack with everything in them, relative to the project folder, with '/'
    // between the names; in the order they are packed
    std::vector<String> Dirs;
    // What to tell the user about the rejected ones
    std::vector<String> Warnings;
};

// Compares two strings as the editor's Array.Sort does (a culture-sensitive comparison) for names
// in ASCII: punctuation, then digits, then letters; the letter case, then hyphens and apostrophes
// decide only between names that are otherwise equal. Bytes outside ASCII come after the letters.
// Returns a negative number, zero or a positive number.
int CompareAsEditorSorts(const String &s1, const String &s2);

// Chooses the custom data folders from the entries of the game setting, as the editor does:
// in sorted order; no absolute path, no "." or ".." in a path, nothing in a folder that the build
// uses itself (Compiled, _Debug, AudioCache, Speech), each of these with a warning; a folder
// which is, or is inside, one already chosen is left out without a word. Names are compared
// without case. An empty entry is skipped (the editor fails on it).
CustomDataDirs SelectCustomDataDirs(const std::vector<String> &setting_dirs);

//-----------------------------------------------------------------------------
// Voice packs (the Speech folder)
//-----------------------------------------------------------------------------

struct VoicePack
{
    // File name of the pack: speech.vox, or sp_<folder name in lower case>.vox
    String PackName;
    // The folder inside Speech that the pack is made of; empty for Speech itself
    String SubDir;
};

// Tells if a file of a Speech folder goes in the folder's voice pack:
// syncdata.dat, *.mp3, *.ogg, *.flac, *.wav, in any letter case
bool IsVoicePackFile(const String &filename);
// The file name of the voice pack made of a folder inside Speech, or of Speech itself (empty name)
String GetVoicePackName(const String &sub_dir);
// The voice packs of a project: speech.vox, then one for each folder inside Speech, sorted by
// name. Two folders whose names differ only by case would give the same pack: the second one is
// left out with a warning.
std::vector<VoicePack> PlanVoicePacks(const std::vector<String> &sub_dirs, std::vector<String> &warnings);
// The name of a file in a voice pack: the path inside Speech (French/ego1.ogg), by which the
// engine looks it up
String GetVoiceAssetName(const VoicePack &pack, const String &filename);
// Tells if this is the file name of a voice pack: speech.vox or sp_<name>.vox, in any letter case
bool IsVoicePackName(const String &filename);
// The voice packs of an earlier build among the files of the output folder, which this build does
// not make (made_packs: the file names of the packs it makes)
std::vector<String> FindStaleVoicePacks(const std::vector<String> &made_packs, const std::vector<String> &dir_files);

//-----------------------------------------------------------------------------
// Patterns and package parts
//-----------------------------------------------------------------------------

// A line for agspak's pattern file (-f) which matches this file of the input folder and no other:
// anchored to the folder, with the wildcard characters escaped
String MakeIncludePattern(const String &filename);

// Tells if this is the name of a part of the package after the first: <package name without
// extension>.NNN, as agspak names them (game.001 for game.ags)
bool IsPackagePartName(const String &package_name, const String &filename);
// The parts of an earlier build among the files of the output folder, which the new package
// (new_parts: its file names, the package itself first) does not have
std::vector<String> FindStalePackageParts(const String &package_name, const std::vector<String> &new_parts,
                                          const std::vector<String> &dir_files);
// Reads the file names of a package's parts from its first file; the package's own name comes first
HError ReadPackagePartNames(const String &package_file, std::vector<String> &parts);

//-----------------------------------------------------------------------------
// Tasks
//-----------------------------------------------------------------------------

// Makes a folder hold exactly the given files, so that agspak can pack everything in it under
// the files' paths in the folder: deletes what the folder has, then links (or copies) the files
// into it. The folder itself must exist.
class TaskStageFiles : public Task
{
public:
    // files: source file path, and the path inside the folder
    TaskStageFiles(const String &name, ITaskManager *mgr, const String &stage_dir,
                   const std::vector<std::pair<String, String>> &files, const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks),
          _stageDir(stage_dir.Clone()),
          _files(Clone(files))
    {
    }

private:
    HError RunImpl() override;

    String _stageDir;
    std::vector<std::pair<String, String>> _files;
};

// Moves a package that agspak wrote, with all of its parts, to the output folder, and deletes
// from there the parts of an earlier build which the package no longer has. The number of parts
// is known only after agspak has run, so it is read from the package.
class TaskDeployPackage : public Task
{
public:
    TaskDeployPackage(const String &name, ITaskManager *mgr, const String &src_dir, const String &out_dir,
                      const String &package_name, const std::vector<TaskInput> &input_tasks)
        : Task(name, mgr, input_tasks),
          _srcDir(src_dir.Clone()),
          _outDir(out_dir.Clone()),
          _packageName(package_name.Clone())
    {
    }

private:
    HError RunImpl() override;

    String _srcDir;
    String _outDir;
    String _packageName;
};

} // namespace AGSBuild
