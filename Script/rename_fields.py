#!/usr/bin/env python3
"""
Renames data members of C++ structs as the compiler resolves them, never by spelling (docs/refactor/PLAN.md 4.1
and 4.3: "IDE or clang-based renaming, never search-and-replace"). The output of a run is a [rename] commit that
Script/compare_objects.py proves identical in object code.

    python Script/rename_fields.py --map <file.tsv> [--build <dir with compile_commands.json>] [--repo <dir>]
                                   [--dry-run] [--strict] [--keep-going] [-j N] [--clang-format <exe>]
    python Script/rename_fields.py --self-test

The map has one rename per line, `Struct::old_name<TAB>NewName`, `#` comments and blank lines. Struct is the struct
or class that declares the field (a field used through a derived class is renamed by its declaring struct's line);
a qualified name (AGS::Common::Foo::bar) is accepted. Static data members count as data members. A line that names
something else (a method, a field or a struct that does not exist) is an error that names the line.

How it works:
  1. The translation units: the first-party sources of the compile database (Common, Engine, Compiler, Tools,
     Plugins; never libsrc, libinclude, _deps), and of those only the ones that can see a struct of the map. The
     include closure of every unit comes from clang-scan-deps (the real preprocessor, with the unit's flags); a unit
     is parsed when its closure has a file that may define a struct of that name (by text: `struct Name`, `class
     Name`, `union Name`). A field can only be named where its struct is complete, so the filter cannot drop a use.
  2. Each unit is parsed by libclang (the clang front end, in clang-cl mode for an MSVC database) in parallel, and
     its cursors in first-party files are walked: every field declaration, member expression (a.f, p->f, a bare f in
     a member function, this->f), member initializer, designated initializer, offsetof operand, pointer to member and
     static member reference whose referenced declaration is a field of the map is collected by its USR, with the
     position of its name token. A token that a macro argument carries is rewritten where it is spelled; a token of
     a macro body, or a member of a dependent expression in a template (the parser cannot tell which struct it
     names), goes to the "by hand" list and is not rewritten.
  3. Checks before anything is written: every map line names a data member; a new name must not collide with
     another member of the struct, of a base or of a derived class (two bases of one class that would both have the
     name included); a local, parameter or other declaration of the new name must not capture a rewritten bare
     reference in its scope; and the new member must not hide a name of the enclosing namespaces (a type, a
     function, a variable) that a member function of the struct or of a derived class uses unqualified. Each case
     is reported. A parse error in a unit is fatal unless --keep-going (a unit that does not parse cannot list its
     references).
  4. The edits of all lines are collected, deduplicated by file and offset (a header is seen by many units),
     applied to the bytes of each file (its encoding and line ends stay), the lines they touch are formatted with
     clang-format (a longer name re-wraps lines), and the file is replaced through a temporary file.

The report: per map line, the references rewritten by file (or "nothing to do" when the struct has the new name
and not the old one: a second run changes nothing); the "by hand" list; comments (and strings) that name an old
name beside the struct's name, an alias of it or a variable that the code uses with the field (ch->defview,
CharacterInfo::defview, chars[i].view, `defview`), for review, not rewritten; and the lines of first-party files
that no unit of the build compiles, or of #if branches that no parsed unit compiled, where an old name follows
`.`, `->` or `::`, found by text, as candidates for review. --dry-run prints the unified diff and the lists and
writes nothing; with --strict it exits 1 when the "by hand" list is not empty.

Exit code 0: done (or would be done); 1: errors (map, collisions, parse errors, --strict); 2: cannot run.
The last check is the compiler: after a run the tree must build, and a "not a member" error on a line the tool did
not list is a bug of the tool.

Needs libclang 19 with its Python bindings, clang-scan-deps 19 and clang-format 19.1.7. On Windows the compile
database is an MSVC one (Ninja, -DCMAKE_EXPORT_COMPILE_COMMANDS=ON, built once so that generated headers such as
SDL2's exist), and libclang.dll and clang-scan-deps.exe come from Visual Studio's "C++ Clang tools for Windows"
component (<VS>/VC/Tools/Llvm/x64/bin; --libclang or LIBCLANG overrides). The bindings, in a venv kept in .compare/:
    C:/ProgramData/miniconda3/python.exe -m venv .compare/renameenv
    .compare/renameenv/Scripts/python.exe -m pip install clang==19.1.7
    .compare/renameenv/Scripts/python.exe Script/rename_fields.py --build .tidy/build --map fields.tsv --dry-run
"""
import argparse
import collections
import ctypes
import difflib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import apply_moves  # noqa: E402  (clang_format_text: formats given lines of a text through stdin)
import compare_objects  # noqa: E402  (msvc_env: the VS developer environment)
import split_bindings as sb  # noqa: E402  (read_eol/write_text: line ends and BOM; strip_code: comments blanked)
import tidy_diff  # noqa: E402  (CHECKED_DIRS, THIRD_PARTY: what first-party code is)

REPO = os.path.abspath(os.path.join(SCRIPT_DIR, '..'))
SOURCES = ('.c', '.cc', '.cpp', '.cxx')
CODE_FILES = SOURCES + ('.h', '.hpp', '.inl', '.mm', '.m')
MAP_LINE = re.compile(r'^\s*((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)::([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*$')
CL_DRIVERS = ('cl', 'cl.exe', 'clang-cl', 'clang-cl.exe')
FUNCTION_KINDS = ('FUNCTION_DECL', 'CXX_METHOD', 'CONSTRUCTOR', 'DESTRUCTOR', 'FUNCTION_TEMPLATE', 'LAMBDA_EXPR',
                  'CONVERSION_FUNCTION')
DATA_KINDS = ('FIELD_DECL', 'VAR_DECL')
# Clang's flags that only change warnings: a /WX build must not stop the parse at one
QUIET = ['-Wno-everything', '-Wno-error', '-fno-color-diagnostics']


class Fail(Exception):
    pass


def is_first_party(rel):
    return rel.startswith(tidy_diff.CHECKED_DIRS) and not tidy_diff.THIRD_PARTY.search(rel) and rel.endswith(CODE_FILES)


def repo_files(repo):
    """{normcase(path): path} of the tracked and untracked (not ignored) files of the repository, '/'-separated."""
    out = subprocess.run(['git', '-C', repo, '-c', 'core.quotepath=off', 'ls-files', '--cached', '--others',
                          '--exclude-standard'], capture_output=True, text=True, encoding='utf-8', check=True).stdout
    return {os.path.normcase(p): p for p in out.splitlines() if p and os.path.isfile(os.path.join(repo, p))}


def rel_path(repo, files, name):
    """The repository path of a file name a tool reports, or None when the file is not one of the repository's."""
    full = os.path.abspath(name)
    root = os.path.abspath(repo)
    if os.path.normcase(full[:len(root) + 1]) != os.path.normcase(root + os.sep):
        return None
    return files.get(os.path.normcase(full[len(root) + 1:].replace(os.sep, '/')))


def read_bytes(repo, path):
    with open(os.path.join(repo, path), 'rb') as f:
        return f.read()


# ---------------------------------------------------------------------------------------------------------------
# The map
# ---------------------------------------------------------------------------------------------------------------

class MapLine:
    def __init__(self, number, text, struct, old, new):
        self.number, self.text, self.struct, self.old, self.new = number, text, struct, old, new
        self.short = struct.split('::')[-1]
        self.record = None   # USR of the struct, once resolved
        self.field = None    # USR of the field
        self.done = False    # the struct has the new name and not the old one

    def __str__(self):
        return '%s::%s -> %s' % (self.struct, self.old, self.new)


