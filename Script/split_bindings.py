#!/usr/bin/env python3
"""
Moves the hand-written script bindings of the engine out of the logic files (docs/refactor/PLAN.md 5.2, step 1):
every top-level `Sc_*` wrapper, `ScPl_*` plugin variant and `Register*API` function of a source file goes, byte for
byte and in its original order, to `Engine/game/script_api/<script type>_api.cpp` (the name comes from API_FILES).

    python Script/split_bindings.py [--repo <dir>] [--dest Engine/game/script_api] [--dry-run] [--no-build-lists]
                                    [--export-statics] (<source.cpp> ... | --all)

Without --export-statics, a run on a file:
  1. finds the bindings by name (BINDING), each with the comment lines directly above it, the comments and `#if`
     blocks that sit between bindings only, and the banner that heads the block (DROP_BANNERS, deleted when only
     blank lines and #include lines separate it from the first binding);
  2. writes the API file: the source's licence header, a one-line description, the includes the moved text needs
     (see "Includes"), the source's `using namespace` lines, the `extern` lines it needs that no header declares
     (reported: --export-statics declares them in a header first), then the moved text;
  3. removes the moved text from the source, and the includes and `extern` lines only the moved text used;
  4. formats the new file and the source with clang-format (AGS_CLANG_FORMAT, 19.1.7) and fails if the moved text
     or a line of the source other than an #include changed;
  5. adds the new file to Engine/CMakeLists.txt, Solutions/Engine.App/Engine.App.vcxproj and .filters and the
     Xcode project OSX/xcode/AGSKit (ENGINE_CMAKE, ENGINE_VCXPROJ, XCODE_PROJECT; filter GUIDs and Xcode IDs are
     derived from the path, so a run on another tree writes the same text), refusing a .cpp stem that Engine.App
     already compiles (MSBuild object names are flat).
A file is moved only if nothing stops it: a binding inside a namespace, an `#if` that holds bindings and other code,
a macro of the source the moved text uses, or a name the moved text needs from the source that no header declares
(a `static` function, a function or a global with no declaration). Those are reported, and the file is left alone.
Code that is not a binding stays where it is, also between bindings (reported as "left in place").

--export-statics is the step before the move, its own commit (it changes linkage, so it is not a move). For every
function and global of the source that the moved text needs and no header declares, it removes `static` (refused
when another .cpp defines the same name) and appends a declaration with a one-line comment (the script names it
implements) to the source's header (HEADERS; a header that does not exist is created and added to the build lists).
For a global of another file that the source declares `extern`, the `extern` goes to the header of the file that
defines it. A type the declarations name is forward-declared (a struct or class) or included (an enum or typedef); a
type of a namespace that the source names under a using-directive is written qualified.

Includes. The API file's includes are derived from the identifiers the moved text names, after expanding the
macros it uses (API_* bodies name functions such as get_translation) and adding the types of the globals it uses:
the source's includes (all of them, plus API_INCLUDES) that declare an identifier themselves are chosen by a greedy
cover, ties to the earlier include; an identifier that only the headers a header includes declare is covered by that
header. A system header is kept when the moved text names something SYSTEM_NAMES gives it (an unlisted one is kept
and reported). The source drops an include that the moved text used (or that sat after the first declaration, or is
in API_INCLUDES) when the text that stays needs nothing it declares and nothing its includes declare that the other
includes do not give. The result depends only on the text of the tree, so a second run gives the same files; the
build proves it compiles. What the parser assumes: the file is clang-formatted (Allman braces, no namespace
indentation), every top-level declaration starts at column 0, a body closes with a `}` at column 0, and only access
specifiers sit at column 0 inside one; a file that breaks this stops the run with the line number.

Running it again on a split file does nothing. Tree-specific facts are the tables below (API_FILES, HEADERS, SKIP,
BINDING, DROP_BANNERS, INCLUDE_ROOTS, the build files); upstream's trees (C++11, not clang-formatted) would need a
format pass first or a parser that does not rely on column 0, their own build lists (upstream has the Makefiles and
no Xcode check), and the C++11 headers checked for what the declarations name.
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys
import uuid

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import check_build_lists  # noqa: E402  (reuse its .pbxproj readers)

# ---------------------------------------------------------------------------------------------------------------
# Tables: the reviewed decisions and the facts of this tree
# ---------------------------------------------------------------------------------------------------------------

DEST = 'Engine/game/script_api'

# source file -> API file stem (the script type in snake_case); the file is <DEST>/<stem>_api.cpp
API_FILES = {
    'Engine/game/audio/audio_channel.cpp': 'audio_channel',
    'Engine/game/audio/audio_clip.cpp': 'audio_clip',
    'Engine/game/dialog/dialog.cpp': 'dialog',
    'Engine/game/dialog/dialog_options_rendering.cpp': 'dialog_options_rendering',
    'Engine/game/dialog/parser.cpp': 'parser',
    'Engine/game/dialog/speech.cpp': 'speech',
    'Engine/game/file.cpp': 'file',
    'Engine/game/game.cpp': 'game',
    'Engine/game/input/joystick.cpp': 'joystick',
    'Engine/game/input/touch.cpp': 'touch',
    'Engine/game/lib/date_time.cpp': 'date_time',
    'Engine/game/lib/math.cpp': 'math',
    'Engine/game/lib/script_containers.cpp': 'containers',
    'Engine/game/lib/string.cpp': 'string',
    'Engine/game/lib/utils_script.cpp': 'utils',
    'Engine/game/navigation/pathfinder_script.cpp': 'pathfinder',
    'Engine/game/render/drawing_surface.cpp': 'drawing_surface',
    'Engine/game/render/dynamic_sprite.cpp': 'dynamic_sprite',
    'Engine/game/render/overlay.cpp': 'overlay',
    'Engine/game/render/screen.cpp': 'screen',
    'Engine/game/render/shader_script.cpp': 'shader',
    'Engine/game/render/viewport_script.cpp': 'viewport',
    'Engine/game/save/savegame.cpp': 'save_info',
    'Engine/game/system.cpp': 'system',
    'Engine/game/ui/button.cpp': 'button',
    'Engine/game/ui/gui.cpp': 'gui',
    'Engine/game/ui/gui_control.cpp': 'gui_control',
    'Engine/game/ui/inv_window.cpp': 'inv_window',
    'Engine/game/ui/label.cpp': 'label',
    'Engine/game/ui/list_box.cpp': 'list_box',
    'Engine/game/ui/mouse.cpp': 'mouse',
    'Engine/game/ui/slider.cpp': 'slider',
    'Engine/game/ui/text_box.cpp': 'text_box',
    'Engine/game/video/video_script.cpp': 'video',
    'Engine/game/world/character.cpp': 'character',
    'Engine/game/world/hotspot.cpp': 'hotspot',
    'Engine/game/world/inventory_item.cpp': 'inventory_item',
    'Engine/game/world/object.cpp': 'object',
    'Engine/game/world/region.cpp': 'region',
    'Engine/game/world/room.cpp': 'room',
    'Engine/game/world/view_frame.cpp': 'view_frame',
    'Engine/game/world/walk_behind.cpp': 'walk_behind',
    'Engine/game/world/walkable_area.cpp': 'walkable_area',
    'Engine/script/dynobj/cc_dynamic_array.cpp': 'dynamic_array',
}

# source file -> the header that gets the declarations of --export-statics, where it is not <same name>.h
HEADERS = {
    'Engine/script/dynobj/cc_dynamic_array.cpp': 'Engine/script/dynobj/cc_dynamic_array.h',
    'Engine/game/save/savegame.cpp': 'Engine/game/save/savegame.h',
}

# files with bindings that are not split, and why (--all lists the others it finds with no API_FILES row)
SKIP = {
    'Engine/game/script_api/global_api.cpp': 'bindings only (the global functions)',
    'Engine/plugin/plugin_stubs.cpp': 'the stand-in functions of missing plugins, registered by name, not a script type',
}

# top-level functions that are bindings (RegisterStaticObjects: game.cpp's registration of the script objects
# game, mouse and palette, called from game/script_api/exports.cpp next to the Register*API functions)
BINDING = re.compile(r'^(?:Sc_\w+|ScPl_\w+|Register\w*API|RegisterStaticObjects)$')
# banners that head the binding block; the API file has its own description, so they are deleted
DROP_BANNERS = ('Script API Functions',)
# headers whose files may include script/script_api.h after the split, and why
SCRIPT_API_INCLUDERS = {
    'Engine/script/script_api.cpp': 'the implementation of script_api.h',
}
# identifiers that tell a system header is needed; an unlisted system header is kept when the moved text names
# anything from the C or C++ library, and reported
SYSTEM_NAMES = {
    '<cstdio>': {'printf', 'sprintf', 'snprintf', 'vsnprintf', 'FILE', 'fopen', 'fclose', 'fprintf', 'sscanf'},
    '<stdio.h>': {'printf', 'sprintf', 'snprintf', 'vsnprintf', 'FILE', 'fopen', 'fclose', 'fprintf', 'sscanf'},
    '<cstring>': {'strlen', 'strcpy', 'strncpy', 'strcmp', 'strcat', 'memcpy', 'memset', 'strchr', 'strstr',
                  'memmove', 'strncmp'},
    '<string.h>': {'strlen', 'strcpy', 'strncpy', 'strcmp', 'strcat', 'memcpy', 'memset', 'strchr', 'strstr',
                   'memmove', 'strncmp'},
    '<cmath>': {'sqrt', 'sin', 'cos', 'tan', 'atan', 'atan2', 'pow', 'floor', 'ceil', 'fabs', 'log', 'exp', 'asin',
                'acos', 'log10', 'sinh', 'cosh', 'tanh', 'fmod', 'round'},
    '<math.h>': {'sqrt', 'sin', 'cos', 'tan', 'atan', 'atan2', 'pow', 'floor', 'ceil', 'fabs', 'log', 'exp', 'asin',
                 'acos', 'log10', 'sinh', 'cosh', 'tanh', 'fmod', 'round'},
    '<cstdlib>': {'abs', 'atoi', 'malloc', 'free', 'rand', 'srand', 'strtol'},
    '<stdlib.h>': {'abs', 'atoi', 'malloc', 'free', 'rand', 'srand', 'strtol'},
    '<algorithm>': {'min', 'max', 'sort', 'find', 'find_if', 'clamp', 'remove_if', 'fill', 'swap'},
    '<vector>': {'vector'},
    '<memory>': {'unique_ptr', 'shared_ptr', 'make_unique', 'make_shared'},
    '<string>': {'string'},
    '<utility>': {'move', 'pair', 'make_pair'},
    '<cstdint>': {'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'int8_t', 'int16_t', 'int64_t'},
    '<limits>': {'numeric_limits'},
    '<cctype>': {'toupper', 'tolower', 'isdigit', 'isalpha', 'isspace'},
    '<ctype.h>': {'toupper', 'tolower', 'isdigit', 'isalpha', 'isspace'},
    '<stdarg.h>': {'va_list', 'va_start', 'va_end'},
    '<cstdarg>': {'va_list', 'va_start', 'va_end'},
    '<deque>': {'deque'},
    '<set>': {'set'},
    '<map>': {'map'},
    '<unordered_map>': {'unordered_map'},
    '<unordered_set>': {'unordered_set'},
    '<queue>': {'queue', 'priority_queue'},
    '<stack>': {'stack'},
    '<array>': {'array'},
    '<tuple>': {'tuple', 'tie', 'make_tuple'},
    '<functional>': {'function', 'bind', 'hash'},
    '<chrono>': {'chrono'},
    '<regex>': {'regex', 'regex_match', 'regex_search'},
    '<cassert>': {'assert'},
    '<assert.h>': {'assert'},
    '<time.h>': {'time_t', 'time', 'localtime', 'gmtime', 'tm'},
    '<ctime>': {'time_t', 'time', 'localtime', 'gmtime', 'tm'},
}
# include roots, in the compilers' order after the includer's own directory (Engine.App: Common, then Engine)
INCLUDE_ROOTS = ('Common', 'Engine', 'Plugins')
# headers every API file may take even where its source did not include them directly (the macros, the registration)
API_INCLUDES = ('Engine/script/script_api.h', 'Engine/script/script_runtime.h')
# directories searched for the definitions of the types a new declaration names, and for name clashes
CODE_DIRS = ('Engine', 'Common')
SKIP_DIRS = ('libsrc', 'libinclude', 'test')

ENGINE_CMAKE = 'Engine/CMakeLists.txt'
ENGINE_VCXPROJ = 'Solutions/Engine.App/Engine.App.vcxproj'
XCODE_PROJECT = 'OSX/xcode/AGSKit/AGSKit.xcodeproj/project.pbxproj'
# the Xcode target whose Sources phase compiles the engine (the one that lists this file)
XCODE_ANCHOR = 'game/save/savegame.cpp'
# namespace of the uuid5 GUIDs and Xcode IDs this tool derives
ID_NAMESPACE = uuid.UUID('6f0b6a52-7c1e-4b0e-9a55-5a2b1e5d3c21')

KEYWORDS = set('''alignas alignof and asm auto bool break case catch char char16_t char32_t class const constexpr
const_cast continue decltype default delete do double dynamic_cast else enum explicit export extern false float for
friend goto if inline int long mutable namespace new noexcept not nullptr operator or private protected public
register reinterpret_cast return short signed sizeof static static_assert static_cast struct switch template this
throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while override final
defined include pragma once ifdef ifndef endif elif define undef error'''.split())


class Fail(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------------
# Text helpers
# ---------------------------------------------------------------------------------------------------------------

def strip_code(text):
    """The text with comments, string and character literals blanked (newlines kept), so that braces and
    identifiers can be read from it."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i))
            i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(''.join(ch if ch == '\n' else ' ' for ch in text[i:j]))
            i = j
        elif c == 'R' and text.startswith('R"', i) and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == '_')):
            m = re.match(r'R"([^(\s]*)\(', text[i:])
            end = text.find(')' + m.group(1) + '"', i) if m else -1
            if m is None or end < 0:
                raise Fail('a raw string literal the parser cannot read')
            end += len(m.group(1)) + 2
            out.append(''.join(ch if ch == '\n' else ' ' for ch in text[i:end]))
            i = end
        elif c in '"\'':
            j = i + 1
            while j < n and text[j] != c:
                if text[j] == '\\':
                    j += 1
                elif text[j] == '\n':
                    break
                j += 1
            out.append(c + ' ' * (j - i - 1) + (c if j < n else ''))
            i = j + 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


