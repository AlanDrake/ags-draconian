#!/usr/bin/env python3
"""
Proves that converting script bindings to the template binder (Script/convert_bindings.py, docs/refactor/PLAN.md
5.2 step 2) changed no wrapper's code and no registration: builds two revisions as Script/compare_objects.py does
(MSVC, MinSizeRel: optimised, no LTO, NDEBUG) and compares, wrapper by wrapper, the disassembly of the function the
script VM calls for each registered name.

    python Script/compare_bindings.py <base-rev> [<new-rev>] [--work-dir <dir>] [--show N] [--tables-only]
                                      [--release] [--reuse]

<new-rev> defaults to the working tree: tracked files with their uncommitted changes (git stash create), so a new
file must be added to the index first.

Pairing. The registration tables of the two revisions are read from the sources (the files of
Engine/game/script_api/): per file, the rows in order, each a script name and the function registered for scripts
(and the one for plugins). A row of the base is {"Name", API_FN_PAIR(X)} (wrapper Sc_X), {"Name", Sc_X, plugin}
or ccAddExternal*Function("Name", Sc_X); a row of the new revision is one of those or a binder row
(ScMethod<&F, tags...>("Name"[, plugin]), ScStatic, ScMethodFormat, ScStaticFormat, or a wrapper
ScMethodWrapper<&F, tags...>() passed to ccAddExternal*Function). The two lists of a file must have the same names
in the same order (a row is paired with the row at the same place), which is also the table check below. The base
wrapper is the symbol ?Sc_X@@... of the base build; the new one, for a binder row, is the instantiation of
ScriptBinderDetail::MethodCall (StaticCall, MethodFormatCall, StaticFormatCall) whose function is F (its qualified
name, read from the decorated name ??$MethodCall@$M<type>1?F@@..., ends as the row writes F) and whose tags are the
row's (read from the decorated name, and checked against dumpbin's undecorated one), found in the objects of the
engine's API files; exactly one code must answer to it (an instantiation has a copy in every object that registers
it). This is sound because the VM calls what the row registers: the name -> wrapper mapping of each revision is
read from the very rows that build its table, and the symbol is the one the row's template arguments instantiate.

The comparison: compare_objects.py's normalised disassembly (addresses and local labels read without their
numbers, call targets and data by symbol name), identical apart from the function's own name. A difference is
printed as a diff of the two listings.

Tables (C): per file, the same names in the same order, and the same plugin function per row (API_FN_PAIR(X) and
a binder row without a plugin argument register their function; an explicit argument registers itself;
ccAddExternal*Function none). --tables-only checks this from the sources without building.

--release compares the executable users get instead: MSVC Release (/O2 /GL, LTCG) of the engine target, linked
with a map file, in <work-dir>/release; each wrapper is paired as above through the map file's decorated names and
read from the executable's disassembly, from its address to the next symbol's, without the int 3 padding at the
end (absolute addresses read as symbol names). --reuse compares the objects of the last run again, without checkout or build.

    python Script/compare_bindings.py --self-test
"""
import argparse
import bisect
import collections
import difflib
import json
import os
import re
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import compare_objects as co  # noqa: E402
import convert_bindings as cb  # noqa: E402

REPO = os.path.abspath(os.path.join(SCRIPT_DIR, '..'))
API_DIR = cb.API_DIR
TAGS = ('ScVoidResult', 'ScUndefinedResult', 'ScBoolResult', 'ScResultAs', 'ScManagedBy', 'ScBoolArg', 'ScIntArg')
# a row's form -> the binder's function (in ScriptBinderDetail) that its rows instantiate as the wrapper
FORMS = {'ScMethod': 'MethodCall', 'ScStatic': 'StaticCall', 'ScMethodFormat': 'MethodFormatCall',
         'ScStaticFormat': 'StaticFormatCall'}


