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
#include "package_files.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <set>
#include <string>

#include "data/multifilelib.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

namespace AGSBuild
{

using AGS::Common::AssetLibInfo;
using AGS::Common::Error;
using AGS::Common::FindFile;
using AGS::Common::Stream;
namespace Directory = AGS::Common::Directory;
namespace File = AGS::Common::File;
namespace MFLUtil = AGS::Common::MFLUtil;
namespace Path = AGS::Common::Path;

namespace
{

std::string LowerName(const String &name)
{
    return name.LowerUTF8().GetCStr();
}

bool EndsWith(const std::string &s, const char *suffix)
{
    const size_t len = strlen(suffix);
    return s.size() >= len && s.compare(s.size() - len, len, suffix) == 0;
}

// flic*.fl? : "flic", anything, ".fl", then one character or none (a '?' at the end of a Windows
// wildcard matches no character too)
bool IsFlicFileName(const std::string &lower)
{
    const size_t prefix_len = 4, ext_len = 3;
    if (lower.compare(0, prefix_len, "flic") != 0)
    {
        return false;
    }
    // The name ends with ".fl", or with ".fl" and one (UTF-8) character: where its last character begins
    size_t last_char = lower.size() - 1;
    while (last_char > 0 && (static_cast<unsigned char>(lower[last_char]) & 0xC0) == 0x80)
    {
        --last_char;
    }
    for (const size_t ext_end : {lower.size(), last_char})
    {
        if (ext_end >= prefix_len + ext_len && lower.compare(ext_end - ext_len, ext_len, ".fl") == 0)
        {
            return true;
        }
    }
    return false;
}

bool IsSeparator(char c)
{
    return c == '/' || c == '\\';
}

// The names between the separators, the empty ones too
std::vector<std::string> SplitPath(const String &path)
{
    std::vector<std::string> parts(1);
    for (const char *ptr = path.GetCStr(); *ptr; ++ptr)
    {
        if (IsSeparator(*ptr))
        {
            parts.emplace_back();
        }
        else
        {
            parts.back().push_back(*ptr);
        }
    }
    return parts;
}

// As .NET's Path.IsPathRooted: starts with a separator, or has a drive
bool IsRootedPath(const String &path)
{
    return IsSeparator(path.GetAt(0)) || (path.GetLength() >= 2 && path.GetAt(1) == ':');
}

bool IsSameOrNested(const std::vector<std::string> &path, const std::vector<std::string> &base)
{
    if (base.size() > path.size())
    {
        return false;
    }
    for (size_t i = 0; i < base.size(); ++i)
    {
        if (String::Wrapper(path[i].c_str()).CompareUtf8NoCase(base[i].c_str()) != 0)
        {
            return false;
        }
    }
    return true;
}

bool IsSameOrNestedAmong(const std::vector<std::string> &path, const std::vector<std::vector<std::string>> &bases)
{
    return std::any_of(bases.begin(), bases.end(),
                       [&](const std::vector<std::string> &base) { return IsSameOrNested(path, base); });
}

// The first-level weight of a character in the editor's sort: 0 for the hyphen and the apostrophe,
// which count only in the end; letters have the weight of their lower case
int SortWeight(unsigned char c)
{
    // ASCII as the Windows word sort orders it; then the digits and the letters
    static const char order[] = " !\"#$%&()*,./:;?@[\\]^_`{|}~+<=>0123456789abcdefghijklmnopqrstuvwxyz";
    if (c == '-' || c == '\'')
    {
        return 0;
    }
    if (c >= 'A' && c <= 'Z')
    {
        c = static_cast<unsigned char>(c - 'A' + 'a');
    }
    const char *at = c ? strchr(order, c) : nullptr;
    if (at)
    {
        return static_cast<int>(at - order) + 1;
    }
    return 256 + c; // control characters and bytes outside ASCII: after everything, by value
}

bool IsUpper(unsigned char c)
{
    return c >= 'A' && c <= 'Z';
}

// Moves a file; copies it and deletes the source if it cannot be moved (another volume)
bool MoveOrCopyFile(const String &src, const String &dst)
{
    if (File::RenameFile(src, dst))
    {
        return true;
    }
    if (!File::CopyFile(src, dst, true))
    {
        return false;
    }
    File::DeleteFile(src);
    return true;
}

} // namespace

//-----------------------------------------------------------------------------
// Files of the project folder
//-----------------------------------------------------------------------------

std::vector<String> SelectPackedProjectFiles(const std::vector<String> &project_files)
{
    std::vector<String> packed;
    for (const String &file : project_files)
    {
        const std::string lower = LowerName(file);
        if (lower == "preload.pcx" || IsFlicFileName(lower) || EndsWith(lower, ".ogv"))
        {
            packed.push_back(file);
        }
    }
    std::sort(packed.begin(), packed.end());
    return packed;
}

//-----------------------------------------------------------------------------
// Custom data folders
//-----------------------------------------------------------------------------

int CompareAsEditorSorts(const String &s1, const String &s2)
{
    const unsigned char *p1 = reinterpret_cast<const unsigned char *>(s1.GetCStr());
    const unsigned char *p2 = reinterpret_cast<const unsigned char *>(s2.GetCStr());
    int case_order = 0;    // the first difference in letter case: lower case first
    int special_order = 0; // the first hyphen or apostrophe which only one of them has: the other first
    for (;;)
    {
        // A hyphen or apostrophe does not take part in the first comparison
        if (*p1 && SortWeight(*p1) == 0 && *p1 != *p2)
        {
            if (special_order == 0)
            {
                special_order = 1;
            }
            ++p1;
            continue;
        }
        if (*p2 && SortWeight(*p2) == 0 && *p1 != *p2)
        {
            if (special_order == 0)
            {
                special_order = -1;
            }
            ++p2;
            continue;
        }
        if (!*p1 || !*p2)
        {
            break;
        }
        const int w1 = SortWeight(*p1), w2 = SortWeight(*p2);
        if (w1 != w2)
        {
            return w1 < w2 ? -1 : 1;
        }
        if (case_order == 0 && IsUpper(*p1) != IsUpper(*p2))
        {
            case_order = IsUpper(*p1) ? 1 : -1;
        }
        ++p1;
        ++p2;
    }
    if (*p1 || *p2)
    {
        return *p1 ? 1 : -1; // the shorter one first
    }
    return case_order != 0 ? case_order : special_order;
}

CustomDataDirs SelectCustomDataDirs(const std::vector<String> &setting_dirs)
{
    // The folders that the build reads or writes itself (AGSEditor.RestrictedGameDirectories)
    static const std::vector<std::vector<std::string>> restricted_dirs = {
        {"Compiled"}, {"_Debug"}, {"AudioCache"}, {"Speech"}};

    std::vector<String> sorted_dirs = setting_dirs;
    std::stable_sort(sorted_dirs.begin(), sorted_dirs.end(),
                     [](const String &d1, const String &d2) { return CompareAsEditorSorts(d1, d2) < 0; });

    CustomDataDirs result;
    std::vector<std::vector<std::string>> used_dirs;
    for (const String &dir : sorted_dirs)
    {
        if (dir.IsEmpty())
        {
            continue;
        }
        if (IsRootedPath(dir))
        {
            result.Warnings.push_back(
                String::FromFormat("Cannot use absolute path as a custom data source: %s", dir.GetCStr()));
            continue;
        }
        std::vector<std::string> names = SplitPath(dir);
        if (std::any_of(names.begin(), names.end(), [](const std::string &n) { return n == "." || n == ".."; }))
        {
            result.Warnings.push_back(String::FromFormat(
                "Cannot use path containing '.' or '..' as a custom data source: %s", dir.GetCStr()));
            continue;
        }
        // "Folder//Sub/" is "Folder/Sub"
        names.erase(std::remove_if(names.begin(), names.end(), [](const std::string &n) { return n.empty(); }),
                    names.end());
        if (names.empty())
        {
            continue;
        }
        if (IsSameOrNestedAmong(names, restricted_dirs))
        {
            result.Warnings.push_back(
                String::FromFormat("Cannot use restricted location as a custom data source: %s", dir.GetCStr()));
            continue;
        }
        if (IsSameOrNestedAmong(names, used_dirs))
        {
            continue; // its files are packed already
        }
        String clean_dir;
        for (const std::string &name : names)
        {
            clean_dir.AppendFmt(clean_dir.IsEmpty() ? "%s" : "/%s", name.c_str());
        }
        result.Dirs.push_back(clean_dir);
        used_dirs.push_back(std::move(names));
    }
    return result;
}

//-----------------------------------------------------------------------------
// Voice packs
//-----------------------------------------------------------------------------

bool IsVoicePackFile(const String &filename)
{
    const std::string lower = LowerName(filename);
    return lower == "syncdata.dat" || EndsWith(lower, ".mp3") || EndsWith(lower, ".ogg") || EndsWith(lower, ".flac") ||
           EndsWith(lower, ".wav");
}

String GetVoicePackName(const String &sub_dir)
{
    if (sub_dir.IsEmpty())
    {
        return "speech.vox";
    }
    return String::FromFormat("sp_%s.vox", sub_dir.LowerUTF8().GetCStr());
}

std::vector<VoicePack> PlanVoicePacks(const std::vector<String> &sub_dirs, std::vector<String> &warnings)
{
    std::vector<String> sorted_dirs = sub_dirs;
    std::sort(sorted_dirs.begin(), sorted_dirs.end());

    std::vector<VoicePack> packs;
    packs.push_back({GetVoicePackName(""), ""});
    for (const String &dir : sorted_dirs)
    {
        if (dir.IsEmpty())
        {
            continue;
        }
        const String pack_name = GetVoicePackName(dir);
        const auto same_pack =
            std::find_if(packs.begin(), packs.end(), [&](const VoicePack &pack) { return pack.PackName == pack_name; });
        if (same_pack != packs.end())
        {
            warnings.push_back(
                String::FromFormat("Speech folder '%s' is not packed: folder '%s' makes the voice pack %s",
                                   dir.GetCStr(), same_pack->SubDir.GetCStr(), pack_name.GetCStr()));
            continue;
        }
        packs.push_back({pack_name, dir});
    }
    return packs;
}

String GetVoiceAssetName(const VoicePack &pack, const String &filename)
{
    if (pack.SubDir.IsEmpty())
    {
        return filename;
    }
    return String::FromFormat("%s/%s", pack.SubDir.GetCStr(), filename.GetCStr());
}

bool IsVoicePackName(const String &filename)
{
    const std::string lower = LowerName(filename);
    const size_t prefix_len = 3, ext_len = 4; // "sp_", ".vox"
    return lower == "speech.vox" ||
           (lower.size() > prefix_len + ext_len && lower.compare(0, prefix_len, "sp_") == 0 && EndsWith(lower, ".vox"));
}

std::vector<String> FindStaleVoicePacks(const std::vector<String> &made_packs, const std::vector<String> &dir_files)
{
    std::vector<String> stale;
    for (const String &file : dir_files)
    {
        if (!IsVoicePackName(file))
        {
            continue;
        }
        if (std::none_of(made_packs.begin(), made_packs.end(),
                         [&](const String &pack) { return pack.CompareNoCase(file) == 0; }))
        {
            stale.push_back(file);
        }
    }
    std::sort(stale.begin(), stale.end());
    return stale;
}

//-----------------------------------------------------------------------------
// Patterns and package parts
//-----------------------------------------------------------------------------

String MakeIncludePattern(const String &filename)
{
    // The leading slash anchors the pattern to the input folder, and keeps a name which begins with
    // '#' or '!' from being read as a comment or an exclusion
    String pattern = "/";
    for (const char *ptr = filename.GetCStr(); *ptr; ++ptr)
    {
        if (*ptr == '[' || *ptr == '*' || *ptr == '?')
        {
            pattern.AppendFmt("[%c]", *ptr); // a set of one character stands for the character
        }
        else
        {
            pattern.AppendChar(*ptr);
        }
    }
    return pattern;
}

bool IsPackagePartName(const String &package_name, const String &filename)
{
    const size_t digits = 3;
    const String base = Path::RemoveExtension(package_name);
    if (filename.GetLength() != base.GetLength() + 1 + digits || filename.CompareLeftNoCase(base) != 0 ||
        filename.GetAt(base.GetLength()) != '.')
    {
        return false;
    }
    for (size_t i = filename.GetLength() - digits; i < filename.GetLength(); ++i)
    {
        if (filename.GetAt(i) < '0' || filename.GetAt(i) > '9')
        {
            return false;
        }
    }
    return true;
}

std::vector<String> FindStalePackageParts(const String &package_name, const std::vector<String> &new_parts,
                                          const std::vector<String> &dir_files)
{
    std::vector<String> stale;
    for (const String &file : dir_files)
    {
        if (!IsPackagePartName(package_name, file))
        {
            continue;
        }
        if (std::none_of(new_parts.begin(), new_parts.end(),
                         [&](const String &part) { return part.CompareNoCase(file) == 0; }))
        {
            stale.push_back(file);
        }
    }
    std::sort(stale.begin(), stale.end());
    return stale;
}

HError ReadPackagePartNames(const String &package_file, std::vector<String> &parts)
{
    std::unique_ptr<Stream> in(File::OpenFileRead(package_file));
    if (!in)
    {
        return new Error("Failed to open package %s", package_file.GetCStr());
    }
    AssetLibInfo lib;
    const MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(lib, in.get());
    if (mfl_err != MFLUtil::kMFLNoError)
    {
        return new Error("Failed to read package %s: %s", package_file.GetCStr(),
                         MFLUtil::GetMFLErrorText(mfl_err).GetCStr());
    }
    // The first part is the file itself, under the name it has on disk
    parts.push_back(Path::GetFilename(package_file));
    for (size_t i = 1; i < lib.LibFileNames.size(); ++i)
    {
        const String &name = lib.LibFileNames[i];
        // The parts lie next to the first one; they are moved and deleted by these names
        if (name.IsEmpty() || name.FindAnyChar("/\\") != String::NoIndex)
        {
            return new Error("Package %s has a part with an unusable name: '%s'", package_file.GetCStr(),
                             name.GetCStr());
        }
        parts.push_back(name);
    }
    return HError::None();
}

//-----------------------------------------------------------------------------
// TaskStageFiles
//-----------------------------------------------------------------------------

HError TaskStageFiles::RunImpl()
{
    // Everything in the folder is packed, so what an earlier build left has to go first
    std::vector<String> old_files;
    for (FindFile ff = FindFile::OpenFilesRecursive(_stageDir); !ff.AtEnd(); ff.Next())
    {
        old_files.push_back(ff.Current());
    }
    for (const String &file : old_files)
    {
        const String path = Path::ConcatPaths(_stageDir, file);
        if (!File::DeleteFile(path) || File::IsFile(path))
        {
            return new Error("Failed to delete the old file %s", path.GetCStr());
        }
    }

    size_t copied = 0;
    std::set<String> made_dirs;
    for (const auto &item : _files)
    {
        if (IsAborted())
        {
            return new Error("Cancelled");
        }
        const String parent = Path::GetParent(item.second);
        if (parent != "." && made_dirs.insert(parent).second && !Directory::CreateAllDirectories(_stageDir, parent))
        {
            return new Error("Failed to create the folder %s in %s", parent.GetCStr(), _stageDir.GetCStr());
        }
        const String dst = Path::ConcatPaths(_stageDir, item.second);
        if (File::LinkFile(item.first, dst, true))
        {
            continue;
        }
        if (!File::CopyFile(item.first, dst, true))
        {
            return new Error("Failed to link or copy file %s to %s", item.first.GetCStr(), dst.GetCStr());
        }
        copied++;
    }
    _logWriter->WriteFormat("Staged %zu file(s) in %s: %zu linked, %zu copied; %zu old file(s) removed\n",
                            _files.size(), _stageDir.GetCStr(), _files.size() - copied, copied, old_files.size());
    return HError::None();
}

//-----------------------------------------------------------------------------
// TaskDeployPackage
//-----------------------------------------------------------------------------

HError TaskDeployPackage::RunImpl()
{
    std::vector<String> parts;
    HError err = ReadPackagePartNames(Path::ConcatPaths(_srcDir, _packageName), parts);
    if (!err)
    {
        return err;
    }

    for (const String &part : parts)
    {
        const String src = Path::ConcatPaths(_srcDir, part), dst = Path::ConcatPaths(_outDir, part);
        if (!File::IsFile(src))
        {
            return new Error("Package part %s does not exist", src.GetCStr());
        }
        if (!MoveOrCopyFile(src, dst))
        {
            return new Error("Failed to move file %s to %s", src.GetCStr(), dst.GetCStr());
        }
        _logWriter->WriteFormat("+ Move: %s -> %s\n", src.GetCStr(), dst.GetCStr());
    }

    // An earlier build with more parts: the engine would not read them, but they do not belong
    // to the game any more
    std::vector<String> out_files;
    Directory::GetFiles(_outDir, out_files);
    for (const String &stale : FindStalePackageParts(_packageName, parts, out_files))
    {
        const String path = Path::ConcatPaths(_outDir, stale);
        if (!File::DeleteFile(path) || File::IsFile(path))
        {
            return new Error("Failed to delete the old package part %s", path.GetCStr());
        }
        _logWriter->WriteFormat("- Delete: %s\n", path.GetCStr());
    }
    return HError::None();
}

} // namespace AGSBuild
