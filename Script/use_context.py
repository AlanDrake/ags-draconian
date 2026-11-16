#!/usr/bin/env python3
"""
Turns the uses of globals into the members of a context object (docs/refactor/PLAN.md 5.1 steps 2 and 3,
docs/refactor/RUNTIME-SPLIT.md 2): `usetup.Translation` becomes `EngineServices::Current().Config.Translation`.

    python Script/use_context.py --context services --dry-run    # what would change, per function
    python Script/use_context.py --context services              # rewrite the files (clang-format them after)
    python Script/use_context.py --context services --check      # exit 1 if a mapped global is still named
    python Script/use_context.py --self-test
    options: --repo <dir> (a git checkout or a plain copy of the tree), --files <path>... (default: the engine's
    sources and headers, third-party code left out), --names <global>... (convert only these names of the table)

The rule (one per use, counted per function body, all the mapped names together):
  * A function that names the globals once: the direct form, `EngineServices::Current().Platform->YieldCPU()`.
  * A function that names them more than once: `EngineServices &services = EngineServices::Current();` as the
    first statement of its body, then `services.Config...`. The locals of several contexts open the body in the
    order of CONTEXTS (services, game, room), whatever the order of the runs.
  * A use inside a loop of the function (the header or the body of a `for`, range-`for`, `while` or
    `do ... while`, the range expression included) gives the function the local even if it is the only use, so
    that the context is not reloaded on every pass; the same for an existing direct form. A use that may not go
    through the local (lambda rule) does not count. If the local's name is taken, the use stays direct and the
    function is listed under "direct form inside a loop, the local's name is taken" for a decision by hand.
  * A global mapped to '' is the context itself: `thisroom.Width` becomes `Room::Current().Width` or
    `room.Width`; `game.chars` stays `game.chars` through the local (only the declaration is added).
  * A use outside a function body (a namespace-scope or class-scope initializer, a constructor's initializer
    list, a default argument, a lambda at namespace scope): the direct form; listed in the output, since such an
    initializer runs during the dynamic initialization of the program.
  * A lambda inside a function: the function's local if the lambda captures by reference by default (`[&]`) or
    captures the local by reference (`[&services]`), the direct form otherwise (no capture is added). A class
    defined inside a function: the direct form.
  * The local must be used in every build: if every use that could go through it is under an `#if` inside the
    body or inside assert(), the function takes the direct form.
  * Not a use: a member access (`.name`, `->name`, `X::name`), a qualifier (`name::`), a member initializer
    (`: name(...)`), a declaration of a variable or parameter that has the name (the function is then left
    alone for that name and reported), a class member of the name in a member function of the class, and
    anything in a comment, a string or a preprocessor line. `::name` is always the global.
  * The local's name is taken in a function that declares a variable or parameter of that name, in a member
    function of a class that has a member of that name, and, when the local's name is not a mapped global's
    (`services`, `room`), in a function that names it at all: every use there takes the direct form, reported
    as "decided by the rule: the local's name is taken". A parameter or variable of the local's name whose type
    is `X &` or `const X &`, X the context's type, is the context: the uses go through it, nothing is added.
  * Incremental runs: a body that holds the declaration (`Game &game = Game::Current();`, recognized by its
    tokens) takes the local for the new uses; a body without it whose direct forms (`Game::Current()...`, from an
    earlier run) and new uses make two or more gets the declaration, and its direct forms are rewritten to the
    local, by the same lambda and #if rules. So a run per batch of names gives what one run over all the names
    gives. Uses above a declaration that is not the body's first statement take the direct form (reported).
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
            # 5.1 step 3, batch services
            'appPath': 'AppPath',
            'appDirectory': 'AppDirectory',
            'cmdGameDataPath': 'CmdGameDataPath',
            'override_start_room': 'OverrideStartRoom',
            'justRunSetup': 'JustRunSetup',
            'justTellInfo': 'JustTellInfo',
            'tellInfoKeys': 'TellInfoKeys',
            'loadSaveGameOnStartup': 'LoadSaveGameOnStartup',
            'load_new_game': 'LoadNewGame',
            'gameHasBeenRestored': 'GameRestoredCount',
            'check_dynamic_sprites_at_exit': 'CheckDynamicSpritesAtExit',
            'GameScaling': 'GameScaling',
            'mousex': 'Input.MouseX',
            'mousey': 'Input.MouseY',
            'ignore_bounds': 'Input.IgnoreBounds',
            'sys_modkeys': 'Input.ModKeys',
            'sys_modkeys_fired': 'Input.ModKeysFired',
            'sys_mouse_x': 'Input.SysMouseX',
            'sys_mouse_y': 'Input.SysMouseY',
            'sys_mouse_z': 'Input.SysMouseZ',
            'editor_debugging_enabled': 'Debug.EditorDebuggingEnabled',
            'editor_debugging_initialized': 'Debug.EditorDebuggingInitialized',
            'editor_debugger_instance_token': 'Debug.EditorDebuggerInstanceToken',
            'editor_debugger': 'Debug.EditorDebugger',
            'break_on_next_script_step': 'Debug.BreakOnNextScriptStep',
            'display_fps': 'Debug.DisplayFps',
            'debug_flags': 'Debug.Flags',
            # 5.1 step 3, batch exceptions
            'gl_SysConfig': 'SysConfig',
            'pluginsWantingDebugHooks': 'PluginsWantingDebugHooks',
            # 5.1 step 3, batch sprites
            'spriteset': 'Sprites',
        },
    },
    # 5.1 step 3: one play-through (the global `game` is the context itself: `class Game : public GameDef`)
    'game': {
        'type': 'Game',
        'accessor': 'Game::Current()',
        'local': 'game',
        'header': 'game/game_context.h',
        'own_files': ('Engine/game/game_context.h',),
        'names': {
            'game': '',
            'play': 'Play',
            # batch world
            'charextra': 'CharExtras',
            'views': 'Views',
            'playerchar': 'PlayerChar',
            '_sc_PlayerCharPtr': 'ScPlayerCharPtr',
            'char_lowest_yp': 'CharLowestYp',
            'obj_lowest_yp': 'ObjLowestYp',
            'displayed_room': 'DisplayedRoom',
            'in_new_room': 'RoomChange.InNewRoom',
            'new_room_was': 'RoomChange.NewRoomWas',
            'new_room_pos': 'RoomChange.Pos',
            'new_room_x': 'RoomChange.X',
            'new_room_y': 'RoomChange.Y',
            'new_room_loop': 'RoomChange.Loop',
            'new_room_placeonwalkable': 'RoomChange.PlaceOnWalkable',
            'frames_per_second': 'FramesPerSecond',
            'game_paused': 'Paused',
            'loaded_game_file_version': 'LoadedDataVersion',
            'scrObj': 'ScObjects',
            'scrHotspot': 'ScHotspots',
            'scrRegion': 'ScRegions',
            'scrWalkarea': 'ScWalkableAreas',
            'scrWalkbehind': 'ScWalkBehinds',
            'scrInv': 'ScInvItems',
            # batch world: the dialog table
            'dialog': 'Dialogs',
            'scrDialog': 'ScDialogs',
            'char_speaking': 'Speech.CharSpeaking',
            'char_speaking_anim': 'Speech.CharSpeakingAnim',
            'said_speech_line': 'Speech.SaidSpeechLine',
            'said_text': 'Speech.SaidText',
            'face_talking': 'Speech.FaceTalking',
            'facetalkview': 'Speech.FaceTalkView',
            'facetalkwait': 'Speech.FaceTalkWait',
            'facetalkframe': 'Speech.FaceTalkFrame',
            'facetalkloop': 'Speech.FaceTalkLoop',
            'facetalkrepeat': 'Speech.FaceTalkRepeat',
            'facetalkAllowBlink': 'Speech.FaceTalkAllowBlink',
            'facetalkBlinkLoop': 'Speech.FaceTalkBlinkLoop',
            'facetalkchar': 'Speech.FaceTalkChar',
            'facetalk_qfg4_override_placement_x': 'Speech.Qfg4OverridePlacementX',
            'facetalk_qfg4_override_placement_y': 'Speech.Qfg4OverridePlacementY',
            'loops_per_character': 'LipSync.LoopsPerCharacter',
            'splipsync': 'LipSync.Lines',
            'numLipLines': 'LipSync.NumLines',
            'curLipLine': 'LipSync.CurLine',
            'curLipLinePhoneme': 'LipSync.CurLinePhoneme',
            'longestline': 'TextLayout.LongestLine',
            'source_text_length': 'TextLayout.SourceTextLength',
            # batch ui
            'guis': 'Guis',
            'guibuts': 'GuiButtons',
            'guiinv': 'GuiInvWindows',
            'guilabels': 'GuiLabels',
            'guilist': 'GuiListBoxes',
            'guislider': 'GuiSliders',
            'guitext': 'GuiTextBoxes',
            'scrGui': 'ScGuis',
            'ifacepopped': 'IfacePopped',
            'mouse_on_iface': 'MouseOnIface',
            'scmouse': 'ScMouse',
            'cursor_gstate': 'Cursor.GState',
            'cur_mode': 'Cursor.Mode',
            'cur_cursor': 'Cursor.Look',
            'mouse_hotx': 'Cursor.HotX',
            'mouse_hoty': 'Cursor.HotY',
            'mouse_frame': 'Cursor.Frame',
            'mouse_delay': 'Cursor.Delay',
            'lastmx': 'Cursor.LastX',
            'lastmy': 'Cursor.LastY',
            'topwindowhandle': 'BuiltinDialogs.TopWindowHandle',
            'cbuttfont': 'BuiltinDialogs.ButtonFont',
            'acdialog_font': 'BuiltinDialogs.Font',
            'smcode': 'BuiltinDialogs.PostedMessage',
            'win_x': 'BuiltinDialogs.WinX',
            'win_y': 'BuiltinDialogs.WinY',
            # batch script
            'gamescript': 'Scripts.Global',
            'dialogScriptsScript': 'Scripts.Dialog',
            'scriptModules': 'Scripts.Modules',
            'moduleRepExecAddr': 'Scripts.ModuleRepExecAddr',
            'numScriptModules': 'Scripts.NumModules',
            'repExecAlways': 'Scripts.RepExecAlways',
            'lateRepExecAlways': 'Scripts.LateRepExecAlways',
            'getDialogOptionsDimensionsFunc': 'Scripts.GetDialogOptionsDimensions',
            'renderDialogOptionsFunc': 'Scripts.RenderDialogOptions',
            'getDialogOptionUnderCursorFunc': 'Scripts.GetDialogOptionUnderCursor',
            'runDialogOptionMouseClickHandlerFunc': 'Scripts.RunDialogOptionMouseClick',
            'runDialogOptionKeyPressHandlerFunc': 'Scripts.RunDialogOptionKeyPress',
            'runDialogOptionTextInputHandlerFunc': 'Scripts.RunDialogOptionTextInput',
            'runDialogOptionRepExecFunc': 'Scripts.RunDialogOptionRepExec',
            'runDialogOptionCloseFunc': 'Scripts.RunDialogOptionClose',
            'scsystem': 'ScSystem',
            'StaticCharacterArray': 'Handles.Characters',
            'StaticObjectArray': 'Handles.Objects',
            'StaticGUIArray': 'Handles.Guis',
            'StaticHotspotArray': 'Handles.Hotspots',
            'StaticRegionArray': 'Handles.Regions',
            'StaticWalkareaArray': 'Handles.WalkableAreas',
            'StaticWalkbehindArray': 'Handles.WalkBehinds',
            'StaticInventoryArray': 'Handles.InvItems',
            'StaticDialogArray': 'Handles.Dialogs',
            'StaticGUIControlsHandles': 'Handles.GuiControls',
            'events': 'Events.Queue',
            'eventClaimed': 'Events.Claimed',
            'in_enters_screen': 'Events.InEntersScreen',
            'done_as_error': 'Events.DoneAsError',
            'in_leaves_screen': 'Events.InLeavesScreen',
            'in_room_transition': 'Events.InRoomTransition',
            'scrAudioChannel': 'ScAudioChannels',
            'palette': 'Palette',
            'maincoltable': 'MainColTable',
            'dynamicallyCreatedSurfaces': 'DynamicSurfaces',
            'saved_viewport_bitmap': 'SavedViewportBitmap',
        },
    },
    # 5.1 step 3: the loaded room (`class Room : public AGS::Common::RoomStruct`)
    'room': {
        'type': 'Room',
        'accessor': 'Room::Current()',
        'local': 'room',
        'header': 'game/world/room_context.h',
        'own_files': ('Engine/game/world/room_context.h',),
        'names': {
            'thisroom': '',
            'croom': 'State',
            'troom': 'TempState',
            'objs': 'Objs',
            'roomscript': 'Script',
            'bg_just_changed': 'BgJustChanged',
            'walkareabackup': 'WalkAreaBackup',
            'walkable_areas_temp': 'WalkableAreasTemp',
            'noWalkBehindsAtAll': 'NoWalkBehindsAtAll',
            'walkBehindsCachedForBgNum': 'WalkBehindsCachedForBgNum',
            'walk_behind_baselines_changed': 'WalkBehindBaselinesChanged',
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
        self.by_ref = by_ref  # a lambda that captures by reference by default, or captures the local by reference
        self.name = None  # a class's name


# A use in a function body: the token index of the name (of the accessor's first token for an existing direct
# form), whether it may go through the local (lambda rule), whether it is written `::name`, whether it is an
# existing direct form `X::Current()`, and the text span the replacement covers.
Use = collections.namedtuple('Use', 'i allowed global_q existing start end')


class Function:
    def __init__(self, name, open_tok, sig):
        self.name = name
        self.open_tok = open_tok  # index of the body's `{`
        self.sig = sig  # token indexes of the declaration before the body
        self.uses = []  # Use
        self.shadowed = set()  # mapped names declared as variables or parameters here
        self.names = set()  # every identifier named in it (signature and body)
        self.anomaly = None
        self.branch = ()  # the #if branches the body's `{` is in
        self.branch_of = {}  # use's token index -> the #if branches it is in
        self.in_assert = set()  # uses inside assert(), which a release build does not compile
        self.cls = None  # the class it is a member function of, if known
        self.ctx_param = False  # a parameter `X &local` or `const X &local`: the context itself
        self.ctx_decls = []  # (token index, at the body's top level as a statement) variables `X &local = ...`
        self.local_declared = False  # a parameter or variable named like the local (not a mapped name)
        self.local_named = False  # the local's name is named (not as a member, not qualified)


class Analysis:
    def __init__(self):
        self.functions = []
        self.scope_uses = []  # (token index, why) uses outside any function body
        self.scope_decls = []  # (token index, scope kind) declarations of a mapped name outside function bodies
        self.anomalies = []  # text
        self.class_members = collections.defaultdict(set)  # class name -> mapped names declared as its members
        self.ctx_params = []  # token indexes of `X &local` outside bodies (parameters, until the body is met)
        self.local_decls = []  # token indexes of other declarations of the local's name outside bodies


def accessor_tokens(ctx):
    return [t.text for t in tokenize(ctx['accessor'])]


def context_typed(toks, i, ctx_type):
    """Whether the declaration of the identifier at i has the type `X &` or `const X &`, X the context's type."""
    return i >= 2 and toks[i - 1].text == '&' and toks[i - 2].kind == 'id' and toks[i - 2].text == ctx_type