class Row:
    def __init__(self, name, plugin, kind, wrapper=None, entry=None, function=None, tags=()):
        self.name = name          # the script name
        self.plugin = plugin      # the plugin function as written, None for none
        self.kind = kind          # 'raw' (an Sc_ wrapper) or 'binder'
        self.wrapper = wrapper    # raw: the Sc_ name
        self.entry = entry        # binder: MethodCall, StaticCall, MethodFormatCall, StaticFormatCall
        self.function = function  # binder: the bound function's name as written (no &)
        self.tags = tuple(tags)   # binder: normalised tags

    def key(self):
        return self.wrapper if self.kind == 'raw' else (self.entry, self.function, self.tags)

    def label(self):
        if self.kind == 'raw':
            return self.wrapper
        return '%s<%s%s>' % (self.entry, self.function, ''.join(', ' + '%s<%s>' % t if t[1] else ', ' + t[0]
                                                             for t in self.tags))


def balanced(text, i, open_ch='<', close_ch='>'):
    """The index after the bracket that closes the one at text[i]."""
    depth = 0
    while i < len(text):
        if text[i] == open_ch:
            depth += 1
        elif text[i] == close_ch:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise cb.Fail('unbalanced %s' % open_ch)


def normal_tag(text):
    """('ScBoolArg', '1') of a tag as written in a row or demangled by dumpbin."""
    m = re.match(r'^\s*(\w+)\s*(?:<(.*)>)?\s*$', text)
    if not m:
        raise cb.Fail('unread tag %r' % text)
    # the last name: a type (struct sc_File), an object (dumpbin writes struct CCDynamicArray globalDynamicArray)
    words = re.findall(r'\w+', m.group(2) or '')
    return m.group(1), words[-1] if words else ''


def function_of(arg):
    """The bound function's name from a template argument: &F, &N::F or static_cast<...>(&F)."""
    m = re.match(r'^static_cast<.*>\(\s*&\s*([\w:]+)\s*\)$', arg.strip(), re.S)
    if m:
        return m.group(1)
    return arg.strip().lstrip('&').strip()


def binder_rows(text):
    """[(position, Row)] of the binder rows and entry points of a text."""
    out = []
    code = cb.sb.strip_code(text)
    for m in re.finditer(r'\b(ScMethod|ScStatic|ScMethodFormat|ScStaticFormat)(Wrapper)?<', code):
        end = balanced(code, m.end() - 1)
        targs = cb.split_args(text[m.end():end - 1])
        function = function_of(targs[0])
        tags = tuple(normal_tag(t) for t in targs[1:])
        if m.group(2):
            d = re.search(r'ccAddExternal(?:Static|Object)Function\(\s*"([^"]+)"\s*,\s*$', text[:m.start()])
            if not d:
                continue  # an entry point named elsewhere (a declaration, a test)
            out.append((m.start(), Row(d.group(1), None, 'binder', entry=FORMS[m.group(1)], function=function,
                                       tags=tags)))
            continue
        rest = text[end:]
        a = re.match(r'\s*\(', rest)
        if not a:
            continue  # the name of a template, not a row
        close = balanced(rest, a.end() - 1, '(', ')')
        args = cb.split_args(rest[a.end():close - 1])
        name = args[0].strip()
        if not (name.startswith('"') and name.endswith('"')):
            raise cb.Fail('a row without a literal name: %s' % text[m.start():m.start() + end + close])
        plugin = args[1].strip() if len(args) > 1 else function
        out.append((m.start(), Row(name.strip('"'), plugin, 'binder', entry=FORMS[m.group(1)], function=function,
                                   tags=tags)))
    return out


def file_rows(text):
    """[Row] of a file in order: raw rows and binder rows."""
    found = []
    for m, name, sc, plugin, kind in cb.rows_of(text):
        found.append((m.start(), Row(name, plugin if kind == 'row' else None, 'raw', wrapper=sc)))
    found += binder_rows(text)
    return [r for _, r in sorted(found, key=lambda x: x[0])]


