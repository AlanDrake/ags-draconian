#!/usr/bin/env python3
"""
Converts the hand-written script bindings of the engine (the `Sc_*` wrappers of Engine/game/script_api/, each built
from an `API_*` macro of Engine/script/script_api.h) into rows of the template binder (Engine/script/script_binder.h,
Engine/game/script_api/bindings.h); docs/refactor/PLAN.md 5.2, step 2.

    python Script/convert_bindings.py [--repo <dir>] [--dry-run] [--verbose] (<file.cpp> ... | --all)

For each registration row of a file ({"Name", API_FN_PAIR(X)}, {"Name", Sc_X, plugin_fn}, and the calls
ccAddExternalStaticFunction / ccAddExternalObjectFunction("Name", Sc_X)), it reads the wrapper Sc_X (in any file
of the API directory) and describes what it does: the macro is expanded from its #define, and the statements are
read as a call of one function F with each script parameter converted (IValue, FValue, Ptr, GetAsBool()), and its
result returned (void as 0 or undefined, SetInt32, SetInt32AsBool, SetFloat, SetScriptObject with a manager or
with the object itself), or as a formatted-text call (API_*_SCRIPT_SPRINTF). Hand-written wrappers in that shape
are read the same way. Then it looks up F's declaration (all headers and sources of Engine/ and Common/) and
compares, position by position, what the wrapper does with what the binder does from the signature alone:

    ScMethod<&F, tags...>("Name"[, plugin_fn])        wrapper with an object (self) parameter
    ScStatic<&F, tags...>("Name"[, plugin_fn])        wrapper without one
    ScMethodFormat<&F>("Name", plugin_fn)             formatted text (translated), as the last parameter of F
    ScStaticFormat<&F>("Name", plugin_fn)
    ccAddExternalStaticFunction("Name", ScStaticWrapper<&F, tags...>())   (a registration without a plugin function)

A tag is written where they differ (ScVoidResult, ScUndefinedResult, ScBoolResult, ScResultAs<T>,
ScManagedBy<manager>, ScBoolArg<i>, ScIntArg<i>), so the generated wrapper does what the old one did. plugin_fn is
written when the row's plugin function is not F. An overloaded F is written as a static_cast to its declared type.
A wrapper the tool cannot describe, or whose difference no tag expresses, is refused: listed, and left in place with
its rows. A wrapper is deleted, with the comment block right above it and a declaration of it in a header, when no
row or code references it any more. The file gets #include "game/script_api/bindings.h"; it loses
"script/script_api.h" when it names nothing of it any more, and the manager headers (cc_*.h, script_string.h,
cc_dynamic_array.h) whose names it no longer uses. The file is then clang-formatted (AGS_CLANG_FORMAT, 19.1.7).

--record <file.tsv> appends, per converted row: file, script name, the old wrapper, the new form, function, tags,
plugin function (what Script/compare_bindings.py can check against).

A second run changes nothing. What the parser assumes: split_bindings.py's (clang-formatted files, top-level items
at column 0); a row on one line or wrapped by clang-format.
"""
import argparse
import collections
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import split_bindings as sb  # noqa: E402  (parser of top-level items, text and clang-format helpers)

API_DIR = 'Engine/game/script_api'
MACROS = 'Engine/script/script_api.h'
BINDINGS_INCLUDE = '#include "game/script_api/bindings.h"'
SCRIPT_API_INCLUDE = '#include "script/script_api.h"'
# files whose Sc_ functions are not script API (the plugin stubs): never converted
SKIP_FILES = ('Engine/plugin/plugin_stubs.cpp',)

# The managers of ScriptManagerOf<T> in bindings.h: the type the function returns -> the manager; a type that is
# not here is its own manager. Keep in step with bindings.h.
MANAGERS = {
    'char': 'myScriptStringImpl',
    'CharacterDef': 'ccDynamicCharacter',
    'ScriptAudioChannel': 'ccDynamicAudio',
    'AudioClip': 'ccDynamicAudioClip',
    'ScriptDialog': 'ccDynamicDialog',
    'ScriptGUI': 'ccDynamicGUI',
    'ScriptHotspot': 'ccDynamicHotspot',
    'ScriptInvItem': 'ccDynamicInv',
    'ScriptObject': 'ccDynamicObject',
    'ScriptRegion': 'ccDynamicRegion',
    'ScriptWalkableArea': 'ccDynamicWalkarea',
    'ScriptWalkbehind': 'ccDynamicWalkbehind',
    'GUIObject': 'ccDynamicGUIControl',
    'GUIControl': 'ccDynamicGUIControl',
    'GUIButton': 'ccDynamicGUIControl',
    'GUIInvWindow': 'ccDynamicGUIControl',
    'GUILabel': 'ccDynamicGUIControl',
    'GUIListBox': 'ccDynamicGUIControl',
    'GUISlider': 'ccDynamicGUIControl',
    'GUITextBox': 'ccDynamicGUIControl',
}