IDENT = re.compile(r'(?<![\w.])(?<!->)[A-Za-z_]\w*')


def identifiers(code):
    """The identifiers of comment-free code, member names after . and -> left out."""
    return {m.group(0) for m in IDENT.finditer(code)} - KEYWORDS


def read_eol(path):
    """(text with LF line ends, the file's line end, the file's BOM); a file mixing CRLF and LF fails."""
    with open(path, 'rb') as f:
        data = f.read()
    bom = b''
    if data.startswith(b'\xef\xbb\xbf'):
        bom, data = data[:3], data[3:]
    text = data.decode('utf-8')
    crlf = text.count('\r\n')
    if crlf and crlf != text.count('\n'):
        raise Fail('%s mixes CRLF and LF line ends' % path)
    return text.replace('\r\n', '\n'), '\r\n' if crlf else '\n', bom


def read_text(path):
    return read_eol(path)[0]


def write_text(path, text, eol='\n', bom=b''):
    """Writes utf-8 through a temporary file, with the given line end (a working tree with core.autocrlf has CRLF
    files; git stores LF either way)."""
    tmp = path + '.split_bindings.tmp'
    with open(tmp, 'wb') as f:
        f.write(bom + text.replace('\n', eol).encode('utf-8'))
    os.replace(tmp, path)


# ---------------------------------------------------------------------------------------------------------------
# Parser: top-level items of a clang-formatted .cpp
# ---------------------------------------------------------------------------------------------------------------

class Item:
    """A top-level piece of a file: lines [start, end) (0-based)."""

    def __init__(self, kind, start, end, name=None, static=False, function=False, header=''):
        self.kind = kind          # blank, comment, pp, code, ns_open, ns_close
        self.start = start
        self.end = end
        self.name = name
        self.static = static
        self.function = function
        self.header = header      # code: the text before the body
        self.namespace = 0        # nesting depth of namespaces around it
        self.pp_inside = False    # code: has preprocessor lines in its body

    def __repr__(self):
        return '%s %d-%d %s' % (self.kind, self.start + 1, self.end, self.name or '')


def parse(lines, where):
    """[Item] of a file's lines (each ending with '\\n'). Fails on a line the assumptions do not cover."""
    stripped = strip_code(''.join(lines)).split('\n')
    items = []
    ns = 0
    i, n = 0, len(lines)
    while i < n:
        raw = lines[i].rstrip('\n')
        code = stripped[i]
        if not raw.strip():
            items.append(Item('blank', i, i + 1))
            i += 1
        elif raw.startswith('//'):
            items.append(Item('comment', i, i + 1))
            i += 1
        elif raw.startswith('/*') and code.strip():
            # a comment before code on the same line (/* static */ Type Class::Method(...))
            items.append(code_item(lines, stripped, i, where))
            items[-1].namespace = ns
            i = items[-1].end
        elif raw.startswith('/*'):
            j = i
            while '*/' not in lines[j]:
                j += 1
                if j >= n:
                    raise Fail('%s:%d: unterminated comment' % (where, i + 1))
            items.append(Item('comment', i, j + 1))
            i = j + 1
        elif raw.startswith('#'):
            j = i
            while lines[j].rstrip('\n').endswith('\\'):
                j += 1
            items.append(Item('pp', i, j + 1))
            items[-1].namespace = ns
            i = j + 1
        elif raw[0] in ' \t':
            raise Fail('%s:%d: indented line at top level (the file is not clang-formatted as assumed)'
                       % (where, i + 1))
        elif re.match(r'(?:inline\s+)?namespace\b[^;=]*$', code) or re.match(r'extern\s+"C"\s*$', raw):
            # namespace X / { on the next line (Allman), or namespace X {
            if code.rstrip().endswith('{'):
                end = i + 1
            elif i + 1 < n and lines[i + 1].strip() == '{':
                end = i + 2
            else:
                raise Fail('%s:%d: namespace without a brace on this or the next line' % (where, i + 1))
            items.append(Item('ns_open', i, end, name=raw.strip()))
            ns += 1
            items[-1].namespace = ns
            i = end
        elif raw.startswith('}') and ns > 0 and re.match(r'^\}\s*(//.*)?$', raw):
            items.append(Item('ns_close', i, i + 1))
            items[-1].namespace = ns
            ns -= 1
            i += 1
        else:
            items.append(code_item(lines, stripped, i, where))
            items[-1].namespace = ns
            i = items[-1].end
    if ns != 0:
        raise Fail('%s: a namespace is not closed at column 0' % where)
    return items