def tables(rev, worktree=None):
    """{API file: [Row]} of a revision (from git), or of a directory's files."""
    result = {}
    if worktree:
        for path in cb.api_files(worktree):
            result[path] = file_rows(cb.sb.read_text(os.path.join(worktree, path)))
        return result
    names = co.git('ls-tree', '--name-only', rev, API_DIR + '/').splitlines()
    for path in names:
        if path.endswith('.cpp'):
            text = co.git('show', '%s:%s' % (rev, path))
            result[path] = file_rows(text.replace('\r\n', '\n'))
    return result


def check_tables(old, new):
    """Prints the differences of names, order and plugin functions; returns (rows compared, problems)."""
    problems = []
    count = 0
    for path in sorted(set(old) | set(new)):
        a, b = old.get(path, []), new.get(path, [])
        if [r.name for r in a] != [r.name for r in b]:
            problems.append('%s: the names or their order differ' % path)
            for line in difflib.unified_diff([r.name for r in a], [r.name for r in b], lineterm='', n=0):
                problems.append('    ' + line)
            continue
        for ra, rb in zip(a, b):
            count += 1
            pa = ra.plugin and re.sub(r'\s', '', ra.plugin).lstrip('&')
            pb = rb.plugin and re.sub(r'\s', '', rb.plugin).lstrip('&')
            if pa != pb:
                problems.append('%s: %s: plugin function %s, was %s' % (path, ra.name, pb, pa))
    return count, problems


# ---------------------------------------------------------------------------------------------------------------
# Code
# ---------------------------------------------------------------------------------------------------------------

# An entry point's decorated name: an auto template parameter is $M<its type>, then the function's name (1?F@N@@
# for N::F) and its own encoding (Y...)
ENTRY = re.compile(r'^\?\?\$(MethodCall|StaticCall|MethodFormatCall|StaticFormatCall)@\$M.*?1\?((?:\w+@)*?\w+)'
                   r'@@Y')


def demangled_tags(label):
    """The normalised tags of an entry point's instantiation, from dumpbin's demangled name."""
    m = re.search(r'\((?:struct )?RuntimeScriptValue __cdecl ScriptBinderDetail::\w+Call<(.*)>\(', label)
    if not m:
        return ()
    targs = m.group(1)
    found = []
    for t in re.finditer(r'\b(?:struct )?(%s)\b(<[^<>]*>)?' % '|'.join(TAGS), targs):
        found.append(normal_tag(t.group(1) + (t.group(2) or '')))
    return tuple(found)


def mangled_number(text):
    """An integer of an MSVC decorated name: $00..$09 are 1..10, $0A@ is 0, $0<hex as A-P>@ the rest."""
    if re.match(r'^\d$', text):
        return int(text) + 1
    return int(''.join('%x' % (ord(c) - ord('A')) for c in text.rstrip('@')) or '0', 16)


def entry_of_symbol(symbol):
    """(entry, qualified function, tags) of an entry point's decorated name (an MSVC map file), or None."""
    m = ENTRY.match(symbol)
    if not m:
        return None
    qualified = '::'.join(reversed(m.group(2).split('@')))
    tags = []
    for t in re.finditer(r'U(ScVoidResult|ScUndefinedResult|ScBoolResult)@@'
                         r'|\?\$(ScBoolArg|ScIntArg)@\$0(\d|[A-P]+@)'
                         r'|\?\$(ScManagedBy)@\$M.*?1\?(\w+)@@'
                         r'|\?\$(ScResultAs)@[UV](\w+)@', symbol):
        if t.group(1):
            tags.append((t.group(1), ''))
        elif t.group(2):
            tags.append((t.group(2), str(mangled_number(t.group(3)))))
        elif t.group(4):
            tags.append((t.group(4), t.group(5)))
        else:
            tags.append((t.group(6), t.group(7)))
    return m.group(1), qualified, tuple(tags)