# Headers a binding file included for the macros' sake (a manager object, its class): dropped when the converted
# file names none of the identifiers listed
MANAGER_HEADERS = {
    '#include "game/dynobj/cc_audio_channel.h"': ('CCAudioChannel', 'ccDynamicAudio'),
    '#include "game/dynobj/cc_audio_clip.h"': ('CCAudioClip', 'ccDynamicAudioClip'),
    '#include "game/dynobj/cc_character.h"': ('CCCharacter', 'ccDynamicCharacter'),
    '#include "game/dynobj/cc_dialog.h"': ('CCDialog', 'ccDynamicDialog'),
    '#include "game/dynobj/cc_gui.h"': ('CCGUI', 'ccDynamicGUI'),
    '#include "game/dynobj/cc_gui_control.h"': ('CCGUIControl', 'ccDynamicGUIControl'),
    '#include "game/dynobj/cc_hotspot.h"': ('CCHotspot', 'ccDynamicHotspot'),
    '#include "game/dynobj/cc_inventory.h"': ('CCInventory', 'ccDynamicInv'),
    '#include "game/dynobj/cc_object.h"': ('CCObject', 'ccDynamicObject'),
    '#include "game/dynobj/cc_region.h"': ('CCRegion', 'ccDynamicRegion'),
    '#include "game/dynobj/cc_walk_behind.h"': ('CCWalkbehind', 'ccDynamicWalkbehind'),
    '#include "game/dynobj/cc_walkable_area.h"': ('CCWalkableArea', 'ccDynamicWalkarea'),
    '#include "game/dynobj/script_string.h"': ('ScriptString', 'myScriptStringImpl'),
    '#include "script/dynobj/cc_dynamic_array.h"': ('CCDynamicArray', 'globalDynamicArray', 'DynamicArrayHelpers'),
}
# names of script/script_api.h that keep its include
SCRIPT_API_NAMES = ('API_', 'ScriptSprintf', 'ScriptVSprintf', 'ScriptAPIFunction', 'ScriptAPIObjectFunction',
                    'ASSERT_SELF', 'ASSERT_PARAM_COUNT', 'ASSERT_OBJ_PARAM_COUNT', 'ASSERT_VARIABLE_VALUE')

Fail = sb.Fail


# ---------------------------------------------------------------------------------------------------------------
# Macros: expanded from their #define
# ---------------------------------------------------------------------------------------------------------------

def read_macros(repo):
    """{name: (parameters, body)} of the function-like #defines of script_api.h."""
    text = sb.read_text(os.path.join(repo, MACROS))
    text = re.sub(r'\\\n', ' ', text)
    macros = {}
    for m in re.finditer(r'^#define\s+(\w+)\(([^)]*)\)\s*(.*)$', text, re.M):
        params = [p.strip() for p in m.group(2).split(',') if p.strip()]
        macros[m.group(1)] = (params, ' '.join(m.group(3).split()))
    return macros


def split_args(text):
    """Top-level comma-separated pieces of an argument list, stripped."""
    return [p.strip() for p in sb.split_top(text, ',')]


def expand(code, macros, depth=0):
    """The code with every macro of script_api.h replaced by its body, recursively."""
    if depth > 5:
        raise Fail('macro expansion too deep')
    out, i = '', 0
    for m in re.finditer(r'\b(API_\w+|ASSERT_\w+)\s*\(', code):
        if m.start() < i:
            continue
        name = m.group(1)
        if name not in macros:
            raise Fail('unknown macro ' + name)
        j, d = m.end(), 1
        while d:
            d += {'(': 1, ')': -1}.get(code[j], 0)
            j += 1
        args = split_args(code[m.end():j - 1])
        params, body = macros[name]
        if len(args) != len(params):
            raise Fail('%s takes %d arguments, %d given' % (name, len(params), len(args)))
        mapping = dict(zip(params, args))
        body = re.sub(r'\b\w+\b', lambda t: mapping.get(t.group(0), t.group(0)), body)
        out += code[i:m.start()] + expand(body, macros, depth + 1)
        i = j
    return out + code[i:]


# ---------------------------------------------------------------------------------------------------------------
# What a wrapper does
# ---------------------------------------------------------------------------------------------------------------

class Wrapper:
    """A Sc_ function described as a call: kind 'static' or 'method', the function, the script parameters' reads
    ([(kind, cast)]: kind int, float, bool, ptr), the result (kind, details), the formatted-text form."""

    def __init__(self, name, path, item):
        self.name = name
        self.path = path
        self.item = item
        self.kind = None          # 'static' or 'method'
        self.function = None      # the called function, as written
        self.params = []          # [(kind, type written in a cast or None)]
        self.result = None        # ('void0'|'void_undefined'|'int'|'bool'|'float'|'obj'|'objauto', detail)
        self.format = None        # None, 'translated' or 'pure'
        self.asserted = None      # the parameter count the wrapper asserts
        self.format_count = None  # the formatted text's parameter count (format included)
        self.self_cast = None     # the class a method's self is cast to
        self.problem = None       # why it cannot be described


