#!/usr/bin/env python3
"""
Applies a file move map (docs/refactor/moves-5.3.tsv, PLAN.md 5.3): moves the files with `git mv`, rewrites every
#include of a moved file, updates every build description that names one, rewrites the scripts that name one by a
plain path, and lists the documents and code comments that a human has to look at.

    python Script/apply_moves.py --map docs/refactor/moves-5.3.tsv [--repo <dir>] [--only <new-path prefix> ...]
                                 [--exact] [--dry-run] [--check-coverage] [--no-build-lists] [--no-format]
                                 [--verbose]

The map: one row per file, `old path<TAB>new path<TAB>note`, paths relative to the repository root, `#` comments.
--only selects the rows whose new path starts with one of the prefixes (a folder: `Engine/game/navigation` takes
`Engine/game/navigation/**`); with --exact a prefix is a folder and only the files directly in it are taken (the
one-commit-per-destination-folder runs of `Engine/game` itself). Without --only every row is selected.

1. Validation, before anything is touched (a failure is listed under "stops" and nothing is changed):
   - a row whose old path exists is pending; a row whose new path exists (and old one does not) was applied and is
     skipped, so a second run, or a run after a partial one, is fine; both existing is an error; neither existing is
     reported and skipped when the note says the file was folded away earlier (FOLDED_NOTE), an error otherwise;
   - no two rows share an old or a new path;
   - every .cpp stem stays unique inside each .vcxproj that compiles a moved file (MSBuild object names are flat;
     an item with its own <ObjectFileName> is left out of the check);
   - a moved file's path below its module root (`game/view_def.h` for `Common/game/view_def.h`) is not also a file under
     the other module roots (MODULE_ROOTS), so a root-relative #include stays unambiguous whatever the -I order;
   - --check-coverage: every tracked file under COVERAGE_DIRS has a row (NO_ROW lists the deliberate exceptions).
2. `git mv` of every pending selected row (destination folders are created; source folders left empty are removed).
3. Includes. Every `#include` line (`^\\s*#\\s*include`, quoted or angled, inside any #if) of every tracked source
   file (SOURCE_EXTS) is resolved the way the compilers do: the includer's own folder first (quoted only), then
   INCLUDE_ROOTS in order. One that resolves to a moved file, or a quoted one that resolved through the includer's
   own folder when the includer moves (the bare sibling includes), is rewritten root-relative (`"game/ui/button.h"`);
   a `../`-relative one (the .rc files) stays relative to the includer, and the same string elsewhere in an .rc
   file (its TEXTINCLUDE copy) is rewritten too. Then every include line of every source is resolved again in the
   moved tree and must reach the same file it reached before: an include that the move makes ambiguous or
   unresolvable, a third-party file (THIRD_PARTY) that includes a moved one, or a first-party quoted include under
   a module folder that does not resolve, stops the run. The include blocks that changed are re-sorted with the
   pinned clang-format (AGS_CLANG_FORMAT, 19.1.7; `--lines` on those blocks only, so the rest of the file is not
   touched) and the result is checked: the file without its #include lines (blank lines collapsed) is unchanged and
   the include lines are the rewritten ones; a file that is not format-clean afterwards is reported. A renamed
   file whose includes do not change is re-sorted the same way, all its include blocks, when it is format-clean
   under its old name and not under its new one: clang-format puts the main header first by the file's name, so
   `gui/guiinv.h` is main for guiinv.cpp and an ordinary include for gui_inv.cpp.
4. Build descriptions (tracked first-party CMakeLists.txt and *.cmake, every .vcxproj and .vcxproj.filters, every
   project.pbxproj), each read with its line ends and BOM kept:
   - CMake: every token that names a source file (relative to the file's folder or to a ${...SOURCE_DIR}) is
     replaced; an entry on its own line in a list that is mostly sorted (SORTED_SHARE) is moved to its sorted
     place, otherwise it stays where it was;
   - .vcxproj and .filters: every item (`<ClCompile>`, `<ClInclude>`, `<None>`, ...; one line or several) gets its
     new path, and moves to its sorted place when its item list is mostly sorted; in a .filters file the item's
     <Filter> becomes the one its new folder's items already use, else the old filter with the folder part
     replaced (`Source Files\\ac\\dynobj` -> `Source Files\\script\\dynobj`), else it stays; a missing <Filter>
     block is created with a uuid5 GUID derived from its name (split_bindings.stable_id), and old filters left with
     no item and no child filter are removed;
   - Xcode: the PBXFileReference of a moved file moves to the group of its new folder (groups created as needed,
     IDs derived from the folder: split_bindings.xcode_id), its `path` and every `/* name */` comment of it and of
     its build files change, and groups left empty are removed.
5. Scripts (every other tracked text file: .py, .sh, .cmd, workflows, packaging...): an old repository path,
   with / or \\, is rewritten in place, except in NO_REWRITE; a module-relative mention (`ac/character.cpp`) is
   listed. Documents (DOCS; the history files LOG.md, BASELINE.md, UPSTREAM.md are not) are not rewritten: the
   lines that name a moved file or an emptied folder are listed. Comments (and strings) in code that cite a moved
   file by path or by a base name only it has are listed too.
6. The report: files moved per destination folder, files whose includes change, build files touched, scripts
   rewritten, what to review, and the rewritten includes and moved files that only non-Windows builds compile
   (inside an #if, or in a platform folder other than windows).

--dry-run computes and prints everything, the clang-format results included, and writes nothing. --repo runs it on
another checkout; the facts of the tree are the tables below (another tree: other build files, maybe no
clang-format: --no-build-lists, --no-format). Exit code 0 when it ran (or would run), 2 when something stops it.
"""
import argparse
import collections
import fnmatch
import os
import posixpath
import re
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import check_build_lists  # noqa: E402  (.pbxproj readers)
import split_bindings as sb  # noqa: E402  (build-list IDs, sorted insertion, .pbxproj groups, clang-format, line ends)

# ---------------------------------------------------------------------------------------------------------------
# Tables: the facts of this tree
# ---------------------------------------------------------------------------------------------------------------

# include search order after the includer's own folder (quoted includes); '' is the repository root (AGS.Native)
INCLUDE_ROOTS = ('Common', 'Engine', 'Tools', 'Compiler', 'Plugins', 'Common/libinclude', 'Editor/Native', '')
# the roots a moved file is included from; a path below one of them must not exist below another
MODULE_ROOTS = ('Common', 'Engine')
# never edited: an include of a moved file from here stops the run
THIRD_PARTY = ('libsrc/', 'Common/libsrc/', 'Common/libinclude/', 'Engine/libsrc/', 'Editor/scintilla/',
               'Editor/AGS.Controls/ScintillaNET/', 'Plugins/agslua/')