def functions_of(dumpbin, env, objects):
    """{'raw': {Sc_ name: set(code)}, 'binder': {(entry, qualified function, tags): set(code)}} of objects."""
    raw = collections.defaultdict(set)
    binder = collections.defaultdict(set)
    for path in objects:
        for label, lines in co.disassembly_functions(dumpbin, env, path):
            symbol = label[:-1].split(' (', 1)[0]
            code = tuple(lines)
            m = re.match(r'^\?(Sc_\w+)@@', symbol)
            if m:
                raw[m.group(1)].add(code)
                continue
            key = entry_of_symbol(symbol)
            if key:
                if tuple(sorted(key[2])) != tuple(sorted(demangled_tags(label))):
                    raise cb.Fail('the tags of %s read %s from the decorated name, %s from the demangled one'
                                  % (symbol, key[2], demangled_tags(label)))
                binder[key].add(code)
    return {'raw': raw, 'binder': binder}


def code_of(row, funcs):
    """(set of codes, symbol description) of the function a row registers, or (None, why)."""
    if row.kind == 'raw':
        codes = funcs['raw'].get(row.wrapper)
        return (codes, row.wrapper) if codes else (None, 'no symbol %s' % row.wrapper)
    want_tags = tuple(sorted(row.tags))
    found = [k for k in funcs['binder'] if k[0] == row.entry and tuple(sorted(k[2])) == want_tags
             and (k[1] == row.function or k[1].endswith('::' + row.function))]
    if len(found) != 1:
        return None, '%d instantiations answer to %s' % (len(found), row.label())
    return funcs['binder'][found[0]], found[0]


# ---------------------------------------------------------------------------------------------------------------
# --release: the engine executable users get (MSVC Release: /O2 /GL, LTCG), paired through its map file
# ---------------------------------------------------------------------------------------------------------------

RELEASE_ARGS = ['-DCMAKE_BUILD_TYPE=Release', '-DAGS_BUILD_TOOLS=0', '-DAGS_TESTS=0',
                '-DCMAKE_EXE_LINKER_FLAGS_RELEASE=/MAP']


class Executable:
    """The functions of a linked executable: symbol -> address from the map file, address -> instructions from
    dumpbin; a function runs from its address to the next address any symbol has."""

    def __init__(self, dumpbin, env, exe, map_file):
        self.symbols = {}
        by_address = collections.defaultdict(list)
        with open(map_file, encoding='utf-8', errors='replace') as f:
            for line in f:
                m = re.match(r'^\s*[0-9a-f]{4}:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8,16})\s', line)
                if m:
                    address = int(m.group(2), 16)
                    self.symbols[m.group(1)] = address
                    by_address[address].append(m.group(1))
        self.names = {a: sorted(n)[0] for a, n in by_address.items()}
        self.addresses = sorted(by_address)
        text = subprocess.run([dumpbin, '/nologo', '/disasm:nobytes', exe], env=env, capture_output=True, text=True,
                              errors='replace').stdout
        self.code = {}
        for line in text.splitlines():
            m = re.match(r'^\s+([0-9A-F]{16}): (.*)$', line)
            if m:
                self.code[int(m.group(1), 16)] = ' '.join(m.group(2).split())
        self.code_addresses = sorted(self.code)

    def body(self, symbol):
        """The normalised instructions of a function: absolute addresses as symbol names (or +offset inside the
        function, or ADDR)."""
        start = self.symbols[symbol]
        i = bisect.bisect_right(self.addresses, start)
        end = self.addresses[i] if i < len(self.addresses) else start + 4096
        j = bisect.bisect_left(self.code_addresses, start)
        out = []
        while j < len(self.code_addresses) and self.code_addresses[j] < end:
            out.append(re.sub(r'\b([0-9A-F]{16})h?\b', lambda m: self.address_name(int(m.group(1), 16), start, end),
                              self.code[self.code_addresses[j]]))
            j += 1
        while out and out[-1] == 'int 3':  # the padding up to the next function depends on where this one is
            out.pop()
        return tuple(out)

    def address_name(self, address, start, end):
        if start <= address < end:
            return '+%x' % (address - start)
        return self.names.get(address, 'ADDR')