ARG_FORMS = [
    (re.compile(r'^\(\s*([\w:\s]+?)\s*\)\s*params\[(\d+)\]\.IValue$'), 'int'),
    (re.compile(r'^static_cast<\s*([\w:\s]+?)\s*>\(\s*params\[(\d+)\]\.IValue\s*\)$'), 'int'),
    (re.compile(r'^()params\[(\d+)\]\.IValue$'), 'int'),
    (re.compile(r'^()params\[(\d+)\]\.FValue$'), 'float'),
    (re.compile(r'^()params\[(\d+)\]\.GetAsBool\(\)$'), 'bool'),
    (re.compile(r'^\(\s*([\w:\s]+?\*)\s*\)\s*params\[(\d+)\]\.Ptr$'), 'ptr'),
    (re.compile(r'^()params\[(\d+)\]\.CStr$'), 'ptr'),
]


def read_arg(arg):
    for rx, kind in ARG_FORMS:
        m = rx.match(arg)
        if m:
            return kind, int(m.group(2)), (m.group(1).strip() or None)
    return None


def read_call(call):
    """(function, [args]) of 'F(args)', or None."""
    m = re.match(r'^([A-Za-z_][\w:]*)\s*\((.*)\)$', call.strip(), re.S)
    if not m:
        return None
    return m.group(1), split_args(m.group(2))


def statements(body):
    """The statements of a function body, comments stripped, whitespace collapsed."""
    code = sb.strip_code(body)
    out, cur, depth = [], '', 0
    for ch in code:
        if ch in '({':
            depth += 1
        elif ch in ')}':
            depth -= 1
        if ch == ';' and depth == 0:
            out.append(' '.join(cur.split()))
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(' '.join(cur.split()))
    # a line break after an opening parenthesis left a space
    return [re.sub(r'\s+\)', ')', re.sub(r'\(\s+', '(', s)) for s in out if s]


def describe(wrapper, body, macros):
    """Fills in wrapper from the body text of its function (inside the braces)."""
    try:
        stmts = statements(expand(body, macros))
    except Fail as e:
        wrapper.problem = str(e)
        return
    calls, result = [], None
    for s in stmts:
        if s in ('(void)params', '(void)param_count'):
            continue
        m = re.match(r'^assert\(\(params != NULL && param_count >= (\d+)\) && ".*"\)$', s)
        if m:
            wrapper.asserted = int(m.group(1))
            continue
        if re.match(r'^assert\(\(self != NULL\) && ".*"\)$', s):
            continue
        if s == 'std::vector<char> ScSfBuffer':
            continue
        m = re.match(r'^const char \*scsf_buffer = ScriptSprintf\(ScSfBuffer, (get_translation\()?params\[(\d+) - 1\]'
                     r'\.CStr\)?, params \+ (\d+), param_count - (\d+)\)$', s)
        if m:
            wrapper.format = 'translated' if m.group(1) else 'pure'
            wrapper.format_count = int(m.group(2))
            if not (m.group(2) == m.group(3) == m.group(4)):
                wrapper.problem = 'formatted text with mixed counts'
            continue
        m = re.match(r'^return RuntimeScriptValue\(\)\.(SetInt32|SetInt32AsBool|SetFloat)\((.*)\)$', s)
        if m:
            result = ({'SetInt32': 'int', 'SetInt32AsBool': 'bool', 'SetFloat': 'float'}[m.group(1)], None)
            calls.append(m.group(2))
            continue
        m = re.match(r'^return RuntimeScriptValue\(\)\.SetScriptObject\(\(void \*\)(?:\(([\w\s:\*]+?) \*\))?(.*), '
                     r'&(\w+)\)$', s)
        if m:
            result = ('obj', (m.group(1), m.group(3)))
            calls.append(m.group(2))
            continue
        m = re.match(r'^(?:const )?(auto|[\w:]+) \*(\w+) = (?:\(([\w:]+) \*\))?(.*)$', s)
        if m:
            local, cast = m.group(2), m.group(3)
            nxt = stmts[stmts.index(s) + 1] if stmts.index(s) + 1 < len(stmts) else ''
            if local == 'scsf_buffer':
                continue
            if nxt == 'return RuntimeScriptValue().SetScriptObject(%s, %s)' % (local, local):
                result = ('objauto', cast or (m.group(1) if m.group(1) != 'auto' else None))
                calls.append(m.group(4))
                continue
            if re.match(r'^return RuntimeScriptValue\(\)\.SetScriptObject\(\(void \*\)%s, &(\w+)\)$' % local, nxt):
                mgr = re.match(r'^.*&(\w+)\)$', nxt).group(1)
                result = ('obj', (m.group(1), mgr))
                calls.append(m.group(4))
                continue
            wrapper.problem = 'unread statement: ' + s
            return
        if re.match(r'^return RuntimeScriptValue\(\)\.SetScriptObject\((\w+), \1\)$', s) and result:
            continue
        if re.match(r'^return RuntimeScriptValue\(\)\.SetScriptObject\(\(void \*\)\w+, &\w+\)$', s) and result:
            continue
        if s in ('return RuntimeScriptValue((int32_t)0)', 'return RuntimeScriptValue(0)'):
            result = result or ('void0', None)
            continue
        if s == 'return RuntimeScriptValue()':
            result = result or ('void_undefined', None)
            continue
        if read_call(s):
            calls.append(s)
            continue
        wrapper.problem = 'unread statement: ' + s
        return
    if len(calls) != 1 or result is None:
        wrapper.problem = 'not one call and one return (%d calls)' % len(calls)
        return
    call = read_call(calls[0])
    if not call:
        wrapper.problem = 'unread call: ' + calls[0]
        return
    wrapper.function, args = call
    if args and re.match(r'^\(\s*([\w:\s]+?)\s*\*\s*\)\s*self$', args[0]):
        wrapper.kind = 'method'
        wrapper.self_cast = re.match(r'^\(\s*([\w:\s]+?)\s*\*\s*\)\s*self$', args[0]).group(1)
        args = args[1:]
    elif args and args[0] == 'self':
        wrapper.kind = 'method'
        wrapper.self_cast = 'void'
        args = args[1:]
    else:
        wrapper.kind = 'static'
    if wrapper.format:
        if not args or args[-1] != 'scsf_buffer':
            wrapper.problem = 'formatted text not passed last'
            return
        args = args[:-1]
        if len(args) != wrapper.format_count - 1:
            wrapper.problem = 'formatted text: %d fixed parameters, format at %d' % (len(args), wrapper.format_count)
            return
    for i, a in enumerate(args):
        r = read_arg(a)
        if r is None:
            wrapper.problem = 'unread argument: ' + a
            return
        kind, index, cast = r
        if index != i:
            wrapper.problem = 'argument %d reads params[%d]' % (i, index)
            return
        wrapper.params.append((kind, cast))
    wrapper.result = result