SOURCE_EXTS = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.inl', '.mm', '.m', '.rc')
FORMAT_EXTS = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.inl')
# other code whose comments are listed (not include-scanned)
COMMENT_EXTS = SOURCE_EXTS + ('.cs',)
# folders the map empties; --check-coverage wants a row for each of their files
COVERAGE_DIRS = ('Engine/ac', 'Common/ac', 'Engine/gui')
NO_ROW = {
    'Common/ac/def_version.h': 'named by CI and packaging on every platform: decided at the end of 5.3 (map header)',
}
# a row whose old and new paths are both missing is skipped when its note says this
FOLDED_NOTE = 'folded'
# build descriptions
CMAKE_GLOBS = ('CMakeLists.txt', '*/CMakeLists.txt', '*.cmake')
VCXPROJ_GLOBS = ('*.vcxproj', '*.vcxproj.filters')
XCODE_GLOBS = ('*/project.pbxproj',)
VCX_KINDS = ('ClCompile', 'ClInclude', 'None', 'ResourceCompile', 'Text', 'Image', 'FxCompile', 'CustomBuild',
             'Midl', 'Natvis')
# an entry list is kept sorted when at least this share of its neighbours is in order
SORTED_SHARE = 0.9
# documents whose lines naming a moved path are listed (current guidance; LOG, BASELINE, UPSTREAM are history)
DOCS = ('AGENTS.md', 'docs/refactor/STYLE.md', 'docs/refactor/PLAN.md', 'docs/refactor/ALIGNMENT.md',
        'docs/context/*.md')
HISTORY = ('docs/refactor/LOG.md', 'docs/refactor/BASELINE.md', 'docs/UPSTREAM.md')
# scripts that name moved paths but are not rewritten (listed instead)
NO_REWRITE = {
    'Script/split_bindings.py': 'the 5.2 tool: its tables name the files it split; owned by its own task',
    'Script/apply_moves.py': 'this tool',
    '.git-blame-ignore-revs': 'commit hashes; the comments are history',
}
# file names a comment may cite that also name standard headers: not listed by base name
STD_NAMES = {'string.h', 'math.h', 'time.h', 'file.h', 'timer.h', 'system.h', 'event.h'}
# folders whose code no Windows build compiles
NON_WINDOWS = re.compile(r'^(Engine|Common)/platform/(?!windows/)(?!base/)|_posix\.|_unix\.|\.mm$|\.m$')
# an #if branch naming one of these is compiled by other platforms only
NON_WINDOWS_NAMES = re.compile(r'ANDROID|_IOS\b|MACOS|EMSCRIPTEN|LINUX|FREEBSD|_BSD|XDG|POSIX|UNIX|__APPLE__|MOBILE')

INCLUDE = re.compile(r'^(\s*#\s*include\s*)(["<])([^">\n]+)([">])(.*)$')
CONDITION = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$')


class Fail(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------
# Files
# ---------------------------------------------------------------------------------------------------------------

class Text:
    """A text file as LF text, with what writing it back needs: line end, BOM, encoding."""

    def __init__(self, repo, path):
        self.path = path
        with open(os.path.join(repo, path), 'rb') as f:
            data = f.read()
        self.bom = b''
        if data.startswith(b'\xef\xbb\xbf'):
            self.bom, data = data[:3], data[3:]
        try:
            text = data.decode('utf-8')
            self.encoding = 'utf-8'
        except UnicodeDecodeError:
            text = data.decode('latin-1')  # round-trips every byte
            self.encoding = 'latin-1'
        crlf = text.count('\r\n')
        lf = text.count('\n')
        if crlf and crlf != lf:
            raise Fail('%s mixes CRLF and LF line ends' % path)
        self.eol = '\r\n' if crlf else '\n'
        self.text = text.replace('\r\n', '\n')
        self.original = self.text

    def data(self):
        return self.bom + self.text.replace('\n', self.eol).encode(self.encoding)


def git(repo, *args):
    r = subprocess.run(['git', *args], cwd=repo, capture_output=True, text=True, encoding='utf-8')
    if r.returncode:
        raise Fail('git %s: %s' % (' '.join(args), r.stderr.strip()))
    return r.stdout


def tracked_files(repo):
    return [p for p in git(repo, 'ls-files', '-z').split('\0') if p]


def is_third_party(path):
    return path.startswith(THIRD_PARTY)


def matches(path, globs):
    return any(fnmatch.fnmatch(path, g) for g in globs)


def module_tail(path):
    """`ac/character.cpp` for `Engine/ac/character.cpp`."""
    return path.split('/', 1)[1] if '/' in path else path


# ---------------------------------------------------------------------------------------------------------------
# The map
# ---------------------------------------------------------------------------------------------------------------

class Row:
    def __init__(self, old, new, note, line):
        self.old, self.new, self.note, self.line = old, new, note, line


def read_map(path):
    rows = []
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip('\r\n')
            if not line.strip() or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) < 2 or not parts[0] or not parts[1]:
                raise Fail('%s:%d: not "old<TAB>new[<TAB>note]"' % (path, n))
            rows.append(Row(parts[0].strip(), parts[1].strip(), parts[2].strip() if len(parts) > 2 else '', n))
    return rows


def selected(row, only, exact):
    if not only:
        return True
    for p in only:
        p = p.rstrip('/')
        if exact and posixpath.dirname(row.new) == p:
            return True
        if not exact and (row.new == p or row.new.startswith(p + '/')):
            return True
    return False


class Report:
    def __init__(self):
        self.stops = []
        self.notes = []
        self.pending = {}            # old -> new, selected rows to apply
        self.applied = []            # rows already applied
        self.folded = []             # rows skipped: folded away earlier
        self.uncovered = []
        self.include_files = {}      # pre path -> number of include lines rewritten
        self.include_lines = 0
        self.platform = []           # (path, line, old include, new include, why)
        self.platform_moved = []     # moved files with platform branches
        self.format_unclean = []
        self.resorted = []           # renamed files whose unchanged includes were re-sorted for the new name
        self.build = collections.Counter()  # build file -> entries changed
        self.filters_added = collections.defaultdict(list)
        self.filters_removed = collections.defaultdict(list)
        self.groups_added = collections.defaultdict(list)
        self.groups_removed = collections.defaultdict(list)
        self.scripts = collections.Counter()  # script -> occurrences rewritten
        self.scripts_listed = []
        self.docs = []
        self.comments = []

    def stop(self, msg):
        self.stops.append(msg)


# ---------------------------------------------------------------------------------------------------------------
# 1. Validation
# ---------------------------------------------------------------------------------------------------------------

