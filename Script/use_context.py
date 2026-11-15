#!/usr/bin/env python3
"""
Turns the uses of globals into the members of a context object (docs/refactor/PLAN.md 5.1 steps 2 and 3,
docs/refactor/RUNTIME-SPLIT.md 2): `usetup.Translation` becomes `EngineServices::Current().Config.Translation`.

    python Script/use_context.py --context services --dry-run    # what would change, per function
    python Script/use_context.py --context services              # rewrite the files (clang-format them after)
    python Script/use_context.py --context services --check      # exit 1 if a mapped global is still named
    python Script/use_context.py --self-test
    options: --repo <dir>, --files <path>... (default: the engine's sources and headers, third-party code left out)

The rule (one per use, counted per function body, all the mapped names together):
  * A function that names the globals once: the direct form, `EngineServices::Current().Platform->YieldCPU()`.
  * A function that names them more than once: `EngineServices &services = EngineServices::Current();` as the
    first statement of its body, then `services.Config...`.
  * A use outside a function body (a namespace-scope or class-scope initializer, a constructor's initializer
    list, a default argument, a lambda at namespace scope): the direct form; listed in the output, since such an
    initializer runs during the dynamic initialization of the program.
  * A lambda inside a function: the function's local if the lambda captures by reference by default (`[&]`),
    the direct form otherwise (no capture is added). A class defined inside a function: the direct form.
  * Not a use: a member access (`.name`, `->name`, `X::name`), a qualifier (`name::`), a member initializer
    (`: name(...)`), a declaration of a variable or parameter that has the name (the function is then left
    alone for that name and reported), and anything in a comment, a string or a preprocessor line.
  * A function that already names the local (`services`) is left alone and reported: choose by hand.
  * A changed file gets `#include "<header>"` in its main include block (clang-format sorts it).

The tokenizer skips comments, string, character and raw string literals and preprocessor lines; the brace
tracker tells function bodies (a `{` after a parameter list, its qualifiers or a constructor's initializer list)
from namespace, class, enum and `extern "C"` blocks and initializers. Code under `#if`/`#else` is read branch by
branch from the state at the `#if`; branches that leave the braces in different states are reported.

The contexts are in CONTEXTS: the type, the accessor, the local's name, the header and the map from a global's
name to its member. --dry-run prints every function it would change; read it before running for real.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from collect_externs import main_include_block  # noqa: E402

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
SOURCES = ('.cpp', '.h', '.inl', '.mm', '.m', '.c', '.cc')
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude)/')

CONTEXTS = {
    # 5.1 step 2: what lives as long as the process
    'services': {
        'type': 'EngineServices',
        'accessor': 'EngineServices::Current()',
        'local': 'services',
        'header': 'platform/base/engine_services.h',
        'own_files': ('Engine/platform/base/engine_services.h', 'Engine/platform/base/engine_services.cpp'),
        'names': {
            'usetup': 'Config',
            'ResPaths': 'ResPaths',
            'platform': 'Platform',
            'gfxDriver': 'GfxDriver',
            'AssetMgr': 'Assets',
            'EngineVersion': 'Version',
            'want_exit': 'WantExit',
            'abort_engine': 'AbortEngine',
            'switched_away': 'SwitchedAway',
            'switching_away_from_game': 'SwitchingAwayFromGame',
            'game_update_suspend': 'GameUpdateSuspend',
            'proper_exit': 'ProperExit',
            'logScriptRTTI': 'LogScriptRTTI',
            'logScriptTOC': 'LogScriptTOC',
        },
    },
}

# --------------------------------------------------------------------------------------------------------------
# Tokenizer

Token = collections.namedtuple('Token', 'kind text start end')  # kind: id, num, str, punct, pp

PUNCT3 = ('<<=', '>>=', '...', '->*', '<=>')
PUNCT2 = ('::', '->', '&&', '||', '<<', '>>', '==', '!=', '<=', '>=', '+=', '-=', '*=', '/=', '%=', '&=', '|=',
          '^=', '++', '--', '.*', '##')
ID_START = re.compile(r'[A-Za-z_]')
ID = re.compile(r'[A-Za-z_]\w*')
NUM = re.compile(r"\.?[0-9](?:[eEpP][+-]|[0-9A-Za-z_.']|)*")
RAW_PREFIX = re.compile(r'(?:u8|u|U|L)?R"([^(\s\\]{0,16})\(')
STR_PREFIX = re.compile(r'(?:u8|u|U|L)?(["\'])')


def tokenize(text):
    """The tokens of C++ source; comments go, a preprocessor directive (with its continuation lines) is one
    token of kind 'pp'."""
    out = []
    i, n = 0, len(text)
    line_start = True  # only whitespace since the last newline
    while i < n:
        c = text[i]
        if c == '\n':
            line_start = True
            i += 1
            continue
        if c in ' \t\r\f\v':
            i += 1
            continue
        if c == '/' and text.startswith('//', i):
            j = text.find('\n', i)
            # a backslash at the end of a line comment continues it
            while j != -1 and text[i:j].rstrip('\r').endswith('\\'):
                j = text.find('\n', j + 1)
            i = n if j == -1 else j
            continue
        if c == '/' and text.startswith('/*', i):
            j = text.find('*/', i + 2)
            i = n if j == -1 else j + 2
            continue
        if c == '#' and line_start:
            j = i
            while True:
                k = text.find('\n', j)
                if k == -1:
                    k = n
                    break
                seg = text[j:k].rstrip('\r')
                # a block comment opened on the directive's line runs on
                if seg.count('/*') > seg.count('*/'):
                    e = text.find('*/', k)
                    j = n if e == -1 else e + 2
                    continue
                if seg.endswith('\\'):
                    j = k + 1
                    continue
                break
            out.append(Token('pp', text[i:k], i, k))
            i = k
            continue
        line_start = False
        m = RAW_PREFIX.match(text, i)
        if m:
            end = text.find(')' + m.group(1) + '"', m.end())
            end = n if end == -1 else end + len(m.group(1)) + 2
            out.append(Token('str', text[i:end], i, end))
            i = end
            continue
        m = STR_PREFIX.match(text, i)
        if m and (m.start(1) == i or not ID.match(text, i) or ID.match(text, i).end() == m.start(1)):
            q = m.group(1)
            j = m.end()
            while j < n and text[j] != q and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            out.append(Token('str', text[i:j], i, j))
            i = j
            continue
        if ID_START.match(c):
            m = ID.match(text, i)
            out.append(Token('id', m.group(0), i, m.end()))
            i = m.end()
            continue
        if c.isdigit() or (c == '.' and i + 1 < n and text[i + 1].isdigit()):
            m = NUM.match(text, i)
            out.append(Token('num', m.group(0), i, m.end()))
            i = m.end()
            continue
        for p in PUNCT3 + PUNCT2:
            if text.startswith(p, i):
                out.append(Token('punct', p, i, i + len(p)))
                i += len(p)
                break
        else:
            out.append(Token('punct', c, i, i + 1))
            i += 1
    return out


# --------------------------------------------------------------------------------------------------------------
# Brace tracker

KEYWORDS_NOT_TYPE = {'return', 'case', 'delete', 'new', 'throw', 'sizeof', 'else', 'do', 'goto', 'co_return',
                     'co_yield', 'typeid', 'alignof', 'not', 'and', 'or', 'if', 'while', 'switch', 'for'}
CLASS_KEYS = {'class', 'struct', 'union', 'enum'}
QUALIFIERS = {'const', 'volatile', 'override', 'final', 'noexcept', 'mutable', '&', '&&', 'try'}


class Scope:
    """A brace block. kind: ns (namespace, extern "C"), class (class, struct, union, enum), body (a function's
    body), lambda (a lambda inside a body or at namespace scope), localclass (a class inside a body), block (any
    other block inside a body), init (an initializer's braces outside a body)."""

    def __init__(self, kind, open_tok, func=None, by_ref=False):
        self.kind = kind
        self.open_tok = open_tok
        self.func = func  # the Function this block belongs to (body, and blocks and lambdas inside it)
        self.by_ref = by_ref  # a lambda that captures by reference by default
        self.name = None  # a class's name


class Function:
    def __init__(self, name, open_tok, sig):
        self.name = name
        self.open_tok = open_tok  # index of the body's `{`
        self.sig = sig  # token indexes of the declaration before the body
        self.uses = []  # (token index, through_local_allowed, global-qualified)
        self.shadowed = set()  # mapped names declared as variables or parameters here
        self.names = set()  # every identifier named in it (signature and body)
        self.anomaly = None
        self.branch = ()  # the #if branches the body's `{` is in
        self.branch_of = {}  # use's token index -> the #if branches it is in
        self.in_assert = set()  # uses inside assert(), which a release build does not compile
        self.cls = None  # the class it is a member function of, if known


class Analysis:
    def __init__(self):
        self.functions = []
        self.scope_uses = []  # (token index, why) uses outside any function body
        self.scope_decls = []  # (token index, scope kind) declarations of a mapped name outside function bodies
        self.anomalies = []  # text
        self.class_members = collections.defaultdict(set)  # class name -> mapped names declared as its members


def statement_head(toks, stmt):
    """The tokens of a statement outside function bodies, without template<...> heads and attributes."""
    return [toks[i] for i in stmt]


def top_level(toks, stmt):
    """Yields (position in stmt, token) for the tokens of stmt not inside () or [] (angle brackets are not
    tracked: they are ambiguous)."""
    depth = 0
    for p, i in enumerate(stmt):
        t = toks[i].text
        if toks[i].kind == 'punct' and t in '([':
            if depth == 0:
                yield p, toks[i]
            depth += 1
            continue
        if toks[i].kind == 'punct' and t in ')]':
            depth -= 1
            if depth == 0:
                yield p, toks[i]
            continue
        if depth == 0:
            yield p, toks[i]


def classify_scope_brace(toks, stmt):
    """Kind of a `{` met outside function bodies, given the statement before it: ('ns'|'class'|'init'|'body'|
    'lambda'|'initbrace', name of the function or None)."""
    words = [toks[i] for i in stmt]
    texts = [t.text for t in words]
    if not texts:
        return 'init', None  # a bare block at namespace scope does not exist; an initializer's inner brace
    if 'namespace' in texts[:2] or (texts[0] == 'extern' and len(words) > 1 and words[1].kind == 'str'):
        return 'ns', None
    top = list(top_level(toks, stmt))
    first_paren = next((p for p, t in top if t.kind == 'punct' and t.text == '('), None)
    first_colon = next((p for p, t in top if t.kind == 'punct' and t.text == ':'), None)
    has_class_key = any(t.kind == 'id' and t.text in CLASS_KEYS for _, t in top)
    eq = next((p for p, t in top if t.kind == 'punct' and t.text == '=' and
               not (p > 0 and texts[p - 1] == 'operator')), None)
    if has_class_key and (first_paren is None or (first_colon is not None and first_colon < first_paren)):
        return 'class', None
    if first_paren is None:
        return 'init', None
    if eq is not None and eq < first_paren:
        # `auto f = [](...) {` is a lambda at namespace scope; `T x = f(...) {` does not exist
        if '[' in texts[eq:first_paren] or ']' == texts[first_paren - 1]:
            return 'lambda', None
        return 'init', None
    if first_paren > 0 and texts[first_paren - 1] == ']':
        return 'lambda', None
    # the parameter list ends at the `)` matching first_paren
    depth, close = 0, None
    for p in range(first_paren, len(words)):
        if words[p].kind != 'punct':
            continue
        if words[p].text in '([':
            depth += 1
        elif words[p].text in ')]':
            depth -= 1
            if depth == 0:
                close = p
                break
    if close is None:
        return 'init', None
    after = texts[close + 1:]
    name_parts = []
    p = first_paren - 1
    while p >= 0 and (words[p].kind == 'id' or texts[p] in ('::', '~')):
        if words[p].kind == 'id' and name_parts and name_parts[0] not in ('::', '~'):
            break
        name_parts.insert(0, texts[p])
        p -= 1
    name = ''.join(name_parts) or '?'
    # a constructor's initializer list: a single `:` after the parameters
    if ':' in after:
        last = texts[-1]
        if last in (')', '}'):
            return 'body', name
        return 'initbrace', None
    return 'body', name


def capture_by_ref(toks, open_bracket, close_bracket):
    inner = [toks[i].text for i in range(open_bracket + 1, close_bracket)]
    return bool(inner) and inner[0] == '&' and (len(inner) == 1 or inner[1] == ',')


def find_lambda_capture(toks, brace, match):
    """If the `{` at index brace opens a lambda's body, the indexes of its capture's [ and ]; else None."""
    j = brace - 1
    # skip qualifiers and a trailing return type back to `)` or `]`
    while j >= 0 and toks[j].kind == 'id' and toks[j].text in ('mutable', 'noexcept', 'constexpr'):
        j -= 1
    if j >= 0 and toks[j].kind != 'punct':
        # `-> Type {`
        k = j
        while k >= 0 and (toks[k].kind == 'id' or toks[k].text in ('::', '*', '&', '<', '>', ',')):
            k -= 1
        if k >= 0 and toks[k].text == '->':
            j = k - 1
    if j < 0:
        return None
    if toks[j].text == ')':
        o = match.get(j)
        if o is None or o == 0:
            return None
        j = o - 1
    if toks[j].text != ']':
        return None
    o = match.get(j)
    if o is None:
        return None
    before = toks[o - 1] if o > 0 else None
    if before is not None and (before.kind in ('id', 'num', 'str') and before.text not in ('return', 'else')
                               or before.text in (')', ']')):
        return None  # a subscript
    return o, j


def is_declaration(toks, i):
    """Whether the identifier at i is declared there (a variable, a parameter, a member)."""
    nxt = toks[i + 1].text if i + 1 < len(toks) else ''
    if nxt not in (';', '=', ',', ')', '[', '{', ':', '('):
        return False
    if i == 0:
        return False
    prev = toks[i - 1]

    def typeish(t):
        return (t.kind == 'id' and t.text not in KEYWORDS_NOT_TYPE) or t.text == '>'

    if typeish(prev):
        if nxt == '(' and prev.text in ('const',):
            return False
        return True
    if prev.text in ('*', '&') and i >= 2:
        pp = toks[i - 2]
        return typeish(pp) or pp.text in ('*', 'const')
    return False


def analyse(text, names, local):
    """The functions and the uses of the mapped names in a file."""
    toks = tokenize(text)
    an = Analysis()
    match = {}  # index of ) or ] -> index of its ( or [
    state = {'scopes': [], 'stmt': [], 'parens': []}
    pp_stack = []  # (state at #if, state at the end of the first branch or None)

    def copy_state(s):
        return {'scopes': list(s['scopes']), 'stmt': list(s['stmt']), 'parens': list(s['parens'])}

    def compare_branches(first, other, t):
        if [s.open_tok for s in first['scopes']] != [s.open_tok for s in other['scopes']]:
            an.anomalies.append('%d: the branches of an #if leave the braces in different states'
                                % line_of(text, t.start))
            for s in first['scopes'] + other['scopes']:
                if s.func is not None:
                    s.func.anomaly = 'braces differ between #if branches'

    branch = []  # ids of the #if branches the current token is in
    next_branch = [0]

    def new_branch():
        next_branch[0] += 1
        return next_branch[0]

    for i, t in enumerate(toks):
        if t.kind == 'pp':
            d = re.match(r'#\s*(\w*)', t.text).group(1)
            if d in ('if', 'ifdef', 'ifndef'):
                branch.append(new_branch())
            elif d in ('elif', 'else') and branch:
                branch[-1] = new_branch()
            elif d == 'endif' and branch:
                branch.pop()
            if d in ('if', 'ifdef', 'ifndef'):
                pp_stack.append([copy_state(state), None])
            elif d in ('elif', 'else') and pp_stack:
                if pp_stack[-1][1] is None:
                    pp_stack[-1][1] = copy_state(state)
                else:
                    compare_branches(pp_stack[-1][1], state, t)
                state = copy_state(pp_stack[-1][0])
            elif d == 'endif' and pp_stack:
                at_if, first = pp_stack.pop()
                if first is not None:
                    compare_branches(first, state, t)
                    state = first
            continue
        scopes, stmt, parens = state['scopes'], state['stmt'], state['parens']
        inner = scopes[-1] if scopes else None
        func = inner.func if inner else None
        if t.kind == 'punct' and t.text in '([':
            parens.append(i)
        elif t.kind == 'punct' and t.text in ')]':
            if parens:
                match[i] = parens.pop()
        if t.kind == 'punct' and t.text == '{':
            if func is not None:
                cap = find_lambda_capture(toks, i, match)
                if cap:
                    scopes.append(Scope('lambda', i, func, capture_by_ref(toks, *cap)))
                else:
                    words = [toks[k].text for k in stmt]
                    top = [x for x in words[:3]]
                    if any(w in CLASS_KEYS for w in top) and '(' not in words:
                        scopes.append(Scope('localclass', i, func))
                    else:
                        scopes.append(Scope('block', i, func))
                state['stmt'] = []
                continue
            if inner is not None and inner.kind in ('init', 'initbrace'):
                scopes.append(Scope('init', i))
                state['stmt'] = stmt + [i] if inner.kind == 'initbrace' else []
                continue
            kind, name = classify_scope_brace(toks, stmt)
            if kind == 'body':
                f = Function(name, i, list(stmt))
                f.branch = tuple(branch)
                for k in stmt:
                    if toks[k].kind == 'id':
                        f.names.add(toks[k].text)
                # a parameter with a mapped name was taken for a declaration outside a body: it is the function's
                in_sig = set(stmt)
                for d in [d for d in an.scope_decls if d[0] in in_sig]:
                    f.shadowed.add(toks[d[0]].text)
                    an.scope_decls.remove(d)
                # and its uses in the initializer list are of the parameter
                for u in [u for u in an.scope_uses if u[0] in in_sig and toks[u[0]].text in f.shadowed]:
                    an.scope_uses.remove(u)
                # the class the function is a member of: `X::f(` or a definition inside the class
                if '::' in name:
                    f.cls = name.rsplit('::', 1)[0].split('::')[-1]
                else:
                    f.cls = next((s.name for s in reversed(scopes) if s.kind == 'class'), None)
                an.functions.append(f)
                scopes.append(Scope('body', i, f))
            elif kind == 'lambda':
                scopes.append(Scope('lambda', i, None))
            elif kind == 'initbrace':
                scopes.append(Scope('initbrace', i))
                state['stmt'] = stmt + [i]
                continue
            else:
                s = Scope(kind, i)
                if kind == 'class':
                    words = [toks[k] for k in stmt]
                    for p, w in enumerate(words):
                        if w.text in CLASS_KEYS:
                            nxt_ids = [x.text for x in words[p + 1:] if x.kind == 'id' and x.text != 'class']
                            s.name = nxt_ids[0] if nxt_ids else None
                            break
                scopes.append(s)
            state['stmt'] = []
            continue
        if t.kind == 'punct' and t.text == '}':
            if scopes:
                closed = scopes.pop()
                if closed.kind == 'initbrace':
                    state['stmt'] = stmt + [i]
                    continue
                # the braces of an initializer inside an initializer list keep the statement going
                if closed.kind == 'init' and scopes and scopes[-1].kind == 'initbrace':
                    state['stmt'] = stmt + [i]
                    continue
            state['stmt'] = []
            continue
        if t.kind == 'punct' and t.text == ';':
            if not (inner and inner.kind == 'init'):
                state['stmt'] = []
            continue
        stmt.append(i)
        if t.kind != 'id':
            continue
        if func is not None:
            func.names.add(t.text)
        if t.text not in names:
            continue
        # what the name is here
        prev = toks[i - 1] if i > 0 else None
        nxt = toks[i + 1] if i + 1 < len(toks) else None
        if prev is not None and prev.text in ('.', '->', '.*', '->*'):
            continue
        global_q = False
        if prev is not None and prev.text == '::':
            before = toks[i - 2] if i >= 2 else None
            if before is not None and (before.kind == 'id' or before.text == '>'):
                continue
            global_q = True
        if nxt is not None and nxt.text == '::':
            continue
        # the second declarator of `T a = 1, name;` outside a function
        later_declarator = func is None and prev is not None and prev.text == ',' and nxt is not None and \
            nxt.text in (';', '=', ',', '[') and not any(toks[k].text in ('(', '{') for k in stmt)
        if (is_declaration(toks, i) or later_declarator) and not global_q:
            if func is not None:
                func.shadowed.add(t.text)
            else:
                an.scope_decls.append((i, inner.kind if inner else 'file'))
                if inner is not None and inner.kind == 'class' and inner.name:
                    an.class_members[inner.name].add(t.text)
            continue
        # a member initializer `: name(` or `, name{` of a constructor
        if func is None and prev is not None and prev.text in (':', ',') and nxt is not None and \
                nxt.text in ('(', '{') and (inner is None or inner.kind in ('class', 'ns')) and \
                any(toks[k].text == ')' for k in stmt[:-1]):
            continue
        if func is None:
            if inner is not None and inner.kind == 'initbrace' or any(toks[k].text == '(' for k in stmt):
                why = 'in a declaration: default argument or initializer list'
            elif inner is not None and inner.kind == 'lambda':
                why = 'lambda outside a function'
            elif inner is not None and inner.kind == 'class':
                why = 'class-scope initializer'
            else:
                why = 'namespace-scope initializer'
            an.scope_uses.append((i, why, global_q))
            continue
        # through the local only if every lambda between the use and the body captures by reference
        allowed = True
        for s in reversed(scopes):
            if s.kind == 'body':
                break
            if (s.kind == 'lambda' and not s.by_ref) or s.kind == 'localclass':
                allowed = False
        func.uses.append((i, allowed, global_q))
        func.branch_of[i] = tuple(branch)
        if any(k > 0 and toks[k - 1].text == 'assert' for k in parens):
            func.in_assert.add(i)
    if state['scopes']:
        an.anomalies.append('end of file inside %d open braces' % len(state['scopes']))
    an.toks = toks
    return an


def line_of(text, pos):
    return text.count('\n', 0, pos) + 1


# --------------------------------------------------------------------------------------------------------------
# Rewriting

class Report:
    def __init__(self):
        self.per_name = collections.defaultdict(lambda: collections.Counter())  # name -> {direct, local}
        self.files_per_name = collections.defaultdict(set)
        self.locals = []  # (file, line, function, uses)
        self.direct_funcs = []
        self.scope_uses = []  # (file, line, name, why)
        self.manual = []  # (file, line, function, why)
        self.scope_decls = []  # (file, line, name, scope kind)
        self.anomalies = []
        self.conditional = []  # (file, line, function, uses): every use under an #if, so the direct form
        self.changed = {}


def class_members(repo, files, ctx):
    """Class name -> the mapped names that are members of it, from the class definitions in the files (a member
    function of such a class means its member by the bare name; base classes are not followed)."""
    out = collections.defaultdict(set)
    for f in files:
        for cls, ms in analyse(read(repo, f), ctx['names'], ctx['local']).class_members.items():
            out[cls] |= ms
    return out


def member_shadows(an, f, members):
    return (members or {}).get(f.cls, set()) | an.class_members.get(f.cls, set()) if f.cls else set()


def rewrite(path, text, ctx, rep, verbose, members=None):
    names = ctx['names']
    local = ctx['local']
    an = analyse(text, names, local)
    toks = an.toks
    for f in an.functions:
        ms = member_shadows(an, f, members)
        if ms & {toks[u[0]].text for u in f.uses}:
            rep.manual.append((path, line_of(text, toks[f.open_tok].start), f.name,
                               'a member of %s has the name %s: not converted there' % (f.cls, ', '.join(sorted(ms)))))
        f.member_shadowed = ms
    edits = []  # (start, end, replacement)
    for a in an.anomalies:
        rep.anomalies.append('%s:%s' % (path, a))
    for i, kind in an.scope_decls:
        rep.scope_decls.append((path, line_of(text, toks[i].start), toks[i].text, kind))

    def replacement(name, through_local):
        member = names[name]
        return ('%s.%s' % (local, member)) if through_local else ('%s.%s' % (ctx['accessor'], member))

    for i, why, global_q in an.scope_uses:
        t = toks[i]
        start = toks[i - 1].start if global_q else t.start
        edits.append((start, t.end, replacement(t.text, False)))
        rep.scope_uses.append((path, line_of(text, t.start), t.text, why))
        rep.per_name[t.text]['direct'] += 1
        rep.files_per_name[t.text].add(path)

    for f in an.functions:
        if not f.uses and not f.shadowed:
            continue
        line = line_of(text, toks[f.open_tok].start)
        uses = [u for u in f.uses if toks[u[0]].text not in f.shadowed | f.member_shadowed]
        if f.shadowed:
            rep.manual.append((path, line, f.name, 'declares a variable or parameter named %s: those names are not '
                               'converted in this function' % ', '.join(sorted(f.shadowed))))
        if not uses:
            continue
        if f.anomaly:
            rep.manual.append((path, line, f.name, f.anomaly + ': left alone'))
            continue
        through = [u for u in uses if u[1]]
        use_local = len(through) >= 2
        # the local must be used in every build: by a use outside the #if blocks of the body and outside assert()
        if use_local and not any(f.branch_of[u[0]] == f.branch and u[0] not in f.in_assert for u in through):
            use_local = False
            rep.conditional.append((path, line, f.name, len(through)))
        if use_local and local in f.names:
            rep.manual.append((path, line, f.name, 'already names `%s`: left alone' % local))
            continue
        for i, allowed, global_q in uses:
            t = toks[i]
            via = use_local and allowed
            start = toks[i - 1].start if global_q else t.start
            edits.append((start, t.end, replacement(t.text, via)))
            rep.per_name[t.text]['local' if via else 'direct'] += 1
            rep.files_per_name[t.text].add(path)
        if use_local:
            brace = toks[f.open_tok]
            ls = text.rfind('\n', 0, brace.start) + 1
            indent = re.match(r'[ \t]*', text[ls:]).group(0) + '    '
            eol = '\r\n' if '\r\n' in text else '\n'
            edits.append((brace.end, brace.end, '%s%s%s &%s = %s;' % (eol, indent, ctx['type'], local,
                                                                       ctx['accessor'])))
            rep.locals.append((path, line, f.name, len(through)))
        else:
            rep.direct_funcs.append((path, line, f.name, len(uses)))
        if verbose:
            print('%s:%d %s: %d use(s), %s' % (path, line, f.name, len(uses),
                                               'local `%s`' % local if use_local else 'direct'))
            for i, allowed, _ in uses:
                print('    %d: %s%s' % (line_of(text, toks[i].start), toks[i].text,
                                        '' if allowed or not use_local else ' (direct: in a lambda or local class)'))
    if verbose:
        for i, why, _ in an.scope_uses:
            print('%s:%d (outside a function: %s): %s' % (path, line_of(text, toks[i].start), why, toks[i].text))
    if not edits:
        return None
    edits.sort(key=lambda e: e[0], reverse=True)
    out = text
    for s, e, r in edits:
        out = out[:s] + r + out[e:]
    return add_include(out, ctx['header'])


def add_include(text, header):
    inc = '#include "%s"' % header
    if re.search(r'^\s*#\s*include\s+"%s"' % re.escape(header), text, re.M):
        return text
    lines = text.split('\n')
    at = main_include_block(lines, len(lines))
    eol = '\r' if lines and lines[0].endswith('\r') else ''
    if at is None:
        # no quoted include: after the last include of any kind, or after #pragma once
        idx = [k for k, l in enumerate(lines) if re.match(r'\s*#\s*(include|pragma\s+once)', l)]
        at = idx[-1] if idx else 0
    lines.insert(at + 1, inc + eol)
    return '\n'.join(lines)


def check_file(path, text, ctx, rep, members=None):
    """Every remaining use or namespace-scope declaration of a mapped name."""
    an = analyse(text, ctx['names'], ctx['local'])
    toks = an.toks
    found = []
    for f in an.functions:
        for i, _, _ in f.uses:
            if toks[i].text not in f.shadowed | member_shadows(an, f, members):
                found.append((i, 'use in %s' % f.name))
    for i, why, _ in an.scope_uses:
        found.append((i, 'use outside a function (%s)' % why))
    for i, kind in an.scope_decls:
        if kind in ('file', 'ns'):
            found.append((i, 'declared at namespace scope'))
    return [(path, line_of(text, toks[i].start), toks[i].text, why) for i, why in found]


def engine_files(repo, ctx):
    out = subprocess.run(['git', '-C', repo, 'ls-files', '--cached', '--others', '--exclude-standard', '--',
                          'Engine'], capture_output=True, text=True, check=True).stdout.split('\n')
    own = set(ctx.get('own_files', ()))
    return sorted({f for f in out if f.endswith(SOURCES) and not THIRD_PARTY.search(f) and f not in own and
                   os.path.exists(os.path.join(repo, f))})


def read(repo, path):
    with open(os.path.join(repo, path), encoding='utf-8', newline='') as f:
        return f.read()


# --------------------------------------------------------------------------------------------------------------

def self_test():
    toks = tokenize('a // x usetup\n/* usetup */ "usetup" \'u\' R"x(usetup)x" u8"s" #x\n#define A usetup \\\n b\nc')
    assert [(t.kind, t.text) for t in toks] == [
        ('id', 'a'), ('str', '"usetup"'), ('str', "'u'"), ('str', 'R"x(usetup)x"'), ('str', 'u8"s"'),
        ('punct', '#'), ('id', 'x'), ('pp', '#define A usetup \\\n b'), ('id', 'c')], toks
    assert [t.text for t in tokenize("x = 1'000 + 0x1F.p+3;")] == ['x', '=', "1'000", '+', '0x1F.p+3', ';']
    ctx = {'type': 'S', 'accessor': 'S::Current()', 'local': 's', 'header': 's.h', 'names': {'g': 'G', 'h': 'H'}}

    def run(src):
        rep = Report()
        return rewrite('t.cpp', src, ctx, rep, False), rep

    src = '''#include "a.h"
namespace N {
int x = g.a;
struct T : B { int g; T() : g(h) {} void m() const { g = 1; } };
void f(int a) {
    g.b();
    if (a) { h = 2; }
    auto l = [&]() { g.c(); };
    auto k = [this]() { ::g.d(); };
}
void one() { x.g = g; }
void shadow(int g) { g = h; }
enum class E : int { A = 1 };
int arr[] = { 1, 2 };
extern int k, g;
}
'''
    out, rep = run(src)
    assert 'int x = S::Current().G.a;' in out, out
    assert 'T() : g(S::Current().H) {}' in out, out
    assert 'void f(int a) {\n    S &s = S::Current();\n    s.G.b();' in out, out
    assert '{ s.H = 2; }' in out and '[&]() { s.G.c(); }' in out, out
    assert '[this]() { S::Current().G.d(); }' in out, out
    assert 'void one() { x.g = S::Current().G; }' in out, out
    assert 'void shadow(int g) { g = S::Current().H; }' in out, out
    assert '#include "a.h"\n#include "s.h"\n' in out, out
    assert 'extern int k, g;' in out and ('t.cpp', 15, 'g', 'ns') in rep.scope_decls, rep.scope_decls
    assert any('shadow' in m[2] for m in rep.manual), rep.manual
    # a member function means the member by the bare name, inside the class and out of line
    assert 'void m() const { g = 1; }' in out, out
    out, rep = run('struct D { int h; D(const D &o); };\nD::D(const D &o) { h = o.h; g = 1; }\n')
    assert 'D::D(const D &o) { h = o.h; S::Current().G = 1; }' in out, out

    src = '''Foo::Foo(int a)
    : _a(a), _b{g}
{
    use(h);
}
#if X
void p(int a) {
#else
void p() {
#endif
    g = h;
}
void q() { int services = 0; g = h; }
'''
    out, rep = run(src)
    assert ': _a(a), _b{S::Current().G}' in out and 'use(S::Current().H);' in out, out
    assert any('#if' in m[3] for m in rep.manual), rep.manual
    ctx['local'] = 'services'
    out, rep = run('void q() { int services = 0; g = h; }\n')
    assert out is None and 'already names' in rep.manual[0][3], rep.manual
    print('self-test passed')
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--repo', default=REPO)
    ap.add_argument('--context', choices=sorted(CONTEXTS), default='services')
    ap.add_argument('--files', nargs='*', help='repository paths (default: every source and header of Engine/)')
    ap.add_argument('--dry-run', action='store_true', help='print the changes per function, write nothing')
    ap.add_argument('--check', action='store_true', help='exit 1 if a mapped global is still named')
    ap.add_argument('--self-test', action='store_true')
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    ctx = CONTEXTS[args.context]
    every = engine_files(args.repo, ctx)
    files = args.files or every
    members = class_members(args.repo, every, ctx)
    rep = Report()
    if args.check:
        left = []
        for f in files:
            left += check_file(f, read(args.repo, f), ctx, rep, members)
        for f, line, name, why in left:
            print('%s:%d: %s: %s' % (f, line, name, why))
        print('%s: %d left' % (args.context, len(left)))
        return 1 if left else 0

    for f in files:
        text = read(args.repo, f)
        new = rewrite(f, text, ctx, rep, args.dry_run, members)
        if new is not None and new != text:
            rep.changed[f] = new

    print('\nper name (direct form / through the local, files):')
    for n in ctx['names']:
        c = rep.per_name.get(n, {})
        print('  %-26s %4d / %4d  %d files' % (n, c.get('direct', 0), c.get('local', 0),
                                                 len(rep.files_per_name.get(n, ()))))
    print('functions given the local: %d; functions with the direct form: %d; files: %d'
          % (len(rep.locals), len(rep.direct_funcs), len(rep.changed)))
    if rep.scope_uses:
        print('uses outside a function body:')
        for f, line, n, why in rep.scope_uses:
            print('  %s:%d: %s (%s)' % (f, line, n, why))
    if rep.scope_decls:
        print('declarations of a mapped name outside function bodies (not changed):')
        for f, line, n, kind in rep.scope_decls:
            print('  %s:%d: %s (%s)' % (f, line, n, kind))
    if rep.conditional:
        print('functions whose uses are all under an #if inside the body (direct form, so that no build has an '
              'unused local):')
        for f, line, fn, n in rep.conditional:
            print('  %s:%d %s: %d uses' % (f, line, fn, n))
    if rep.manual:
        print('MANUAL:')
        for f, line, fn, why in rep.manual:
            print('  %s:%d %s: %s' % (f, line, fn, why))
    if rep.anomalies:
        print('ANOMALIES:')
        for a in rep.anomalies:
            print('  ' + a)
    if not args.dry_run:
        for f, text in rep.changed.items():
            with open(os.path.join(args.repo, f), 'w', encoding='utf-8', newline='') as out:
                out.write(text)
        print('rewritten: %d files' % len(rep.changed))
    return 0


if __name__ == '__main__':
    sys.exit(main())