# ---------------------------------------------------------------------------------------------------------------
# Declarations: what each function's signature says
# ---------------------------------------------------------------------------------------------------------------

TYPE_WORDS = {'int', 'char', 'float', 'double', 'bool', 'void', 'long', 'short', 'unsigned', 'signed', 'const',
              'struct', 'class', 'enum', 'volatile'}


class Declarations:
    """{qualified name: {(return type, (parameter types))}} of the functions declared at namespace scope in the
    headers and sources of Engine/ and Common/ (third-party code and tests left out)."""

    DECL = re.compile(r'^((?:(?:extern|static|inline)\s+)*)((?:const\s+)?(?:struct\s+|class\s+|enum\s+)?[\w:<>]+'
                      r'(?:\s*const)?[\s\*&]+?)(\w+)\s*\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)\s*(?:const\s*)?(;|\{)',
                      re.M)

    def __init__(self, repo):
        self.functions = collections.defaultdict(set)
        for ext in ('.h', '.cpp'):
            for path in sb.code_files(repo, ext):
                self.read(sb.strip_code(sb.read_text(os.path.join(repo, path))))

    def read(self, code):
        code = re.sub(r'^\s*#.*$', '', code, flags=re.M)
        scopes = self.namespace_scopes(code)
        for m in self.DECL.finditer(code):
            ret = ' '.join(m.group(2).split())
            if ret in ('return', 'else', 'new', 'delete', 'case') or ret.startswith(('return', 'typedef', 'using')):
                continue
            params = tuple(param_type(p) for p in split_args(' '.join(m.group(4).split())))
            if params == ('void',):
                params = ()
            ns = self.scope_at(scopes, m.start())
            if ns is None:
                continue  # inside a class or a function body
            name = '::'.join(ns + [m.group(3)])
            self.functions[name].add((ret, params))

    @staticmethod
    def namespace_scopes(code):
        """[(position, namespace stack or None inside a non-namespace block)] at each brace."""
        out = [(0, [])]
        stack = []
        for m in re.finditer(r'\bnamespace\s+([\w:]+)\s*\{|\bnamespace\s*\{|\{|\}', code):
            if m.group(0).startswith('namespace'):
                stack.append(('ns', m.group(1) or ''))
            elif m.group(0) == '{':
                stack.append(('block', ''))
            elif stack:
                stack.pop()
            if all(k == 'ns' for k, _ in stack):
                names = []
                for _, n in stack:
                    names += [x for x in n.split('::') if x]
                out.append((m.end(), names))
            else:
                out.append((m.end(), None))
        return out

    @staticmethod
    def scope_at(scopes, pos):
        current = []
        for p, s in scopes:
            if p > pos:
                break
            current = s
        return current

    def lookup(self, name):
        """The signatures of a function named as a call writes it (unqualified, or qualified by a namespace)."""
        found = set()
        for qual, sigs in self.functions.items():
            if qual == name or qual.endswith('::' + name):
                found |= sigs
        # declarations that differ only in spelling (String and AGS::Common::String) are one
        return {(ret, params) for ret, params in found}