def lambda_allows(scopes):
    """Whether a use in the innermost of scopes may go through the function's local: every lambda between it and
    the body captures by reference by default or captures the local by reference, and no local class."""
    for s in reversed(scopes):
        if s.kind == 'body':
            break
        if (s.kind == 'lambda' and not s.by_ref) or s.kind == 'localclass':
            return False
    return True


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


def capture_by_ref(toks, open_bracket, close_bracket, local=None):
    """Whether a lambda's capture is by reference by default (`[&]`, `[&, x]`) or names `&local`."""
    inner = [toks[i].text for i in range(open_bracket + 1, close_bracket)]
    if bool(inner) and inner[0] == '&' and (len(inner) == 1 or inner[1] == ','):
        return True
    items, item = [], []
    for x in inner + [',']:
        if x == ',':
            items.append(item)
            item = []
        else:
            item.append(x)
    return local is not None and ['&', local] in items


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

    if prev.text == '>':
        # `std::vector<T> name`, not a comparison (`option > name[i]`, `static_cast<int>(x) > name`): the `>`
        # must close a `<` without a parenthesis, a statement or an operator of an expression between them
        depth = 1
        for j in range(i - 2, -1, -1):
            t = toks[j].text
            if t == '>':
                depth += 1
            elif t == '<':
                depth -= 1
                if depth == 0:
                    return True
            elif t in ('(', ')', ';', '{', '}', '&&', '||', '=', '?', 'return'):
                return False
        return False
    if typeish(prev):
        if nxt == '(' and prev.text in ('const',):
            return False
        return True
    if prev.text in ('*', '&') and i >= 2:
        pp = toks[i - 2]
        return typeish(pp) or pp.text in ('*', 'const')
    return False