def code_item(lines, stripped, start, where):
    depth = 0
    seen_brace = False
    header = []
    pp_inside = False
    i = start
    n = len(lines)
    while i < n:
        code = stripped[i]
        if i > start and lines[i].lstrip().startswith('#'):
            pp_inside = True
            j = i
            while lines[j].rstrip('\n').endswith('\\'):
                j += 1
            i = j + 1
            continue
        if not seen_brace and not lines[i].strip():
            raise Fail('%s:%d: a declaration with a blank line before its body or semicolon' % (where, i + 1))
        for ch in code:
            if ch == '{':
                if not seen_brace:
                    seen_brace = True
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth < 0:
                    raise Fail('%s:%d: unbalanced closing brace' % (where, i + 1))
        if not seen_brace:
            header.append(code)
        elif depth > 0 and len(header) == i - start:
            header.append(code[:code.find('{')] if '{' in code else code)
        last = code.rstrip()
        i += 1
        if depth == 0 and last and last[-1] in ';}':
            break
        if depth > 0 and i < n and not lines[i].startswith('}') and lines[i][:1] not in ' \t#\n' \
                and lines[i].strip() and seen_brace and not re.match(r'(?:public|protected|private)\s*:', lines[i]):
            raise Fail('%s:%d: a line at column 0 inside a body (not clang-formatted as assumed)' % (where, i + 1))
    else:
        raise Fail('%s:%d: declaration not closed' % (where, start + 1))
    head = ' '.join(h.strip() for h in header)
    item = Item('code', start, i, header=head)
    item.pp_inside = pp_inside
    body = seen_brace and '(' in head and '=' not in head.split('(')[0]
    m = re.search(r'([A-Za-z_]\w*)\s*\(', head) if '(' in head else None
    if m and not head.startswith(('struct ', 'class ', 'enum ', 'union ', 'typedef ', 'using ')):
        item.name = m.group(1)
        item.function = body
        if not body and head.rstrip().endswith(')') is False and '=' in head.split('(')[0]:
            item.function = False
    else:
        m = re.match(r'(?:struct|class|union|enum(?:\s+class)?)\s+(\w+)', head)
        if m:
            item.name = m.group(1)
        else:
            m = re.search(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?:=.*|\{.*|;)?$', head.split('=')[0].rstrip(' ;'))
            item.name = m.group(1) if m else None
    item.static = bool(re.match(r'(?:inline\s+)?static\b', head))
    return item


# ---------------------------------------------------------------------------------------------------------------
# Header index: what each header declares
# ---------------------------------------------------------------------------------------------------------------

def blank_function_bodies(code):
    """Comment-free code with the bodies of inline functions blanked, so that what is left declares."""
    out = list(code)
    i = 0
    n = len(code)
    while i < n:
        if code[i] == '{':
            j = i - 1
            while j >= 0 and code[j].isspace():
                j -= 1
            k = j
            while k >= 0 and (code[k].isalnum() or code[k] == '_'):
                k -= 1
            word = code[k + 1:j + 1]
            if (j >= 0 and code[j] == ')') or word in ('const', 'override', 'noexcept', 'final'):
                depth = 0
                for e in range(i, n):
                    if code[e] == '{':
                        depth += 1
                    elif code[e] == '}':
                        depth -= 1
                        if depth == 0:
                            break
                for x in range(i + 1, e):
                    if out[x] != '\n':
                        out[x] = ' '
                i = e
        i += 1
    return ''.join(out)


class HeaderIndex:
    def __init__(self, repo):
        self.repo = repo
        self.direct_cache = {}
        self.includes_cache = {}
        self.closure_cache = {}
        self.macros = {}  # macro name -> identifiers of its body (from every header read)

    def resolve(self, name, includer_dir):
        """The repository path of a quoted include, or None (system or outside)."""
        candidates = [os.path.join(includer_dir, name)] + [os.path.join(r, name) for r in INCLUDE_ROOTS]
        for c in candidates:
            if os.path.isfile(os.path.join(self.repo, c)):
                return os.path.normpath(c).replace(os.sep, '/')
        return None

    def includes_of(self, path):
        if path not in self.includes_cache:
            text = read_text(os.path.join(self.repo, path))
            found = []
            for m in re.finditer(r'^\s*#\s*include\s+"([^"]+)"', text, re.M):
                r = self.resolve(m.group(1), os.path.dirname(path))
                if r:
                    found.append(r)
            self.includes_cache[path] = found
        return self.includes_cache[path]

    def direct(self, path):
        """Names a header declares itself."""
        if path in self.direct_cache:
            return self.direct_cache[path]
        text = read_text(os.path.join(self.repo, path))
        code = strip_code(text)
        names = set()
        joined = re.sub(r'\\\n', ' ', code)
        for m in re.finditer(r'^\s*#\s*define\s+(\w+)(\([^)]*\))?(.*)$', joined, re.M):
            names.add(m.group(1))
            self.macros[m.group(1)] = identifiers(m.group(3)) - set(re.findall(r'\w+', m.group(2) or ''))
        body = blank_function_bodies(re.sub(r'^\s*#.*$', '', joined, flags=re.M))
        # type definitions, not forward declarations (which every header has for RuntimeScriptValue and the like)
        # nor namespaces (which every header opens)
        names |= set(re.findall(r'\b(?:struct|class|union)\s+(\w+)\s*(?:final\s*)?[:{]', body))
        names |= set(re.findall(r'\benum\s+(?:class\s+)?(\w+)', body))
        names |= set(re.findall(r'\busing\s+(\w+)\s*=', body))
        names |= set(re.findall(r'\btypedef\b[^;{]*?(\w+)\s*(?:\[[^\]]*\])?\s*;', body))
        names |= set(re.findall(r'\btypedef\b[^;]*?\(\s*\*\s*(\w+)\s*\)', body))
        names |= set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', body)) - KEYWORDS
        names |= set(re.findall(r'\bextern\b[^;(]*?(\w+)\s*(?:\[[^\]]*\])*\s*;', body))
        for m in re.finditer(r'\benum\b[^;{]*\{([^}]*)\}', body):
            for part in m.group(1).split(','):
                mm = re.match(r'\s*([A-Za-z_]\w*)', part)
                if mm:
                    names.add(mm.group(1))
        # globals and constants declared at file scope: "Type name;" "const T name = ..."
        names |= set(re.findall(r'^\s*(?:static\s+|inline\s+)?(?:const(?:expr)?\s+)?(?!(?:struct|class|union|enum|friend|'
                                r'return|delete|using|typedef)\b)[\w:<>]+[\s*&]+(\w+)\s*(?:\[[^\]]*\])*'
                                r'\s*(?:=[^;]*)?;', body, re.M))
        names -= KEYWORDS
        self.direct_cache[path] = names
        return names

    def closure(self, path, seen=None):
        """Names a header declares, itself or through the headers it includes."""
        if path in self.closure_cache:
            return self.closure_cache[path]
        seen = set() if seen is None else seen
        seen.add(path)
        names = set(self.direct(path))
        for inc in self.includes_of(path):
            if inc not in seen:
                names |= self.closure(inc, seen)
        self.closure_cache[path] = names
        return names

    def closure_files(self, path, seen=None):
        seen = set() if seen is None else seen
        if path in seen:
            return seen
        seen.add(path)
        for inc in self.includes_of(path):
            self.closure_files(inc, seen)
        return seen

    def expand_macros(self, names):
        """The names plus the names the bodies of the macros among them use, recursively."""
        result = set(names)
        todo = [n for n in names if n in self.macros]
        while todo:
            m = todo.pop()
            for x in self.macros[m]:
                if x not in result:
                    result.add(x)
                    if x in self.macros:
                        todo.append(x)
        return result


# ---------------------------------------------------------------------------------------------------------------
# Analysis of one source
# ---------------------------------------------------------------------------------------------------------------

def item_text(lines, item):
    return ''.join(lines[item.start:item.end])


def is_banner(lines, item_group):
    """A run of // lines framed by //==== lines."""
    texts = [lines[i].rstrip('\n') for it in item_group for i in range(it.start, it.end)]
    return len(texts) >= 3 and re.match(r'^//=+$', texts[0]) and re.match(r'^//=+$', texts[-1])


class Plan:
    """What a run would do to one source."""

    def __init__(self, source):
        self.source = source
        self.moved = []             # [Item] in order (functions, their comments, #if wrappers, banners)
        self.functions = []         # names of the moved functions
        self.dropped = []           # [Item] deleted (banner heading the block)
        self.leftovers = []         # (line, text) inside the binding region and not moved
        self.after = 0              # lines of code after the last binding
        self.blockers = []          # reasons the file cannot be moved
        self.statics = []           # static functions the moved text calls
        self.undeclared = []        # non-static names of the source the moved text needs, no header declares
        self.externs_copied = []    # extern lines written into the API file
        self.externs_moved = []     # for each: True if the source loses the line (moved), False if it keeps it
        self.externs_dropped = []   # extern lines removed from the source
        self.api_includes = []
        self.includes_dropped = []
        self.includes_before = 0
        self.includes_after = 0
        self.pp_branches = []       # #if lines inside the moved text
        self.notes = []


def group_comments(items):
    """For each code item, the comment items directly above it (no blank line between)."""
    attached = {}
    for k, it in enumerate(items):
        if it.kind == 'code':
            j = k - 1
            group = []
            while j >= 0 and items[j].kind == 'comment':
                group.insert(0, items[j])
                j -= 1
            attached[k] = group
    return attached


def analyse(repo, index, source, dest):
    """A Plan for one source (no file is written)."""
    path = os.path.join(repo, source)
    text = read_text(path)
    lines = text.splitlines(keepends=True)
    if lines and not lines[-1].endswith('\n'):
        raise Fail('%s: no newline at the end' % source)
    items = parse(lines, source)
    plan = Plan(source)
    plan.text = text
    plan.lines = lines
    plan.items = items
    attached = group_comments(items)
    # definitions, and declarations of bindings defined elsewhere (walkablearea.cpp declares global_api.cpp's
    # Sc_GetDrawingSurfaceForWalkableArea for its table): both go with the table
    binding_idx = [k for k, it in enumerate(items) if it.kind == 'code' and it.name and BINDING.match(it.name)
                   and (it.function or '(' in it.header)]
    plan.binding_idx = binding_idx
    if not binding_idx:
        return plan
    for k in binding_idx:
        if items[k].namespace:
            plan.blockers.append('%s:%d: %s is inside a namespace' % (source, items[k].start + 1, items[k].name))
    moved = set()
    for k in binding_idx:
        moved.add(k)
        for c in attached[k]:
            moved.add(items.index(c))
    first, last = binding_idx[0], binding_idx[-1]
    # banners and #if blocks between bindings only
    k = first
    while k <= last:
        it = items[k]
        if it.kind == 'pp' and re.match(r'#\s*if', lines[it.start]):
            depth, j, inner = 0, k, []
            while j < len(items):
                t = items[j]
                if t.kind == 'pp' and re.match(r'#\s*if', lines[t.start]):
                    depth += 1
                elif t.kind == 'pp' and re.match(r'#\s*endif', lines[t.start]):
                    depth -= 1
                    if depth == 0:
                        break
                inner.append(j)
                j += 1
            body = [x for x in inner[1:] if items[x].kind not in ('blank', 'pp', 'comment')]
            if body and all(x in moved for x in body):
                moved.update(x for x in inner if items[x].kind != 'blank' or True)
                moved.add(j)
                plan.pp_branches.append(lines[it.start].strip())
            elif any(x in moved for x in body):
                plan.blockers.append('%s:%d: "%s" holds bindings and other code'
                                     % (source, it.start + 1, lines[it.start].strip()))
            k = j + 1
            continue
        k += 1
    # comment groups (blank-separated) whose neighbours on both sides are moved
    k = first
    while k <= last:
        if items[k].kind == 'comment' and k not in moved:
            j = k
            while j < len(items) and items[j].kind == 'comment':
                j += 1
            prev = k - 1
            while prev >= 0 and items[prev].kind == 'blank':
                prev -= 1
            nxt = j
            while nxt < len(items) and items[nxt].kind == 'blank':
                nxt += 1
            if prev in moved and nxt in moved:
                moved.update(range(k, j))
            k = j
            continue
        k += 1
    # the banner that heads the block: the last comment group before the first moved item, with only blanks and
    # #include lines between
    k = min(moved) - 1
    includes_above = []
    while k >= 0 and (items[k].kind == 'blank' or (items[k].kind == 'pp' and lines[items[k].start].startswith('#include'))):
        if items[k].kind == 'pp':
            includes_above.insert(0, k)
        k -= 1
    if k >= 0 and items[k].kind == 'comment':
        j = k
        while j > 0 and items[j - 1].kind == 'comment':
            j -= 1
        group = items[j:k + 1]
        if is_banner(lines, group) and any(b in item_text(lines, g) for g in group for b in DROP_BANNERS):
            plan.dropped = group
            plan.dropped_idx = list(range(j, k + 1))
    plan.mid_includes = includes_above
    if not plan.dropped:
        # code between a DROP_BANNERS banner and the first binding keeps the banner where it is: report it
        for j in range(min(moved) - 1, -1, -1):
            if items[j].kind == 'comment' and any(b in item_text(lines, items[j]) for b in DROP_BANNERS):
                plan.leftovers += [(items[x].start + 1, 'code', '%s (under the banner)' % items[x].name)
                                   for x in range(j, min(moved)) if items[x].kind == 'code']
                break
            if items[j].kind == 'comment' and re.match(r'^//=+$', lines[items[j].start].rstrip('\n')) and j > 0 \
                    and items[j - 1].kind != 'comment':
                break  # the top of another banner
    # leftovers in the region, and code after it
    region_start = min(moved)
    for k in range(region_start, last + 1):
        it = items[k]
        if k in moved or it.kind == 'blank':
            continue
        if it.kind == 'pp' and k in includes_above:
            continue
        first_line = lines[it.start].rstrip('\n')
        plan.leftovers.append((it.start + 1, it.kind, it.name or first_line[:60]))
    plan.after = sum(it.end - it.start for it in items[last + 1:] if it.kind == 'code')
    plan.moved_idx = sorted(moved)
    plan.moved = [items[k] for k in plan.moved_idx]
    plan.functions = [items[k].name for k in binding_idx if items[k].function]
    for it in plan.moved:
        if it.kind == 'code' and it.pp_inside:
            for i in range(it.start, it.end):
                if re.match(r'\s*#\s*if', lines[i]):
                    plan.pp_branches.append('%s (in %s)' % (lines[i].strip(), it.name))
    analyse_names(repo, index, plan, dest)
    return plan


def top_includes(plan):
    """Item indexes of the #include lines before the first code item."""
    out = []
    for k, it in enumerate(plan.items):
        if it.kind in ('code', 'ns_open'):
            break
        if it.kind == 'pp' and plan.lines[it.start].startswith('#include'):
            out.append(k)
    return out


def include_name(line):
    m = re.match(r'#include\s+([<"][^>"]+[>"])', line)
    return m.group(1) if m else None


def analyse_names(repo, index, plan, dest):
    lines, items = plan.lines, plan.items
    moved_set = set(plan.moved_idx)
    dropped_set = set(getattr(plan, 'dropped_idx', []))
    moved_code = strip_code(''.join(item_text(lines, it) for it in plan.moved))
    remaining_idx = [k for k in range(len(items)) if k not in moved_set and k not in dropped_set]
    # read every header the source reaches first: the macro table that expand_macros uses fills as headers are read
    for it in items:
        name = include_name(lines[it.start]) if it.kind == 'pp' else None
        path = index.resolve(name[1:-1], os.path.dirname(plan.source)) if name and name[0] == '"' else None
        if path:
            index.closure(path)
    for path in API_INCLUDES:
        index.closure(path)
    used_moved = used_names(repo, index, moved_code, [it for it in items if it.kind == 'code'])
    # names the source defines at top level (outside the moved text) and the ones its externs declare
    defined = {}
    externs = {}
    for k in remaining_idx:
        it = items[k]
        if it.kind != 'code' or not it.name:
            continue
        if it.header.startswith('extern ') and not it.function:
            externs[it.name] = k
        elif it.header.startswith(('using namespace', 'using ')) and not it.function:
            continue
        elif it.name not in defined or (it.function and not items[defined[it.name]].function):
            defined[it.name] = k  # a function's definition wins over its prototype earlier in the file
    src_dir = os.path.dirname(plan.source)
    top = top_includes(plan)
    plan.top_includes = top
    # every #include of the source, resolved (the ones after the first declaration are the bindings' group, or
    # the group of some other part of the file: all are candidates)
    all_inc = []
    for k, it in enumerate(items):
        if it.kind == 'pp' and lines[it.start].startswith('#include'):
            name = include_name(lines[it.start])
            path = index.resolve(name[1:-1], src_dir) if name and name[0] == '"' else None
            all_inc.append((k, name, path))
    plan.all_includes = all_inc
    inc_items = [k for k, _, _ in all_inc]
    candidates = list(all_inc)
    for extra in API_INCLUDES:
        if extra not in [p for _, _, p in all_inc]:
            candidates.append((None, '"%s"' % include_path(extra), extra))
    # what the moved text needs from the source itself
    own_header = header_for(repo, plan.source)
    declared_anywhere = set()
    for k, name, path in all_inc:
        if path:
            declared_anywhere |= index.closure(path)
    for name in sorted(used_moved & set(defined)):
        it = items[defined[name]]
        if it.static:
            plan.statics.append((name, it.start + 1))
        elif name not in declared_anywhere:
            plan.undeclared.append((name, it.start + 1, 'function' if it.function else 'definition'))
    # #define at file level used by the moved text
    for k in remaining_idx:
        it = items[k]
        m = re.match(r'#\s*define\s+(\w+)', lines[it.start]) if it.kind == 'pp' else None
        if m and m.group(1) in used_moved:
            plan.blockers.append('%s:%d: the moved text uses the macro %s defined in the source'
                                 % (plan.source, it.start + 1, m.group(1)))
    for name, line, kind in plan.undeclared:
        plan.blockers.append('%s:%d: the moved text needs %s (%s), which no header declares'
                             % (plan.source, line, name, kind))
    for name, line in plan.statics:
        plan.blockers.append('%s:%d: the moved text calls the static function %s (--export-statics first)'
                             % (plan.source, line, name))
    # externs: from a header if one declares the name, else copied
    remaining_code = strip_code(''.join(item_text(lines, items[k]) for k in remaining_idx
                                        if items[k].kind == 'code' and k not in externs.values()))
    used_remaining = index.expand_macros(identifiers(remaining_code))
    plan.extern_headers = []
    plan.extern_names = []
    for name, k in sorted(externs.items(), key=lambda x: x[1]):
        if name in used_moved:
            hdr = extern_header(repo, index, name)
            if hdr:
                if hdr not in plan.extern_headers:
                    plan.extern_headers.append(hdr)
            else:
                plan.externs_copied.append(item_text(lines, items[k]).rstrip('\n'))
                plan.extern_names.append((name, items[k].start + 1))
                # the source loses the line when nothing that stays names the global: then it moved, else the
                # count of extern lines in .cpp files rises by one
                plan.externs_moved.append(name not in used_remaining)
        if name in used_moved and name not in used_remaining:
            plan.externs_dropped.append(k)
    # the extern declarations that stay need their types
    for name, k in externs.items():
        if k not in plan.externs_dropped:
            used_remaining |= identifiers(re.sub(r'\[[^\]]*\]', '', items[k].header)) - {name}
    # API file includes: greedy cover of the moved names by the candidates, direct declarations first
    cand_paths = [(k, n, p) for k, n, p in candidates if p]
    chosen = derive_includes(index, used_moved, cand_paths)
    names = [n for _, n, _ in chosen]
    for hdr in plan.extern_headers:
        name = '"%s"' % include_path(hdr)
        if name not in names:
            names.append(name)
    # the own header of the source, first, if the moved text uses anything it declares
    if own_header:
        own = '"%s"' % include_path(own_header)
        if own in names:
            names.remove(own)
            names.insert(0, own)
    # system headers of the source
    names += system_includes([name for k, name, path in all_inc if k in inc_items], used_moved, plan.notes)
    plan.api_includes = names
    # source includes to drop: used by the moved text (directly) or in the binding group, and nothing the remaining
    # text needs comes from them that the kept includes do not give
    chosen_paths = {p for _, _, p in chosen}
    drop = []
    keep_paths = [p for k, n, p in all_inc if p]
    for k, name, path in reversed(all_inc):
        if not path:
            continue
        # the bindings' own includes: chosen for the API file, or in a group after the first declaration
        related = path in chosen_paths or k not in top or path in API_INCLUDES
        if not related:
            continue
        if index.direct(path) & used_remaining:
            continue
        others = [p for p in keep_paths if p != path]
        given = set()
        for p in others:
            given |= index.closure(p)
        if (index.closure(path) & used_remaining) - given:
            continue
        drop.append(k)
        keep_paths.remove(path)
    plan.includes_dropped = sorted(drop)
    plan.includes_before = len(all_inc)
    plan.includes_after = len(all_inc) - len(drop)
    plan.defined = defined
    plan.used_moved = used_moved


def used_names(repo, index, code, code_items):
    """The names comment-free code needs: its identifiers, the names the macros among them use, and the names of
    the types of the globals it uses (ccDynamicCharacter.GetType() needs the type complete), read from an `extern`
    declaration among code_items (Items whose header starts with extern) or else from a header."""
    used = index.expand_macros(identifiers(code))
    for it in code_items:
        if it.header.startswith('extern ') and it.name in used:
            used |= identifiers(re.sub(r'\[[^\]]*\]', '', it.header)) - {it.name}
    for name in sorted(used):
        for _, decl in extern_decls(repo, name)[:1]:
            used |= identifiers(re.sub(r'\[[^\]]*\]', '', decl)) - {name}
    return used


def include_cover(need, sets):
    """Greedy set cover: [(key, names)] -> the keys giving most of the names still needed, ties to the earlier one.
    Names no set has are left out."""
    uncovered = {x for x in need if any(x in s for _, s in sets)}
    picked = []
    while uncovered:
        best = max(range(len(sets)), key=lambda i: (len(sets[i][1] & uncovered), -i))
        picked.append(sets[best][0])
        uncovered -= sets[best][1]
    return picked


def derive_includes(index, used, candidates, direct=None, closure=None):
    """The candidates (tuples whose last element is a header's repository path) that cover the used names: first
    by what each declares itself, then what is left by what each brings through its includes. direct and closure
    (path -> names) replace the index's where given (a header with declarations still to be added)."""
    direct = direct or index.direct
    closure = closure or index.closure
    chosen = include_cover(used, [(c, direct(c[-1])) for c in candidates])
    covered = set()
    for c in chosen:
        covered |= closure(c[-1])
    rest = used - covered
    chosen += [c for c in include_cover(rest, [(c, closure(c[-1])) for c in candidates]) if c not in chosen]
    return chosen


def system_includes(names, used, notes):
    """The system headers among include names (<...>) that the used names need (SYSTEM_NAMES); an unlisted one is
    kept and noted."""
    out = []
    for name in names:
        if name and name.startswith('<'):
            known = SYSTEM_NAMES.get(name)
            if known is None:
                notes.append('kept the unlisted system header %s' % name)
                out.append(name)
            elif known & used:
                out.append(name)
    return out


def include_path(repo_path):
    for r in INCLUDE_ROOTS:
        if repo_path.startswith(r + '/'):
            return repo_path[len(r) + 1:]
    return repo_path


def header_for(repo, source):
    h = HEADERS.get(source, os.path.splitext(source)[0] + '.h')
    return h if os.path.isfile(os.path.join(repo, h)) else None


_EXTERN_CACHE = {}


def extern_decls(repo, name):
    """[(header, declaration)] of the extern declarations of the global `name` in the headers of CODE_DIRS."""
    if not _EXTERN_CACHE:
        for p in code_files(repo, '.h'):
            body = strip_code(read_text(os.path.join(repo, p)))
            for m in re.finditer(r'^extern\b[^;(]*?(\w+)\s*(?:\[[^\]]*\])*\s*;', body, re.M):
                _EXTERN_CACHE.setdefault(m.group(1), []).append((p, m.group(0)))
        _EXTERN_CACHE[None] = []
    return sorted(_EXTERN_CACHE.get(name, []))


def extern_header(repo, index, name):
    """A header of CODE_DIRS that declares the global `name` with extern, or None."""
    found = extern_decls(repo, name)
    return found[0][0] if found else None


def code_files(repo, ext):
    """Repository paths of the files with this extension under CODE_DIRS, third-party and tests left out."""
    out = []
    for root in CODE_DIRS:
        for d, dirs, files in os.walk(os.path.join(repo, root)):
            dirs[:] = sorted(x for x in dirs if x not in SKIP_DIRS)
            for f in sorted(files):
                if f.endswith(ext):
                    out.append(os.path.relpath(os.path.join(d, f), repo).replace(os.sep, '/'))
    return out


# ---------------------------------------------------------------------------------------------------------------
# clang-format
# ---------------------------------------------------------------------------------------------------------------

def clang_format():
    exe = os.environ.get('AGS_CLANG_FORMAT', 'clang-format')
    try:
        out = subprocess.run([exe, '--version'], capture_output=True, text=True).stdout
    except OSError:
        raise Fail('clang-format not found: set AGS_CLANG_FORMAT to clang-format 19.1.7')
    if 'version 19.' not in out:
        raise Fail('%s is not clang-format 19 (%s): set AGS_CLANG_FORMAT' % (exe, out.strip()))
    return exe


def format_file(repo, path, lines_ranges=None):
    """Formats a file in place, whole or only the given 1-based line ranges."""
    cmd = [clang_format(), '-i', '--style=file']
    for a, b in lines_ranges or []:
        cmd.append('--lines=%d:%d' % (a, b))
    r = subprocess.run(cmd + [os.path.join(repo, path)], capture_output=True, text=True)
    if r.returncode != 0:
        raise Fail('clang-format failed on %s: %s' % (path, r.stderr.strip()))


def format_clean(repo, path):
    r = subprocess.run([clang_format(), '--dry-run', '-Werror', '--style=file', os.path.join(repo, path)],
                       capture_output=True, text=True)
    return r.returncode == 0


# ---------------------------------------------------------------------------------------------------------------
# Types named by a declaration (for --export-statics)
# ---------------------------------------------------------------------------------------------------------------

class TypeIndex:
    """name -> [(kind, header, namespace)] of the types defined at namespace scope in the headers of CODE_DIRS."""

    PATTERN = re.compile(r'\bnamespace\s+([\w:]+)\s*\{'
                         r'|\b(struct|class|union)\s+(?:alignas\([^)]*\)\s*)?(\w+)\s*(?:final\s*)?(?::[^;{]*)?\{'
                         r'|\b(struct|class|union)\s+(\w+)\s*;'
                         r'|\benum\s+(?:class\s+|struct\s+)?(\w+)\s*(?::\s*[\w:]+\s*)?\{'
                         r'|\btypedef\b[^;{}]*?(\w+)\s*(?:\[[^\]]*\])?\s*;'
                         r'|\busing\s+(\w+)\s*='
                         r'|\{|\}')

    def __init__(self, repo):
        self.types = {}
        for h in code_files(repo, '.h'):
            code = re.sub(r'^\s*#.*$', '', strip_code(read_text(os.path.join(repo, h))), flags=re.M)
            stack = []
            for m in self.PATTERN.finditer(code):
                at_scope = all(s[0] == 'ns' for s in stack)
                ns = '::'.join(s[1] for s in stack)
                if m.group(1):
                    stack.append(('ns', m.group(1)))
                elif m.group(3):
                    if at_scope:
                        self.add(m.group(3), m.group(2), h, ns)
                    stack.append(('type', m.group(3)))
                elif m.group(5):
                    if at_scope:
                        self.add(m.group(5), 'forward ' + m.group(4), h, ns)
                elif m.group(6):
                    if at_scope:
                        self.add(m.group(6), 'enum', h, ns)
                    stack.append(('type', m.group(6)))
                elif m.group(7):
                    if at_scope:
                        self.add(m.group(7), 'typedef', h, ns)
                elif m.group(8):
                    if at_scope:
                        self.add(m.group(8), 'using', h, ns)
                elif m.group(0) == '{':
                    stack.append(('block', ''))
                elif stack:
                    stack.pop()

    def add(self, name, kind, header, ns):
        self.types.setdefault(name, []).append((kind, header, ns))

    def lookup(self, name):
        """(kind, header, namespace) of the definition of a type (not a forward declaration), or None."""
        defs = [d for d in self.types.get(name, []) if not d[0].startswith('forward')]
        if not defs:
            return None
        return sorted(defs, key=lambda d: (d[2] != '', d[1]))[0]


STD_TYPES = {'int8_t': '<cstdint>', 'int16_t': '<cstdint>', 'int32_t': '<cstdint>', 'int64_t': '<cstdint>',
             'uint8_t': '<cstdint>', 'uint16_t': '<cstdint>', 'uint32_t': '<cstdint>', 'uint64_t': '<cstdint>',
             'size_t': '<cstddef>', 'intptr_t': '<cstdint>', 'uintptr_t': '<cstdint>'}


def split_top(text, sep):
    parts, depth, cur = [], 0, ''
    for ch in text:
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            depth -= 1
        if ch == sep and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    if cur.strip():
        parts.append(cur)
    return parts


def signature_types(sig, name):
    """[(identifier, qualified)] of the types a function signature names (parameter names left out)."""
    m = re.search(r'\b%s\s*\(' % re.escape(name), sig)
    ret, rest = sig[:m.start()], sig[m.end():]
    depth, end = 1, 0
    for end, ch in enumerate(rest):
        depth += (ch == '(') - (ch == ')')
        if depth == 0:
            break
    params = rest[:end]
    pieces = [ret]
    for p in split_top(params, ','):
        p = p.split('=')[0]
        tokens = re.findall(r'[A-Za-z_]\w*', p)
        if p.strip() in ('...', 'void', ''):
            continue
        if len(tokens) >= 2 and tokens[-1] not in KEYWORDS and not re.search(r'::\s*%s\s*$' % tokens[-1], p) \
                and not re.search(r'%s\s*>' % tokens[-1], p):
            p = p[:p.rfind(tokens[-1])]
        pieces.append(p)
    found = []
    for piece in pieces:
        for mm in re.finditer(r'(::\s*)?([A-Za-z_]\w*)(\s*::)?', piece):
            ident = mm.group(2)
            if ident in KEYWORDS or ident in ('static', 'inline') or mm.group(3):
                continue  # a keyword, or a namespace or class that qualifies the next name
            found.append((ident, bool(mm.group(1))))
    return found


# ---------------------------------------------------------------------------------------------------------------
# --export-statics
# ---------------------------------------------------------------------------------------------------------------

def registrations(text):
    """{plain or Sc_ function name: [script names]} from the ScFnRegister tables of a text."""
    names = {}
    for m in re.finditer(r'\{\s*"([^"]+)"\s*,\s*(?:API_FN_PAIR\(\s*(\w+)\s*\)|(\w+)(?:\s*,\s*([\w:]+))?)\s*\}', text):
        script = m.group(1)
        for fn in (m.group(2), 'Sc_' + m.group(2) if m.group(2) else None, m.group(3), m.group(4)):
            if fn:
                names.setdefault(fn, []).append(script)
    return names


def licence_end(lines):
    """Index after the licence block (a // block framed by //==== lines at the top)."""
    if not lines or not re.match(r'^//=+$', lines[0].rstrip('\n')):
        raise Fail('no licence block at the top')
    for i in range(1, len(lines)):
        if re.match(r'^//=+$', lines[i].rstrip('\n')):
            return i + 1
    raise Fail('the licence block is not closed')


def description_lines(lines):
    """The // lines of the description block that follows the licence, or []."""
    start = licence_end(lines)
    if start < len(lines) and lines[start].startswith('//'):
        for i in range(start, len(lines)):
            if re.match(r'^//=+$', lines[i].rstrip('\n')):
                return [l for l in lines[start:i] if l.strip() != '//']
            if not lines[i].startswith('//'):
                return []
    return []


def body_open(lines, item):
    for i in range(item.start, item.end):
        if lines[i].rstrip('\n') == '{':
            return i
    raise Fail('line %d: %s has no body brace on a line of its own' % (item.start + 1, item.name))


class FunctionNames:
    """name -> [file] of the functions defined at column 0 in the .cpp files of CODE_DIRS."""

    def __init__(self, repo):
        self.where = {}
        line = re.compile(r'^(?![\s#/{}])[^;=(){}]*?\b([A-Za-z_]\w*)\s*\([^;]*$', re.M)
        for f in code_files(repo, '.cpp'):
            for m in line.finditer(strip_code(read_text(os.path.join(repo, f)))):
                self.where.setdefault(m.group(1), set()).add(f)


GLOBAL_DEF = r'^(?!(?:static|extern|using|typedef|return|const|constexpr)\b)(\w[\w:<>, \t*&]*?\b%s(?:\[[^\]\n]*\])*)' \
             r'[ \t]*(?:=[^;\n]*|\{[^;\n]*\})?;$'


class GlobalDefinitions:
    """name -> [(file, 'Type name[N]')] of the one-line global definitions at column 0 of the .cpp files."""

    def __init__(self, repo):
        self.where = {}
        line = re.compile(GLOBAL_DEF % r'(\w+)', re.M)
        for f in code_files(repo, '.cpp'):
            for m in line.finditer(strip_code(read_text(os.path.join(repo, f)))):
                self.where.setdefault(m.group(2), []).append((f, m.group(1)))


def header_of(source):
    return HEADERS.get(source, os.path.splitext(source)[0] + '.h')


def plan_export(repo, index, types, functions, globals_, plan, dest):
    """What --export-statics does for a source: {'header', 'create', 'unstatic': [item index], 'blockers': [...],
    'parts': {header: {'decls': [(name, comment, text)], 'forward', 'ns_forward', 'includes'}}}. The functions and
    globals of the source that the moved text needs and no header declares go to the source's header (HEADERS);
    a global of another file that the source declares `extern` itself goes to the header of the file defining it."""
    source = plan.source
    header = header_of(source)
    create = not os.path.isfile(os.path.join(repo, header))
    regs = registrations(''.join(item_text(plan.lines, it) for it in plan.moved))
    moved_bodies = {it.name: strip_code(item_text(plan.lines, it)) for it in plan.moved if it.kind == 'code'}
    api_file = '%s/%s_api.cpp' % (dest, API_FILES[source])
    used_by = '// Used by the script bindings in %s' % api_file.split('Engine/', 1)[-1]
    out = {'header': header, 'create': create, 'unstatic': [], 'blockers': [], 'parts': {}, 'externs_left': []}
    knowns = {}

    def part(h):
        if h not in out['parts']:
            out['parts'][h] = {'decls': [], 'forward': set(), 'ns_forward': {}, 'includes': set()}
            new = h == header and create
            using = set()
            for f in ([] if new else sorted(index.closure_files(h))):
                using |= set(re.findall(r'^using\s+[\w:]+::(\w+)\s*;', read_text(os.path.join(repo, f)), re.M))
            # names a header of the closure brings into the global namespace (using AGS::Common::GUILabel;)
            knowns[h] = (set() if new else index.closure(h), using)
        return out['parts'][h]

    def declare_global(h, name, typed, where):
        """Adds `extern <typed>;` to header h with what its type needs; returns the reasons it cannot (and then
        adds nothing)."""
        p = part(h)
        known = knowns[h][0]
        forward, includes, problems = set(), set(), []
        for ident in identifiers(re.sub(r'\[[^\]]*\]', '', typed)) - {name}:
            if ident in known:
                continue
            d = types.lookup(ident)
            if d and d[0] in ('struct', 'class') and d[2] == '':
                forward.add('%s %s;' % (d[0], ident))
            elif d and d[2] == '':
                includes.add('"%s"' % include_path(d[1]))
            else:
                problems.append('%s: the global %s has a type the tool cannot declare in %s (%s)'
                                % (where, name, h, ident))
        for size in re.findall(r'\[([^\]]*)\]', typed):
            for ident in identifiers(size):
                if ident not in known:
                    problems.append('%s: the size of the global %s names %s, which %s does not have'
                                    % (where, name, ident, h))
        if not problems:
            p['forward'] |= forward
            p['includes'] |= includes
            p['decls'].append((name, used_by, 'extern %s;' % typed))
        return problems

    part(header)
    # globals of other files that the source declares extern and no header declares
    for name, line in plan.extern_names:
        # an extern no header can take does not block the file: the move moves the line to the API file (copies
        # it when the source still names the global) and reports it
        defs = globals_.where.get(name, [])
        if len(defs) != 1:
            out['externs_left'].append('%s:%d: extern %s: %d definitions found' % (source, line, name, len(defs)))
            continue
        target = header_of(defs[0][0])
        if not os.path.isfile(os.path.join(repo, target)) and target != header:
            out['externs_left'].append('%s:%d: extern %s: %s has no header %s'
                                       % (source, line, name, defs[0][0], target))
            continue
        out['externs_left'] += declare_global(target, name, defs[0][1], '%s:%d' % (source, line))
    targets = [n for n, _ in plan.statics] + [n for n, _, _ in plan.undeclared]
    p = part(header)
    known, using_declared = knowns[header]
    for name in sorted(targets, key=lambda n: plan.items[plan.defined[n]].start):
        k = plan.defined[name]
        it = plan.items[k]
        if it.namespace:
            out['blockers'].append('%s:%d: %s is inside a namespace: declare it by hand' % (source, it.start + 1, name))
            continue
        if not it.function:
            m = re.match(GLOBAL_DEF % re.escape(name), strip_code(item_text(plan.lines, it)).strip())
            if it.static or not m:
                out['blockers'].append('%s:%d: the global %s has no declaration in a header: declare it by hand'
                                       % (source, it.start + 1, name))
                continue
            out['blockers'] += declare_global(header, name, m.group(1), '%s:%d' % (source, it.start + 1))
            continue
        if it.static:
            others = sorted(functions.where.get(name, set()) - {source})
            if others:
                out['blockers'].append('%s:%d: static %s has namesakes in %s: rename by hand'
                                       % (source, it.start + 1, name, ', '.join(others)))
                continue
            out['unstatic'].append(k)
        sig = ''.join(plan.lines[it.start:body_open(plan.lines, it)]).rstrip()
        sig = re.sub(r'^(inline\s+)?static\s+', r'\1', sig)
        qualify = set()
        for ident, qualified in signature_types(strip_code(sig), name):
            if ident in STD_TYPES and ident not in known:
                p['includes'].add(STD_TYPES[ident])
                continue
            d = types.lookup(ident)
            global_def = any(x[2] == '' and not x[0].startswith('forward') for x in types.types.get(ident, []))
            if ident in known and (qualified or global_def or ident in using_declared or d is None):
                continue
            if d is None or (qualified and d[2] == ''):
                out['blockers'].append('%s:%d: %s names %s, which the tool cannot find a definition of'
                                       % (source, it.start + 1, name, ident))
            elif d[2] and qualified:
                p['includes'].add('"%s"' % include_path(d[1]))
            elif d[2]:
                # the source names it under a using-directive; the header, which has none, qualifies it
                qualify.add((ident, d[2]))
                if ident not in known:
                    if d[0] in ('struct', 'class'):
                        p['ns_forward'].setdefault(d[2], set()).add('%s %s;' % (d[0], ident))
                    else:
                        p['includes'].add('"%s"' % include_path(d[1]))
            elif ident in known:
                continue
            elif d[0] in ('struct', 'class'):
                p['forward'].add('%s %s;' % (d[0], ident))
            else:
                p['includes'].add('"%s"' % include_path(d[1]))
        for ident, ns in sorted(qualify):
            sig = re.sub(r'(?<![\w:])%s\b(?!\s*\()' % re.escape(ident), '%s::%s' % (ns, ident), sig)
        scripts = list(regs.get(name, []))
        for sc, body in sorted(moved_bodies.items()):
            if sc and re.search(r'\b%s\b' % re.escape(name), body):
                scripts += [s for s in regs.get(sc, []) if s not in scripts]
        if scripts:
            shown = scripts[:3] + (['...'] if len(scripts) > 3 else [])
            comment = '// Script API: %s' % ', '.join(shown)
        else:
            comment = used_by
        p['decls'].append((name, comment, sig + ';'))
    return out


def header_part_text(part, existing=''):
    """The forward declarations and declarations of an export part, as text to append to a header; what the
    header has already (an earlier file's export) is left out."""
    have = set(l.strip() for l in existing.split('\n'))
    forward = ''.join('%s\n' % f for f in sorted(part['forward']) if f not in have)
    for ns, decls in sorted(part['ns_forward'].items()):
        names = ns.split('::')
        forward += (''.join('namespace %s\n{\n' % n for n in names) + ''.join('%s\n' % d for d in sorted(decls))
                    + ''.join('} // namespace %s\n' % n for n in reversed(names)))
    text = ('\n' + forward) if forward else ''
    for name, comment, decl in part['decls']:
        if decl not in have:
            text += '\n%s\n%s\n' % (comment, decl)
    return text


def apply_export(repo, plan, ex):
    """Writes an export plan: the source loses `static` (and includes its header if it did not), each header gets
    its declarations at the end. Returns the headers it created."""
    source, header = plan.source, ex['header']
    src_text, src_eol, src_bom = read_eol(os.path.join(repo, source))
    lines = src_text.splitlines(keepends=True)
    for k in ex['unstatic']:
        i = plan.items[k].start
        lines[i] = re.sub(r'^(inline\s+)?static\s+', r'\1', lines[i])
    own_part = ex['parts'].get(header)
    if own_part and own_part['decls']:
        own = '"%s"' % include_path(header)
        if not any(include_name(l) == own for l in lines):
            top = top_includes(plan)
            at = plan.items[top[0]].start if top else licence_end(lines)
            lines.insert(at, '#include %s\n' % own)
    write_text(os.path.join(repo, source), ''.join(lines), src_eol, src_bom)
    created = []
    for h, part in sorted(ex['parts'].items()):
        if not part['decls']:
            continue
        if h == header and ex['create']:
            lic = lines[:licence_end(lines)]
            desc = description_lines(lines) or ['// Functions of %s that the script bindings call.\n'
                                                % os.path.basename(source)]
            text = ''.join(lic) + '//\n' + ''.join(desc) + '//\n' + lic[-1] + '#pragma once\n'
            if part['includes']:
                text += '\n' + ''.join('#include %s\n' % i for i in sorted(part['includes']))
            text += header_part_text(part)
            eol, bom = src_eol, b''
            created.append(h)
        else:
            text, eol, bom = read_eol(os.path.join(repo, h))
            hl = text.splitlines(keepends=True)
            if hl and hl[-1].startswith('#endif'):
                raise Fail('%s ends with #endif: add the declarations by hand' % h)
            add = ['#include %s\n' % i for i in sorted(part['includes']) if '#include %s\n' % i not in hl]
            if add:
                incs = [i for i, l in enumerate(hl) if l.startswith('#include')]
                at = incs[-1] + 1 if incs else next(i for i, l in enumerate(hl) if l.startswith('#pragma once')) + 1
                hl[at:at] = add if incs else ['\n'] + add
            text = ''.join(hl).rstrip('\n') + '\n' + header_part_text(part, text)
        write_text(os.path.join(repo, h), text, eol, bom)
        format_file(repo, h)
        if not format_clean(repo, h):
            raise Fail('%s is not clang-format clean after the export' % h)
    top = [plan.items[k] for k in top_includes(plan)]
    first = top[0].start + 1 if top else 1
    format_file(repo, source, [(first, (top[-1].end + 1) if top else first + 1)])
    if not format_clean(repo, source):
        raise Fail('%s is not clang-format clean after the export' % source)
    return created


# ---------------------------------------------------------------------------------------------------------------
# The move
# ---------------------------------------------------------------------------------------------------------------

def register_names(plan):
    return [n for n in plan.functions if re.match(r'^Register\w*API$', n)]


def joined_text(lines, items, indexes):
    """The text of the items at the given indexes (ascending), in order: the blank lines between two of them are
    kept where nothing else separates them in the file, one blank line stands for what was between them."""
    text = ''
    prev = None
    for k in indexes:
        if prev is not None:
            gap = range(items[prev].end, items[k].start)
            if all(not lines[i].strip() for i in gap) and all(items[j].kind == 'blank' for j in range(prev + 1, k)):
                text += ''.join(lines[i] for i in gap)
            else:
                text += '\n'
        text += item_text(lines, items[k])
        prev = k
    return text


def api_file_text(plan, licence):
    """The API file's text before clang-format, and the moved text it must keep verbatim."""
    lines, items = plan.lines, plan.items
    moved_text = joined_text(lines, items, plan.moved_idx)
    regs = register_names(plan)
    if regs:
        desc = '// Script bindings registered by %s.\n' % ' and '.join('%s()' % r for r in regs)
    else:
        desc = '// Script bindings.\n'
    text = ''.join(licence) + '//\n' + desc + '//\n' + licence[-1]
    text += ''.join('#include %s\n' % n for n in plan.api_includes)
    usings = [item_text(lines, it) for it in items if it.kind == 'code' and it.header.startswith('using namespace')
              and not it.namespace]
    if usings:
        text += '\n' + ''.join(usings)
    if plan.externs_copied:
        text += '\n' + ''.join(e + '\n' for e in plan.externs_copied)
    text += '\n' + moved_text
    return text, moved_text


def logic_file_text(plan):
    """The source without the moved text, the dropped banner, includes and externs; blank runs at the joins merged."""
    lines, items = plan.lines, plan.items
    gone = set(plan.moved_idx) | set(getattr(plan, 'dropped_idx', [])) | set(plan.includes_dropped) \
        | set(plan.externs_dropped)
    out = []          # [(line, removed_before)]
    removed = False
    for k, it in enumerate(items):
        if k in gone:
            removed = True
            continue
        for i in range(it.start, it.end):
            out.append((lines[i], removed))
            removed = False
    # merge blank runs around each removal point: the longer of the two runs stays
    result = []
    i = 0
    while i < len(out):
        line, cut = out[i]
        if line.strip():
            result.append(line)
            i += 1
            continue
        j = i
        runs, run, cut_inside = [], 0, False
        while j < len(out) and not out[j][0].strip():
            if out[j][1] and j > i:
                runs.append(run)
                run = 0
                cut_inside = True
            run += 1
            j += 1
        runs.append(run)
        keep = max(runs) if (cut_inside or cut) else run
        if cut and not result:
            keep = 0
        result.extend(['\n'] * keep)
        i = j
    while result and not result[-1].strip():
        result.pop()
    return ''.join(result)


def apply_move(repo, plan, dest):
    stem = API_FILES[plan.source]
    api_path = '%s/%s_api.cpp' % (dest, stem)
    full = os.path.join(repo, api_path)
    if os.path.exists(full):
        raise Fail('%s exists and %s still has bindings: merge by hand' % (api_path, plan.source))
    src_text, eol, bom = read_eol(os.path.join(repo, plan.source))
    licence = plan.lines[:licence_end(plan.lines)]
    text, moved_text = api_file_text(plan, licence)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    write_text(full, text, eol)
    format_file(repo, api_path)
    formatted = read_text(full)
    if moved_text not in formatted:
        raise Fail('clang-format changed the moved text in %s' % api_path)
    logic = logic_file_text(plan)
    write_text(os.path.join(repo, plan.source), logic, eol, bom)
    format_file(repo, plan.source)
    after = read_text(os.path.join(repo, plan.source))

    def body(t):
        return [l for l in t.split('\n') if l.strip() and not l.startswith('#include')]
    if body(after) != body(logic):
        raise Fail('clang-format changed %s outside its include blocks' % plan.source)
    for f in (api_path, plan.source):
        if not format_clean(repo, f):
            raise Fail('%s is not clang-format clean' % f)
    return api_path


# ---------------------------------------------------------------------------------------------------------------
# Build lists
# ---------------------------------------------------------------------------------------------------------------

def stable_id(*parts):
    return str(uuid.uuid5(ID_NAMESPACE, ':'.join(parts)))


def xcode_id(*parts):
    return hashlib.sha1(':'.join(('split_bindings',) + parts).encode('utf-8')).hexdigest()[:24].upper()


def insert_sorted(entries, new):
    """Index of the entry after which `new` goes, in a list that is mostly sorted: after the last entry of the
    same directory that sorts before it, else after the last one of the parent directories that does."""
    d = new
    while True:
        d = d.rsplit('/', 1)[0] if '/' in d else ''
        same = [i for i, e in enumerate(entries) if e.startswith(d + '/' if d else '') and e < new]
        if same:
            return same[-1]
        if not d:
            return -1


def add_to_cmake(repo, path):
    """Adds an Engine file to the engine target's list in Engine/CMakeLists.txt; False if it is there."""
    cm = os.path.join(repo, ENGINE_CMAKE)
    text, eol, bom = read_eol(cm)
    rel = path[len('Engine/'):]
    lines = text.split('\n')
    start = lines.index('target_sources(engine')
    end = lines.index(')', start)
    entries = [(i, l.strip()) for i, l in enumerate(lines[start + 1:end], start + 1)
               if re.match(r'^    [\w/.\-]+$', l)]
    if rel in [e for _, e in entries]:
        return False
    at = insert_sorted([e for _, e in entries], rel)
    line = entries[at][0] + 1 if at >= 0 else start + 1
    lines.insert(line, '    ' + rel)
    write_text(cm, '\n'.join(lines), eol, bom)
    return True


def win(path):
    return '..\\..\\' + path.replace('/', '\\')


def add_to_vcxproj(repo, path):
    kind = 'ClCompile' if path.endswith('.cpp') else 'ClInclude'
    vc = os.path.join(repo, ENGINE_VCXPROJ)
    text, eol, bom = read_eol(vc)
    item = win(path)
    if 'Include="%s"' % item in text:
        return False
    lines = text.split('\n')
    if kind == 'ClCompile':
        stem = os.path.splitext(os.path.basename(path))[0].lower()
        for l in lines:
            m = re.search(r'<ClCompile Include="([^"]+)"', l)
            if m and os.path.splitext(m.group(1).split('\\')[-1])[0].lower() == stem:
                raise Fail('%s: the object name %s.obj is taken by %s' % (ENGINE_VCXPROJ, stem, m.group(1)))
    rows = [(i, re.search(r'Include="([^"]+)"', l).group(1)) for i, l in enumerate(lines)
            if re.match(r'^    <%s Include="\.\.\\\.\.\\Engine\\' % kind, l)]
    at = insert_sorted([r.replace('\\', '/') for _, r in rows], item.replace('\\', '/'))
    lines.insert(rows[at][0] + 1, '    <%s Include="%s" />' % (kind, item))
    write_text(vc, '\n'.join(lines), eol, bom)
    return True


def add_to_filters(repo, path):
    kind = 'ClCompile' if path.endswith('.cpp') else 'ClInclude'
    fl = os.path.join(repo, ENGINE_VCXPROJ + '.filters')
    text, eol, bom = read_eol(fl)
    item = win(path)
    if 'Include="%s"' % item in text:
        return False
    group = 'Source Files' if kind == 'ClCompile' else 'Header Files'
    sub = os.path.dirname(path)[len('Engine/'):].replace('/', '\\')
    filt = group + '\\' + sub
    lines = text.split('\n')
    if '<Filter Include="%s">' % filt not in text:
        parent = filt.rsplit('\\', 1)[0]
        p = lines.index('    <Filter Include="%s">' % parent)
        close = lines.index('    </Filter>', p)
        lines[close + 1:close + 1] = ['    <Filter Include="%s">' % filt,
                                      '      <UniqueIdentifier>{%s}</UniqueIdentifier>' % stable_id('filter', filt),
                                      '    </Filter>']
    rows = [(i, re.search(r'Include="([^"]+)"', l).group(1)) for i, l in enumerate(lines)
            if re.match(r'^    <%s Include="\.\.\\\.\.\\Engine\\' % kind, l)]
    at = insert_sorted([r.replace('\\', '/') for _, r in rows], item.replace('\\', '/'))
    close = lines.index('    </%s>' % kind, rows[at][0])
    lines[close + 1:close + 1] = ['    <%s Include="%s">' % (kind, item), '      <Filter>%s</Filter>' % filt,
                                  '    </%s>' % kind]
    write_text(fl, '\n'.join(lines), eol, bom)
    return True


def pbx_groups(text):
    """{id: (path, sourceTree, [children])} of the PBXGroup objects, and {child: parent}."""
    ident = r'(\w{24})(?: /\*[^\n]*?\*/)?'
    groups, parent = {}, {}
    for m in re.finditer(r'\t\t%s = \{\n\t\t\tisa = PBXGroup;\n(.*?)\n\t\t\};\n' % ident,
                         check_build_lists.pbx_section(text, 'PBXGroup'), re.S):
        body = m.group(2)
        kids = re.findall(ident + ',', re.search(r'children = \((.*?)\);', body, re.S).group(1))
        groups[m.group(1)] = (check_build_lists.pbx_value(body, 'path'), check_build_lists.pbx_value(body, 'sourceTree'),
                              kids)
        for c in kids:
            parent[c] = m.group(1)
    return groups, parent


def pbx_insert_object(text, section, oid, block):
    """Inserts an object into a section of the .pbxproj, in the order of the IDs (as Xcode writes them)."""
    begin = text.index('/* Begin %s section */\n' % section) + len('/* Begin %s section */\n' % section)
    end = text.index('/* End %s section */' % section)
    pos = end
    for m in re.finditer(r'^\t\t(\w{24}) ', text[begin:end], re.M):
        if m.group(1) > oid:
            pos = begin + m.start()
            break
    return text[:pos] + block + text[pos:]


def pbx_add_child(text, group, child, comment):
    m = re.search(r'^\t\t%s(?: /\*[^\n]*?\*/)? = \{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = \(\n(.*?)\t\t\t\);' % group,
                  text, re.S | re.M)
    at = m.end(1)
    return text[:at] + '\t\t\t\t%s /* %s */,\n' % (child, comment) + text[at:]


def add_to_xcode(repo, path):
    """Adds a file to the AGSKit project: a reference in the group of its directory (made if missing), and for a
    .cpp a build file in the Sources phase that compiles XCODE_ANCHOR."""
    px = os.path.join(repo, XCODE_PROJECT)
    text, eol, bom = read_eol(px)
    name = os.path.basename(path)
    ref = xcode_id('ref', path)
    if ref in text:
        return False
    project_dir = os.path.dirname(os.path.dirname(px))
    groups, parent = pbx_groups(text)

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

    by_dir = {}
    for g in sorted(groups):
        r = resolve(g)
        if r and g in parent:
            rel = os.path.relpath(r, repo).replace(os.sep, '/')
            by_dir.setdefault(rel, g)
    target_dir = os.path.dirname(path)
    missing = []
    d = target_dir
    while d not in by_dir:
        missing.insert(0, d)
        d = os.path.dirname(d)
        if not d:
            raise Fail('%s: no group for any parent directory of %s' % (XCODE_PROJECT, path))
    group = by_dir[d]
    for m in missing:
        gid = xcode_id('group', m)
        base = os.path.basename(m)
        block = ('\t\t%s /* %s */ = {\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n\t\t\t);\n\t\t\tpath = %s;\n'
                 '\t\t\tsourceTree = "<group>";\n\t\t};\n' % (gid, base, base))
        text = pbx_insert_object(text, 'PBXGroup', gid, block)
        text = pbx_add_child(text, group, gid, base)
        group = gid
    ftype = 'sourcecode.cpp.cpp' if path.endswith('.cpp') else 'sourcecode.c.h'
    text = pbx_insert_object(text, 'PBXFileReference', ref,
                             '\t\t%s /* %s */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = %s; '
                             'path = %s; sourceTree = "<group>"; };\n' % (ref, name, ftype, name))
    text = pbx_add_child(text, group, ref, name)
    if path.endswith('.cpp'):
        bid = xcode_id('build', path)
        text = pbx_insert_object(text, 'PBXBuildFile', bid,
                                 '\t\t%s /* %s in Sources */ = {isa = PBXBuildFile; fileRef = %s /* %s */; };\n'
                                 % (bid, name, ref, name))
        anchor = os.path.basename(XCODE_ANCHOR)
        m = re.search(r'\t\t\t\t(\w{24}) /\* %s in Sources \*/,\n' % re.escape(anchor), text)
        phase = text.rfind('isa = PBXSourcesBuildPhase;', 0, m.start())
        close = text.index('\t\t\t);', m.start())
        if phase < 0 or text.find('isa = ', phase + 10, close) >= 0:
            raise Fail('%s: cannot find the Sources phase of %s' % (XCODE_PROJECT, XCODE_ANCHOR))
        text = text[:close] + '\t\t\t\t%s /* %s in Sources */,\n' % (bid, name) + text[close:]
    write_text(px, text, eol, bom)
    return True


def add_build_lists(repo, path):
    done = []
    for name, fn in (('CMake', add_to_cmake), ('vcxproj', add_to_vcxproj), ('filters', add_to_filters),
                     ('Xcode', add_to_xcode)):
        if fn(repo, path):
            done.append(name)
    return done


# ---------------------------------------------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------------------------------------------

def unmapped_sources(repo):
    """Files of CODE_DIRS with a binding definition that are in neither API_FILES nor SKIP."""
    out = []
    for f in code_files(repo, '.cpp'):
        if f in API_FILES or f in SKIP or f.startswith(DEST + '/'):
            continue
        if re.search(r'^RuntimeScriptValue Sc_\w+\(|^void Register\w*API\(', read_text(os.path.join(repo, f)), re.M):
            out.append(f)
    return out


def script_api_includers(repo):
    out = []
    for ext in ('.cpp', '.h'):
        for f in code_files(repo, ext):
            if f.startswith(DEST + '/'):
                continue
            if re.search(r'^#include "script/script_api\.h"', read_text(os.path.join(repo, f)), re.M):
                out.append(f)
    return out


def summary_row(plan, moved_lines):
    lefts = len([l for l in plan.leftovers if l[1] == 'code'])
    return '%-40s %4d %6d %5d %5d %5d %4s %s' % (
        plan.source, len(plan.functions), moved_lines, len(plan.statics), len(plan.undeclared), lefts,
        '%d->%d' % (plan.includes_before, plan.includes_after), plan.after)


def report(plan, verbose):
    out = []
    lines = sum(it.end - it.start for it in plan.moved)
    out.append('%s -> %s: %d functions, %d lines; includes %d -> %d (dropped %s); API file includes %s'
               % (plan.source, API_FILES.get(plan.source, '?'), len(plan.functions), lines, plan.includes_before,
                  plan.includes_after, ', '.join(plan.lines[plan.items[k].start].strip()[9:]
                                                 for k in plan.includes_dropped) or 'none',
                  ', '.join(plan.api_includes)))
    if plan.dropped:
        out.append('  banner deleted: lines %d-%d' % (plan.dropped[0].start + 1, plan.dropped[-1].end))
    for line, moved in zip(plan.externs_copied, plan.externs_moved):
        if moved:
            out.append('  extern moved to the API file (no header declares it; the source no longer names it): %s'
                       % line)
        else:
            out.append('  extern copied to the API file (no header declares it; the source still names it, so the '
                       'count of extern lines in .cpp files rises by one): %s' % line)
    if plan.extern_headers:
        out.append('  extern replaced by its header: %s' % ', '.join(plan.extern_headers))
    if plan.pp_branches:
        out.append('  #if in the moved text: %s' % '; '.join(plan.pp_branches))
    if plan.after:
        out.append('  %d lines of code after the last binding (they stay)' % plan.after)
    lefts = [l for l in plan.leftovers if l[1] == 'code']
    if lefts:
        shown = lefts if verbose else lefts[:6]
        out.append('  left in place between bindings: %s%s' % (', '.join('%s:%d' % (n, ln) for ln, _, n in shown),
                                                               '' if len(shown) == len(lefts) else ', ...'))
    for n in plan.notes:
        out.append('  note: ' + n)
    blockers = plan.blockers if verbose else plan.blockers[:8]
    for b in blockers:
        out.append('  BLOCKED: ' + b)
    if len(blockers) < len(plan.blockers):
        out.append('  BLOCKED: ... %d more (--verbose)' % (len(plan.blockers) - len(blockers)))
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('sources', nargs='*', help='source files (repository paths)')
    ap.add_argument('--all', action='store_true', help='every file of API_FILES')
    ap.add_argument('--repo', default=os.path.dirname(SCRIPT_DIR))
    ap.add_argument('--dest', default=DEST)
    ap.add_argument('--dry-run', action='store_true', help='report only, write nothing')
    ap.add_argument('--no-build-lists', action='store_true', help='leave the build descriptions alone')
    ap.add_argument('--export-statics', action='store_true',
                    help='only the step before the move: declare in the header what the bindings need')
    ap.add_argument('--verbose', action='store_true', help='list every blocker and leftover')
    args = ap.parse_args()
    repo = os.path.abspath(args.repo)
    sources = list(API_FILES) if args.all else [s.replace('\\', '/') for s in args.sources]
    if not sources:
        ap.error('name the source files, or --all')
    for s in sources:
        if s not in API_FILES:
            ap.error('%s has no row in API_FILES%s' % (s, (' (SKIP: %s)' % SKIP[s]) if s in SKIP else ''))
    if not args.dry_run:
        clang_format()
    index = HeaderIndex(repo)
    status = 0
    written = []
    totals = [0, 0]
    rows = []
    types = functions = None
    for source in sources:
        try:
            plan = analyse(repo, index, source, args.dest)
        except Fail as e:
            print('%s: FAILED: %s' % (source, e))
            status = 1
            continue
        if not plan.functions:
            print('%s: no bindings (already split)' % source)
            continue
        moved_lines = sum(it.end - it.start for it in plan.moved)
        rows.append(summary_row(plan, moved_lines))
        if args.export_statics:
            if types is None:
                types, functions, globals_ = TypeIndex(repo), FunctionNames(repo), GlobalDefinitions(repo)
            ex = plan_export(repo, index, types, functions, globals_, plan, args.dest)
            for h, part in sorted(ex['parts'].items()):
                if part['decls'] or h == ex['header']:
                    print('%s: export to %s%s: %d declarations (%d static), %d forward declarations, includes %s'
                          % (source, h, ' (new)' if h == ex['header'] and ex['create'] and part['decls'] else '',
                             len(part['decls']), len(ex['unstatic']) if h == ex['header'] else 0,
                             len(part['forward']) + sum(len(v) for v in part['ns_forward'].values()),
                             ', '.join(sorted(part['includes'])) or 'none'))
            for b in ex['externs_left']:
                print('  extern left to the move (no header can take it): ' + b)
            for b in ex['blockers']:
                print('  BLOCKED: ' + b)
            if ex['blockers']:
                status = 1
                continue
            if not any(p['decls'] for p in ex['parts'].values()) or args.dry_run:
                continue
            try:
                for new in apply_export(repo, plan, ex):
                    written.append(new)
                    if not args.no_build_lists:
                        print('  %s added to %s' % (new, ', '.join(add_build_lists(repo, new)) or 'nothing'))
            except Fail as e:
                print('  FAILED: %s' % e)
                status = 1
            # the headers changed: read them again for the next file
            index = HeaderIndex(repo)
            _EXTERN_CACHE.clear()
            continue
        print(report(plan, args.verbose))
        if plan.blockers:
            status = 1
            continue
        totals[0] += len(plan.functions)
        totals[1] += moved_lines
        if args.dry_run:
            continue
        try:
            api_path = apply_move(repo, plan, args.dest)
        except Fail as e:
            print('  FAILED: %s' % e)
            status = 1
            continue
        written.append(api_path)
        if not args.no_build_lists:
            print('  %s added to %s' % (api_path, ', '.join(add_build_lists(repo, api_path)) or 'nothing'))
    if len(sources) > 1 and not args.export_statics:
        print('\n%-40s %4s %6s %5s %5s %5s %4s %s' % ('source', 'fns', 'lines', 'stat', 'undec', 'left', 'incl',
                                                     'after'))
        print('\n'.join(rows))
        print('movable now: %d functions, %d lines' % tuple(totals))
    if args.all:
        extra = unmapped_sources(repo)
        if extra:
            print('\nfiles with bindings and no API_FILES or SKIP row: %s' % ', '.join(extra))
    if not args.dry_run and not args.export_statics:
        rest = [f for f in script_api_includers(repo) if f not in SCRIPT_API_INCLUDERS]
        print('\nfiles outside %s that include script/script_api.h: %s' % (args.dest, ', '.join(rest) or 'none'))
    return status


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Fail as e:
        print('split_bindings: %s' % e, file=sys.stderr)
        sys.exit(2)