def param_type(param):
    """The type of a declared parameter, its name and default value dropped."""
    p = param.split('=')[0].strip()
    p = re.sub(r'\bAGS_FORMAT_STRING_ARG\b', '', p).strip()
    tokens = re.findall(r'::|[A-Za-z_]\w*|\*|&|\[\]', p)
    if len(tokens) >= 2 and re.match(r'^[A-Za-z_]\w*$', tokens[-1]) and tokens[-1] not in TYPE_WORDS \
            and tokens[-2] != '::':
        p = p[:p.rfind(tokens[-1])].strip()
        if p.endswith('[]'):
            p = p[:-2].strip() + ' *'
    elif p.endswith('[]'):
        p = p[:-2].strip() + ' *'
    return ' '.join(p.split())


def type_kind(t):
    """(kind, base type name) of a declared type: void, bool, float, int, ptr, string (const String &), ref."""
    t = re.sub(r'\b(const|volatile|struct|class|enum)\b', '', t).strip()
    t = ' '.join(re.sub(r'\b\w+::', '', t).split())  # namespaces: one type however a declaration spells it
    if t.endswith('&&'):
        return 'ref', t[:-2].strip()
    if t.endswith('*'):
        return 'ptr', t[:-1].strip()
    if t.endswith('&'):
        base = t[:-1].strip()
        return ('string' if base == 'String' else 'ref'), base
    if t == 'void':
        return 'void', 'void'
    if t == 'bool':
        return 'bool', 'bool'
    if t in ('float', 'double'):
        return 'float', t
    return 'int', t


def normal_sig(sig):
    ret, params = sig
    return (type_kind(ret), tuple(type_kind(p) for p in params))


# ---------------------------------------------------------------------------------------------------------------
# Deciding a row
# ---------------------------------------------------------------------------------------------------------------

class Decision:
    def __init__(self):
        self.form = None        # ScMethod, ScStatic, ScMethodFormat, ScStaticFormat
        self.function = None    # the template argument (&F or a static_cast)
        self.tags = []
        self.problem = None
        self.notes = []


def decide(wrapper, decls):
    d = Decision()
    if wrapper.problem:
        d.problem = wrapper.problem
        return d
    if wrapper.format == 'pure':
        d.problem = 'formatted text without translation (no form for it)'
        return d
    name = wrapper.function
    sigs = decls.lookup(name)
    want = len(wrapper.params) + (1 if wrapper.kind == 'method' else 0) + (1 if wrapper.format else 0)
    matching = {s for s in sigs if len(s[1]) == want}
    if not matching:
        d.problem = 'no declaration of %s with %d parameters (%d found)' % (name, want, len(sigs))
        return d
    overloaded = len({normal_sig(s)[1] for s in sigs}) > 1
    # one signature per spelling of the same types; an overload that the wrapper's reads fit is the one it calls
    groups = {}
    for s in sorted(matching):
        groups.setdefault(normal_sig(s), s)
    decided = [decide_for(wrapper, sig, name, overloaded) for sig in groups.values()]
    fitting = [x for x in decided if not x.problem]
    if len(fitting) == 1:
        return fitting[0]
    if not fitting:
        return decided[0] if len(decided) == 1 else problem_of('no overload of %s fits: %s' % (
            name, '; '.join(x.problem for x in decided)))
    return problem_of('declarations of %s differ: %s' % (name, sorted(matching)))


def problem_of(text):
    d = Decision()
    d.problem = text
    return d