def declares_first(toks, stmt):
    """Whether the statement made of these token indexes begins with a declaration (`int a = 0, b...`): its first
    declarator, the first identifier before `=`, `,`, `[` or `;`, is declared there."""
    for k in stmt:
        t = toks[k]
        if t.text == ',':
            return False
        if t.kind == 'id' and k + 1 < len(toks) and toks[k + 1].text in ('=', ',', '[', ';'):
            return is_declaration(toks, k)
    return False


def skip_pp(toks, j):
    while j < len(toks) and toks[j].kind == 'pp':
        j += 1
    return j


def closer_of(toks, j):
    """The index of the `)`, `]` or `}` that closes the bracket at j (preprocessor lines are not followed)."""
    depth = 0
    for k in range(j, len(toks)):
        t = toks[k]
        if t.kind != 'punct':
            continue
        if t.text in '([{':
            depth += 1
        elif t.text in ')]}':
            depth -= 1
            if depth == 0:
                return k
    return len(toks) - 1


def statement_end(toks, j):
    """The index of the last token of the statement that starts at j: a block's `}`, a plain statement's `;`, or the
    end of the last branch of an if/else, a loop's body, a do-while's `;`."""
    j = skip_pp(toks, j)
    if j >= len(toks):
        return len(toks) - 1
    t = toks[j]
    if t.text == '{':
        return closer_of(toks, j)
    if t.kind == 'id' and t.text in ('for', 'while', 'switch', 'if'):
        p = skip_pp(toks, j + 1)
        if t.text == 'if' and p < len(toks) and toks[p].text == 'constexpr':
            p = skip_pp(toks, p + 1)
        if p >= len(toks) or toks[p].text != '(':
            return p
        e = statement_end(toks, closer_of(toks, p) + 1)
        if t.text == 'if':
            n = skip_pp(toks, e + 1)
            if n < len(toks) and toks[n].text == 'else':
                return statement_end(toks, n + 1)
        return e
    if t.kind == 'id' and t.text == 'do':
        e = statement_end(toks, j + 1)
        w = skip_pp(toks, e + 1)
        if w < len(toks) and toks[w].text == 'while':
            p = skip_pp(toks, w + 1)
            if p < len(toks) and toks[p].text == '(':
                return skip_pp(toks, closer_of(toks, p) + 1)
        return e
    depth = 0
    for k in range(j, len(toks)):
        x = toks[k]
        if x.kind != 'punct':
            continue
        if x.text in '([{':
            depth += 1
        elif x.text in ')]}':
            if depth == 0:
                return k - 1  # the enclosing block ends first
            depth -= 1
        elif x.text == ';' and depth == 0:
            return k
    return len(toks) - 1