def release_build(worktree, bdir, env, rev, keep):
    co.checkout(worktree, rev)
    cmake = co.tool('cmake', env)
    if not os.path.isfile(os.path.join(bdir, 'build.ninja')):
        co.run([cmake, '-S', worktree, '-B', bdir, '-G', 'Ninja', *RELEASE_ARGS], env=env, capture=True)
    co.run([cmake, '--build', bdir, '--target', 'ags'], env=env, capture=True)
    os.makedirs(keep, exist_ok=True)
    for name in ('ags.exe', 'ags.map'):
        with open(os.path.join(bdir, name), 'rb') as f, open(os.path.join(keep, name), 'wb') as g:
            g.write(f.read())
    return os.path.join(keep, 'ags.exe'), os.path.join(keep, 'ags.map')


def release_symbol(row, exe):
    """The decorated name of the function a row registers, in an executable."""
    if row.kind == 'raw':
        found = [s for s in exe.symbols if s.startswith('?%s@@' % row.wrapper)]
    else:
        want = tuple(sorted(row.tags))
        found = []
        for s in exe.symbols:
            key = entry_of_symbol(s)
            if key and key[0] == row.entry and tuple(sorted(key[2])) == want and \
                    (key[1] == row.function or key[1].endswith('::' + row.function)):
                found.append(s)
    return found[0] if len(found) == 1 else None


def compare_release(args, base, new, old_rows, new_rows):
    env = co.build_env()
    dumpbin = co.tool('dumpbin', env)
    worktree = os.path.join(args.work_dir, 'src')
    bdir = os.path.join(args.work_dir, 'release')
    print('building base (Release, LTCG)...', flush=True)
    old_exe = Executable(dumpbin, env,
                         *release_build(worktree, bdir, env, base, os.path.join(args.work_dir, 'rel-base')))
    print('building new (Release, LTCG)...', flush=True)
    new_exe = Executable(dumpbin, env,
                         *release_build(worktree, bdir, env, new, os.path.join(args.work_dir, 'rel-new')))
    pairs, missing = {}, []
    for path in sorted(old_rows):
        for ra, rb in zip(old_rows[path], new_rows.get(path, [])):
            sa, sb_ = release_symbol(ra, old_exe), release_symbol(rb, new_exe)
            if not sa or not sb_:
                missing.append('%s %s: %s' % (path, ra.name, ra.label() if not sa else rb.label()))
                continue
            pairs.setdefault((sa, sb_), []).append(ra.name)
    same = [k for k in pairs if old_exe.body(k[0]) == new_exe.body(k[1])]
    differ = [k for k in pairs if k not in same]
    insns = (sum(len(old_exe.body(a)) for a, _ in pairs), sum(len(new_exe.body(b)) for _, b in pairs))
    calls = (sum(sum(1 for i in old_exe.body(a) if i.startswith(('call', 'jmp ')) and '+' not in i) for a, _ in pairs),
             sum(sum(1 for i in new_exe.body(b) if i.startswith(('call', 'jmp ')) and '+' not in i) for _, b in pairs))
    print('release: %d wrapper pairs, %d identical, %d differ, %d rows not found; instructions %d -> %d, calls and '
          'tail jumps %d -> %d' % (len(pairs), len(same), len(differ), len(missing), insns[0], insns[1], calls[0],
                                   calls[1]))
    for m in missing[:20]:
        print('  not found: ' + m)
    for k in differ[:args.show]:
        print('\n  DIFFER: %s -> %s (%s)' % (k[0], k[1], ', '.join(pairs[k])))
        for line in difflib.unified_diff(list(old_exe.body(k[0])), list(new_exe.body(k[1])), 'base', 'new',
                                         lineterm='', n=2):
            print('    ' + line)
    ok = not differ and not missing
    print('\nRESULT: release: %d of %d wrappers identical' % (len(same), len(pairs)) if ok else '\nRESULT: differences')
    return 0 if ok else 1