def decide_for(wrapper, sig, name, overloaded):
    d = Decision()
    d.function = '&' + name
    if overloaded:
        d.function = 'static_cast<%s (*)(%s)>(&%s)' % (sig[0], ', '.join(sig[1]), name)
        d.notes.append('overloaded: ' + d.function)
    (rkind, rbase), pkinds = normal_sig(sig)
    pkinds = list(pkinds)
    if wrapper.kind == 'method':
        self_kind = pkinds.pop(0)
        if self_kind[0] != 'ptr':
            d.problem = 'method on a function whose first parameter is not a pointer'
            return d
        if wrapper.self_cast and re.sub(r'^.*::', '', wrapper.self_cast.replace('const ', '').strip()) != self_kind[1]:
            d.notes.append('self cast %s, parameter %s' % (wrapper.self_cast, self_kind[1]))
    if wrapper.format:
        last = pkinds.pop()
        if last != ('ptr', 'char'):
            d.problem = 'formatted text for a last parameter of type %s' % (last,)
            return d
    # parameters
    for i, ((mkind, cast), (fkind, fbase)) in enumerate(zip(wrapper.params, pkinds)):
        if mkind == fkind:
            if mkind == 'ptr' and cast and re.sub(r'^.*::', '', cast.replace('const', '').replace('*', '').strip()) \
                    != fbase:
                d.notes.append('parameter %d: cast %s, parameter %s' % (i, cast, fbase))
            continue
        if mkind == 'int' and fkind == 'bool':
            d.tags.append('ScIntArg<%d>' % i)
        elif mkind == 'bool' and fkind == 'int':
            d.tags.append('ScBoolArg<%d>' % i)
        elif mkind == 'ptr' and fkind == 'string':
            pass  # a String made from the script string, as the call converted it
        else:
            d.problem = 'parameter %d: the wrapper reads %s, the function takes %s' % (i, mkind, fbase)
            return d
    # result
    mres, detail = wrapper.result
    if mres == 'void0':
        if rkind != 'void':
            d.tags.append('ScVoidResult')
    elif mres == 'void_undefined':
        if rkind != 'void':
            d.problem = 'an undefined result for a function that returns %s' % rbase
            return d
        d.tags.append('ScUndefinedResult')
    elif rkind == 'void':
        d.problem = 'the wrapper returns %s of a void function' % mres
        return d
    elif mres == 'int':
        if rkind not in ('int', 'bool'):
            d.problem = 'SetInt32 of a %s' % rbase
            return d
    elif mres == 'bool':
        if rkind == 'int':
            d.tags.append('ScBoolResult')
        elif rkind != 'bool':
            d.problem = 'SetInt32AsBool of a %s' % rbase
            return d
    elif mres == 'float':
        if rkind != 'float':
            d.problem = 'SetFloat of a %s' % rbase
            return d
    elif mres == 'obj':
        cls, mgr = detail
        if rkind != 'ptr':
            d.problem = 'an object result of a %s' % rbase
            return d
        if MANAGERS.get(rbase) != mgr:
            d.tags.append('ScManagedBy<%s>' % mgr)
        if cls and re.sub(r'^.*::', '', cls.replace('const', '').strip()) != rbase:
            d.notes.append('result cast %s, function returns %s' % (cls, rbase))
    elif mres == 'objauto':
        if rkind != 'ptr':
            d.problem = 'an object result of a %s' % rbase
            return d
        if rbase == 'void':
            if not detail:
                d.problem = 'a void * result with no class'
                return d
            d.tags.append('ScResultAs<%s>' % detail)
        elif rbase in MANAGERS:
            d.problem = 'a self-managed result of %s, which has a manager' % rbase
            return d
        elif detail and re.sub(r'^.*::', '', detail) != rbase:
            d.problem = 'a self-managed result cast to %s from %s' % (detail, rbase)
            return d
    if wrapper.format:
        if d.tags:
            d.problem = 'a formatted-text form with tags %s' % d.tags
            return d
        d.form = 'ScMethodFormat' if wrapper.kind == 'method' else 'ScStaticFormat'
    else:
        d.form = 'ScMethod' if wrapper.kind == 'method' else 'ScStatic'
    if wrapper.asserted is not None and wrapper.asserted != len(wrapper.params) + (1 if wrapper.format else 0):
        d.notes.append('asserted %d parameters, reads %d' % (wrapper.asserted, len(wrapper.params)))
    return d


def template_args(d):
    return ', '.join([d.function] + d.tags)


# ---------------------------------------------------------------------------------------------------------------
# Rows
# ---------------------------------------------------------------------------------------------------------------

ROW = re.compile(r'\{\s*"([^"]+)"\s*,\s*(?:API_FN_PAIR\(\s*(\w+)\s*\)|(Sc_\w+)\s*(?:,\s*([\w:&]+))?)\s*\}')
DIRECT = re.compile(r'\b(ccAddExternal(?:Static|Object)Function)\(\s*("[^"]+")\s*,\s*(Sc_\w+)\s*\)')


def rows_of(text):
    """[(match, script name, wrapper, plugin expression or None, 'row'|'direct')] of a file's text."""
    out = []
    for m in ROW.finditer(text):
        if m.group(2):
            out.append((m, m.group(1), 'Sc_' + m.group(2), m.group(2), 'row'))
        else:
            out.append((m, m.group(1), m.group(3), m.group(4), 'row'))
    for m in DIRECT.finditer(text):
        out.append((m, m.group(2).strip('"'), m.group(3), None, 'direct'))
    return sorted(out, key=lambda r: r[0].start())


def same_function(a, b):
    return a.lstrip('&').strip() == b.lstrip('&').strip()


def new_row(name, wrapper, d, plugin, kind):
    """The text of a converted row."""
    if kind == 'direct':
        return '%sWrapper<%s>()' % (d.form, template_args(d))
    args = '"%s"' % name
    if d.form.endswith('Format') or (plugin and not same_function(plugin, wrapper.function)):
        if not plugin:
            raise Fail('a formatted-text row without a plugin function: %s' % name)
        args += ', ' + plugin
    return '%s<%s>(%s)' % (d.form, template_args(d), args)