def loop_spans(toks):
    """(index of the keyword, index of the last token) of every `for`, range-`for`, `while` and `do ... while`
    statement: the header and the body. The `while` that ends a do-while is a span of its own, inside the do's."""
    out = []
    for i, t in enumerate(toks):
        if t.kind != 'id' or t.text not in ('for', 'while', 'do'):
            continue
        if i > 0 and toks[i - 1].text in ('.', '->', '::'):
            continue
        if t.text == 'do':
            out.append((i, statement_end(toks, i)))
            continue
        p = skip_pp(toks, i + 1)
        if p < len(toks) and toks[p].text == '(':
            out.append((i, statement_end(toks, closer_of(toks, p) + 1)))
    return out


def analyse(text, ctx):
    """The functions and the uses of the mapped names in a file, the existing direct forms of the accessor, and
    the declarations and namings of the local's name."""
    names, local, ctx_type = ctx['names'], ctx['local'], ctx['type']
    acc = accessor_tokens(ctx)
    decl_head = [ctx_type, '&', local, '=']
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
                    scopes.append(Scope('lambda', i, func, capture_by_ref(toks, cap[0], cap[1], local)))
                    # a name in the capture list is not a use (`[&game]` captures the local)
                    func.uses = [u for u in func.uses if not cap[0] < u.i < cap[1]]
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
                # a parameter of the local's name: the context itself (`Game &game`) or something else
                for k in [k for k in an.ctx_params if k in in_sig]:
                    f.ctx_param = True
                    an.ctx_params.remove(k)
                for k in [k for k in an.local_decls if k in in_sig]:
                    f.local_declared = True
                    an.local_decls.remove(k)
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
        if t.text not in names and t.text != local and t.text != acc[0]:
            continue
        # what the name is here
        prev = toks[i - 1] if i > 0 else None
        nxt = toks[i + 1] if i + 1 < len(toks) else None
        if prev is not None and prev.text in ('.', '->', '.*', '->*'):
            continue
        # the accessor: an existing direct form (an earlier run's), or the initializer of the local's declaration
        if t.text == acc[0] and [x.text for x in toks[i:i + len(acc)]] == acc:
            if func is not None and [x.text for x in toks[max(i - 4, 0):i]] != decl_head:
                start = i
                while start >= 1 and toks[start - 1].text == '::':
                    if start >= 2 and toks[start - 2].kind == 'id':
                        start -= 2
                    else:
                        start -= 1
                        break
                func.uses.append(Use(i, lambda_allows(scopes), False, True, toks[start].start,
                                     toks[i + len(acc) - 1].end))
                func.branch_of[i] = tuple(branch)
                if any(k > 0 and toks[k - 1].text == 'assert' for k in parens):
                    func.in_assert.add(i)
            continue
        global_q = False
        if prev is not None and prev.text == '::':
            before = toks[i - 2] if i >= 2 else None
            if before is not None and (before.kind == 'id' or before.text == '>'):
                continue
            global_q = True
        if nxt is not None and nxt.text == '::':
            continue
        # the local's name: a declaration of the context (`Game &game`), another declaration, or a naming
        if t.text == local and not global_q:
            if is_declaration(toks, i):
                if context_typed(toks, i, ctx_type):
                    if func is not None:
                        head = [toks[k].text for k in stmt[:-1]]
                        func.ctx_decls.append((i, inner.kind == 'body' and head in ([ctx_type, '&'],
                                                                                    ['const', ctx_type, '&'])))
                    else:
                        an.ctx_params.append(i)
                    continue
                if local not in names:
                    if func is not None:
                        func.local_declared = True
                    else:
                        an.local_decls.append(i)
                        if inner is not None and inner.kind == 'class' and inner.name:
                            an.class_members[inner.name].add(local)
                    continue
            elif local not in names:
                if func is not None:
                    func.local_named = True
                continue
        if t.text not in names:
            continue
        # a later declarator of `T a = 1, name;` (inside a function, only if the statement begins with a
        # declaration: `a = 1, name = 2;` is a comma expression)
        later_declarator = prev is not None and prev.text == ',' and nxt is not None and \
            nxt.text in (';', '=', ',', '[') and not any(toks[k].text in ('(', '{') for k in stmt) and \
            (func is None or declares_first(toks, stmt))
        if (is_declaration(toks, i) or later_declarator) and not global_q:
            if func is not None:
                func.shadowed.add(t.text)
            else:
                # inside parentheses: a parameter (of a prototype, or of the definition whose body follows)
                an.scope_decls.append((i, 'param' if parens else inner.kind if inner else 'file'))
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
        func.uses.append(Use(i, lambda_allows(scopes), global_q, False, toks[i - 1].start if global_q else t.start,
                             t.end))
        func.branch_of[i] = tuple(branch)
        if any(k > 0 and toks[k - 1].text == 'assert' for k in parens):
            func.in_assert.add(i)
    if state['scopes']:
        an.anomalies.append('end of file inside %d open braces' % len(state['scopes']))
    an.toks = toks
    an.loops = loop_spans(toks)
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
        self.taken = []  # (file, line, function, why, uses): the local's name is taken, so the direct form
        self.extended = []  # (file, line, function, uses): the body already had the local, new uses go through it
        self.rewritten = []  # (file, line, function, n): existing direct forms rewritten to the local
        self.loop_locals = []  # (file, line, function, line of the use): the local only because of a use in a loop
        self.loop_taken = []  # (file, line, function, why, line of the use): a use in a loop, the local's name taken
        self.changed = {}