def read_map(path):
    lines, errors = [], []
    with open(path, encoding='utf-8') as f:
        for number, text in enumerate(f.read().splitlines(), 1):
            code = text.split('#', 1)[0]
            if not code.strip():
                continue
            m = MAP_LINE.match(code)
            if not m:
                errors.append('map line %d: expected Struct::old_name<TAB>NewName: %s' % (number, text.strip()))
                continue
            lines.append(MapLine(number, text.strip(), m.group(1), m.group(2), m.group(3)))
    seen = {}
    for line in lines:
        key = (line.struct, line.old)
        if key in seen:
            errors.append('map line %d: %s::%s is renamed on line %d already' % (line.number, line.struct, line.old,
                                                                                 seen[key]))
        seen[key] = line.number
    return lines, errors


# ---------------------------------------------------------------------------------------------------------------
# libclang and clang-scan-deps
# ---------------------------------------------------------------------------------------------------------------

def llvm_bin(env):
    """The bin directory of Visual Studio's LLVM component (VCINSTALLDIR of a developer environment), or None."""
    vc = env.get('VCINSTALLDIR')
    path = os.path.join(vc, 'Tools', 'Llvm', 'x64', 'bin') if vc else None
    return path if path and os.path.isdir(path) else None


def tool_env():
    """The environment for libclang and clang-scan-deps: on Windows that of a VS developer prompt (INCLUDE)."""
    if os.name != 'nt':
        return dict(os.environ)
    env = compare_objects.msvc_env()
    env.pop('CFLAGS', None)
    env.pop('CXXFLAGS', None)
    return env


def find_libclang(explicit, env):
    for cand in (explicit, os.environ.get('LIBCLANG')):
        if cand:
            return cand
    if os.name == 'nt':
        b = llvm_bin(env)
        return os.path.join(b, 'libclang.dll') if b and os.path.isfile(os.path.join(b, 'libclang.dll')) else None
    return ''  # the bindings' own search


def find_scan_deps(env):
    b = llvm_bin(env) if os.name == 'nt' else None
    if b and os.path.isfile(os.path.join(b, 'clang-scan-deps.exe')):
        return os.path.join(b, 'clang-scan-deps.exe')
    path = env.get('PATH') or env.get('Path')
    return shutil.which('clang-scan-deps', path=path) or shutil.which('clang-scan-deps-19', path=path)


_LOADED = {}


def load_libclang(library):
    """Loads libclang once per process and registers the functions the bindings leave out."""
    if 'cindex' in _LOADED:
        return _LOADED['cindex']
    try:
        from clang import cindex
    except ImportError:
        raise Fail('the libclang Python bindings are missing: pip install clang==19.1.7 (see the docstring)')
    if library:
        cindex.Config.set_library_file(library)
    lib = cindex.conf.lib

    class RangeList(ctypes.Structure):
        _fields_ = [('count', ctypes.c_uint), ('ranges', ctypes.POINTER(cindex.SourceRange))]

    lib.clang_getFileLocation.argtypes = [cindex.SourceLocation, ctypes.POINTER(cindex.c_object_p),
                                          ctypes.POINTER(ctypes.c_uint), ctypes.POINTER(ctypes.c_uint),
                                          ctypes.POINTER(ctypes.c_uint)]
    lib.clang_getFileLocation.restype = None
    lib.clang_getAllSkippedRanges.argtypes = [cindex.TranslationUnit]
    lib.clang_getAllSkippedRanges.restype = ctypes.POINTER(RangeList)
    lib.clang_disposeSourceRangeList.argtypes = [ctypes.POINTER(RangeList)]
    lib.clang_disposeSourceRangeList.restype = None
    _LOADED['cindex'] = cindex
    return cindex


def compile_jobs(cindex, build, repo, files):
    """[(repo path, absolute file, directory, [clang arguments])] of the database's first-party C and C++ units.

    The arguments are those of the build without the program and the source; an MSVC command gets clang-cl's
    driver mode, as clang-tidy gives it to the same database (Script/tidy_diff.py)."""
    try:
        commands = cindex.CompilationDatabase.fromDirectory(build).getAllCompileCommands()
    except cindex.CompilationDatabaseError:
        raise Fail('no usable compile_commands.json in %s' % build)
    jobs, seen = [], set()
    for cmd in commands:
        full = os.path.normpath(os.path.join(cmd.directory, cmd.filename))
        rel = rel_path(repo, files, full)
        if rel is None or not is_first_party(rel) or not rel.endswith(SOURCES) or rel in seen:
            continue
        seen.add(rel)
        args = list(cmd.arguments)
        rest = [a for a in args[1:]
                if os.path.normcase(os.path.normpath(os.path.join(cmd.directory, a))) != os.path.normcase(full)]
        if os.path.basename(args[0]).lower() in CL_DRIVERS:
            rest = ['--driver-mode=cl'] + rest
        jobs.append((rel, full, os.path.abspath(cmd.directory), rest))
    return jobs


def scan_deps(exe, env, jobs, workers, tmp):
    """{repo path of a unit: {normcase'd absolute paths it includes}}; a unit that failed to scan is missing."""
    entries = []
    for rel, full, directory, args in jobs:
        # clang-scan-deps reads the driver mode from the program's name
        if args and args[0] == '--driver-mode=cl':
            entries.append({'directory': directory, 'file': full, 'arguments': ['clang-cl'] + args[1:] + [full]})
        else:
            entries.append({'directory': directory, 'file': full, 'arguments': ['clang'] + args + [full]})
    path = os.path.join(tmp, 'compile_commands.json')
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(entries, f)
    r = subprocess.run([exe, '-compilation-database=' + path, '-j', str(workers), '-format=experimental-full'],
                       env=env, capture_output=True, text=True, encoding='utf-8', errors='replace')
    try:
        out = json.loads(r.stdout) if r.stdout.strip() else {}
    except ValueError:
        out = {}
    by_file = {os.path.normcase(full): rel for rel, full, directory, args in jobs}
    deps = {}
    for unit in out.get('translation-units', []):
        for command in unit.get('commands', []):
            rel = by_file.get(os.path.normcase(os.path.normpath(command.get('input-file', ''))))
            if rel is not None:
                deps[rel] = {os.path.normcase(os.path.normpath(p)) for p in command.get('file-deps', [])}
    return deps


# ---------------------------------------------------------------------------------------------------------------
# The walk of one translation unit (in a worker process)
# ---------------------------------------------------------------------------------------------------------------

_WORKER = {}


def worker_init(env, library, repo, files, old_names, new_names, struct_names):
    os.environ.clear()
    os.environ.update(env)
    _WORKER.update(cindex=load_libclang(library), repo=repo, files=files, old=frozenset(old_names),
                   new=frozenset(new_names), structs=frozenset(struct_names))