# ---------------------------------------------------------------------------------------------------------------
# The run
# ---------------------------------------------------------------------------------------------------------------

def api_files(repo):
    d = os.path.join(repo, API_DIR)
    return sorted('%s/%s' % (API_DIR, f) for f in os.listdir(d) if f.endswith('.cpp'))


def wrapper_index(repo, macros):
    """{Sc_ name: Wrapper} of every Sc_ function defined in the API files."""
    index = {}
    for path in api_files(repo):
        lines = sb.read_text(os.path.join(repo, path)).splitlines(True)
        for item in sb.parse(lines, path):
            if item.kind == 'code' and item.function and item.name and item.name.startswith('Sc_'):
                text = sb.item_text(lines, item)
                body = text[text.find('{') + 1:text.rfind('}')]
                w = Wrapper(item.name, path, item)
                describe(w, body, macros)
                index[item.name] = w
    return index


def sc_references(repo):
    """{Sc_ name: [(path, line index)]} of every Sc_ identifier in the engine's sources and headers (comments and
    strings left out), tests included."""
    found = collections.defaultdict(list)
    paths = sb.code_files(repo, '.cpp') + sb.code_files(repo, '.h')
    test_dir = os.path.join(repo, 'Engine', 'test')
    paths += sorted('Engine/test/' + f for f in os.listdir(test_dir) if f.endswith(('.cpp', '.h')))
    for path in paths:
        if not path.startswith('Engine/'):
            continue
        code = sb.strip_code(sb.read_text(os.path.join(repo, path)))
        for n, line in enumerate(code.split('\n')):
            if DECLARATION.match(line):
                continue  # a declaration goes with the definition
            for m in re.finditer(r'\bSc_\w+', line):
                found[m.group(0)].append((path, n))
            for m in re.finditer(r'\bAPI_FN_PAIR\(\s*(\w+)\s*\)', line):  # names Sc_<its argument>
                found['Sc_' + m.group(1)].append((path, n))
    return found


def remove_lines(lines, start, end):
    """Removes lines [start, end) and the comment block directly above them; collapses the blank lines met."""
    while start > 0 and lines[start - 1].lstrip().startswith('//'):
        start -= 1
    del lines[start:end]
    # one blank line where the removal leaves two or more
    while start < len(lines) and start > 0 and not lines[start].strip() and not lines[start - 1].strip():
        del lines[start]
    return lines


DECLARATION = re.compile(r'^(?:extern\s+)?RuntimeScriptValue\s+(Sc_\w+)\s*\(.*\);\s*$')


def remove_declaration(repo, name, dry_run):
    """Removes the declarations of a Sc_ function (a header's, or an extern line of a source) from the engine's
    files, each with the comment lines right above it. Returns the files changed."""
    changed = []
    for path in sb.code_files(repo, '.h') + sb.code_files(repo, '.cpp'):
        if not path.startswith('Engine/'):
            continue
        full = os.path.join(repo, path)
        text, eol, bom = sb.read_eol(full)
        if name not in text:
            continue
        lines = text.splitlines(True)
        idx = [i for i, line in enumerate(lines)
               if DECLARATION.match(line) and DECLARATION.match(line).group(1) == name]
        if not idx:
            continue
        for i in reversed(idx):
            start = i
            while start > 0 and lines[start - 1].lstrip().startswith('//'):
                start -= 1
            del lines[start:i + 1]
        if not dry_run:
            sb.write_text(full, ''.join(lines), eol, bom)
        changed.append(path)
    return changed


def fix_includes(text):
    """Adds bindings.h after the last game/ include, drops script_api.h and the manager headers the text no
    longer uses."""
    lines = text.splitlines(True)
    if BINDINGS_INCLUDE + '\n' not in lines:
        idx = [i for i, line in enumerate(lines) if line.startswith('#include "')]
        if not idx:
            raise Fail('no #include lines')
        lines.insert(idx[-1] + 1, BINDINGS_INCLUDE + '\n')
    code_wo_includes = sb.strip_code(''.join(line for line in lines if not line.startswith('#include')))
    dropped = []
    if not any(re.search(r'\b%s' % n, code_wo_includes) for n in SCRIPT_API_NAMES):
        if SCRIPT_API_INCLUDE + '\n' in lines:
            lines.remove(SCRIPT_API_INCLUDE + '\n')
            dropped.append(SCRIPT_API_INCLUDE)
    for inc, names in MANAGER_HEADERS.items():
        if inc + '\n' in lines and not any(re.search(r'\b%s\b' % n, code_wo_includes) for n in names):
            lines.remove(inc + '\n')
            dropped.append(inc)
    return ''.join(lines), dropped