def class_members(repo, files, ctx):
    """Class name -> the mapped names (and the local's name) that are members of it, from the class definitions
    in the files (a member function of such a class means its member by the bare name; base classes are not
    followed)."""
    out = collections.defaultdict(set)
    for f in files:
        for cls, ms in analyse(read(repo, f), ctx).class_members.items():
            out[cls] |= ms
    return out


def member_shadows(an, f, members):
    return (members or {}).get(f.cls, set()) | an.class_members.get(f.cls, set()) if f.cls else set()


def body_context(f):
    """(token index of the local's declaration at the top level of the body or None, whether a declaration of the
    context's type with the local's name is somewhere else in the body: an inner block, a lambda's parameters)."""
    top = [k for k, at_top in f.ctx_decls if at_top]
    return (top[0] if top else None), any(not at_top for _, at_top in f.ctx_decls)


def taken_reason(f, local, names, member_shadowed):
    """Why the local's name is taken in the function (so no local can be declared), or None."""
    if local in f.shadowed or f.local_declared:
        return 'declares a variable or parameter named `%s`' % local
    if local in member_shadowed:
        return 'a member of %s is named `%s`' % (f.cls, local)
    if local not in names and f.local_named:
        return 'names `%s`' % local
    return None


def context_order(registry, ctx):
    return next((k for k, c in enumerate(registry.values()) if c['type'] == ctx['type']), len(registry))


def leading_context_decls(toks, open_tok, registry):
    """The declarations of context locals that open a body, in order: [(order in the registry, index of the
    local's token, index of the `;`)]."""
    pats = [(k, [c['type'], '&', c['local'], '='] + accessor_tokens(c) + [';'])
            for k, c in enumerate(registry.values())]
    out = []
    j = open_tok + 1
    while True:
        m = next(((k, p) for k, p in pats if [t.text for t in toks[j:j + len(p)]] == p), None)
        if m is None:
            return out
        out.append((m[0], j + 2, j + len(m[1]) - 1))
        j += len(m[1])


def rewrite(path, text, ctx, rep, verbose, members=None, registry=None):
    registry = CONTEXTS if registry is None else registry
    names = ctx['names']
    local = ctx['local']
    an = analyse(text, ctx)
    toks = an.toks
    for f in an.functions:
        ms = member_shadows(an, f, members)
        if ms & {toks[u.i].text for u in f.uses if not u.existing and not u.global_q}:
            rep.manual.append((path, line_of(text, toks[f.open_tok].start), f.name,
                               'a member of %s has the name %s: not converted there' % (f.cls, ', '.join(sorted(ms)))))
        f.member_shadowed = ms
    edits = []  # (start, end, replacement)
    for a in an.anomalies:
        rep.anomalies.append('%s:%s' % (path, a))
    for i, kind in an.scope_decls:
        rep.scope_decls.append((path, line_of(text, toks[i].start), toks[i].text, kind))

    def replacement(name, through_local):
        base = local if through_local else ctx['accessor']
        return '%s.%s' % (base, names[name]) if names[name] else base

    for i, why, global_q in an.scope_uses:
        t = toks[i]
        start = toks[i - 1].start if global_q else t.start
        edits.append((start, t.end, replacement(t.text, False)))
        rep.scope_uses.append((path, line_of(text, t.start), t.text, why))
        rep.per_name[t.text]['direct'] += 1
        rep.files_per_name[t.text].add(path)

    order = context_order(registry, ctx)
    for f in an.functions:
        if not f.uses and not f.shadowed:
            continue
        line = line_of(text, toks[f.open_tok].start)
        hidden = f.shadowed | f.member_shadowed
        # a name declared here or a member is not the global, unless written `::name`
        uses = [u for u in f.uses if u.existing or u.global_q or toks[u.i].text not in hidden]
        if f.shadowed:
            rep.manual.append((path, line, f.name, 'declares a variable or parameter named %s: those names are not '
                               'converted in this function' % ', '.join(sorted(f.shadowed))))
        if not uses:
            continue
        if f.anomaly:
            rep.manual.append((path, line, f.name, f.anomaly + ': left alone'))
            continue
        decl_at, inner_decl = body_context(f)
        if inner_decl:
            rep.manual.append((path, line, f.name, 'declares `%s &%s` in an inner block or a lambda\'s parameters: '
                               'left alone' % (ctx['type'], local)))
            continue
        has_local = f.ctx_param or decl_at is not None
        leading = leading_context_decls(toks, f.open_tok, registry)
        taken = None if has_local else taken_reason(f, local, names, f.member_shadowed)
        through = [u for u in uses if u.allowed]
        # a use inside a loop of this function (header or body) would reload the context on every pass
        looped = [u for u in through if any(f.open_tok < s < u.i <= e for s, e in an.loops)]
        add_decl = False
        if has_local:
            use_local = True
            if decl_at is not None and decl_at not in [d[1] for d in leading]:
                above = [u for u in uses if u.i < decl_at]
                rep.manual.append((path, line, f.name, 'the declaration of `%s` is not the first statement: %d use(s) '
                                   'above it take the direct form' % (local, len(above))))
        elif taken:
            use_local = False
            if looped:
                rep.loop_taken.append((path, line, f.name, taken, line_of(text, toks[looped[0].i].start)))
        else:
            use_local = len(through) >= 2 or bool(looped)
            # the local must be used in every build: by a use outside the #if blocks of the body and outside
            # assert()
            if use_local and not any(f.branch_of[u.i] == f.branch and u.i not in f.in_assert for u in through):
                use_local = False
                rep.conditional.append((path, line, f.name, len(through)))
            add_decl = use_local
        before = len(edits)
        converted = 0  # uses of the globals converted by this run
        rewritten = 0  # existing direct forms rewritten to the local
        notes = []
        for u in uses:
            via = use_local and u.allowed and (decl_at is None or u.i > decl_at)
            if u.existing:
                if via:
                    edits.append((u.start, u.end, local))
                    rewritten += 1
                    notes.append((u.i, 'direct form -> local'))
                continue
            name = toks[u.i].text
            new = replacement(name, via)
            if text[u.start:u.end] != new:
                edits.append((u.start, u.end, new))
            # `game.x` after the body's own `Game &game` is the local already
            if has_local and via and name == local and not u.global_q:
                continue
            converted += 1
            rep.per_name[name]['local' if via else 'direct'] += 1
            rep.files_per_name[name].add(path)
            notes.append((u.i, name + ('' if via or not use_local else ' (direct: in a lambda or local class, or '
                                                                         'above the declaration)') +
                          (' (in a loop)' if u in looped else '')))
        if add_decl:
            # after the locals of the contexts before this one that open the body, before the later ones'
            at = f.open_tok
            for k, _, semi in leading:
                if k >= order:
                    break
                at = semi
            brace = toks[f.open_tok]
            ls = text.rfind('\n', 0, brace.start) + 1
            indent = re.match(r'[ \t]*', text[ls:]).group(0) + '    '
            eol = '\r\n' if '\r\n' in text else '\n'
            edits.append((toks[at].end, toks[at].end, '%s%s%s &%s = %s;' % (eol, indent, ctx['type'], local,
                                                                             ctx['accessor'])))
            rep.locals.append((path, line, f.name, len(through)))
            if len(through) < 2:
                rep.loop_locals.append((path, line, f.name, line_of(text, toks[looped[0].i].start)))
        elif has_local and converted:
            rep.extended.append((path, line, f.name, converted))
        elif not use_local and converted:
            rep.direct_funcs.append((path, line, f.name, converted))
        if taken and len(through) >= 2 and converted:
            rep.taken.append((path, line, f.name, taken, len(uses)))
        if rewritten:
            rep.rewritten.append((path, line, f.name, rewritten))
        if verbose and (len(edits) > before or converted):
            how = 'local `%s`%s' % (local, ' (declared)' if add_decl else ' (there already)') if use_local else \
                'direct' + (' (the local\'s name is taken: %s)' % taken if taken else '')
            print('%s:%d %s: %d use(s), %s' % (path, line, f.name, len(uses), how))
            for i, note in notes:
                print('    %d: %s' % (line_of(text, toks[i].start), note))
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
    """Every remaining use or namespace-scope declaration of a mapped name. A use of a mapped name that is the
    local's name (`game`) is the local, not left, after the body's declaration of it or in a function that has
    a parameter of the context's type with that name."""
    an = analyse(text, ctx)
    toks = an.toks
    local = ctx['local']
    found = []
    for f in an.functions:
        hidden = f.shadowed | member_shadows(an, f, members)
        decl_at, _ = body_context(f)
        for u in f.uses:
            name = toks[u.i].text
            if u.existing or (name in hidden and not u.global_q):
                continue
            if name == local and not u.global_q and u.allowed and \
                    (f.ctx_param or (decl_at is not None and u.i > decl_at)):
                continue
            found.append((u.i, 'use in %s' % f.name))
    for i, why, _ in an.scope_uses:
        found.append((i, 'use outside a function (%s)' % why))
    for i, kind in an.scope_decls:
        if kind in ('file', 'ns'):
            found.append((i, 'declared at namespace scope'))
    return [(path, line_of(text, toks[i].start), toks[i].text, why) for i, why in found]