def validate(repo, rows, files, only, exact, check_coverage, rep):
    fileset = set(files)
    by_old, by_new = {}, {}
    for r in rows:
        if r.old in by_old:
            rep.stop('map line %d: %s is also on line %d' % (r.line, r.old, by_old[r.old].line))
        if r.new in by_new:
            rep.stop('map line %d: destination %s is also on line %d' % (r.line, r.new, by_new[r.new].line))
        by_old[r.old], by_new[r.new] = r, r
    for r in rows:
        old_here, new_here = r.old in fileset, r.new in fileset or os.path.exists(os.path.join(repo, r.new))
        if old_here and new_here:
            rep.stop('map line %d: %s and its destination %s both exist' % (r.line, r.old, r.new))
        elif not selected(r, only, exact):
            continue
        elif old_here:
            rep.pending[r.old] = r.new
        elif new_here:
            rep.applied.append(r)
        elif FOLDED_NOTE in r.note:
            rep.folded.append(r)
        else:
            rep.stop('map line %d: neither %s nor %s exists' % (r.line, r.old, r.new))
    if check_coverage:
        for p in files:
            if any(p.startswith(d + '/') for d in COVERAGE_DIRS) and p not in by_old and p not in NO_ROW:
                rep.uncovered.append(p)
                rep.stop('coverage: %s is in an emptied folder and has no row' % p)
    post = (fileset - set(rep.pending)) | set(rep.pending.values())
    # module-root ambiguity
    for new in sorted(rep.pending.values()):
        for root in MODULE_ROOTS:
            if new.startswith(root + '/'):
                tail = new[len(root) + 1:]
                for other in MODULE_ROOTS:
                    if other != root and other + '/' + tail in post:
                        rep.stop('%s: "%s" would also name %s/%s' % (new, tail, other, tail))
    # flat MSBuild object names
    for proj in files:
        if not proj.endswith('.vcxproj') or is_third_party(proj):
            continue
        items = vcx_items(Text(repo, proj).text.split('\n'))
        base = posixpath.dirname(proj)
        stems = collections.defaultdict(list)
        touches = False
        for it in items:
            if it.kind != 'ClCompile' or it.object_name:
                continue
            path = vcx_resolve(base, it.include)
            if path is None:
                continue
            if path in rep.pending:
                touches = True
                path = rep.pending[path]
            stems[posixpath.splitext(posixpath.basename(path))[0].lower()].append(path)
        if touches:
            for stem, paths in sorted(stems.items()):
                if len(paths) > 1 and any(p in rep.pending.values() for p in paths):
                    rep.stop('%s: two sources would compile to %s.obj: %s' % (proj, stem, ', '.join(paths)))
    return post


# ---------------------------------------------------------------------------------------------------------------
# 3. Includes
# ---------------------------------------------------------------------------------------------------------------

class Resolver:
    def __init__(self, tree):
        self.tree = tree
        self.lower = {}
        for p in tree:
            self.lower.setdefault(p.lower(), p)

    def resolve(self, includer, text, angle):
        """(path, 'own'|'root'|'case') or (None, None)."""
        cands = []
        if not angle:
            cands.append(('own', posixpath.join(posixpath.dirname(includer), text)))
        for r in INCLUDE_ROOTS:
            cands.append(('root', posixpath.join(r, text) if r else text))
        for kind, c in cands:
            c = posixpath.normpath(c)
            if c in self.tree:
                return c, kind
        for kind, c in cands:
            c = self.lower.get(posixpath.normpath(c).lower())
            if c:
                return c, 'case'
        return None, None


def non_windows_branch(cond):
    """Whether an #if branch (`#if X / #else`) is one that no Windows build compiles."""
    last = cond.split(' / ')[-1]
    if NON_WINDOWS_NAMES.search(last):
        return True
    if 'AGS_PLATFORM_OS_WINDOWS' in cond and (last.startswith(('#else', '#elif')) or '#ifndef' in last
                                               or re.search(r'!\s*AGS_PLATFORM_OS_WINDOWS', last)):
        return True
    return False


def include_text_for(target, includer, old_text):
    """The text that includes `target` from `includer`: relative when the old one was, else root-relative."""
    if old_text.startswith('.'):
        return posixpath.relpath(target, posixpath.dirname(includer))
    for r in INCLUDE_ROOTS:
        if r and target.startswith(r + '/'):
            return target[len(r) + 1:]
    return target


def include_blocks(lines, changed):
    """1-based (first, last) ranges of the include blocks holding the changed lines (0-based indices)."""
    ranges = []
    for i in sorted(changed):
        a = b = i
        while a > 0 and (INCLUDE.match(lines[a - 1]) or not lines[a - 1].strip()):
            a -= 1
        while b + 1 < len(lines) and (INCLUDE.match(lines[b + 1]) or not lines[b + 1].strip()):
            b += 1
        while not lines[a].strip():
            a += 1
        while not lines[b].strip():
            b -= 1
        if ranges and a <= ranges[-1][1]:
            ranges[-1] = (ranges[-1][0], max(b + 1, ranges[-1][1]))
        else:
            ranges.append((a + 1, b + 1))
    return ranges


def without_includes(text):
    out = []
    for l in text.split('\n'):
        if INCLUDE.match(l):
            continue
        if not l.strip() and out and not out[-1].strip():
            continue
        out.append(l)
    return out


def include_set(text):
    return sorted(re.sub(r'\s+', '', l) for l in text.split('\n') if INCLUDE.match(l))


_CLANG_FORMAT = []


def clang_format_exe():
    if not _CLANG_FORMAT:
        _CLANG_FORMAT.append(sb.clang_format())
    return _CLANG_FORMAT[0]


def clang_format_text(repo, path, text, ranges):
    cmd = [clang_format_exe(),'--style=file', '--assume-filename=%s' % os.path.join(repo, path)]
    cmd += ['--lines=%d:%d' % r for r in ranges]
    r = subprocess.run(cmd, input=text.encode('utf-8'), capture_output=True)
    if r.returncode:
        raise Fail('clang-format failed on %s: %s' % (path, r.stderr.decode('utf-8', 'replace').strip()))
    return r.stdout.decode('utf-8')


def format_clean(repo, path, text):
    r = subprocess.run([clang_format_exe(), '--style=file', '--dry-run', '-Werror',
                        '--assume-filename=%s' % os.path.join(repo, path)],
                       input=text.encode('utf-8'), capture_output=True)
    return r.returncode == 0