class Walker:
    """Collects what the parent needs to decide, for one translation unit (keyed by USR, never by spelling):

    refs     (field usr, path, offset, line, how, bare, captured, macro) per reference to a data member named like
             an old name: how is 'edit' (the name token is at offset in the file) or 'macro' (a token of the body
             of the macro used at offset); captured: the new names declared in the enclosing function scopes before
             a bare reference
    bases    {'path:line': identifier before the . or -> of a reference}, for the comment report
    hand     (path, offset, line, name, why) of dependent members named like an old name
    records  {usr: {name, qualname, path, line, bases, members: [(name, kind, usr)]}} of the classes that have a
             base or a name of the map, and of their bases
    aliases  {record usr: {typedef and alias names}} of the structs of the map
    ctxrefs  (class usr, referenced usr, its parent's usr, its parent's kind, its kind, path, line, name) of the
             unqualified uses of a new name inside a class's scope, for the hiding check
    """

    def __init__(self, tu):
        self.cindex = cindex = _WORKER['cindex']
        self.lib = cindex.conf.lib
        self.repo, self.files = _WORKER['repo'], _WORKER['files']
        self.old, self.new, self.structs = _WORKER['old'], _WORKER['new'], _WORKER['structs']
        self.tu = tu
        self.paths, self.texts = {}, {}
        self.refs, self.hand, self.ctxrefs = [], [], []
        self.bases = {}
        self.records, self.aliases = {}, collections.defaultdict(set)
        K = self.K = cindex.CursorKind
        self.REF_KINDS = {K.MEMBER_REF_EXPR, K.MEMBER_REF, K.DECL_REF_EXPR, K.FIELD_DECL, K.VAR_DECL}
        self.RECORDS = {K.STRUCT_DECL, K.CLASS_DECL, K.UNION_DECL, K.CLASS_TEMPLATE,
                        K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION}
        self.FUNCS = {K.FUNCTION_DECL, K.CXX_METHOD, K.CONSTRUCTOR, K.DESTRUCTOR, K.CONVERSION_FUNCTION,
                      K.FUNCTION_TEMPLATE}
        self.SCOPES = self.FUNCS | {K.LAMBDA_EXPR, K.COMPOUND_STMT, K.FOR_STMT, K.CXX_FOR_RANGE_STMT, K.IF_STMT,
                                    K.WHILE_STMT, K.SWITCH_STMT, K.CXX_CATCH_STMT}
        self.CONTAINERS = {K.TRANSLATION_UNIT, K.NAMESPACE, K.LINKAGE_SPEC, K.UNEXPOSED_DECL}
        self.CTX_REFS = {K.TYPE_REF, K.TEMPLATE_REF, K.DECL_REF_EXPR, K.OVERLOADED_DECL_REF}
        self.NOT_MEMBERS = {K.CXX_ACCESS_SPEC_DECL, K.CXX_BASE_SPECIFIER, K.FRIEND_DECL, K.STATIC_ASSERT}

    # -- locations and text

    def path_of(self, file_ptr):
        key = ctypes.cast(file_ptr, ctypes.c_void_p).value
        if key not in self.paths:
            name = self.cindex.File(file_ptr).name
            self.paths[key] = rel_path(self.repo, self.files, name) if name else None
        return self.paths[key]

    def file_location(self, loc):
        """(repo path or None, offset, line) of the file position of a location: where a macro argument's token is
        written, or the use of the macro for a token of its body (SourceManager::getFileLoc)."""
        f, line, col, off = self.cindex.c_object_p(), ctypes.c_uint(), ctypes.c_uint(), ctypes.c_uint()
        self.lib.clang_getFileLocation(loc, ctypes.byref(f), ctypes.byref(line), ctypes.byref(col),
                                       ctypes.byref(off))
        if not f:
            return None, 0, 0
        return self.path_of(f), off.value, line.value

    def text(self, path):
        if path not in self.texts:
            self.texts[path] = read_bytes(self.repo, path)
        return self.texts[path]

    def token_at(self, path, offset):
        m = re.match(rb'[A-Za-z_]\w*', self.text(path)[offset:offset + 256])
        return m.group(0).decode() if m else ''

    def before(self, path, offset):
        data = self.text(path)
        return data[data.rfind(b'\n', 0, offset) + 1:offset].decode('utf-8', 'replace')

    def after(self, path, offset):
        data = self.text(path)
        end = data.find(b'\n', offset)
        return data[offset:end if end >= 0 else len(data)].decode('utf-8', 'replace')

    # -- the walk

    def run(self):
        self.visit(self.tu.cursor, [], None, None)
        return self

    def first_party(self, cursor):
        f = cursor.location.file
        rel = rel_path(self.repo, self.files, f.name) if f is not None else None
        return rel is not None and is_first_party(rel)

    def visit(self, cursor, scopes, ctx, ctx_after):
        K = self.K
        kind = cursor.kind
        if kind in self.CONTAINERS:
            for child in cursor.get_children():
                if child.kind in self.CONTAINERS or self.first_party(child):
                    self.visit(child, scopes, ctx, ctx_after)
            return
        spelling = cursor.spelling
        if kind in self.RECORDS and cursor.is_definition():
            self.record(cursor)
            ctx, ctx_after = cursor.get_usr(), None
        elif kind in (K.TYPEDEF_DECL, K.TYPE_ALIAS_DECL):
            decl = cursor.underlying_typedef_type.get_declaration()
            if decl is not None and decl.kind in self.RECORDS and decl.spelling in self.structs:
                self.aliases[decl.get_usr()].add(spelling)
        elif kind in self.FUNCS:
            parent = cursor.semantic_parent
            if parent is not None and parent.kind in self.RECORDS:
                ctx = parent.get_usr()
                lexical = cursor.lexical_parent
                if lexical is None or lexical.get_usr() != ctx:
                    _, ctx_after, _ = self.file_location(cursor.location)  # class scope after the declarator's name
        if scopes and spelling in self.new and kind.is_declaration():
            scopes[-1].add(spelling)
        pushed = kind in self.SCOPES
        if pushed:
            scopes = scopes + [set()]
        if spelling in self.old and kind in self.REF_KINDS:
            self.reference(cursor, kind, spelling, scopes)
        elif not spelling and kind in (K.MEMBER_REF_EXPR, K.DECL_REF_EXPR):
            self.dependent(cursor)
        elif ctx is not None and kind in self.CTX_REFS and spelling:
            name = re.split(r'[\s:]+', spelling)[-1]
            if name in self.new:
                self.context_reference(cursor, name, ctx, ctx_after)
        for child in cursor.get_children():
            self.visit(child, scopes, ctx, ctx_after)

    def dependent(self, cursor):
        """A member expression that names no declaration (t.x with a dependent t in a template, T::x): libclang gives
        it no spelling, so its name is the last identifier of its extent; one named like an old name is listed."""
        if cursor.referenced is not None:
            return
        path, end, line = self.file_location(cursor.extent.end)
        if path is None:
            return
        m = re.search(r'([A-Za-z_]\w*)\s*$', self.before(path, end))
        if m and m.group(1) in self.old:
            self.hand.append((path, end - len(m.group(1)), line, m.group(1), 'a member of a dependent expression in '
                              'a template: the parser cannot tell which struct it names'))

    def reference(self, cursor, kind, name, scopes):
        K = self.K
        decl = cursor if kind in (K.FIELD_DECL, K.VAR_DECL) else cursor.referenced
        path, offset, line = self.file_location(cursor.location)
        if path is None:
            return
        if decl is None:
            if kind in (K.MEMBER_REF_EXPR, K.DECL_REF_EXPR):
                self.hand.append((path, offset, line, name, 'a member of a dependent expression in a template: the '
                                  'parser cannot tell which struct it names'))
            return
        if decl.kind not in (K.FIELD_DECL, K.VAR_DECL):
            return
        parent = decl.semantic_parent
        if parent is None or parent.kind not in self.RECORDS:
            return  # a variable, not a member
        usr = decl.get_usr()
        token = self.token_at(path, offset)
        if token != name:
            self.refs.append((usr, path, offset, line, 'macro', False, (), token))
            return
        bare = False
        if kind == K.MEMBER_REF_EXPR:
            bare = next(cursor.get_children(), None) is None  # libclang leaves out an implicit this
        elif kind == K.DECL_REF_EXPR:
            bare = not re.search(r'(::|\.|->)\s*$', self.before(path, offset))
        captured = tuple(sorted(set().union(*scopes))) if bare and scopes else ()
        m = re.search(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:->|\.)\s*$', self.before(path, offset))
        if m and kind == K.MEMBER_REF_EXPR:
            self.bases['%s:%d' % (path, line)] = m.group(1)
        self.refs.append((usr, path, offset, line, 'edit', bare, captured, ''))

    def context_reference(self, cursor, name, ctx, ctx_after):
        path, offset, line = self.file_location(cursor.location)
        if path is None or self.token_at(path, offset) != name:
            return
        if ctx_after is not None and offset <= ctx_after:
            return  # before the declarator's name of an out-of-line member: namespace scope
        if re.search(r'(::|\.|->|~)\s*$|\b(class|struct|union|enum|typename|namespace)\s+$', self.before(path, offset)):
            return  # qualified, or a lookup that a data member cannot hide
        if self.after(path, offset + len(name)).lstrip().startswith('::'):
            return  # a nested-name-specifier: only types and namespaces are looked up there
        ref = cursor.referenced
        if ref is None:
            return
        parent = ref.semantic_parent
        self.ctxrefs.append((ctx, ref.get_usr(), parent.get_usr() if parent is not None else '',
                             parent.kind.name if parent is not None else '', ref.kind.name, path, line, name))

    def record(self, cursor):
        usr = cursor.get_usr()
        if usr in self.records:
            return
        bases = []
        for child in cursor.get_children():
            if child.kind == self.K.CXX_BASE_SPECIFIER:
                ref = child.referenced
                if ref is not None:
                    ref = ref.get_definition() or ref
                    bases.append(ref.get_usr())
                    if ref.is_definition():
                        self.record(ref)
                        self.records.setdefault(ref.get_usr(), self.describe(ref, []))
        if bases or cursor.spelling in self.structs:
            self.records[usr] = self.describe(cursor, bases)

    def describe(self, cursor, bases):
        members = []
        self.members(cursor, members)
        path, _, line = self.file_location(cursor.location)
        return {'name': cursor.spelling, 'qualname': self.qualname(cursor), 'path': path, 'line': line,
                'bases': bases, 'members': members}

    def members(self, cursor, out):
        for child in cursor.get_children():
            kind = child.kind
            if kind in self.NOT_MEMBERS or not kind.is_declaration():
                continue
            if kind in self.RECORDS and child.is_anonymous():
                self.members(child, out)  # the fields of an anonymous struct or union are the class's
                continue
            if kind == self.K.ENUM_DECL and not child.is_scoped_enum():
                self.members(child, out)  # unscoped enumerators are names of the class
            if child.spelling:
                out.append((child.spelling, kind.name, child.get_usr()))

    def qualname(self, cursor):
        parts = []
        c = cursor
        while c is not None and c.kind != self.K.TRANSLATION_UNIT:
            if c.spelling:
                parts.append(c.spelling)
            c = c.semantic_parent
        return '::'.join(reversed(parts))

    def skipped(self):
        """{path: [(first line, last line)]} of the #if branches that this unit did not compile."""
        out = collections.defaultdict(list)
        ranges = self.lib.clang_getAllSkippedRanges(self.tu)
        if not ranges:
            return out
        try:
            for i in range(ranges.contents.count):
                r = ranges.contents.ranges[i]
                path, _, first = self.file_location(r.start)
                _, _, last = self.file_location(r.end)
                if path is not None:
                    out[path].append((first, last))
        finally:
            self.lib.clang_disposeSourceRangeList(ranges)
        return out