def engine_files(repo, ctx):
    """The engine's sources and headers; in a directory that is not a git checkout (a copy of the tree to try
    the tool on), every such file under Engine/."""
    git = subprocess.run(['git', '-C', repo, 'rev-parse', '--show-toplevel'], capture_output=True, text=True)
    if git.returncode == 0 and os.path.samefile(git.stdout.strip(), repo):
        out = subprocess.run(['git', '-C', repo, 'ls-files', '--cached', '--others', '--exclude-standard', '--',
                              'Engine'], capture_output=True, text=True, check=True).stdout.split('\n')
    else:
        out = [os.path.relpath(os.path.join(d, n), repo).replace(os.sep, '/')
               for d, _, ns in os.walk(os.path.join(repo, 'Engine')) for n in ns]
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
    # a function that declares a variable of the local's name: the direct form (rule "the local's name is taken")
    ctx['local'] = 'services'
    out, rep = run('void q() { int services = 0; g = h; }\n')
    assert 'void q() { int services = 0; S::Current().G = S::Current().H; }' in out, out
    assert rep.taken and 'declares' in rep.taken[0][3], rep.taken

    # a later declarator of a declaration inside a function is a local (`int a = 0, b = 1, g = 2;` in
    # DialogOptions::Begin), the same names in a comma expression are the globals
    ctx['local'] = 's'
    out, rep = run('void d() { int a = 0, b = 1, g = 2; use(g); }\nvoid e(int a) { a = 1, g = 2; }\n')
    assert 'void d() { int a = 0, b = 1, g = 2; use(g); }' in out, out
    assert 'void e(int a) { a = 1, S::Current().G = 2; }' in out, out

    self_test_contexts()
    print('self-test passed')
    return 0