def rewrite_includes(repo, files, post, texts, rep, do_format):
    pending = rep.pending
    pre_res, post_res = Resolver(set(files)), Resolver(post)
    module_dirs = {p.split('/')[1] for p in files if p.startswith(MODULE_ROOTS) and p.count('/') >= 2}
    jobs, renamed = [], []
    for path in files:
        if not path.endswith(SOURCE_EXTS):
            continue
        try:
            t = texts.get(path) or Text(repo, path)
        except Fail as e:
            if path in pending:
                rep.stop(str(e))
            continue
        lines = t.text.split('\n')
        new_path = pending.get(path, path)
        third = is_third_party(path)
        changed, stack, rewrites = [], [], {}
        for i, line in enumerate(lines):
            c = CONDITION.match(line)
            if c:
                kw, cond = c.group(1), c.group(2).strip()
                if kw in ('if', 'ifdef', 'ifndef'):
                    stack.append('#%s %s' % (kw, cond))
                elif kw in ('elif', 'else') and stack:
                    stack[-1] = '%s / #%s %s' % (stack[-1], kw, cond)
                elif kw == 'endif' and stack:
                    stack.pop()
                continue
            m = INCLUDE.match(line)
            if not m:
                continue
            angle = m.group(2) == '<'
            text = m.group(3)
            target, kind = pre_res.resolve(path, text, angle)
            if target is None:
                first = text.split('/')[0]
                if not angle and not third and '/' in text and first in module_dirs:
                    rep.stop('%s:%d: #include "%s" does not resolve' % (path, i + 1, text))
                continue
            if kind == 'case' and target in pending:
                rep.stop('%s:%d: #include "%s" names %s with another case' % (path, i + 1, text, target))
                continue
            new_target = pending.get(target, target)
            need = target in pending or (path in pending and kind == 'own')
            if need and third:
                rep.stop('%s:%d: third-party code includes the moved %s' % (path, i + 1, target))
                continue
            new_text = include_text_for(new_target, new_path, text) if need else text
            got, _ = post_res.resolve(new_path, new_text, angle)
            if got != new_target:
                rep.stop('%s:%d: after the move #include %s%s%s would reach %s instead of %s'
                         % (path, i + 1, m.group(2), new_text, m.group(4), got, new_target))
                continue
            if new_text != text:
                lines[i] = m.group(1) + m.group(2) + new_text + m.group(4) + m.group(5)
                changed.append(i)
                rewrites[text] = new_text
                if any(non_windows_branch(s) for s in stack) or NON_WINDOWS.search(new_path):
                    rep.platform.append((path, i + 1, text, new_text, ' > '.join(stack) or 'platform folder'))
        if path in pending and NON_WINDOWS.search(path):
            rep.platform_moved.append((path, 'platform folder'))
        if not changed:
            if do_format and path.endswith(FORMAT_EXTS) and posixpath.basename(path) != posixpath.basename(new_path):
                renamed.append((path, new_path, t, lines))
            continue
        text = '\n'.join(lines)
        if path.endswith('.rc'):
            for old, new in rewrites.items():
                if old.startswith('.'):
                    text = text.replace('"%s' % old, '"%s' % new).replace('""%s""' % old, '""%s""' % new)
        t.text = text
        texts[path] = t
        rep.include_files[path] = len(changed)
        rep.include_lines += len(changed)
        if do_format and path.endswith(FORMAT_EXTS):
            jobs.append((path, new_path, t, include_blocks(lines, changed)))
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        # a renamed file: its main header is found by name, so the new name may sort its includes differently
        def dirty(r):
            return not format_clean(repo, r[1], r[2].text) and format_clean(repo, r[0], r[2].text)
        for (path, new_path, t, lines), d in zip(renamed, pool.map(dirty, renamed)):
            if d:
                texts[path] = t
                rep.resorted.append(path)
                jobs.append((path, new_path, t, include_blocks(lines, [i for i, l in enumerate(lines)
                                                                       if INCLUDE.match(l)])))
        for path, msg, unclean in pool.map(lambda j: sort_includes(repo, *j), jobs):
            if msg:
                rep.stop('%s: %s' % (path, msg))
            if unclean:
                rep.format_unclean.append(path)


def sort_includes(repo, path, new_path, t, ranges):
    """Re-sorts the changed include blocks of t.text; (path, what went wrong or None, not format-clean)."""
    expected = include_set(t.text)
    formatted = clang_format_text(repo, new_path, t.text, ranges)
    if without_includes(formatted) != without_includes(t.original):
        return path, 're-sorting the includes changed other lines', False
    if include_set(formatted) != expected:
        return path, 'clang-format changed the include lines themselves', False
    if not format_clean(repo, new_path, formatted) and format_clean(repo, path, t.original):
        # an include with a trailing comment that lands next to a code line re-aligns that line's comment: format
        # one line more on each side, and accept a change of spaces only
        wider = [(max(1, a - 1), b + 1) for a, b in ranges]
        again = clang_format_text(repo, new_path, t.text, wider)
        squeeze = lambda lines: [re.sub(r'\s+', ' ', l) for l in lines]  # noqa: E731
        if squeeze(without_includes(again)) != squeeze(without_includes(t.original)) \
                or include_set(again) != expected or not format_clean(repo, new_path, again):
            t.text = formatted
            return path, None, True
        formatted = again
    t.text = formatted
    return path, None, False


def platform_branches(repo, rep, texts):
    """Moved files with platform #if branches: parts of them only other platforms compile."""
    plat = re.compile(r'^\s*#\s*(?:if|ifdef|ifndef|elif)\b.*\b(AGS_PLATFORM_(?:OS_\w+|MOBILE|IS_\w+|WINDOWS_\w+)|_WIN32|__APPLE__|__ANDROID__|'
                      r'ANDROID|__linux__|__EMSCRIPTEN__|AGS_HAS_OPENGL|AGS_NO_VIDEO_PLAYER)', re.M)
    for old in sorted(rep.pending):
        if not old.endswith(SOURCE_EXTS):
            continue
        t = texts.get(old) or Text(repo, old)
        found = sorted(set(m.group(1) for m in plat.finditer(t.text)))
        if found:
            rep.platform_moved.append((old, ', '.join(found)))


# ---------------------------------------------------------------------------------------------------------------
# 4. Build descriptions
# ---------------------------------------------------------------------------------------------------------------

def mostly_sorted(keys):
    if len(keys) < 4:
        return False
    good = sum(1 for a, b in zip(keys, keys[1:]) if a <= b)
    return good >= SORTED_SHARE * (len(keys) - 1)


def sorted_position(keys, new):
    """Index of the key after which `new` goes (split_bindings.insert_sorted), -1 for the front."""
    return sb.insert_sorted(keys, new)


SRC_EXT = r'(?:cpp|cc|cxx|c|h|hpp|inl|mm|m|rc)'
CMAKE_TOKEN = re.compile(r'(?<![\w.${}/+-])((?:\$\{(\w+)\}/)?(?:[\w.+-]+/)*[\w.+-]+\.%s)(?![\w.+-])' % SRC_EXT)
CMAKE_ENTRY = re.compile(r'^(\s+)(\S+)(\s+#.*)?\s*$')


def cmake_resolve(base, token, var):
    rest = token.split('}/', 1)[1] if var else token
    if var in ('CMAKE_SOURCE_DIR', 'PROJECT_SOURCE_DIR'):
        return posixpath.normpath(rest)
    if var in (None, 'CMAKE_CURRENT_SOURCE_DIR', 'CMAKE_CURRENT_LIST_DIR'):
        return posixpath.normpath(posixpath.join(base, rest))
    return None