def scan_unit(job):
    rel, full, directory, args = job
    cindex = _WORKER['cindex']
    start = time.time()
    os.chdir(directory)
    try:
        # the detailed preprocessing record keeps the skipped #if branches
        tu = cindex.Index.create().parse(full, args=args + QUIET,
                                         options=cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    except cindex.TranslationUnitLoadError as e:
        return {'unit': rel, 'errors': ['libclang could not parse it: %s' % e]}
    errors = ['%s:%d: %s' % (d.location.file.name if d.location.file else '?', d.location.line, d.spelling)
              for d in tu.diagnostics if d.severity >= cindex.Diagnostic.Error]
    w = Walker(tu).run()
    return {'unit': rel, 'errors': errors, 'refs': w.refs, 'hand': w.hand, 'bases': w.bases,
            'records': w.records, 'aliases': {k: sorted(v) for k, v in w.aliases.items()}, 'ctxrefs': w.ctxrefs,
            'skipped': dict(w.skipped()), 'seconds': time.time() - start}


# ---------------------------------------------------------------------------------------------------------------
# Decisions (in the parent process)
# ---------------------------------------------------------------------------------------------------------------

class Run:
    def __init__(self, repo):
        self.repo = repo
        self.errors = []
        self.records = {}
        self.aliases = collections.defaultdict(set)
        self.refs = {}       # (path, offset) -> (usr, how, line, macro)
        self.bare = []       # (usr, path, line, captured) of bare references with new names in scope
        self.hand = {}       # (path, line) -> (name, why)
        self.base_at = {}    # 'path:line' -> identifier before . or ->
        self.ctxrefs = set()
        self.skipped = {}    # path -> lines skipped in every parsed unit that includes the file
        self.units = 0
        self.seconds = 0.0

    def merge(self, result, unit_files):
        self.units += 1
        self.seconds += result.get('seconds', 0.0)
        for usr, data in result.get('records', {}).items():
            self.records.setdefault(usr, data)
        for usr, names in result.get('aliases', {}).items():
            self.aliases[usr].update(names)
        for usr, path, offset, line, how, bare, captured, macro in result.get('refs', []):
            self.refs[(path, offset)] = (usr, how, line, macro)
            if bare and captured:
                self.bare.append((usr, path, line, captured))
        for path, offset, line, name, why in result.get('hand', []):
            self.hand[(path, line)] = (name, why)
        self.base_at.update(result.get('bases', {}))
        self.ctxrefs.update(tuple(x) for x in result.get('ctxrefs', []))
        skipped = result.get('skipped', {})
        for path in set(unit_files) | set(skipped):
            lines = set()
            for a, b in skipped.get(path, ()):
                lines.update(range(a, b + 1))
            self.skipped[path] = self.skipped[path] & lines if path in self.skipped else lines

    def ancestors(self, usr, out=None):
        out = set() if out is None else out
        for b in self.records.get(usr, {}).get('bases', []):
            if b not in out:
                out.add(b)
                self.ancestors(b, out)
        return out

    def descendants(self, usr):
        children = collections.defaultdict(set)
        for u, data in self.records.items():
            for b in data['bases']:
                children[b].add(u)
        out, todo = set(), [usr]
        while todo:
            for c in children[todo.pop()]:
                if c not in out:
                    out.add(c)
                    todo.append(c)
        return out

    def resolve(self, lines):
        """Finds each line's struct and field among the records the units saw; an error names the line."""
        for line in lines:
            matches = sorted({u for u, d in self.records.items()
                              if d['qualname'] == line.struct or d['qualname'].endswith('::' + line.struct)})
            exact = [u for u in matches if self.records[u]['qualname'] == line.struct]
            if exact:
                matches = exact  # a name that is a record's whole name beats the same name inside a namespace
            if not matches:
                self.errors.append('map line %d (%s): no struct or class %s in the parsed code' %
                                   (line.number, line.text, line.struct))
                continue
            if len(matches) > 1:
                self.errors.append('map line %d (%s): %s is ambiguous (%s): qualify it' % (
                    line.number, line.text, line.struct, ', '.join(self.records[u]['qualname'] for u in matches)))
                continue
            line.record = matches[0]
            members = self.records[line.record]['members']
            old = [m for m in members if m[0] == line.old]
            if old and old[0][1] in DATA_KINDS:
                line.field = old[0][2]
            elif old:
                self.errors.append('map line %d (%s): %s::%s is not a data member (%s)' % (
                    line.number, line.text, line.struct, line.old, old[0][1].lower().replace('_', ' ')))
            elif any(m[0] == line.new and m[1] in DATA_KINDS for m in members):
                line.done = True
            else:
                self.errors.append('map line %d (%s): %s has no data member %s' % (line.number, line.text,
                                                                                     line.struct, line.old))

    def check_collisions(self, lines):
        active = [line for line in lines if line.field]
        renamed = {line.field: line.new for line in active}
        reported = set()
        for line in active:
            family = {line.record} | self.descendants(line.record)
            for derived in sorted(family):
                for cls in [derived] + sorted(self.ancestors(derived)):
                    data = self.records.get(cls)
                    if data is None:
                        continue
                    clash = [m for m in data['members'] if renamed.get(m[2], m[0]) == line.new and m[2] != line.field]
                    if data['name'] == line.new:
                        clash.append((data['name'], 'class name', cls))
                    for m in clash:
                        key = tuple(sorted((line.field, m[2])))
                        if key in reported:
                            continue
                        reported.add(key)
                        other = [o for o in active if o.field == m[2]]
                        what = 'the new name of map line %d' % other[0].number if other else \
                            '%s %s::%s' % (m[1].lower().replace('_', ' '), data['qualname'], m[0])
                        where = '' if derived == line.record else ' (both visible in %s)' % \
                            self.records[derived]['qualname']
                        self.errors.append('map line %d (%s): %s is %s at %s:%d%s' % (
                            line.number, line.text, line.new, what, data['path'], data['line'], where))
        by_field = {line.field: line for line in active}
        seen = set()
        for usr, path, number, captured in self.bare:
            line = by_field.get(usr)
            if line and line.new in captured and (path, number) not in seen:
                seen.add((path, number))
                self.errors.append('map line %d (%s): at %s:%d a local or parameter named %s would capture the '
                                   'renamed member' % (line.number, line.text, path, number, line.new))
        for line in active:
            family = {line.record} | self.descendants(line.record)
            for ctx, ref, parent, parent_kind, kind, path, number, name in sorted(self.ctxrefs):
                if name != line.new or ctx not in family:
                    continue
                if parent in ({ctx} | self.ancestors(ctx)) or parent_kind in FUNCTION_KINDS or kind == 'PARM_DECL':
                    continue  # a member (checked above), or a local, which hides the member and is checked above
                self.errors.append('map line %d (%s): at %s:%d %s names a %s outside the class, which the new member '
                                   'would hide' % (line.number, line.text, path, number, name,
                                                   kind.lower().replace('_', ' ')))


def struct_files(repo, files, names):
    """{name: [repo paths]} of the first-party files where a struct, class or union of that name may be defined."""
    found = collections.defaultdict(list)
    pattern = re.compile(r'\b(?:struct|class|union)\s+(?:[A-Za-z_]\w*\s+)*?(%s)\b(?!\s*;)' %
                         '|'.join(map(re.escape, sorted(names))))
    for path in files.values():
        if is_first_party(path):
            for m in pattern.finditer(read_bytes(repo, path).decode('utf-8', 'replace')):
                if path not in found[m.group(1)]:
                    found[m.group(1)].append(path)
    return found


# ---------------------------------------------------------------------------------------------------------------
# Edits and reports
# ---------------------------------------------------------------------------------------------------------------

def plan_edits(run, lines):
    """{path: [(offset, old, new)]} and {map line number: Counter(path)}; macro-body uses go to run.hand."""
    by_field = {line.field: line for line in lines if line.field}
    edits = collections.defaultdict(list)
    counts = collections.defaultdict(collections.Counter)
    for (path, offset), (usr, how, number, macro) in sorted(run.refs.items()):
        line = by_field.get(usr)
        if line is None:
            continue
        if how == 'edit':
            edits[path].append((offset, line.old, line.new))
            counts[line.number][path] += 1
        else:
            run.hand[(path, number)] = (line.old, 'in the body of the macro %s used here (defined at %s)' % (
                macro, macro_definition(run.repo, macro)))
    return edits, counts


_DEFINES = {}


def macro_definition(repo, name):
    """Where a macro is defined, by text (for the report only)."""
    if name not in _DEFINES:
        _DEFINES[name] = 'no first-party file'
        pattern = re.compile(rb'^[ \t]*#[ \t]*define[ \t]+' + re.escape(name.encode()) + rb'\b', re.M)
        for path in sorted(repo_files(repo).values()):
            if is_first_party(path):
                data = read_bytes(repo, path)
                m = pattern.search(data)
                if m:
                    _DEFINES[name] = '%s:%d' % (path, data.count(b'\n', 0, m.start()) + 1)
                    break
    return _DEFINES[name]


def apply_edits(repo, path, items):
    """(old text, new text) of a file: its edits applied to its bytes, the lines they touch clang-formatted."""
    data = read_bytes(repo, path)
    out = data
    touched = set()
    for offset, old, new in sorted(set(items), reverse=True):
        if out[offset:offset + len(old)] != old.encode():
            raise Fail('%s: the text at offset %d is not %s (did the file change during the run?)' % (path, offset,
                                                                                                     old))
        out = out[:offset] + new.encode() + out[offset + len(old):]
        touched.add(data.count(b'\n', 0, offset) + 1)
    ranges = []
    for n in sorted(touched):
        if ranges and ranges[-1][1] >= n - 1:
            ranges[-1] = (ranges[-1][0], n)
        else:
            ranges.append((n, n))
    try:
        text = apply_moves.clang_format_text(repo, path, out.decode('utf-8'), ranges)
    except apply_moves.Fail as e:
        raise Fail(str(e))
    return data.decode('utf-8'), text


def write_file(repo, path, text):
    """Writes through a temporary file, with the file's line ends and BOM (split_bindings.write_text)."""
    full = os.path.join(repo, path)
    _, eol, bom = sb.read_eol(full)
    sb.write_text(full, text.lstrip('\ufeff').replace('\r\n', '\n'), eol, bom)


def comment_report(run, lines, files):
    """(path, line, text) of comments and strings that name an old name next to its struct's name, an alias, a
    variable that the code uses with that field, or alone in backquotes."""
    bases = collections.defaultdict(set)
    for (path, offset), (usr, how, number, macro) in run.refs.items():
        base = run.base_at.get('%s:%d' % (path, number))
        if base:
            bases[usr].add(base)
    patterns = []
    for line in lines:
        if line.field:
            names = {run.records[line.record]['name']} | run.aliases.get(line.record, set()) | bases[line.field]
            patterns.append(re.compile(r'\b(?:%s)\s*(?:\[[^\]]*\])?\s*(?:->|\.|::)\s*%s\b|`%s`' % (
                '|'.join(sorted(map(re.escape, names))), re.escape(line.old), re.escape(line.old))))
    found = []
    for path in sorted(files.values()):
        if not patterns or not is_first_party(path):
            continue
        text = read_bytes(run.repo, path).decode('utf-8', 'replace').replace('\r\n', '\n')
        if not any(p.search(text) for p in patterns):
            continue
        try:
            code_lines = sb.strip_code(text).split('\n')
        except sb.Fail:
            continue
        for number, l in enumerate(text.split('\n'), 1):
            for p in patterns:
                if any(code_lines[number - 1][m.start():m.end()] != m.group(0) for m in p.finditer(l)):
                    found.append((path, number, l.strip()))
                    break
    return found


def uncompiled_report(run, lines, files, compiled):
    """(path, line, text, why) of the lines that no parsed unit compiled where an old name follows . -> or ::"""
    olds = sorted({line.old for line in lines if line.field})
    if not olds:
        return []
    pattern = re.compile(r'(?:\.|->|::)\s*(?:%s)\b' % '|'.join(map(re.escape, olds)))
    found = []
    for path in sorted(files.values()):
        if not is_first_party(path):
            continue
        never = path not in compiled
        skipped = run.skipped.get(path, set())
        if not never and not skipped:
            continue
        text = read_bytes(run.repo, path).decode('utf-8', 'replace').replace('\r\n', '\n')
        if not pattern.search(text):
            continue
        try:
            code_lines = sb.strip_code(text).split('\n')
        except sb.Fail:
            code_lines = text.split('\n')
        all_lines = text.split('\n')
        for number, l in enumerate(code_lines, 1):
            if pattern.search(l) and (never or number in skipped):
                found.append((path, number, all_lines[number - 1].strip(),
                              'no unit of the build compiles the file' if never else
                              'an #if branch that no parsed unit compiled'))
    return found


def find_clang_format(explicit, repo):
    """clang-format 19: --clang-format, AGS_CLANG_FORMAT, or .compare/cfenv of the repository or of this script's."""
    cands = [explicit, os.environ.get('AGS_CLANG_FORMAT')]
    for root in (repo, REPO):
        cands += [os.path.join(root, '.compare', 'cfenv', 'Scripts', 'clang-format.exe'),
                  os.path.join(root, '.compare', 'cfenv', 'bin', 'clang-format')]
    for cand in cands:
        if cand and (os.path.isfile(cand) or shutil.which(cand)):
            os.environ['AGS_CLANG_FORMAT'] = cand
            try:
                return sb.clang_format()  # checks the version
            except sb.Fail:
                continue
    return None


def read_line(repo, path, number):
    lines = read_bytes(repo, path).decode('utf-8', 'replace').split('\n')
    return lines[number - 1].strip() if 0 < number <= len(lines) else ''


# ---------------------------------------------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------------------------------------------

def rename(args, out=print):
    """Runs the tool with parsed arguments; returns the exit code. out receives the report's lines."""
    started = time.time()
    repo = os.path.abspath(args.repo)
    build = os.path.abspath(args.build or os.path.join(repo, '.tidy', 'build'))
    lines, errors = read_map(args.map)
    for e in errors:
        out('error: ' + e)
    if errors:
        return 1
    if not lines:
        out('error: the map has no lines')
        return 1
    clang_format = find_clang_format(args.clang_format, repo)
    if clang_format is None:
        out('clang-format 19 not found: pass --clang-format or set AGS_CLANG_FORMAT')
        return 2
    os.environ['AGS_CLANG_FORMAT'] = clang_format
    apply_moves._CLANG_FORMAT[:] = [clang_format]

    env = tool_env()
    library = find_libclang(args.libclang, env)
    if library is None:
        out('libclang not found: install Visual Studio\'s "C++ Clang tools for Windows" or pass --libclang')
        return 2
    scan = find_scan_deps(env)
    try:
        cindex = load_libclang(library)
        files = repo_files(repo)
        jobs = compile_jobs(cindex, build, repo, files)
    except Fail as e:
        out(str(e))
        return 2

    names = {line.short for line in lines}
    defined = struct_files(repo, files, names)
    missing = [line for line in lines if not defined.get(line.short)]
    for line in missing:
        out('error: map line %d (%s): no first-party file defines a struct, class or union %s' % (
            line.number, line.text, line.short))
    if missing:
        return 1
    deps = {}
    if scan:
        tmp = tempfile.mkdtemp(prefix='rename_fields_')
        try:
            deps = scan_deps(scan, env, jobs, args.jobs, tmp)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    else:
        out('warning: clang-scan-deps not found: every unit is parsed')
    unit_files = {}
    for rel, full_paths in deps.items():
        unit_files[rel] = {p for p in (rel_path(repo, files, f) for f in full_paths) if p is not None}
    compiled = set().union(*unit_files.values()) if unit_files else set()
    compiled |= {j[0] for j in jobs}
    if len(unit_files) < len(jobs):
        # without every unit's includes a header cannot be said to be compiled by none: only sources are listed
        compiled |= {p for p in files.values() if not p.endswith(SOURCES + ('.mm', '.m'))}
    wanted = {p for ps in defined.values() for p in ps}
    selected = [j for j in jobs if j[0] not in unit_files or unit_files[j[0]] & wanted]
    unscanned = [j[0] for j in jobs if j[0] not in unit_files]
    out('rename_fields: %d map line(s); %d first-party units in the database, %d can see the structs%s' % (
        len(lines), len(jobs), len(selected), ' (%d could not be scanned and are parsed)' % len(unscanned)
        if scan and unscanned else ''))

    run = Run(repo)
    parse_errors = []
    old_names = {line.old for line in lines}
    with ProcessPoolExecutor(max_workers=args.jobs, initializer=worker_init,
                             initargs=(env, library, repo, files, old_names, {line.new for line in lines},
                                       names)) as pool:
        for result in pool.map(scan_unit, selected):
            if result['errors']:
                parse_errors.append((result['unit'], result['errors']))
            run.merge(result, unit_files.get(result['unit'], ()))
    for unit, errs in parse_errors:
        out('%s %s: %d error(s) in the parse:' % ('warning:' if args.keep_going else 'error:', unit, len(errs)))
        for e in errs[:5]:
            out('    ' + e)
    if parse_errors and not args.keep_going:
        out('nothing written: a unit that does not parse cannot list its references (generated headers missing? '
            'build the tree once) or pass --keep-going')
        return 1

    run.resolve(lines)
    run.check_collisions(lines)
    if run.errors:
        for e in run.errors:
            out('error: ' + e)
        out('nothing written: %d error(s)' % len(run.errors))
        return 1

    edits, counts = plan_edits(run, lines)
    results = {}
    try:
        for path, items in sorted(edits.items()):
            results[path] = apply_edits(repo, path, items)
    except Fail as e:
        out('error: ' + str(e))
        return 1
    for path, (before, after) in sorted(results.items()):
        if args.dry_run:
            out(''.join(difflib.unified_diff(before.replace('\r\n', '\n').splitlines(True),
                                             after.replace('\r\n', '\n').splitlines(True),
                                             'a/' + path, 'b/' + path)).rstrip('\n'))
        elif before != after:
            write_file(repo, path, after)

    out('')
    for line in lines:
        if line.done:
            out('%s: nothing to do (the struct has %s and no %s)' % (line, line.new, line.old))
            continue
        out('%s: %d reference(s) in %d file(s)' % (line, sum(counts[line.number].values()),
                                                  len(counts[line.number])))
        for path, n in sorted(counts[line.number].items()):
            out('    %4d  %s' % (n, path))
    out('')
    out('by hand, not rewritten (%d):' % len(run.hand))
    for (path, number), (name, why) in sorted(run.hand.items()):
        out('    %s:%d: %s: %s\n        %s' % (path, number, name, why, read_line(repo, path, number)))
    comments = comment_report(run, lines, files)
    out('comments and strings that name an old name, for review (%d):' % len(comments))
    for path, number, text in comments:
        out('    %s:%d: %s' % (path, number, text))
    uncompiled = uncompiled_report(run, lines, files, compiled)
    out('code no parsed unit compiled, .old ->old ::old by text, for review (%d):' % len(uncompiled))
    for path, number, text, why in uncompiled:
        out('    %s:%d: %s  [%s]' % (path, number, text, why))
    if not args.dry_run:
        for path in results:
            if not sb.format_clean(repo, path):
                out('warning: %s is not clang-format clean (was it before the run?)' % path)
    out('')
    out('%d units parsed in %.0f s (%.0f s of parse and walk, %d jobs); %d file(s) %s' % (
        run.units, time.time() - started, run.seconds, args.jobs, len(results),
        'would change' if args.dry_run else 'changed'))
    return 1 if args.dry_run and args.strict and run.hand else 0


# ---------------------------------------------------------------------------------------------------------------
# Self-test
# ---------------------------------------------------------------------------------------------------------------

SELF_TEST_HEADER = '''\
#pragma once

#include <cstddef>

#define FLAGS_OF(p) ((p)->flags & 1)
#define SAME(v)     (v)

struct Room
{
    int number = 0;
};

// A has a field x; B has one too (never renamed); `x` is the field
struct A
{
    int x = 0;
    int y = 0;
    int flags = 0;
    int view = 0;
    int room = 0;
    int name = 0; // \u00e4: the file is UTF-8
    static int count;

    A() : x(1), flags(2) {}
    int get_x() const { return x; }
    int sum(int x) const { return x + this->x; } // the parameter x is not the field
    void set_view(int View) { view = View; }
    void method() {}
    Room *where();
};

struct B
{
    int x = 0;
};

struct Pod
{
    int x;
    int y;
    int z;
};

struct Other
{
    int Shared = 0;
    int RuntimeFlags = 0;
};

struct D : A, Other
{
    int twice() const { return x * 2 + flags; }
};

template <typename T>
int dependent_x(const T &t)
{
    return t.x;
}

template <int N>
int fixed_x(const A &a)
{
    return a.x + N;
}
'''

SELF_TEST_SOURCE = '''\
#include "a.h"

int A::count = 0;

Room *A::where()
{
    static Room r;
    r.number = room;
    return &r;
}

int use(A &a, B &b, D &d)
{
    int x = a.x + b.x; // a.x is the field, b.x another struct's, x a local
    A *p = &a;
    p->x += d.x;
    int A::*member = &A::x;
    std::size_t at = offsetof(A, x);
    decltype(a.x) copy = sizeof(a.x);
    auto lambda = [&]() { return a.x + p->flags; };
    Pod pod = {.x = 3, .y = 4};
    const char *text = "a.x";
    return x + a.*member + int(at) + copy + lambda() + pod.x + pod.y + SAME(a.x) + FLAGS_OF(p) + dependent_x(a) +
           fixed_x<1>(a) + A::count + a.name + d.twice() + d.Shared + d.RuntimeFlags + text[0];
}
'''

SELF_TEST_OTHER = '''\
#include "a.h"

int other(A &a)
{
#if 0
    return a.x;
#else
    return a.y;
#endif
}
'''

SELF_TEST_PLATFORM = '''\
#include "a.h"

int platform(A *ch)
{
    return ch->x; // a file no unit compiles
}
'''

SELF_TEST_MAP = '''\
# the renames of the self-test
A::x\tX
A::flags\tFlags
A::name\tName
A::count\tCount
Pod::y\tY
Pod::z\tDepth
'''

SELF_TEST_WRAP = '''\
#include "a.h"

int wrap(Pod &pod)
{
    return pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.z + pod.x;
}
'''

SELF_TEST_BAD_MAP = '''\
A::view\tView
A::room\tRoom
A::flags\tShared
A::y\tPos
A::name\tPos
A::method\tMethod
A::nothing\tNothing
'''


def self_test():
    """Runs the tool on generated sources: the refused maps, a dry run, a run whose result is compared with the
    expected text and compiled, and a second run that changes nothing."""
    env = tool_env()
    library = find_libclang(None, env)
    if library is None:
        print('self-test: libclang not found')
        return 2
    root = tempfile.mkdtemp(prefix='rename_fields_selftest_')
    failures = []

    def check(cond, what):
        print('%s %s' % ('ok  ' if cond else 'FAIL', what))
        if not cond:
            failures.append(what)

    try:
        os.makedirs(os.path.join(root, 'Engine'))
        files = {'Engine/a.h': SELF_TEST_HEADER, 'Engine/use.cpp': SELF_TEST_SOURCE,
                 'Engine/other.cpp': SELF_TEST_OTHER, 'Engine/platform_other.cpp': SELF_TEST_PLATFORM,
                 'Engine/wrap.cpp': SELF_TEST_WRAP}
        for path, text in files.items():
            with open(os.path.join(root, path), 'wb') as f:
                f.write(text.replace('\n', '\r\n').encode('utf-8'))
        shutil.copy(os.path.join(REPO, '.clang-format'), os.path.join(root, '.clang-format'))
        subprocess.run(['git', 'init', '-q', root], check=True)
        build = os.path.join(root, 'build')
        os.makedirs(build)
        cl = os.name == 'nt'
        compiler = shutil.which('cl.exe', path=env.get('PATH') or env.get('Path')) if cl else 'c++'
        flags = ['/nologo', '/TP', '/std:c++20', '/EHsc', '/c', '/I' + os.path.join(root, 'Engine')] if cl else \
            ['-std=c++20', '-c', '-I' + os.path.join(root, 'Engine')]
        db = [{'directory': build, 'file': os.path.join(root, 'Engine', name),
               'arguments': [compiler] + flags + [os.path.join(root, 'Engine', name)]}
              for name in ('use.cpp', 'other.cpp', 'wrap.cpp')]
        with open(os.path.join(build, 'compile_commands.json'), 'w', encoding='utf-8') as f:
            json.dump(db, f, indent=1)
        maps = {}
        for name, text in (('good', SELF_TEST_MAP), ('bad', SELF_TEST_BAD_MAP), ('nostruct', 'Nope::x\tX\n')):
            maps[name] = os.path.join(root, name + '.tsv')
            with open(maps[name], 'w', encoding='utf-8') as f:
                f.write(text)
        clang_format = find_clang_format(None, REPO)
        if clang_format is None:
            print('self-test: clang-format 19 not found (AGS_CLANG_FORMAT)')
            return 2

        def tool(name, dry=False, strict=False):
            ns = argparse.Namespace(map=maps[name], build=build, repo=root, dry_run=dry, strict=strict,
                                    keep_going=False, jobs=2, clang_format=clang_format, libclang=library)
            report = []
            code = rename(ns, out=report.append)
            print('\n'.join('    | ' + r for r in '\n'.join(report).split('\n')))
            return code, '\n'.join(report)

        def snapshot():
            return {path: read_bytes(root, path) for path in files}

        before = snapshot()
        code, report = tool('nostruct')
        check(code == 1 and 'no first-party file defines a struct, class or union Nope' in report,
              'a struct that does not exist is refused')
        code, report = tool('bad')
        check(code == 1, 'the bad map is refused')
        check(snapshot() == before, 'the bad map writes nothing')
        for needle in ('a local or parameter named View would capture the renamed member',
                       'Room names a struct decl outside the class',
                       'Shared is field decl Other::Shared', 'both visible in D',
                       'Pos is the new name of map line 5',
                       'A::method is not a data member', 'A has no data member nothing'):
            check(needle in report, 'the bad map reports: ' + needle)

        code, report = tool('good', dry=True, strict=True)
        check(code == 1, '--dry-run --strict fails on the "by hand" list')
        check(snapshot() == before, '--dry-run writes nothing')
        check('-    int x = 0;' in report and '+    int X = 0;' in report, '--dry-run prints the diff')

        code, report = tool('good')
        check(code == 0, 'the map runs')
        after = snapshot()
        expected = {
            'Engine/a.h': SELF_TEST_HEADER
            .replace('    int x = 0;\n    int y', '    int X = 0;\n    int y')
            .replace('    int flags = 0;', '    int Flags = 0;')
            .replace('    int name = 0;', '    int Name = 0;')
            .replace('static int count;', 'static int Count;')
            .replace('A() : x(1), flags(2) {}', 'A() : X(1), Flags(2) {}')
            .replace('{ return x; }', '{ return X; }')
            .replace('return x + this->x;', 'return x + this->X;')
            .replace('return x * 2 + flags;', 'return X * 2 + Flags;')
            .replace('return a.x + N;', 'return a.X + N;')
            .replace('    int x;\n    int y;\n    int z;', '    int x;\n    int Y;\n    int Depth;'),
            'Engine/use.cpp': SELF_TEST_SOURCE
            .replace('int A::count', 'int A::Count')
            .replace('int x = a.x + b.x;', 'int x = a.X + b.x;')
            .replace('p->x += d.x;', 'p->X += d.X;')
            .replace('&A::x;', '&A::X;')
            .replace('offsetof(A, x)', 'offsetof(A, X)')
            .replace('decltype(a.x) copy = sizeof(a.x);', 'decltype(a.X) copy = sizeof(a.X);')
            .replace('return a.x + p->flags;', 'return a.X + p->Flags;')
            .replace('{.x = 3, .y = 4}', '{.x = 3, .Y = 4}')
            .replace('pod.y + SAME(a.x)', 'pod.Y + SAME(a.X)')
            .replace('A::count + a.name', 'A::Count + a.Name'),
            'Engine/other.cpp': SELF_TEST_OTHER,
            'Engine/platform_other.cpp': SELF_TEST_PLATFORM,
        }
        wrapped = after['Engine/wrap.cpp'].decode()
        check(wrapped.count('pod.Depth') == 12 and 'pod.z' not in wrapped and 'pod.x;' in wrapped and
              max(len(l) for l in wrapped.split('\r\n')) <= 120 and wrapped.count('\r\n') == wrapped.count('\n') and
              sb.format_clean(root, 'Engine/wrap.cpp'), 'a longer name re-wraps the line (clang-format, CRLF kept)')
        for path, text in expected.items():
            want = text.replace('\n', '\r\n').encode('utf-8')
            check(after[path] == want, '%s is the expected text, CRLF and UTF-8 kept' % path)
            if after[path] != want:
                print(''.join(difflib.unified_diff(want.decode().splitlines(True),
                                                   after[path].decode().splitlines(True), 'expected', 'result')))
        check('A::x -> X: 15 reference(s) in 2 file(s)' in report, 'A::x: 15 references (B::x, Pod::x, locals left)')
        check('A::count -> Count: 3 reference(s)' in report, 'the static member A::count: 3 references')
        check('Pod::y -> Y: 3 reference(s)' in report, 'Pod::y: 3 references, the designated initializer included')
        check(re.search(r'Engine/a\.h:\d+: x: a member of a dependent expression.*\n\s*return t\.x;', report)
              is not None,
              'the dependent member in a template is listed by hand')
        check(re.search(r'Engine/a\.h:\d+: flags: in the body of the macro FLAGS_OF', report) is None and
              re.search(r'Engine/use\.cpp:\d+: flags: in the body of the macro FLAGS_OF used here \(defined at '
                        r'Engine/a\.h:5\)', report) is not None,
              'the macro body is listed by hand, with its definition')
        check('Engine/a.h:13:' in report, 'the comment naming `x` is listed')
        check('Engine/platform_other.cpp:5:' in report and 'Engine/other.cpp:6:' in report and
              'Engine/other.cpp:8:' not in report, 'the file no unit compiles and the #if 0 branch are listed')
        check('"a.x"' in after['Engine/use.cpp'].decode(), 'the string is not touched')

        # the "by hand" lines, done by hand; then the result must compile
        for path, old, new in (('Engine/a.h', '((p)->flags & 1)', '((p)->Flags & 1)'),
                               ('Engine/a.h', 'return t.x;', 'return t.X;')):
            data = read_bytes(root, path)
            with open(os.path.join(root, path), 'wb') as f:
                f.write(data.replace(old.encode(), new.encode()))
        after = snapshot()
        for entry in db:
            r = subprocess.run(entry['arguments'] + (['/Fo' + build + os.sep] if cl else
                                                     ['-o', os.path.join(build, 'unit.o')]),
                               cwd=build, env=env, capture_output=True, text=True, errors='replace')
            check(r.returncode == 0, 'the result compiles: %s' % os.path.basename(entry['file']))
            if r.returncode:
                print(r.stdout + r.stderr)

        code, report = tool('good')
        check(code == 0 and snapshot() == after, 'a second run changes nothing')
        check(report.count('nothing to do') == 6, 'a second run says "nothing to do" for each line')
    finally:
        shutil.rmtree(root, ignore_errors=True)
    print('self-test: %s' % ('%d failure(s)' % len(failures) if failures else 'passed'))
    return 1 if failures else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--map', help='the renames: Struct::old_name<TAB>NewName per line')
    ap.add_argument('--build', help='directory with compile_commands.json (default: .tidy/build of the repository)')
    ap.add_argument('--repo', default=REPO, help='the repository to rewrite (default: this one)')
    ap.add_argument('--dry-run', action='store_true', help='print the diff and the lists, write nothing')
    ap.add_argument('--strict', action='store_true', help='with --dry-run: exit 1 when something is left by hand')
    ap.add_argument('--keep-going', action='store_true', help='a unit that does not parse is a warning, not an error')
    ap.add_argument('-j', '--jobs', type=int, default=min(8, os.cpu_count() or 4))
    ap.add_argument('--clang-format', help='clang-format 19.1.7 (default: .compare/cfenv/Scripts/clang-format.exe)')
    ap.add_argument('--libclang', help='the libclang library (default: LIBCLANG, or Visual Studio\'s)')
    ap.add_argument('--self-test', action='store_true', help='run the tool on generated sources and check the result')
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not args.map:
        ap.error('--map is required')
    return rename(args)


if __name__ == '__main__':
    sys.exit(main())