def self_test_contexts():
    """The cases of 5.1 step 3: several contexts, a global that is the context, the local's name taken, runs in
    several batches."""
    game, room, services = CONTEXTS['game'], CONTEXTS['room'], CONTEXTS['services']

    def run(src, ctx, members=None):
        if '#include' not in src:
            src = '#include "a.h"\n' + src  # where the context's include goes
        rep = Report()
        out = rewrite('t.cpp', src, ctx, rep, False, members)
        return (src if out is None else out), rep

    def runs(src, *ctxs):
        for c in ctxs:
            src = run(src, c)[0]
        return src

    def body(out, name):
        """The text of a function from its name to the next function or the end."""
        s = out.index(name)
        e = out.find('\nvoid ', s + 1)
        return out[s:] if e == -1 else out[s:e]

    # 1. two contexts, two locals, in the order services, game, room whatever the order of the runs
    src = '#include "a.h"\nvoid f()\n{\n    usetup.x = play.y;\n    play.z = usetup.w;\n    thisroom.a = croom->b;\n}\n'
    want = ('void f()\n{\n    EngineServices &services = EngineServices::Current();\n    Game &game = Game::Current();\n'
            '    Room &room = Room::Current();\n    services.Config.x = game.Play.y;\n    game.Play.z = services.Config.w;\n'
            '    room.a = room.State->b;\n}\n')
    for order in ((services, game, room), (room, game, services), (game, room, services), (room, services, game)):
        out = runs(src, *order)
        assert body(out, 'void f()') == want, (out, [c['type'] for c in order])
        assert f'#include "{game["header"]}"' in out and f'#include "{room["header"]}"' in out, out

    # 2. a global that is the context itself
    src = ('void a() { foo(thisroom); }\nvoid b()\n{\n    int w = thisroom.Width;\n    bar(&thisroom);\n}\n'
           'void c() { x = game.chars[0]; }\nvoid d()\n{\n    x = game.chars[0];\n    y = game.numviews;\n}\n')
    out = run(src, room)[0]
    assert 'void a() { foo(Room::Current()); }' in out, out
    assert 'void b()\n{\n    Room &room = Room::Current();\n    int w = room.Width;\n    bar(&room);\n}' in out, out
    out = run(src, game)[0]
    assert 'void c() { x = Game::Current().chars[0]; }' in out, out
    assert 'void d()\n{\n    Game &game = Game::Current();\n    x = game.chars[0];\n    y = game.numviews;\n}' in out, out

    # 3. the local's name taken: a parameter `int room`, a definition passed as `game`, a member, a naming
    src = ('void e(int room)\n{\n    thisroom.Width = room;\n    thisroom.Height = 1;\n}\n'
           'void m(RoomState *rs)\n{\n    foo(x.room, A::room);\n    thisroom.a = 1;\n    croom->b = 2;\n}\n'
           'void n()\n{\n    for (auto &room : rooms)\n        thisroom.a = room;\n    croom->b = 2;\n}\n')
    out, rep = run(src, room)
    assert 'void e(int room)\n{\n    Room::Current().Width = room;\n    Room::Current().Height = 1;\n}' in out, out
    assert 'void m(RoomState *rs)\n{\n    Room &room = Room::Current();\n    foo(x.room, A::room);\n    room.a' in out, out
    assert 'Room::Current().a = room;\n    Room::Current().State->b = 2;' in out, out
    assert [t[2] for t in rep.taken] == ['e', 'n'], rep.taken
    src = ('void s(const GameDef &game)\n{\n    play.a = game.x;\n    play.b = 1;\n}\n'
           'void s2(GameDef &game) { ::game.x = game.y; ::game.z = 1; }\n'
           'void p(Game &game)\n{\n    play.a = 1;\n    game.b = 2;\n}\n')
    out, rep = run(src, game)
    assert 'void s(const GameDef &game)\n{\n    Game::Current().Play.a = game.x;\n    Game::Current().Play.b = 1;' in out, out
    assert 'void s2(GameDef &game) { Game::Current().x = game.y; Game::Current().z = 1; }' in out, out
    assert 'void p(Game &game)\n{\n    game.Play.a = 1;\n    game.b = 2;\n}' in out, out
    assert [t[2] for t in rep.taken] == ['s', 's2'], rep.taken
    # a member function of a class that has a member `game` or `play`
    src = ('struct K\n{\n    GameDef *game;\n    void m();\n};\nvoid K::m()\n{\n    play.a = 1;\n    play.b = 2;\n}\n'
           'struct P\n{\n    int play;\n    void n();\n};\nvoid P::n()\n{\n    play = game.x;\n    game.y = play;\n}\n')
    out, rep = run(src, game)
    assert 'void K::m()\n{\n    Game::Current().Play.a = 1;\n    Game::Current().Play.b = 2;\n}' in out, out
    assert 'void P::n()\n{\n    Game &game = Game::Current();\n    play = game.x;\n    game.y = play;\n}' in out, out
    assert [t[2] for t in rep.taken] == ['K::m'], rep.taken

    # 4. incremental runs: `play` first, then the whole table, equals one run over the whole table
    play_only = dict(game, names={'play': 'Play'})
    src = ('#include "a.h"\nvoid i1()\n{\n    play.a = game.b;\n    play.c = 2;\n}\nvoid i2() { play.a = 1; }\n'
           'void i3()\n{\n    play.a = 1;\n    x = game.y;\n}\nvoid i4()\n{\n    play.a = 1;\n    play.b = 2;\n}\n'
           'void i5()\n{\n#if A\n    play.a = 1;\n#endif\n    x = game.y;\n}\n'
           'void i6()\n{\n    auto f = [=]() { play.a = 1; };\n    x = game.y;\n    z = game.w;\n}\n')
    once = run(src, game)[0]
    first = run(src, play_only)[0]
    assert 'Game::Current().Play.a = game.b;' in first and 'game.Play.a = 1;\n    game.Play.b = 2;' in first, first
    twice, rep = run(first, game)
    assert twice == once, (once, twice)
    assert sum(r[3] for r in rep.rewritten) == 4, rep.rewritten
    assert run(once, game)[0] == once, 'not idempotent'
    assert 'void i5()\n{\n    Game &game = Game::Current();\n#if A\n    game.Play.a = 1;' in once, once
    assert '[=]() { Game::Current().Play.a = 1; }' in once, once
    # the same code path for services
    src = 'void sv()\n{\n    EngineServices::Current().Config.a = 1;\n    b = EngineServices::Current().Platform;\n}\n'
    out = run(src, services)[0]
    assert body(out, 'void sv()') == ('void sv()\n{\n    EngineServices &services = EngineServices::Current();\n'
                   '    services.Config.a = 1;\n    b = services.Platform;\n}\n'), out
    assert run(out, services)[0] == out

    # lambdas in a function that has the declaration; a declaration that is not the first statement
    src = ('void l()\n{\n    Game &game = Game::Current();\n    auto a = [&]() { play.x = 1; };\n'
           '    auto b = [=]() { play.y = 2; };\n    auto c = [&game]() { play.z = game.q; };\n    game.w = 0;\n}\n'
           'void nf()\n{\n    int a = play.x;\n    Game &game = Game::Current();\n    play.y = a;\n}\n')
    out, rep = run(src, game)
    assert '[&]() { game.Play.x = 1; }' in out and '[=]() { Game::Current().Play.y = 2; }' in out, out
    assert '[&game]() { game.Play.z = game.q; }' in out and '    game.w = 0;' in out, out
    assert 'int a = Game::Current().Play.x;\n    Game &game = Game::Current();\n    game.Play.y = a;' in out, out
    assert any('not the first statement' in m[3] for m in rep.manual), rep.manual

    # 5. the uses only in different #if branches: the direct form; a member path
    src = 'void c2()\n{\n#if A\n    play.a = 1;\n#else\n    play.b = 2;\n#endif\n}\nvoid cm() { cur_mode = 1; }\n'
    out, rep = run(src, dict(game, names={'play': 'Play', 'cur_mode': 'Cursor.Mode'}))
    assert '    Game::Current().Play.a = 1;\n#else\n    Game::Current().Play.b = 2;' in out, out
    assert 'void cm() { Game::Current().Cursor.Mode = 1; }' in out and rep.conditional, (out, rep.conditional)
    out = run('void cm2()\n{\n    cur_mode = 1;\n    x = cur_mode;\n}\n', dict(game, names={'cur_mode': 'Cursor.Mode'}))[0]
    assert 'game.Cursor.Mode = 1;\n    x = game.Cursor.Mode;' in out, out

    # 6. --check: `game` after the declaration or as a `Game &` parameter is the local, not left
    src = ('void ck1() { x = game.a; }\nvoid ck2()\n{\n    Game &game = Game::Current();\n    x = game.a;\n}\n'
           'void ck3(const GameDef &game) { x = game.a; }\nvoid ck4(Game &game) { x = game.a; }\n'
           'void ck5()\n{\n    Game &game = Game::Current();\n    x = ::game.a;\n}\n')
    left = check_file('t.cpp', src, game, Report())
    assert [l[1] for l in left] == [1, 12], left

    # 7. a use inside a loop gives the local even when it is the only use
    decl = '    Game &game = Game::Current();\n'
    src = ('void f1()\n{\n    for (int i = 0; i < game.numviews; i++)\n        x(i);\n}\n'
           'void f2()\n{\n    while (a)\n        play.b = 1;\n}\n'
           'void f3()\n{\n    do\n    {\n        play.b = 1;\n    } while (a);\n}\n'
           'void f4()\n{\n    for (auto &c : chars)\n    {\n        if (c)\n        {\n            play.b = 1;\n        }\n'
           '    }\n}\n'
           'void f5()\n{\n    for (int i = 0; i < 2; i++)\n        x(i);\n    play.b = 1;\n}\n'
           'void f6()\n{\n    auto l = [=]() { for (;;) play.b = 1; };\n}\n'
           'void f7(int game)\n{\n    while (a)\n        play.b = game;\n}\n'
           'void f8()\n{\n    for (auto &c : play.chars)\n        x(c);\n}\n'
           'void f9()\n{\n    if (a)\n        while (b) x();\n    else\n        y();\n    play.c = 1;\n}\n'
           'void f10()\n{\n    auto l = [&]() { while (a) play.b = 1; };\n}\n')
    out, rep = run(src, game)
    assert 'void f1()\n{\n' + decl + '    for (int i = 0; i < game.numviews; i++)' in out, out
    assert 'void f2()\n{\n' + decl + '    while (a)\n        game.Play.b = 1;' in out, out
    assert 'void f3()\n{\n' + decl + '    do\n    {\n        game.Play.b = 1;' in out, out
    assert 'void f4()\n{\n' + decl in out and '            game.Play.b = 1;' in out, out
    assert 'x(i);\n    Game::Current().Play.b = 1;' in out, out  # after the loop: still direct
    assert '[=]() { for (;;) Game::Current().Play.b = 1; }' in out, out  # a loop inside a [=] lambda
    assert 'void f7(int game)\n{\n    while (a)\n        Game::Current().Play.b = game;' in out, out
    assert [t[2] for t in rep.loop_taken] == ['f7'], rep.loop_taken
    assert 'void f8()\n{\n' + decl + '    for (auto &c : game.Play.chars)' in out, out  # the range expression
    assert '        y();\n    Game::Current().Play.c = 1;' in out, out  # after an if/else holding a loop
    assert 'void f10()\n{\n' + decl + '    auto l = [&]() { while (a) game.Play.b = 1; };' in out, out
    assert sorted(t[2] for t in rep.loop_locals) == ['f1', 'f10', 'f2', 'f3', 'f4', 'f8'], rep.loop_locals
    assert run(out, game)[0] == out, 'loops: not idempotent'
    # an existing direct form in a loop, in a function without the declaration (a run without the loop rule)
    src = 'void g1()\n{\n    for (int i = 0; i < Game::Current().numviews; i++)\n        x(i);\n}\n'
    out, rep = run(src, game)
    assert 'void g1()\n{\n' + decl + '    for (int i = 0; i < game.numviews; i++)' in out, out
    assert rep.rewritten and run(out, game)[0] == out, rep.rewritten
    # a single loop use under #if: direct, listed
    out, rep = run('void g2()\n{\n#if A\n    while (a)\n        play.b = 1;\n#endif\n}\n', game)
    assert 'Game::Current().Play.b = 1;' in out and rep.conditional, (out, rep.conditional)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--repo', default=REPO)
    ap.add_argument('--context', choices=sorted(CONTEXTS), default='services')
    ap.add_argument('--files', nargs='*', help='repository paths (default: every source and header of Engine/)')
    ap.add_argument('--dry-run', action='store_true', help='print the changes per function, write nothing')
    ap.add_argument('--check', action='store_true', help='exit 1 if a mapped global is still named')
    ap.add_argument('--names', nargs='+', help='convert only these globals of the context\'s table (a batch)')
    ap.add_argument('--self-test', action='store_true')
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    ctx = CONTEXTS[args.context]
    if args.names:
        unknown = [n for n in args.names if n not in ctx['names']]
        if unknown:
            ap.error('not in the table of %s: %s' % (args.context, ', '.join(unknown)))
        ctx = dict(ctx, names={n: m for n, m in ctx['names'].items() if n in args.names})
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
    print('functions given the local: %d; functions that had it and got new uses: %d; functions with the direct '
          'form: %d; files: %d' % (len(rep.locals), len(rep.extended), len(rep.direct_funcs), len(rep.changed)))
    print('direct forms rewritten to the local: %d in %d functions; decided by the rule "the local\'s name is '
          'taken": %d functions, %d uses' % (sum(r[3] for r in rep.rewritten), len(rep.rewritten), len(rep.taken),
                                             sum(t[4] for t in rep.taken)))
    if rep.taken:
        print('decided by the rule: the local\'s name is taken (direct form):')
        for f, line, fn, why, n in rep.taken:
            print('  %s:%d %s: %s, %d uses' % (f, line, fn, why, n))
    if rep.loop_locals:
        print('functions given the local because of a use inside a loop (one use otherwise): %d'
              % len(rep.loop_locals))
        for f, line, fn, uline in rep.loop_locals:
            print('  %s:%d %s: use at line %d' % (f, line, fn, uline))
    if rep.loop_taken:
        print('direct form inside a loop, the local\'s name is taken (decide by hand):')
        for f, line, fn, why, uline in rep.loop_taken:
            print('  %s:%d %s: %s, use at line %d' % (f, line, fn, why, uline))
    if rep.rewritten:
        print('direct forms rewritten to the local:')
        for f, line, fn, n in rep.rewritten:
            print('  %s:%d %s: %d' % (f, line, fn, n))
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