def cmake_block(lines, i):
    """(first, last) of the run of single-entry lines around line i with its indentation."""
    m = CMAKE_ENTRY.match(lines[i])
    ind = m.group(1)

    def entry(j):
        e = CMAKE_ENTRY.match(lines[j])
        return e and e.group(1) == ind and ('/' in e.group(2) or '.' in e.group(2))

    a = b = i
    while a > 0 and entry(a - 1):
        a -= 1
    while b + 1 < len(lines) and entry(b + 1):
        b += 1
    return a, b


def update_cmake(path, t, pending, rep):
    base = posixpath.dirname(path)
    lines = t.text.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        hit = None
        for m in CMAKE_TOKEN.finditer(line):
            target = cmake_resolve(base, m.group(1), m.group(2))
            if target in pending:
                hit = (m, target)
                break
        if not hit:
            i += 1
            continue
        m, target = hit
        new = pending[target]
        var = m.group(2)
        if var in ('CMAKE_SOURCE_DIR', 'PROJECT_SOURCE_DIR'):
            token = '${%s}/%s' % (var, new)
        else:
            token = ('${%s}/' % var if var else '') + posixpath.relpath(new, base or '.')
        lines[i] = line[:m.start(1)] + token + line[m.end(1):]
        rep.build[path] += 1
        e = CMAKE_ENTRY.match(lines[i])
        if e and e.group(2) == token:
            a, b = cmake_block(lines, i)
            others = [(j, CMAKE_ENTRY.match(lines[j]).group(2)) for j in range(a, b + 1) if j != i]
            if mostly_sorted([k for _, k in others]):
                moved = lines.pop(i)
                others = [(j - (j > i), k) for j, k in others]
                at = sorted_position([k for _, k in others], token)
                dest = others[at][0] + 1 if at >= 0 else others[0][0]
                lines.insert(dest, moved)
        i = 0  # start again: lines may have moved
    t.text = '\n'.join(lines)


class VcxItem:
    def __init__(self, start, end, kind, include, group, filt, object_name):
        self.start, self.end, self.kind, self.include = start, end, kind, include
        self.group, self.filter, self.object_name = group, filt, object_name


VCX_ITEM = re.compile(r'^\s*<(%s) Include="([^"]+)"\s*(/?)>\s*$' % '|'.join(VCX_KINDS))


def vcx_items(lines):
    items, group, i = [], -1, 0
    while i < len(lines):
        line = lines[i]
        if '<ItemGroup' in line:
            group = i
        m = VCX_ITEM.match(line)
        if not m:
            i += 1
            continue
        kind = m.group(1)
        end = i
        if not m.group(3):
            end = i + 1
            while lines[end].strip() != '</%s>' % kind:
                end += 1
        filt = obj = None
        for j in range(i + 1, end):
            f = re.match(r'^\s*<Filter>([^<]*)</Filter>\s*$', lines[j])
            if f:
                filt = (j, f.group(1))
            if '<ObjectFileName>' in lines[j]:
                obj = True
        items.append(VcxItem(i, end, kind, m.group(2), group, filt, obj))
        i = end + 1
    return items


def vcx_resolve(base, include):
    if '$(' in include or '%(' in include or '*' in include:
        return None
    p = posixpath.normpath(posixpath.join(base, include.replace('\\', '/')))
    return None if p.startswith('../') else p


def vcx_key(include):
    return include.replace('\\', '/').lower()


def new_filter(items, base, item, old_path, new_path):
    """The filter for an item moved from old_path to new_path (see the module docstring)."""
    new_dir = posixpath.dirname(new_path)
    seen = collections.Counter()
    for it in items:
        if it is item or it.kind != item.kind or not it.filter:
            continue
        p = vcx_resolve(base, it.include)
        if p and posixpath.dirname(p) == new_dir:
            seen[it.filter[1]] += 1
    if seen:
        return seen.most_common(1)[0][0]
    old = item.filter[1]
    fparts = old.split('\\')
    dparts = posixpath.dirname(old_path).split('/')
    k = 0
    while k < len(fparts) - 1 and k < len(dparts) and fparts[-1 - k] == dparts[-1 - k]:
        k += 1
    if k == 0:
        return old
    root = dparts[:len(dparts) - k]
    nparts = new_dir.split('/')
    if nparts[:len(root)] != root:
        return old
    return '\\'.join(fparts[:len(fparts) - k] + nparts[len(root):])


def filter_blocks(lines):
    out = []
    for i, l in enumerate(lines):
        m = re.match(r'^(\s*)<Filter Include="([^"]+)">\s*$', l)
        if m:
            j = i + 1
            while lines[j].strip() != '</Filter>':
                j += 1
            out.append((i, j, m.group(2), m.group(1)))
    return out


def ensure_filter(path, lines, name, rep):
    blocks = filter_blocks(lines)
    names = [b[2] for b in blocks]
    if name in names or not blocks:
        return
    parent = name.rsplit('\\', 1)[0] if '\\' in name else None
    if parent:
        ensure_filter(path, lines, parent, rep)
        blocks = filter_blocks(lines)
    keys = [b[2].replace('\\', '/').lower() for b in blocks]
    at = sorted_position(keys, name.replace('\\', '/').lower())
    ind = blocks[0][3]
    pos = blocks[at][1] + 1 if at >= 0 else blocks[0][0]
    lines[pos:pos] = ['%s<Filter Include="%s">' % (ind, name),
                      '%s  <UniqueIdentifier>{%s}</UniqueIdentifier>' % (ind, sb.stable_id('filter', name)),
                      '%s</Filter>' % ind]
    rep.filters_added[path].append(name)


def remove_empty_filters(path, lines, candidates, rep):
    while True:
        used = set()
        for l in lines:
            f = re.match(r'^\s*<Filter>([^<]*)</Filter>\s*$', l)
            if f:
                used.add(f.group(1))
        blocks = filter_blocks(lines)
        names = [b[2] for b in blocks]
        gone = None
        for (a, b, name, _) in blocks:
            if name in candidates and name not in used and not any(n.startswith(name + '\\') for n in names):
                gone = (a, b, name)
                break
        if not gone:
            return
        del lines[gone[0]:gone[1] + 1]
        rep.filters_removed[path].append(gone[2])