def api_objects(bdir, src):
    """The object files of the engine target compiled from the API directory."""
    with open(os.path.join(bdir, 'compile_commands.json'), encoding='utf-8') as f:
        entries = json.load(f)
    out = {}
    for e in entries:
        source = os.path.abspath(os.path.join(e['directory'], e['file'])).replace('\\', '/')
        if '/%s/' % API_DIR in source and 'engine.dir' in e['output'].replace('\\', '/'):
            out[source.rsplit('/', 1)[1]] = os.path.join(e['directory'], e['output'])
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('base', nargs='?')
    ap.add_argument('new', nargs='?')
    ap.add_argument('--work-dir', default=os.path.join(REPO, '.compare', 'cmp'))
    ap.add_argument('--show', type=int, default=10, help='diffs to print (default 10)')
    ap.add_argument('--tables-only', action='store_true')
    ap.add_argument('--reuse', action='store_true',
                    help='compare the objects of the last run again (no checkout, no build)')
    ap.add_argument('--release', action='store_true',
                    help='compare the Release engine executable (LTCG) instead of the MinSizeRel objects')
    ap.add_argument('--self-test', action='store_true')
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not args.base:
        ap.error('the base revision is required')
    base = co.git('rev-parse', '--verify', args.base + '^{commit}')
    new, new_label = co.resolve_new(args.new)
    print('base: %s (%s)\nnew:  %s (%s)' % (args.base, base[:10], new_label, new[:10]))

    old_rows, new_rows = tables(base), tables(new)
    count, problems = check_tables(old_rows, new_rows)
    print('tables: %d rows in %d files; %s' % (count, len(new_rows), 'same names, order and plugin functions'
                                                if not problems else '%d differences' % len(problems)))
    for p in problems:
        print('  ' + p)
    if args.tables_only:
        return 1 if problems else 0
    if args.release:
        return compare_release(args, base, new, old_rows, new_rows) or (1 if problems else 0)

    env = co.build_env()
    dumpbin = co.tool('dumpbin', env)
    worktree = os.path.join(args.work_dir, 'src')
    bdir = os.path.join(args.work_dir, 'build')
    kept = os.path.join(args.work_dir, 'binding-objects')
    if args.reuse:
        old_objects = sorted(os.path.join(kept, f) for f in os.listdir(kept))
    else:
        print('building base...', flush=True)
        co.checkout(worktree, base)
        co.build(worktree, bdir, env)
        if os.path.isdir(kept):
            co.shutil.rmtree(kept)
        os.makedirs(kept)
        old_objects = []
        for name, path in sorted(api_objects(bdir, worktree).items()):
            copy = os.path.join(kept, name + '.obj')
            with open(path, 'rb') as f, open(copy, 'wb') as g:
                g.write(f.read())
            old_objects.append(copy)
        print('building new...', flush=True)
        co.checkout(worktree, new)
        co.build(worktree, bdir, env)
    new_objects = sorted(api_objects(bdir, worktree).values())
    old_funcs = functions_of(dumpbin, env, old_objects)
    new_funcs = functions_of(dumpbin, env, new_objects)

    pairs = {}
    missing = []
    for path in sorted(old_rows):
        for ra, rb in zip(old_rows[path], new_rows.get(path, [])):
            if ra.name != rb.name:
                continue
            a, why_a = code_of(ra, old_funcs)
            b, why_b = code_of(rb, new_funcs)
            if a is None or b is None:
                missing.append('%s %s: %s' % (path, ra.name, why_a if a is None else why_b))
                continue
            pairs.setdefault((ra.label(), rb.label()), (a, b, []))[2].append(ra.name)
    same = [k for k, (a, b, _) in pairs.items() if len(a) == 1 and a == b]
    copies = [k for k, (a, b, _) in pairs.items() if len(a) > 1 or len(b) > 1]
    differ = [k for k in pairs if k not in same and k not in copies]
    converted = [k for k in pairs if k[0] != k[1]]
    rows = sum(len(n) for _, _, n in pairs.values())
    print('wrappers: %d pairs for %d rows (%d converted to the binder, %d unchanged); %d identical, %d differ, '
          '%d with copies of different code, %d not found'
          % (len(pairs), rows, len(converted), len(pairs) - len(converted), len(same), len(differ), len(copies),
             len(missing)))
    for m in missing:
        print('  not found: ' + m)
    for k in copies:
        print('  copies differ: %s -> %s' % k)
    for k in differ[:args.show]:
        a, b, names = pairs[k]
        print('\n  DIFFER: %s -> %s (%s)' % (k[0], k[1], ', '.join(names)))
        for line in difflib.unified_diff(list(next(iter(a))), list(next(iter(b))), 'base', 'new', lineterm='', n=2):
            print('    ' + line)
    if not differ and not copies and not missing and not problems:
        print('\nRESULT: %d of %d wrappers identical (%d converted), tables unchanged' % (len(same), len(pairs),
                                                                                         len(converted)))
        return 0
    print('\nRESULT: differences')
    return 1