def convert_file(repo, path, index, decls, args, record):
    """Converts the rows of one file. Returns (converted rows, refused [(row, reason)], deleted wrappers)."""
    full = os.path.join(repo, path)
    text, eol, bom = sb.read_eol(full)
    converted, refused, notes = [], [], []
    pieces, pos = [], 0
    for m, name, sc, plugin, kind in rows_of(text):
        w = index.get(sc)
        if w is None:
            refused.append((name, 'no wrapper %s in %s' % (sc, API_DIR)))
            continue
        if os.path.normpath(w.path) in [os.path.normpath(s) for s in SKIP_FILES]:
            continue
        d = decide(w, decls)
        if d.problem:
            refused.append((name, '%s: %s' % (sc, d.problem)))
            continue
        row = new_row(name, w, d, plugin, kind)
        if kind == 'direct':
            replacement = text[m.start():m.end()].replace(sc, row)
        else:
            replacement = row
        pieces.append(text[pos:m.start()] + replacement)
        pos = m.end()
        converted.append((name, sc, row, d))
        for n in d.notes:
            notes.append('%s (%s): %s' % (name, sc, n))
        if record is not None:
            record.append('\t'.join([path, name, sc, d.form if kind == 'row' else kind, d.function, ' '.join(d.tags),
                                     plugin or '']))
    pieces.append(text[pos:])
    new_text = ''.join(pieces)
    return new_text, eol, bom, converted, refused, notes


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('files', nargs='*', help='API files to convert (repository paths)')
    ap.add_argument('--all', action='store_true', help='every file of %s' % API_DIR)
    ap.add_argument('--repo', default=os.path.abspath(os.path.join(SCRIPT_DIR, '..')))
    ap.add_argument('--dry-run', action='store_true', help='report only, write nothing')
    ap.add_argument('--verbose', action='store_true', help='list every converted row')
    ap.add_argument('--record', help='append the converted rows to this TSV file')
    args = ap.parse_args()
    repo = os.path.abspath(args.repo)
    files = api_files(repo) if args.all else [f.replace('\\', '/') for f in args.files]
    if not files:
        ap.error('name the files or pass --all')
    try:
        macros = read_macros(repo)
        index = wrapper_index(repo, macros)
        decls = Declarations(repo)
        record = [] if args.record else None
        total_conv, total_ref = 0, 0
        written = {}
        converted_wrappers = {}
        for path in files:
            new_text, eol, bom, converted, refused, notes = convert_file(repo, path, index, decls, args, record)
            converted_wrappers[path] = [sc for _, sc, _, _ in converted]
            total_conv += len(converted)
            total_ref += len(refused)
            print('%s: %d rows converted, %d refused' % (path, len(converted), len(refused)))
            for name, why in refused:
                print('  refused %s: %s' % (name, why))
            for n in notes:
                print('  note %s' % n)
            if args.verbose:
                for name, sc, row, d in converted:
                    print('  %s <- %s' % (row, sc))
            if converted:
                written[path] = (new_text, eol, bom)
        if args.dry_run:
            print('dry run: %d rows would be converted, %d refused' % (total_conv, total_ref))
            return 0
        for path, (new_text, eol, bom) in written.items():
            sb.write_text(os.path.join(repo, path), new_text, eol, bom)
        # the wrappers of the converted rows that nothing references any more: deleted where they are defined,
        # with a declaration in a header
        deleted = []
        refs = sc_references(repo)
        candidates = sorted({sc for path in written for sc in converted_wrappers.get(path, ())})
        by_file = collections.defaultdict(list)
        for sc in candidates:
            by_file[index[sc].path].append(sc)
        for path, names in sorted(by_file.items()):
            full = os.path.join(repo, path)
            text, eol, bom = sb.read_eol(full)
            lines = text.splitlines(True)
            items = {it.name: it for it in sb.parse(lines, path) if it.kind == 'code' and it.function}
            doomed = []
            for sc in names:
                item = items.get(sc)
                if item is None:
                    continue
                others = [(p, n) for p, n in refs.get(sc, ()) if not (p == path and item.start <= n < item.end)
                          and not p.endswith('.h')]
                if not others:
                    doomed.append(item)
            for item in sorted(doomed, key=lambda it: -it.start):
                lines = remove_lines(lines, item.start, item.end)
                headers = remove_declaration(repo, item.name, False)
                deleted.append((item.name, path, headers))
            sb.write_text(full, ''.join(lines), eol, bom)
            if path not in written:
                written[path] = None
        for path in written:
            full = os.path.join(repo, path)
            text, eol, bom = sb.read_eol(full)
            text, dropped = fix_includes(text)
            sb.write_text(full, text, eol, bom)
            sb.format_file(repo, path)
            if dropped:
                print('%s: includes dropped: %s' % (path, ', '.join(dropped)))
        for sc, path, headers in deleted:
            print('deleted %s (%s)%s' % (sc, path, (', declaration in ' + ', '.join(headers)) if headers else ''))
        if record is not None:
            with open(args.record, 'a', encoding='utf-8') as f:
                for line in record:
                    f.write(line + '\n')
        print('%d rows converted, %d refused, %d wrappers deleted' % (total_conv, total_ref, len(deleted)))
    except Fail as e:
        print('convert_bindings: ' + str(e), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