def update_vcxproj(path, t, pending, rep):
    base = posixpath.dirname(path)
    filters = path.endswith('.filters')
    lines = t.text.split('\n')
    old_filters = set()
    while True:
        items = vcx_items(lines)
        item = target = None
        for it in items:
            p = vcx_resolve(base, it.include)
            if p in pending:
                item, target = it, p
                break
        if item is None:
            break
        new = pending[target]
        include = posixpath.relpath(new, base).replace('/', '\\')
        lines[item.start] = lines[item.start].replace('Include="%s"' % item.include, 'Include="%s"' % include)
        rep.build[path] += 1
        if filters and item.filter:
            nf = new_filter(items, base, item, target, new)
            if nf != item.filter[1]:
                old_filters.add(item.filter[1])
                j = item.filter[0]
                lines[j] = lines[j].replace('<Filter>%s</Filter>' % item.filter[1], '<Filter>%s</Filter>' % nf)
                ensure_filter(path, lines, nf, rep)
                items = vcx_items(lines)
                item = next(it for it in items if it.kind == item.kind and it.include == include)
        # keep a sorted item list sorted
        same = [it for it in items if it.group == item.group and it.kind == item.kind and it is not item]
        if mostly_sorted([vcx_key(it.include) for it in same]):
            block = lines[item.start:item.end + 1]
            del lines[item.start:item.end + 1]
            n = item.end + 1 - item.start
            keys = [vcx_key(it.include) for it in same]
            at = sorted_position(keys, vcx_key(include))
            if at >= 0:
                dest = same[at].end + 1
            else:
                dest = same[0].start
            if dest > item.start:
                dest -= n
            lines[dest:dest] = block
    if filters and old_filters:
        cands = set()
        for f in old_filters:
            parts = f.split('\\')
            for k in range(1, len(parts) + 1):
                cands.add('\\'.join(parts[:k]))
        remove_empty_filters(path, lines, cands, rep)
    t.text = '\n'.join(lines)


PBX_REF = re.compile(r'^\t\t(\w{24}) /\* ([^\n]*?) \*/ = \{isa = PBXFileReference;([^\n]*)\};$', re.M)


def pbx_refs(text):
    out = {}
    for m in PBX_REF.finditer(check_build_lists.pbx_section(text, 'PBXFileReference')):
        body = m.group(3)
        out[m.group(1)] = (check_build_lists.pbx_value(body, 'path'), check_build_lists.pbx_value(body, 'name'),
                           check_build_lists.pbx_value(body, 'sourceTree'))
    return out


class Pbx:
    def __init__(self, path, text):
        self.path = path
        self.project_dir = posixpath.dirname(posixpath.dirname(path))
        self.text = text
        self.parse()

    def parse(self):
        self.refs = pbx_refs(self.text)
        self.groups, self.parent = sb.pbx_groups(self.text)

    def resolve(self, node):
        if node in self.refs:
            p, _, tree = self.refs[node]
        else:
            p, tree, _ = self.groups[node]
        if tree == 'SOURCE_ROOT':
            base = self.project_dir
        elif tree == '<group>':
            base = self.resolve(self.parent[node]) if node in self.parent else self.project_dir
        else:
            return None
        if base is None:
            return None
        return posixpath.normpath(posixpath.join(base, p)) if p else base

    def group_of_dir(self):
        by_dir = {}
        for g in sorted(self.groups, key=lambda g: (self.groups[g][0] is None, g)):
            if g not in self.parent:
                continue
            r = self.resolve(g)
            if r:
                by_dir.setdefault(r, g)
        return by_dir


def pbx_remove_child(text, group, child):
    m = re.search(r'^\t\t%s(?: /\*[^\n]*?\*/)? = \{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = \(\n(.*?)\t\t\t\);'
                  % group, text, re.S | re.M)
    body = m.group(1)
    new_body = re.sub(r'^\t+%s(?: /\*[^\n]*?\*/)?,\n' % child, '', body, count=1, flags=re.M)
    return text[:m.start(1)] + new_body + text[m.end(1):]


def pbx_remove_group(text, group):
    return re.sub(r'^\t\t%s(?: /\*[^\n]*?\*/)? = \{\n\t\t\tisa = PBXGroup;\n.*?\n\t\t\};\n' % group, '', text,
                  count=1, flags=re.S | re.M)


def update_xcode(path, t, pending, rep):
    px = Pbx(path, t.text)
    old_parents = set()
    for old in sorted(pending):
        new = pending[old]
        hits = [r for r in px.refs if px.resolve(r) == old]
        for ref in hits:
            p, name, tree = px.refs[ref]
            if tree != '<group>' or ref not in px.parent:
                raise Fail('%s: %s is not a file reference inside a group' % (path, old))
            text = px.text
            by_dir = px.group_of_dir()
            new_dir = posixpath.dirname(new)
            missing, d = [], new_dir
            while d not in by_dir:
                missing.insert(0, d)
                d = posixpath.dirname(d)
                if not d:
                    raise Fail('%s: no group for any parent folder of %s' % (path, new))
            group = by_dir[d]
            for m in missing:
                gid = sb.xcode_id('group', m)
                b = posixpath.basename(m)
                block = ('\t\t%s /* %s */ = {\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n\t\t\t);\n\t\t\tpath = %s;\n'
                         '\t\t\tsourceTree = "<group>";\n\t\t};\n' % (gid, b, b))
                text = sb.pbx_insert_object(text, 'PBXGroup', gid, block)
                text = sb.pbx_add_child(text, group, gid, b)
                rep.groups_added[path].append(m)
                group = gid
            oldbase, newbase = posixpath.basename(old), posixpath.basename(new)
            parent = px.parent[ref]
            old_parents.add(parent)
            text = pbx_remove_child(text, parent, ref)
            text = sb.pbx_add_child(text, group, ref, newbase)
            # the reference itself
            m = re.search(r'^\t\t%s /\* [^\n]*? \*/ = \{isa = PBXFileReference;[^\n]*\};$' % ref, text, re.M)
            line = m.group(0)
            body = re.sub(r'\bname = [^;]+; ', '', line)
            body = re.sub(r'\bpath = [^;]+;', 'path = %s;' % newbase, body)
            text = text[:m.start()] + body + text[m.end():]
            # comments naming it: the reference, its build files
            text = text.replace('%s /* %s */' % (ref, oldbase), '%s /* %s */' % (ref, newbase))
            for bid, phase in re.findall(r'^\t\t(\w{24}) /\* [^\n]*? in (\w+) \*/ = \{isa = PBXBuildFile; fileRef = %s '
                                         % ref, text, re.M):
                text = text.replace('%s /* %s in %s */' % (bid, oldbase, phase),
                                    '%s /* %s in %s */' % (bid, newbase, phase))
            px.text = text
            px.parse()
            rep.build[path] += 1
    # groups left empty
    while True:
        empty = [g for g in sorted(old_parents) if g in px.groups and not px.groups[g][2]]
        if not empty:
            break
        for g in empty:
            par = px.parent.get(g)
            dirpath = px.resolve(g)
            text = pbx_remove_group(px.text, g)
            if par:
                text = pbx_remove_child(text, par, g)
                old_parents.add(par)
            px.text = text
            px.parse()
            old_parents.discard(g)
            rep.groups_removed[path].append(dirpath)
    t.text = px.text