def self_test():
    text = '''
    ScFnRegister t[] = {
        {"A::B^1", API_FN_PAIR(A_B)},
        ScMethod<&A_C, ScBoolArg<1>, ScManagedBy<globalDynamicArray>>("A::C^2"),
        {"A::D", Sc_A_D, ScPl_A_D},
        ScStaticFormat<static_cast<const char *(*)(const char *)>(&Make)>(
            "A::Format^101", ScPl_Format),
        ScStatic<&Mouse::SetSpeed, ScUndefinedResult>("Mouse::set_Speed"),
    };
    ccAddExternalStaticFunction("__Builtin^1", ScStaticWrapper<&DynamicArray_Length>());
'''
    rows = file_rows(text)
    assert [r.name for r in rows] == ['A::B^1', 'A::C^2', 'A::D', 'A::Format^101', 'Mouse::set_Speed', '__Builtin^1'], \
        [r.name for r in rows]
    assert rows[0].wrapper == 'Sc_A_B' and rows[0].plugin == 'A_B'
    assert rows[1].tags == (('ScBoolArg', '1'), ('ScManagedBy', 'globalDynamicArray')), rows[1].tags
    assert rows[1].plugin == 'A_C' and rows[1].entry == 'MethodCall'
    assert rows[3].function == 'Make' and rows[3].plugin == 'ScPl_Format', (rows[3].function, rows[3].plugin)
    assert rows[4].function == 'Mouse::SetSpeed' and rows[4].tags == (('ScUndefinedResult', ''),)
    assert rows[5].entry == 'StaticCall' and rows[5].plugin is None
    # names as MSVC 19.44 decorates them and dumpbin prints them
    split = ('??$MethodCall@$MP6APEAXPEBD0H@Z1?String_Split@@YAPEAX00H@ZU?$TagList@U?$ScManagedBy@$MUCCDynamicArray@@'
             '1?globalDynamicArray@@3U1@A@@@ScriptBinderDetail@@$0A@$00@ScriptBinderDetail@@YA?AURuntimeScriptValue@@'
             'PEAXPEBU1@H@Z')
    split_label = split + (' (struct RuntimeScriptValue __cdecl ScriptBinderDetail::MethodCall<&void * __cdecl '
                           'String_Split(char const *,char const *,int),struct ScriptBinderDetail::TagList<struct '
                           'ScManagedBy<struct CCDynamicArray globalDynamicArray> >,0,1>(void *,struct '
                           'RuntimeScriptValue const *,int)):')
    tag = ('ScManagedBy', 'globalDynamicArray')
    assert entry_of_symbol(split) == ('MethodCall', 'String_Split', (tag,)), entry_of_symbol(split)
    assert demangled_tags(split_label) == (tag,), demangled_tags(split_label)
    replace = ('??$MethodCall@$MP6APEBDPEBD00H@Z1?String_Replace@@YAPEBD000H@ZU?$TagList@U?$ScBoolArg@$01@@@'
               'ScriptBinderDetail@@$0A@$00$01@ScriptBinderDetail@@YA?AURuntimeScriptValue@@PEAXPEBU1@H@Z')
    assert entry_of_symbol(replace) == ('MethodCall', 'String_Replace', (('ScBoolArg', '2'),)), entry_of_symbol(replace)
    assert mangled_number('A@') == 0 and mangled_number('9') == 10 and mangled_number('BA@') == 16
    print('self-test passed')
    return 0


if __name__ == '__main__':
    sys.exit(main())
