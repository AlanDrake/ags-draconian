#!/usr/bin/env python3
"""
Folds the top-level code of source files into other files, item by item and byte for byte, as a reviewed map says
(docs/refactor/PLAN.md 5.3, step 2: the thirteen Engine/ac/global_*.cpp files go into the files of their domain).

    python Script/move_functions.py --map docs/refactor/fold-5.3.tsv [--repo <dir>] [--only <source.cpp> ...]
                                    [--dry-run] [--verbose]

The map is a tab-separated table, one row per top-level item of a source file (lines starting with # are comments):

    kind  name  source  destination .cpp  destination header  note

  kind         function, static function, variable, type, declaration-only, macro; new-file names a file to create
  name         the identifier; a function's parameter types in parentheses tell overloads apart (spaces, std:: and
               parameter names do not matter; (...) matches any); "struct X; (forward)" is a forward declaration;
               a new-file row has the file's one-line description here
  source       the .cpp, or its header for what only the header has (types, forward declarations)
  destination  a .cpp, NEW:<path> for a file the run creates, - for none
  header       where the item's declaration goes (a header, NEW:<path>), - for none, DROP to delete the declaration

A run, for each selected source (every source of the map without --only):
  1. checks the map against the tree before it writes anything: every top-level item of the .cpp and every
     declaration of its header (MAP_HEADERS, else the same name with .h) has exactly one row, every row names
     something there, or something already in its destination (a second run changes nothing), and an item that
     no header declares (a static function, a variable, a type of the .cpp, an undeclared function) is used only
     by text that goes to its own destination. #include, `using namespace` and `extern` lines need no row, they are
     derived (below); comments go with the item directly below them; the licence and description blocks and
     #pragma once go with the file. A mismatch stops the run and lists both sides;
  2. appends the definitions, each with the comment lines directly above it, in their source order, to the end of
     their destination .cpp: after its last line, whatever that is (a closing namespace, an #endif or a trailing
     #include block included), at file scope, one blank line below it. Two items that were separated only by blank
     lines keep them, anything else between them becomes one blank line;
  3. appends the declarations of the header (types and forward declarations too, in the header's order, which
     compiles) with their comments to the end of their destination header the same way;
  4. adds to each destination what the moved text needs: the includes, chosen among the source's and the run's
     destination headers by the identifiers the text names (split_bindings.derive_includes), the source's
     `using namespace` lines it lacks, and for each global the text uses through an `extern` line of the source:
     nothing if the destination defines or declares it, else the header that declares it, else a copy of the
     line after the destination's last `extern` line (reported: no header declares it). Every other file that
     included the source's header gets, in its place, the destination headers whose moved declarations it names,
     and the headers the source's header included for names it no longer gets otherwise; or nothing (reported as
     stale);
  5. formats with clang-format (AGS_CLANG_FORMAT, 19.1.7) only the include blocks it touched and the new files,
     fails if the moved text or any other line changed, and checks that every touched file is format-clean;
  6. removes the emptied pair with git rm and from Engine/CMakeLists.txt, Engine.App.vcxproj and .filters and the
     Xcode project, adds the new files to them (split_bindings' build-list code), and reports per source: items per
     destination, declarations moved, includers rewritten and dropped, externs, what was dropped.

Not done, by design: a definition is never copied (an item used from two destinations is a map error), nothing is
renamed or made static, and moved text is never reformatted. What it assumes is split_bindings.py's parser: the
files are clang-formatted (top-level declarations at column 0, Allman braces). What it does not prove: that the
result compiles (the build does) and that the code is the same (Script/compare_objects.py does: "move: N
functions moved between objects, 0 changed, 0 added, 0 removed (mechanical)").
"""
import argparse
import os
import re
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import split_bindings as sb  # noqa: E402  (the parser, the include derivation, clang-format and the build lists)

# ---------------------------------------------------------------------------------------------------------------
# Tables: the facts of this tree
# ---------------------------------------------------------------------------------------------------------------

# source .cpp -> its header, where it is not the same name with .h
MAP_HEADERS = {}
# directories searched for the files that include a source's header
INCLUDER_DIRS = ('Engine', 'Common', 'Plugins', 'Tools', 'Editor/AGS.Native', 'Compiler')
INCLUDER_EXTS = ('.cpp', '.h', '.hpp', '.c', '.mm', '.m', '.inl')
KINDS = ('function', 'static function', 'variable', 'type', 'declaration-only', 'macro', 'new-file')
# the qualifiers a parameter type may start with; a parameter that has only these before its last word is unnamed
QUALIFIERS = {'const', 'volatile', 'struct', 'class', 'enum', 'union', 'typename'}