def update_build_lists(repo, files, texts, rep):
    for path in files:
        if is_third_party(path):
            continue
        name = posixpath.basename(path)
        if matches(path, CMAKE_GLOBS) or name == 'CMakeLists.txt':
            fn = update_cmake
        elif matches(path, VCXPROJ_GLOBS):
            fn = update_vcxproj
        elif matches(path, XCODE_GLOBS):
            fn = update_xcode
        else:
            continue
        t = texts.get(path) or Text(repo, path)
        before = t.text
        fn(path, t, rep.pending, rep)
        if t.text != before:
            texts[path] = t


# ---------------------------------------------------------------------------------------------------------------
# 5. Scripts, documents, comments
# ---------------------------------------------------------------------------------------------------------------

def is_binary(repo, path):
    with open(os.path.join(repo, path), 'rb') as f:
        return b'\0' in f.read(8192)


def emptied_dirs(files, pending):
    dirs = collections.defaultdict(lambda: [0, 0])
    for p in files:
        d = posixpath.dirname(p)
        while d:
            dirs[d][0] += 1
            if p in pending:
                dirs[d][1] += 1
            d = posixpath.dirname(d)
    return sorted(d for d, (n, k) in dirs.items() if n and n == k)


class Mentions:
    """One regular expression for the names of many moved files; find() gives (match, old path, new path)."""

    END = r'(?![\w]|\.\w)'  # `character` must not take `character.h` for `character.cpp`

    def __init__(self, entries, before):
        """entries: (text, optional suffix, old, new); a match is the text, with or without the suffix."""
        self.rows = {}
        alts = set()
        for text, suffix, old, new in entries:
            self.rows.setdefault(text + suffix, (old, new))
            if suffix:
                self.rows.setdefault(text, (old, new))
                alts.add('%s(?:%s)?' % (re.escape(text), re.escape(suffix)))
            else:
                alts.add(re.escape(text))
        alts = sorted(alts, key=lambda x: (-len(x), x))
        self.regex = re.compile('%s(%s)%s' % (before, '|'.join(alts), self.END)) if alts else None

    def find(self, line):
        if self.regex is None:
            return
        for m in self.regex.finditer(line):
            old, new = self.rows[m.group(1)]
            yield m, old, new

    def search(self, text):
        return self.regex is not None and self.regex.search(text) is not None


def mention_patterns(files, pending):
    """Mentions of the moved files: full paths (with / or \\), module-relative paths (`ac/character.cpp`), and the
    base names that only one tracked file has."""
    names = collections.Counter(posixpath.basename(p) for p in files)
    full, tail, base = [], [], []
    for old in sorted(pending):
        new = pending[old]
        stem_old, ext = posixpath.splitext(old)
        full.append((stem_old, ext, old, new))
        full.append((old.replace('/', '\\'), '', old.replace('/', '\\'), new.replace('/', '\\')))
        t = module_tail(stem_old)
        if '/' in t:
            tail.append((t, ext, old, new))
        b = posixpath.basename(old)
        if names[b] == 1 and b not in STD_NAMES:
            base.append((b, '', old, new))
    return (Mentions(full, r'(?<![\w-])'), Mentions(tail, r'(?<![\w/.\\-])'), Mentions(base, r'(?<![\w/.\\-])'))


def scan_scripts_docs(repo, files, texts, rep):
    pending = rep.pending
    if not pending:
        return
    full, tail, _ = mention_patterns(files, pending)
    dirs = emptied_dirs(files, pending)
    dir_re = re.compile(r'(?<![\w-])(%s)(?![\w-])' % '|'.join(re.escape(d) for d in dirs)) if dirs else None
    builds = CMAKE_GLOBS + VCXPROJ_GLOBS + XCODE_GLOBS
    for path in files:
        if is_third_party(path) or path in HISTORY:
            continue
        ext = posixpath.splitext(path)[1].lower()
        is_doc = matches(path, DOCS)
        if path.endswith('.md') and not is_doc:
            continue
        if path.endswith(COMMENT_EXTS) or matches(path, builds) or posixpath.basename(path) == 'CMakeLists.txt' \
                or ext == '.tsv':
            continue
        if ext in ('.png', '.ico', '.bmp', '.jpg', '.gif', '.dll', '.exe', '.zip', '.ttf', '.wav', '.ogg', '.pdf') \
                or is_binary(repo, path):
            continue
        try:
            t = texts.get(path) or Text(repo, path)
        except (Fail, OSError):
            continue
        if not (full.search(t.text) or tail.search(t.text) or (dir_re and dir_re.search(t.text))):
            continue
        lines = t.text.split('\n')
        if is_doc:
            for i, l in enumerate(lines):
                hits = [o for _, o, _ in full.find(l)] + [o for _, o, _ in tail.find(l)]
                hits += [m.group(1) + '/' for m in dir_re.finditer(l)] if dir_re else []
                if hits:
                    rep.docs.append((path, i + 1, l.strip(), sorted(set(hits))))
            continue
        rewrite = path not in NO_REWRITE
        changed = 0
        for i, l in enumerate(lines):
            if rewrite:
                hits = list(full.find(l))
                for m, old, new in reversed(hits):
                    text = m.group(1).replace(os.path.splitext(old)[0], os.path.splitext(new)[0])
                    l = l[:m.start(1)] + text + l[m.end(1):]
                changed += len(hits)
                lines[i] = l
            listed = [o for _, o, _ in tail.find(l)] + ([] if rewrite else [o for _, o, _ in full.find(l)])
            if listed:
                rep.scripts_listed.append((path, i + 1, l.strip(), sorted(set(listed))))
        if changed:
            t.text = '\n'.join(lines)
            texts[path] = t
            rep.scripts[path] += changed


def comment_mentions(repo, files, texts, rep):
    pending = rep.pending
    if not pending:
        return
    kinds = mention_patterns(files, pending)
    for path in files:
        if is_third_party(path) or not path.endswith(COMMENT_EXTS) or path.endswith('.rc'):
            continue
        try:
            t = texts.get(path) or Text(repo, path)
        except Fail:
            continue
        lines = t.text.split('\n')
        found = []
        for i, l in enumerate(lines):
            if INCLUDE.match(l):
                continue
            for k in kinds:
                for m, old, new in k.find(l):
                    found.append((i, m.start(1), m.end(1), old, new))
        if not found:
            continue
        try:
            code_lines = sb.strip_code(t.text).split('\n')
        except sb.Fail:
            code_lines = [''] * len(lines)
        seen = set()
        for i, a, b, old, new in found:
            if i in seen or code_lines[i][a:b] == lines[i][a:b]:
                continue  # one entry per line; a name in code (not in a comment or a string) is not listed
            seen.add(i)
            rep.comments.append((pending.get(path, path), i + 1, lines[i].strip(), old, new))


# ---------------------------------------------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------------------------------------------

def apply(repo, texts, rep):
    for old in sorted(rep.pending):
        new = rep.pending[old]
        os.makedirs(os.path.dirname(os.path.join(repo, new)), exist_ok=True)
        git(repo, 'mv', old, new)
    for path, t in sorted(texts.items()):
        if t.text == t.original:
            continue
        dest = os.path.join(repo, rep.pending.get(path, path))
        tmp = dest + '.apply_moves.tmp'
        with open(tmp, 'wb') as f:
            f.write(t.data())
        os.replace(tmp, dest)
    for old in sorted(rep.pending, key=lambda p: -p.count('/')):
        d = os.path.dirname(os.path.join(repo, old))
        while os.path.isdir(d) and not os.listdir(d) and os.path.abspath(d) != os.path.abspath(repo):
            os.rmdir(d)
            d = os.path.dirname(d)


def short(s, n=150):
    return s if len(s) <= n else s[:n - 3] + '...'


def print_report(rep, dry, verbose):
    out = []
    w = out.append
    w('apply_moves: %s' % ('dry run, nothing written' if dry else 'applied'))
    w('rows: %d to move, %d already applied, %d folded away earlier (skipped)'
      % (len(rep.pending), len(rep.applied), len(rep.folded)))
    for r in rep.folded:
        w('  folded: %s -> %s (map line %d: %s)' % (r.old, r.new, r.line, r.note))
    dests = collections.Counter(posixpath.dirname(n) for n in rep.pending.values())
    renamed = sum(1 for o, n in rep.pending.items() if posixpath.basename(o) != posixpath.basename(n))
    w('files moved per destination folder (%d files, %d renamed):' % (len(rep.pending), renamed))
    for d, n in sorted(dests.items()):
        w('  %-36s %d' % (d, n))
    w('includes: %d lines rewritten in %d files' % (rep.include_lines, len(rep.include_files)))
    by_dir = collections.Counter()
    for p, n in rep.include_files.items():
        by_dir['/'.join(p.split('/')[:2])] += n
    for d, n in sorted(by_dir.items()):
        w('  %-36s %d lines' % (d, n))
    if verbose:
        for p in sorted(rep.include_files):
            w('    %s: %d' % (p, rep.include_files[p]))
    w('build files touched: %d' % len(rep.build))
    for p, n in sorted(rep.build.items()):
        w('  %-60s %d entries' % (p, n))
    for kind, table in (('filters added', rep.filters_added), ('filters removed', rep.filters_removed),
                        ('Xcode groups added', rep.groups_added), ('Xcode groups removed', rep.groups_removed)):
        for p, names in sorted(table.items()):
            w('  %s in %s: %s' % (kind, p, ', '.join(names)))
    w('scripts rewritten: %d' % len(rep.scripts))
    for p, n in sorted(rep.scripts.items()):
        w('  %s: %d' % (p, n))
    if rep.resorted:
        w('includes re-sorted for a new file name (main header): %d' % len(rep.resorted))
        for p in rep.resorted:
            w('  %s' % p)
    if rep.format_unclean:
        w('not format-clean after the include sort (were clean before): %d' % len(rep.format_unclean))
        for p in rep.format_unclean:
            w('  %s' % p)
    w('to review, scripts (not plain paths, or not rewritten): %d lines' % len(rep.scripts_listed))
    for p, n, l, hits in rep.scripts_listed:
        note = NO_REWRITE.get(p)
        w('  %s:%d: %s%s' % (p, n, short(l), ('  [%s]' % note) if note else ''))
    w('to review, documents: %d lines' % len(rep.docs))
    for p, n, l, hits in rep.docs:
        w('  %s:%d: %s' % (p, n, short(l)))
    w('to review, comments and strings in code: %d' % len(rep.comments))
    for p, n, l, old, new in rep.comments:
        w('  %s:%d: %s  [%s -> %s]' % (p, n, short(l, 110), old, new))
    w('only non-Windows builds compile: %d rewritten includes' % len(rep.platform))
    for p, n, a, b, why in rep.platform:
        w('  %s:%d: "%s" -> "%s"  (%s)' % (p, n, a, b, short(why, 80)))
    w('moved files with platform branches: %d' % len(rep.platform_moved))
    for p, why in rep.platform_moved:
        w('  %s  (%s)' % (p, why))
    w('stops: %d' % len(rep.stops))
    for s in rep.stops:
        w('  ' + s)
    print('\n'.join(out))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0].strip(),
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--map', required=True, help='the move map (.tsv), relative to the current folder or --repo')
    ap.add_argument('--repo', default=os.path.dirname(SCRIPT_DIR), help='the checkout to change')
    ap.add_argument('--only', nargs='+', default=[], help='new-path prefixes of the rows to apply')
    ap.add_argument('--exact', action='store_true', help='--only names folders: only their direct files')
    ap.add_argument('--dry-run', action='store_true', help='report, write nothing')
    ap.add_argument('--check-coverage', action='store_true', help='every file of COVERAGE_DIRS needs a row')
    ap.add_argument('--no-build-lists', action='store_true', help='leave the build descriptions alone')
    ap.add_argument('--no-format', action='store_true', help='do not re-sort the includes with clang-format')
    ap.add_argument('--verbose', action='store_true', help='list every file whose includes change')
    a = ap.parse_args()
    repo = os.path.abspath(a.repo)
    map_path = a.map if os.path.isfile(a.map) else os.path.join(repo, a.map)
    rep = Report()
    files = tracked_files(repo)
    rows = read_map(map_path)
    post = validate(repo, rows, files, a.only, a.exact, a.check_coverage, rep)
    texts = {}
    if not a.no_format and rep.pending:
        clang_format_exe()  # fails early when the pinned one is missing
    started = time.time()

    def phase(name):
        print('apply_moves: %s (%.0f s)' % (name, time.time() - started), file=sys.stderr, flush=True)

    phase('validated')
    rewrite_includes(repo, files, post, texts, rep, not a.no_format)
    platform_branches(repo, rep, texts)
    phase('includes')
    if not a.no_build_lists:
        update_build_lists(repo, files, texts, rep)
        phase('build lists')
    scan_scripts_docs(repo, files, texts, rep)
    comment_mentions(repo, files, texts, rep)
    phase('scripts, documents, comments')
    if rep.stops:
        print_report(rep, True, a.verbose)
        print('apply_moves: stopped, nothing written', file=sys.stderr)
        return 2
    if not a.dry_run:
        apply(repo, texts, rep)
    print_report(rep, a.dry_run, a.verbose)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (Fail, sb.Fail) as e:
        print('apply_moves: %s' % e, file=sys.stderr)
        sys.exit(2)