class Fail(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------
# The map
# ---------------------------------------------------------------------------------------------------------------

class Row:
    def __init__(self, fields, line):
        self.line = line
        self.kind, self.name, self.source, dest, header = fields[:5]
        self.note = fields[5] if len(fields) > 5 else ''
        self.dest_new = dest.startswith('NEW:')
        self.dest = None if dest == '-' else dest[4:] if self.dest_new else dest
        self.header_new = header.startswith('NEW:')
        self.drop_decl = header == 'DROP'
        self.header = None if header in ('-', 'DROP') else header[4:] if self.header_new else header
        m = re.match(r'\s*(?:(?:struct|class|union|enum)\s+)?([A-Za-z_]\w*)(\((.*)\))?', self.name)
        self.ident = m.group(1) if m else None
        self.sig = normal_params(split_params(m.group(3)), named=False) if m and m.group(2) and \
            m.group(3).strip() != '...' else None

    def __str__(self):
        return 'map line %d: %s %s (%s)' % (self.line, self.kind, self.name, self.source)


def read_map(path):
    rows = []
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip('\r\n')
            if not line.strip() or line.startswith('#'):
                continue
            fields = line.split('\t')
            if len(fields) < 5:
                raise Fail('%s:%d: %d columns, at least 5 expected' % (path, n, len(fields)))
            row = Row(fields, n)
            if row.kind not in KINDS:
                raise Fail('%s:%d: unknown kind "%s"' % (path, n, row.kind))
            if row.dest == 'DELETE':
                raise Fail('%s:%d: DELETE is a [delete] commit, not a fold' % (path, n))
            rows.append(row)
    return rows


def header_of(source):
    return MAP_HEADERS.get(source, os.path.splitext(source)[0] + '.h')


def split_params(text):
    return [p.strip() for p in sb.split_top(text or '', ',') if p.strip()]


def normal_params(params, named=True):
    """A comparable form of a parameter list: no parameter names, defaults, spaces, std:: or ALL_CAPS macros."""
    out = []
    for p in params:
        p = re.sub(r'\b[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+\b', '', p.split('=')[0])
        p = re.sub(r'\bunsigned\s+int\b', 'unsigned', p).strip()
        p = re.sub(r'\b(?:\w+\s*::\s*)+(?=\w)', '', p)  # AGS::Common::String reads as String
        tokens = re.findall(r'[A-Za-z_]\w*', p)
        if named and len(tokens) >= 2 and tokens[-1] not in sb.KEYWORDS:
            rest = p[:p.rfind(tokens[-1])]
            if set(re.findall(r'[A-Za-z_]\w*', rest)) - QUALIFIERS and not rest.rstrip().endswith('::'):
                p = rest
        out.append(re.sub(r'\s+', '', p.replace('std::', '')))
    return ','.join(x for x in out if x not in ('', 'void'))


def item_params(item, ident):
    """The normal parameter list of a function item, or None."""
    m = re.search(r'\b%s\s*\(' % re.escape(ident), item.header)
    if not m:
        return None
    depth, rest = 1, item.header[m.end():]
    for end, ch in enumerate(rest):
        depth += (ch == '(') - (ch == ')')
        if depth == 0:
            return normal_params(split_params(rest[:end]))
    return None


# ---------------------------------------------------------------------------------------------------------------
# Files
# ---------------------------------------------------------------------------------------------------------------

class File:
    """A parsed file of the tree (or a file to create: exists False)."""

    def __init__(self, repo, path):
        self.path = path
        full = os.path.join(repo, path)
        self.exists = os.path.isfile(full)
        if self.exists:
            self.text, self.eol, self.bom = sb.read_eol(full)
            self.lines = self.text.splitlines(keepends=True)
            if self.lines and not self.lines[-1].endswith('\n'):
                raise Fail('%s: no newline at the end' % path)
            self.items = sb.parse(self.lines, path)
        else:
            self.text, self.eol, self.bom, self.lines, self.items = '', '\n', b'', [], []
        self.attached = sb.group_comments(self.items)

    def code(self):
        return [(k, it) for k, it in enumerate(self.items) if it.kind == 'code']

    def is_extern(self, it):
        return it.kind == 'code' and it.header.startswith('extern ') and not it.function

    def is_using(self, it):
        return it.kind == 'code' and it.header.startswith('using namespace') and not it.namespace

    def include_items(self):
        return [k for k, it in enumerate(self.items) if it.kind == 'pp' and self.lines[it.start].startswith('#include')]

    def top_include_items(self):
        out = []
        for k, it in enumerate(self.items):
            if it.kind in ('code', 'ns_open'):
                break
            if it.kind == 'pp' and self.lines[it.start].startswith('#include'):
                out.append(k)
        return out

    def include_names(self):
        return [sb.include_name(self.lines[self.items[k].start]) for k in self.include_items()]

    def quoted_includes(self, index):
        """[(include name, repository path)] of the quoted includes that resolve in the tree."""
        out = []
        for name in self.include_names():
            if name and name[0] == '"':
                p = index.resolve(name[1:-1], os.path.dirname(self.path))
                if p:
                    out.append((name, p))
        return out

    def defined(self):
        """Names the file defines or declares at top level, externs and using-directives aside."""
        return {it.name for _, it in self.code() if it.name and not self.is_extern(it) and not self.is_using(it)}

    def externs(self):
        return {it.name: k for k, it in self.code() if self.is_extern(it) and it.name}


def kind_matches(row, f, it):
    """Whether an item of a source can be what a row names (its name aside)."""
    if row.kind in ('function', 'static function'):
        return it.kind == 'code' and it.function and it.static == (row.kind == 'static function')
    if row.kind == 'macro':
        return it.kind == 'pp'
    if it.kind != 'code' or f.is_extern(it) or f.is_using(it):
        return False
    is_type = re.match(r'(?:struct|class|union|enum|typedef|using)\b', it.header) is not None
    if row.kind == 'type':
        # a definition, not a forward declaration (that is a declaration-only row)
        return is_type and ('{' in sb.strip_code(sb.item_text(f.lines, it)) or it.header.startswith(('typedef',
                                                                                                      'using')))
    if row.kind == 'declaration-only':
        return not it.function
    return not it.function and not is_type and '(' not in it.header.split('=')[0]  # variable


def item_ident(f, it):
    if it.kind == 'pp':
        m = re.match(r'#\s*define\s+(\w+)', f.lines[it.start])
        return m.group(1) if m else None
    return it.name


def find_items(row, f, decl=False):
    """Indexes of the items of a parsed file that a row names; decl: the declarations of a function or variable
    row in the source's header."""
    found = []
    for k, it in enumerate(f.items):
        if item_ident(f, it) != row.ident or it.namespace:
            continue
        if decl:
            if it.kind != 'code' or (row.kind != 'variable' and '(' not in it.header) or \
                    (row.kind == 'variable' and not f.is_extern(it)):
                continue
        elif not kind_matches(row, f, it):
            continue
        found.append(k)
    if len(found) > 1 and row.sig is not None:
        found = [k for k in found if item_params(f.items[k], row.ident) == row.sig]
    elif len(found) == 1 and row.sig is not None and f.items[found[0]].kind == 'code' and \
            '(' in f.items[found[0]].header and item_params(f.items[found[0]], row.ident) != row.sig:
        found = []
    return found


# ---------------------------------------------------------------------------------------------------------------
# Planning one source
# ---------------------------------------------------------------------------------------------------------------

class Part:
    """What goes into one destination file."""

    def __init__(self, path, new, header):
        self.path = path
        self.new = new
        self.header = header        # a destination header (declarations) or a .cpp (definitions)
        self.items = []             # (File, item index) in order
        self.rows = []
        self.names = set()          # names the moved text defines or declares
        self.text = ''
        self.includes = []          # include names to add
        self.why = {}               # include name -> the names it is added for
        self.usings = []            # using lines to add
        self.externs_copied = []    # extern lines to add
        self.extern_headers = []    # headers included for an extern of the source
        self.skipped = []           # what is there already
        self.notes = []


class Fold:
    def __init__(self, source):
        self.source = source
        self.header = header_of(source)
        self.parts = {}             # path -> Part
        self.order = []             # paths in order of first appearance in the map
        self.dropped = []           # text dropped with the files (declarations the map drops, forward declarations)
        self.done = []              # rows already in their destination
        self.problems = []
        self.includers = []         # (path, [include names], [pass-through names])
        self.removed = []
        self.description = None


def part_for(fold, path, new, header):
    if path not in fold.parts:
        fold.parts[path] = Part(path, new, header)
        fold.order.append(path)
    return fold.parts[path]


def already_there(repo, row, path):
    if not path or not os.path.isfile(os.path.join(repo, path)):
        return False
    f = File(repo, path)
    return any(item_ident(f, it) == row.ident and not f.is_extern(it) for it in f.items)


def plan_fold(repo, index, rows, source):
    """A Fold for one source (nothing written); fold.problems lists what stops it."""
    fold = Fold(source)
    mine = [r for r in rows if r.source in (source, fold.header)]
    for r in mine:
        if r.kind == 'new-file':
            fold.description = r.name
    mine = [r for r in mine if r.kind != 'new-file']
    if not os.path.isfile(os.path.join(repo, source)):
        for r in mine:
            target = r.dest if r.source == source else (r.header or r.dest)
            if target and not already_there(repo, r, target):
                fold.problems.append('%s: the source is gone and %s does not have it' % (r, target))
            elif target:
                fold.done.append(r)
        return fold
    src = File(repo, source)
    hdr = File(repo, fold.header)
    fold.src, fold.hdr = src, hdr
    claimed = {}                    # (file path, item index) -> row

    def claim(f, k, r):
        key = (f.path, k)
        if key in claimed:
            fold.problems.append('%s:%d: %s is named by two rows: %s and %s'
                                 % (f.path, f.items[k].start + 1, f.items[k].name, claimed[key], r))
        claimed[key] = r

    for r in mine:
        f = src if r.source == source else hdr
        found = find_items(r, f)
        if len(found) != 1:
            target = r.dest if f is src else (r.header or r.dest)
            if not found and target and already_there(repo, r, target):
                fold.done.append(r)
                continue
            fold.problems.append('%s: %s' % (r, 'not in %s' % f.path if not found else
                                             '%d items in %s match it' % (len(found), f.path)))
            continue
        k = found[0]
        claim(f, k, r)
        if f is src:
            if r.dest is None:
                if r.kind != 'declaration-only':
                    fold.problems.append('%s: a definition with no destination' % r)
                else:
                    fold.dropped.append(sb.item_text(f.lines, f.items[k]).strip())
            else:
                p = part_for(fold, r.dest, r.dest_new, False)
                p.items.append((f, k))
                p.rows.append(r)
                p.names.add(r.ident)
            if r.kind in ('function', 'variable') and hdr.exists:
                decls = find_items(r, hdr, decl=True)
                if len(decls) > 1:
                    fold.problems.append('%s: %d declarations in %s' % (r, len(decls), hdr.path))
                elif decls and r.header:
                    claim(hdr, decls[0], r)
                    p = part_for(fold, r.header, r.header_new, True)
                    p.items.append((hdr, decls[0]))
                    p.rows.append(r)
                    p.names.add(r.ident)
                elif decls and r.drop_decl:
                    claim(hdr, decls[0], r)
                    fold.dropped.append(sb.item_text(hdr.lines, hdr.items[decls[0]]).strip())
                elif decls:
                    claim(hdr, decls[0], r)
                    fold.problems.append('%s: %s declares it and the row has no header (DROP deletes the '
                                         'declaration)' % (r, hdr.path))
                elif r.header:
                    fold.problems.append('%s: the row names the header %s, and %s has no declaration to move'
                                         % (r, r.header, hdr.path))
        else:
            target, is_header = (r.header, True) if r.header else (r.dest, False)
            if target is None:
                fold.dropped.append(sb.item_text(f.lines, f.items[k]).strip())
                continue
            new = r.header_new if r.header else r.dest_new
            p = part_for(fold, target, new, is_header)
            line = sb.item_text(f.lines, f.items[k]).strip()
            if p.header and not new and line in (l.strip() for l in sb.read_text(os.path.join(repo, target))
                                                 .split('\n')):
                p.skipped.append(line)
                continue
            p.items.append((f, k))
            p.rows.append(r)
            p.names.add(r.ident)
    # the other side: items with no row
    for f in (src, hdr):
        for k, it in enumerate(f.items):
            if (f.path, k) in claimed or it.kind in ('blank', 'comment', 'ns_open', 'ns_close'):
                continue
            first = f.lines[it.start].rstrip('\n')
            if it.kind == 'pp' and re.match(r'#\s*(include|pragma once)\b', first):
                continue
            if f.is_extern(it) or f.is_using(it):
                continue
            fold.problems.append('%s:%d: no row for %s' % (f.path, it.start + 1, it.name or first[:60]))
        for k, it in enumerate(f.items):
            if it.namespace and (f.path, k) in claimed:
                fold.problems.append('%s:%d: %s is inside a namespace' % (f.path, it.start + 1, it.name))
    for path in fold.order:
        if fold.parts[path].new and not fold.description:
            fold.problems.append('%s: a NEW: destination needs a new-file row (its one-line description in the name '
                                 'column, the source in the source column)' % path)
    if fold.problems:
        return fold
    for path in fold.order:
        p = fold.parts[path]
        idx = {}
        for f, k in p.items:
            idx.setdefault(f.path, (f, set()))[1].add(k)
            idx[f.path][1].update(fold_index(f, k))
        texts = []
        for fpath in (hdr.path, src.path):     # a header's types before a .cpp's definitions
            if fpath in idx:
                f, ks = idx[fpath]
                texts.append(sb.joined_text(f.lines, f.items, sorted(ks)))
        p.text = '\n'.join(texts)
    check_local_use(fold)
    if not fold.problems:
        plan_headers(repo, index, fold)
        plan_sources(repo, index, fold)
    return fold


def fold_index(f, k):
    """The item and the comment lines directly above it."""
    return [f.items.index(c) for c in f.attached.get(k, [])]


def check_local_use(fold):
    """An item that no header declares can only be used by text that goes to its own destination."""
    local = {}
    for path in fold.order:
        p = fold.parts[path]
        if p.header:
            continue
        for (f, k), r in zip(p.items, p.rows):
            declared = any(r in fold.parts[h].rows for h in fold.order if fold.parts[h].header)
            if not declared:
                local[r.ident] = (path, r)
    for path in fold.order:
        p = fold.parts[path]
        if p.header:
            continue
        used = sb.identifiers(sb.strip_code(p.text))
        for name in sorted(used & set(local)):
            home, r = local[name]
            if home != path:
                fold.problems.append('%s: goes to %s, and the text that goes to %s uses it with no declaration '
                                     '(one destination for both, or a header)' % (r, home, path))


def moved_names(fold, path):
    p = fold.parts.get(path)
    return set(p.names) if p and p.header else set()


def header_views(index, fold):
    """direct and closure functions that see the run's destination headers with their declarations added."""
    def direct(path):
        p = fold.parts.get(path)
        base = index.direct(path) if not (p and p.new) else set()
        return base | moved_names(fold, path)

    def closure(path):
        p = fold.parts.get(path)
        base = index.closure(path) if not (p and p.new) else set()
        if p and p.header:
            base = base | moved_names(fold, path)
            for name in p.includes:
                r = index.resolve(name[1:-1], os.path.dirname(path)) if name[0] == '"' else None
                if r:
                    base = base | index.closure(r)
        return base
    return direct, closure


def plan_headers(repo, index, fold):
    hdr = fold.hdr
    cands = [p for _, p in hdr.quoted_includes(index)]
    for path in fold.order:
        p = fold.parts[path]
        if not p.header:
            continue
        used = set()
        for (f, k), r in zip(p.items, p.rows):
            code = sb.strip_code(sb.item_text(f.lines, f.items[k]))
            if r.kind in ('function', 'static function'):
                # the types of a declaration and its macros (AGS_FORMAT_STRING), not its parameter names
                macros = set(re.findall(r'\b[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+\b', code))
                code = re.sub(r'\b[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+(?:\([^)]*\))?', ' ', code)
                used |= {t for t, _ in sb.signature_types(code, r.ident)} | index.expand_macros(macros)
            else:
                used |= sb.used_names(repo, index, code, [])
        used -= p.names
        have = set()
        if not p.new:
            have = index.closure(path)
            existing = {n for n in File(repo, path).include_names()}
        else:
            existing = set()
        need = used - have
        # another destination header is a candidate only for what this run moves into it (no new header-to-header
        # include for anything else, which could close a cycle)
        others = [h for h in fold.order if fold.parts[h].header and h != path and h != fold.header]
        options = [(c,) for c in cands + others if c != path and c != fold.header]
        direct, closure = header_views(index, fold)

        def limited(view):
            return lambda h: moved_names(fold, h) if h in others else view(h)
        add_includes(index, p, path, need, options, limited(direct), limited(closure), existing)
        add_system_includes(p, hdr.include_names(), used, existing)


def add_includes(index, p, path, need, options, direct, closure, existing):
    """Adds the includes derived for the needed names to a part, with the names each one is for (p.why)."""
    for (c,) in sb.derive_includes(index, need, options, direct, closure):
        name = '"%s"' % sb.include_path(c)
        if name not in existing and name not in p.includes:
            p.includes.append(name)
            p.why[name] = sorted(need & closure(c))


def reach_of(index, fold, path, includes):
    """The headers a file reaches through the given repository paths and include names (new files left out)."""
    files = set()
    for r in includes:
        if r and r.startswith('"'):
            r = index.resolve(r[1:-1], os.path.dirname(path))
        if r and not (r in fold.parts and fold.parts[r].new):
            files |= index.closure_files(r)
    return files


def add_overload_includes(index, fold, p, path, used, dest_inc, src_cands, existing):
    """A name that several headers the source reaches declare (overloads spread over headers: load_sound_clip in
    audio.h and sound.h) is covered by any one of them for the include derivation, which reads names, not
    signatures; the destination must reach every one of them that the source reached, so the same overload is
    chosen."""
    src_reach = {}
    for c in src_cands:
        for h in index.closure_files(c):
            src_reach.setdefault(h, c)
    reach = reach_of(index, fold, path, dest_inc + p.includes)
    # unqualified calls only (std::move is not an overload of a move in some header)
    called = set(re.findall(r'(?<![\w:.>])([A-Za-z_]\w*)\s*\(', sb.strip_code(p.text))) & used
    for name in sorted(called):
        decl = sorted(h for h in src_reach if name in index.direct(h))
        if len(decl) < 2:
            continue
        for h in decl:
            if h in reach:
                continue
            c = h if h in src_cands else src_reach[h]
            inc = '"%s"' % sb.include_path(c)
            if inc not in existing and inc not in p.includes:
                p.includes.append(inc)
                p.why[inc] = ['%s (an overload in %s)' % (name, h)]
            reach |= index.closure_files(c)


def add_system_includes(p, names, used, existing):
    """The source's system headers that the moved text needs (SYSTEM_NAMES); an unlisted one is not added: the
    build shows whether the destination lacks it."""
    notes = []
    for name in sb.system_includes(names, used, notes):
        if name not in sb.SYSTEM_NAMES:
            continue  # reported once per source (report())
        if name not in existing and name not in p.includes:
            p.includes.append(name)
            p.why[name] = sorted(sb.SYSTEM_NAMES[name] & used)


def plan_sources(repo, index, fold):
    src = fold.src
    externs = src.externs()
    code_items = [it for _, it in src.code()]
    direct, closure = header_views(index, fold)
    src_cands = [p for _, p in src.quoted_includes(index) if p != fold.header]
    usings = [sb.item_text(src.lines, it) for _, it in src.code() if src.is_using(it)]
    headers = [h for h in fold.order if fold.parts[h].header]
    for path in fold.order:
        p = fold.parts[path]
        if p.header:
            continue
        dest = File(repo, path)
        code = sb.strip_code(p.text)
        used = sb.used_names(repo, index, code, code_items)
        have = dest.defined() | set(dest.externs()) | p.names
        dest_inc = [r for _, r in dest.quoted_includes(index)]
        own = header_of(path)
        if p.new and own in fold.parts:
            p.includes.append('"%s"' % sb.include_path(own))
            p.why[p.includes[-1]] = ['its own header']
            dest_inc.append(own)
        for r in dest_inc:
            have |= closure(r)
        need = used - have
        options = [(c,) for c in src_cands + headers if c not in dest_inc and c != path]
        existing = set(dest.include_names())
        add_includes(index, p, path, need, options, direct, closure, existing)
        add_overload_includes(index, fold, p, path, used, dest_inc, src_cands, existing)
        add_system_includes(p, src.include_names(), used, existing)
        # using-directives of the source that the destination lacks
        dest_usings = {sb.item_text(dest.lines, it) for _, it in dest.code() if dest.is_using(it)}
        p.usings = [u for u in usings if u not in dest_usings]
        # externs of the source that the moved text uses
        reach = set()
        for r in dest_inc + [index.resolve(n[1:-1], os.path.dirname(path)) for n in p.includes if n[0] == '"']:
            if r and not (r in fold.parts and fold.parts[r].new):
                reach |= index.closure_files(r)
        for name, k in sorted(externs.items(), key=lambda x: x[1]):
            if name not in used or name in dest.defined() or name in dest.externs() or name in p.names:
                continue
            decls = [h for h, _ in sb.extern_decls(repo, name)]
            if any(h in reach for h in decls):
                continue
            if decls:
                name_inc = '"%s"' % sb.include_path(decls[0])
                if name_inc not in p.includes:
                    p.includes.append(name_inc)
                p.extern_headers.append('%s (%s)' % (decls[0], name))
            else:
                p.externs_copied.append(sb.item_text(src.lines, src.items[k]).rstrip('\n'))


# ---------------------------------------------------------------------------------------------------------------
# Includers of the source's header
# ---------------------------------------------------------------------------------------------------------------

def tracked_files(repo):
    r = subprocess.run(['git', '-C', repo, 'ls-files', '--'] + list(INCLUDER_DIRS), capture_output=True, text=True)
    if r.returncode != 0:
        files = []
        for root in INCLUDER_DIRS:
            for d, _, names in os.walk(os.path.join(repo, root)):
                files += [os.path.relpath(os.path.join(d, n), repo).replace(os.sep, '/') for n in names]
        return files
    return r.stdout.split('\n')


def includers(repo, index, header):
    base = os.path.basename(header)
    out = []
    for f in tracked_files(repo):
        if not f.endswith(INCLUDER_EXTS) or f == header:
            continue
        full = os.path.join(repo, f)
        if not os.path.isfile(full):
            continue
        with open(full, 'rb') as fh:
            data = fh.read()
        if base.encode() not in data:
            continue
        text = data.decode('utf-8', errors='replace')  # a file to rewrite is read again strictly
        for m in re.finditer(r'^#\s*include\s+"([^"]+)"', text, re.M):
            if index.resolve(m.group(1), os.path.dirname(f)) == header:
                out.append(f)
                break
    return out


def plan_includers(repo, index, fold):
    """For each file that includes the source's header (the source aside): the includes that replace it."""
    hdr = fold.hdr
    headers = [h for h in fold.order if fold.parts[h].header]
    direct, closure = header_views(index, fold)
    through = [p for _, p in hdr.quoted_includes(index)]
    passed = set()
    for p in through:
        passed |= index.closure(p)
    out = []
    for f in includers(repo, index, fold.header):
        if f == fold.source:
            continue
        # not parsed: an includer need not follow the parser's layout; its includes and names are enough
        text = sb.read_text(os.path.join(repo, f))
        code = sb.strip_code(text)
        used = sb.identifiers(code)
        incs = [index.resolve(m.group(1), os.path.dirname(f))
                for m in re.finditer(r'^#\s*include\s+"([^"]+)"', text, re.M)]
        incs = [r for r in incs if r and r != fold.header]
        named = [h for h in headers if h != f and used & moved_names(fold, h)]
        new = [h for h in named if h not in incs]
        after = set()
        for r in incs + new:
            after |= closure(r)
        lost = (used & passed) - after
        extra = [c for (c,) in sb.derive_includes(index, lost, [(t,) for t in through if t not in incs])]
        out.append((f, ['"%s"' % sb.include_path(h) for h in new], ['"%s"' % sb.include_path(h) for h in extra],
                    ['"%s"' % sb.include_path(h) for h in named if h in incs]))
    fold.includers = out


# ---------------------------------------------------------------------------------------------------------------
# Writing
# ---------------------------------------------------------------------------------------------------------------

def body_lines(text):
    return [l for l in text.split('\n') if l.strip() and not l.startswith('#include')]


def write_formatted(repo, path, lines, ranges, eol, bom, moved=()):
    """Writes the lines, formats the given 1-based line ranges (the whole file if None), checks that nothing but
    #include lines changed and that the moved texts are there verbatim, and that the file is format-clean."""
    full = os.path.join(repo, path)
    text = ''.join(lines)
    sb.write_text(full, text, eol, bom)
    if ranges is None or ranges:
        sb.format_file(repo, path, ranges)
    after = sb.read_text(full)
    if ranges is not None and body_lines(after) != body_lines(text):
        raise Fail('clang-format changed %s outside its include blocks' % path)
    for m in moved:
        if m not in after:
            raise Fail('clang-format changed the moved text in %s' % path)
    if not sb.format_clean(repo, path):
        raise Fail('%s is not clang-format clean' % path)


def insert_after(lines, at, new):
    """Inserts lines after line index `at` (-1: at the top)."""
    lines[at + 1:at + 1] = new


def include_range(lines, at):
    """1-based (first, last) of the preprocessor and blank lines around line index `at` (the include block that
    holds it), blank lines at its ends left out."""
    a = b = at
    while a > 0 and (not lines[a - 1].strip() or lines[a - 1].startswith('#')):
        a -= 1
    while b + 1 < len(lines) and (not lines[b + 1].strip() or lines[b + 1].startswith('#')):
        b += 1
    while a < at and not lines[a].strip():
        a += 1
    while b > at and not lines[b].strip():
        b -= 1
    return a + 1, b + 1


def merge_blank(lines, i):
    """After a line was removed at index i: two blank lines that met there become one."""
    if 0 < i < len(lines) and not lines[i].strip() and not lines[i - 1].strip():
        del lines[i]


def new_file_head(fold, path):
    lic = fold.src.lines[:sb.licence_end(fold.src.lines)]
    if not fold.description:
        raise Fail('%s: a NEW: destination needs a new-file row with its one-line description' % path)
    head = ''.join(lic) + '//\n' + '// %s\n' % fold.description + '//\n' + lic[-1]
    return head + ('#pragma once\n' if path.endswith('.h') else '')


def apply_part(repo, fold, p):
    if p.new:
        text = new_file_head(fold, p.path)
        incs = [i for i in p.includes]
        if incs:
            text += ('\n' if p.header else '') + ''.join('#include %s\n' % i for i in incs)
        if p.usings:
            text += '\n' + ''.join(p.usings)
        if p.externs_copied:
            text += '\n' + ''.join(e + '\n' for e in p.externs_copied)
        text += '\n' + p.text
        os.makedirs(os.path.dirname(os.path.join(repo, p.path)), exist_ok=True)
        write_formatted(repo, p.path, [text], None, fold.src.eol, b'', [p.text])
        return
    f = File(repo, p.path)
    lines = list(f.lines)
    while lines and not lines[-1].strip():
        lines.pop()
    lines += ['\n'] + p.text.splitlines(keepends=True)
    # externs after the last extern line at file scope, else after the using-directives, else after the includes
    top = f.top_include_items()
    after_inc = f.items[top[-1]].end - 1 if top else sb.licence_end(f.lines) - 1
    using_k = [k for k, it in f.code() if f.is_using(it)]
    extern_k = [k for k, it in f.code() if f.is_extern(it) and not it.namespace]
    if p.externs_copied:
        if extern_k:
            insert_after(lines, f.items[extern_k[-1]].end - 1, [e + '\n' for e in p.externs_copied])
        elif using_k:
            insert_after(lines, f.items[using_k[-1]].end - 1, ['\n'] + [e + '\n' for e in p.externs_copied])
        else:
            insert_after(lines, after_inc, ['\n'] + [e + '\n' for e in p.externs_copied])
    if p.usings:
        if using_k:
            insert_after(lines, f.items[using_k[-1]].end - 1, p.usings)
        else:
            insert_after(lines, after_inc, ['\n'] + p.usings)
    ranges = []
    if p.includes:
        insert_after(lines, after_inc, ['#include %s\n' % i for i in p.includes] if top else
                     ['\n'] + ['#include %s\n' % i for i in p.includes])
        ranges.append(include_range(lines, after_inc + (1 if top else 2)))
    write_formatted(repo, p.path, lines, ranges, f.eol, f.bom, [p.text])


def apply_includer(repo, index, fold, path, new, extra):
    text, eol, bom = sb.read_eol(os.path.join(repo, path))
    lines = text.splitlines(keepends=True)
    hits = [i for i, l in enumerate(lines) if l.startswith('#include "') and
            index.resolve(sb.include_name(l)[1:-1], os.path.dirname(path)) == fold.header]
    have = {sb.include_name(l) for l in lines if l.startswith('#include')}
    add = ['#include %s\n' % n for n in new + extra if n not in have]
    ranges = []
    for i in reversed(hits):
        lines[i:i + 1] = add if i == hits[0] else []
        if i != hits[0] or not add:
            merge_blank(lines, i)
    first = hits[0]
    if add:
        ranges.append(include_range(lines, first))
    else:
        # the block the include left, if any of it remains
        near = [j for j in (first - 1, first) if 0 <= j < len(lines) and lines[j].startswith('#include')]
        if near:
            ranges.append(include_range(lines, near[0]))
    write_formatted(repo, path, lines, ranges, eol, bom)


def git(repo, *args):
    r = subprocess.run(['git', '-C', repo] + list(args), capture_output=True, text=True)
    if r.returncode != 0:
        raise Fail('git %s: %s' % (' '.join(args), r.stderr.strip()))
    return r.stdout


def remove_from_cmake(repo, path):
    cm = os.path.join(repo, sb.ENGINE_CMAKE)
    text, eol, bom = sb.read_eol(cm)
    line = '    ' + path[len('Engine/'):]
    lines = text.split('\n')
    if line not in lines:
        return False
    lines.remove(line)
    sb.write_text(cm, '\n'.join(lines), eol, bom)
    return True


def remove_from_vcxproj(repo, path):
    vc = os.path.join(repo, sb.ENGINE_VCXPROJ)
    text, eol, bom = sb.read_eol(vc)
    kind = 'ClCompile' if path.endswith('.cpp') else 'ClInclude'
    line = '    <%s Include="%s" />' % (kind, sb.win(path))
    lines = text.split('\n')
    if line not in lines:
        return False
    lines.remove(line)
    sb.write_text(vc, '\n'.join(lines), eol, bom)
    return True


def remove_from_filters(repo, path):
    fl = os.path.join(repo, sb.ENGINE_VCXPROJ + '.filters')
    text, eol, bom = sb.read_eol(fl)
    kind = 'ClCompile' if path.endswith('.cpp') else 'ClInclude'
    lines = text.split('\n')
    head = '    <%s Include="%s">' % (kind, sb.win(path))
    if head not in lines:
        return False
    i = lines.index(head)
    j = lines.index('    </%s>' % kind, i)
    del lines[i:j + 1]
    sb.write_text(fl, '\n'.join(lines), eol, bom)
    return True


def remove_from_xcode(repo, path):
    """Removes a file's reference, its build files and their entries in the build phases and its group."""
    px = os.path.join(repo, sb.XCODE_PROJECT)
    text, eol, bom = sb.read_eol(px)
    groups, parent = sb.pbx_groups(text)
    project_dir = os.path.dirname(os.path.dirname(px))

    def resolve(g):
        p, tree, _ = groups[g]
        if tree == 'SOURCE_ROOT':
            base = project_dir
        elif tree == '<group>':
            base = resolve(parent[g]) if g in parent else project_dir
        else:
            return None
        if base is None:
            return None
        return os.path.normpath(os.path.join(base, p)) if p else base

    target = os.path.normpath(os.path.join(repo, path))
    refs = []
    for m in re.finditer(r'^\t\t(\w{24}) /\* [^\n]*? \*/ = \{isa = PBXFileReference;[^\n]*?path = ([^;]+);[^\n]*\n',
                         text, re.M):
        if m.group(1) in parent and os.path.basename(m.group(2).strip('"')) == os.path.basename(path):
            base = resolve(parent[m.group(1)])
            if base and os.path.normpath(os.path.join(base, m.group(2).strip('"'))) == target:
                refs.append(m.group(1))
    if not refs:
        return False
    for ref in refs:
        builds = re.findall(r'^\t\t(\w{24}) /\*[^\n]*?\*/ = \{isa = PBXBuildFile; fileRef = %s\b' % ref, text, re.M)
        for b in builds:
            text = re.sub(r'^\t\t%s /\*[^\n]*\n' % b, '', text, flags=re.M)
            text = re.sub(r'^\t\t\t\t%s /\*[^\n]*?\*/,\n' % b, '', text, flags=re.M)
        text = re.sub(r'^\t\t%s /\*[^\n]*\n' % ref, '', text, flags=re.M)
        text = re.sub(r'^\t\t\t\t%s /\*[^\n]*?\*/,\n' % ref, '', text, flags=re.M)
    sb.write_text(px, text, eol, bom)
    return True


def remove_build_lists(repo, path):
    done = []
    for name, fn in (('CMake', remove_from_cmake), ('vcxproj', remove_from_vcxproj),
                     ('filters', remove_from_filters), ('Xcode', remove_from_xcode)):
        if fn(repo, path):
            done.append(name)
    return done


def apply_fold(repo, index, fold, build_lists=True):
    for path in fold.order:
        if fold.parts[path].header:
            apply_part(repo, fold, fold.parts[path])
    for path in fold.order:
        if not fold.parts[path].header:
            apply_part(repo, fold, fold.parts[path])
    index = sb.HeaderIndex(repo)
    for path, new, extra, _ in fold.includers:
        apply_includer(repo, index, fold, path, new, extra)
    for path in fold.order:
        if fold.parts[path].new:
            git(repo, 'add', '--', path)
            if build_lists:
                fold.removed.append('%s added to %s' % (path, ', '.join(sb.add_build_lists(repo, path)) or 'nothing'))
    for path in (fold.source, fold.header):
        if not os.path.isfile(os.path.join(repo, path)):
            continue
        git(repo, 'rm', '-q', '--', path)
        if build_lists:
            fold.removed.append('%s removed from %s' % (path, ', '.join(remove_build_lists(repo, path)) or 'nothing'))
        else:
            fold.removed.append('%s removed' % path)
    sb._EXTERN_CACHE.clear()


# ---------------------------------------------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------------------------------------------

def report(fold, verbose):
    out = ['%s + %s:' % (fold.source, os.path.basename(fold.header))]
    if fold.done and not fold.parts:
        out.append('  already folded (%d rows in their destinations)' % len(fold.done))
        return '\n'.join(out), None
    for problem in fold.problems if verbose else fold.problems[:20]:
        out.append('  STOP: ' + problem)
    if len(fold.problems) > 20 and not verbose:
        out.append('  STOP: ... %d more (--verbose)' % (len(fold.problems) - 20))
    if fold.problems:
        return '\n'.join(out), None
    for path in fold.order:
        p = fold.parts[path]
        kinds = {}
        for r in p.rows:
            kinds[r.kind] = kinds.get(r.kind, 0) + 1
        what = ', '.join('%d %s' % (n, k) for k, n in kinds.items())
        line = '  -> %s%s: %s, %d lines' % (path, ' (new)' if p.new else '', what, len(p.text.split('\n')))
        if p.includes:
            line += '; includes + %s' % ' '.join(p.includes)
        out.append(line)
        if verbose:
            for name in p.includes:
                out.append('     %s for %s' % (name, ', '.join(p.why.get(name, [])[:8]) or '?'))
        if p.usings:
            out.append('     using-directives added: %s' % ' '.join(u.strip() for u in p.usings))
        for e in p.externs_copied:
            out.append('     extern copied (no header declares it): %s' % e)
        for e in p.extern_headers:
            out.append('     extern replaced by its header: %s' % e)
        for s in p.skipped:
            out.append('     already there: %s' % s)
        for n in p.notes:
            out.append('     note: %s' % n)
    for d in fold.dropped:
        out.append('  dropped: %s' % d.split('\n')[0])
    unlisted = sorted({n for f in (fold.src, fold.hdr) for n in f.include_names()
                       if n and n.startswith('<') and n not in sb.SYSTEM_NAMES})
    if unlisted:
        out.append('  not added anywhere: the system headers %s of the source (SYSTEM_NAMES does not list them; the '
                   'build shows whether a destination needs one)' % ' '.join(unlisted))
    if fold.done:
        out.append('  already in their destinations: %s' % ', '.join(r.ident for r in fold.done))
    rewritten = [(f, n, e) for f, n, e, _ in fold.includers if n or e]
    covered = [(f, h) for f, n, e, h in fold.includers if not n and not e and h]
    stale = [f for f, n, e, h in fold.includers if not n and not e and not h]
    out.append('  includers of %s: %d; rewritten %d, dropped as it includes the destination already %d, dropped '
               'as stale %d' % (fold.header, len(fold.includers), len(rewritten), len(covered), len(stale)))
    for f, n, e in rewritten:
        out.append('     %s -> %s%s' % (f, ' '.join(n) or '-', (' (pass-through: %s)' % ' '.join(e)) if e else ''))
    for f, h in covered:
        out.append('     %s: has %s' % (f, ' '.join(h)))
    if stale:
        out.append('     stale (names nothing it declared): %s' % ', '.join(stale))
    copied = sum(len(fold.parts[p].externs_copied) for p in fold.order)
    externs = len(fold.src.externs())
    out.append('  extern lines in .cpp files: %d leave with the source, %d copied into destinations%s'
               % (externs, copied, ' (the count rises)' if copied > externs else ''))
    row = (fold.source, ', '.join('%s %d' % (os.path.basename(p), len(fold.parts[p].rows)) for p in fold.order),
           len(rewritten), len(covered), len(stale), copied)
    return '\n'.join(out), row


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--map', required=True, help='the fold map (.tsv)')
    ap.add_argument('--repo', default=os.path.dirname(SCRIPT_DIR))
    ap.add_argument('--only', nargs='+', default=[], help='source files (repository paths) to fold')
    ap.add_argument('--dry-run', action='store_true', help='check and report, write nothing')
    ap.add_argument('--no-build-lists', action='store_true', help='leave the build descriptions alone')
    ap.add_argument('--verbose', action='store_true')
    args = ap.parse_args()
    repo = os.path.abspath(args.repo)
    rows = read_map(args.map if os.path.isabs(args.map) else os.path.join(repo, args.map))
    sources = []
    for r in rows:
        s = r.source if r.source.endswith('.cpp') else None
        if s and s not in sources:
            sources.append(s)
    if args.only:
        only = [s.replace('\\', '/') for s in args.only]
        for s in only:
            if s not in sources:
                ap.error('%s has no rows in the map' % s)
        sources = only
    if not args.dry_run:
        sb.clang_format()
    index = sb.HeaderIndex(repo)
    folds = []
    status = 0
    for s in sources:
        try:
            fold = plan_fold(repo, index, rows, s)
            if not fold.problems and fold.parts:
                plan_includers(repo, index, fold)
        except (Fail, sb.Fail) as e:
            print('%s: FAILED: %s' % (s, e))
            status = 1
            continue
        folds.append(fold)
        if fold.problems:
            status = 1
    if not args.dry_run and status:
        for fold in folds:
            print(report(fold, args.verbose)[0])
        print('\nnothing written: the map and the tree disagree')
        return status
    table = []
    for k, fold in enumerate(folds):
        if not args.dry_run and fold.parts:
            if k:
                # the tree changed: plan again on what is there now
                index = sb.HeaderIndex(repo)
                fold = plan_fold(repo, index, rows, fold.source)
                if fold.problems:
                    print(report(fold, args.verbose)[0])
                    return 1
                plan_includers(repo, index, fold)
            try:
                apply_fold(repo, index, fold, not args.no_build_lists)
            except (Fail, sb.Fail) as e:
                print(report(fold, args.verbose)[0])
                print('  FAILED while writing: %s (the tree is partly written: git checkout and git clean)' % e)
                return 1
        text, row = report(fold, args.verbose)
        print(text)
        for line in fold.removed:
            print('  ' + line)
        if row:
            table.append(row)
    if len(folds) > 1:
        print('\n%-36s %-62s %4s %4s %5s %6s' % ('source', 'items per destination', 'rewr', 'had', 'stale',
                                                  'extern'))
        for r in table:
            print('%-36s %-62s %4d %4d %5d %6d' % r)
        stops = [f.source for f in folds if f.problems]
        print('stopped: %s' % (', '.join(stops) or 'none'))
    return status


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (Fail, sb.Fail) as e:
        print('move_functions: %s' % e, file=sys.stderr)
        sys.exit(2)
