#!/usr/bin/env python3
"""
Summarises the log files that ASan, LSan and UBSan wrote (ASAN_OPTIONS/UBSAN_OPTIONS log_path) as a
Markdown list of the distinct findings, most frequent first (docs/refactor/PLAN.md, 1.5).

    python Script/sanitizer_summary.py <directory with the logs> [--repo <checkout root>]

A finding is a UBSan "runtime error" line, or an ASan error or a direct leak named by the first frame
of its stack that is in this repository's own code. The same finding in many processes (ctest runs
every test case in its own) counts once, with the number of logs it is in. Exit code 0: no findings;
1: findings, listed on stdout.
"""
import argparse
import collections
import os
import re
import sys

RUNTIME_ERROR = re.compile(r'^(\S+?):(\d+):(?:\d+:)? runtime error: (.*)$')
ASAN_ERROR = re.compile(r'ERROR: AddressSanitizer: ([\w-]+)')
LEAK = re.compile(r'^Direct leak of \d+ byte')
FRAME = re.compile(r'^#\d+ 0x[0-9a-f]+ in (.+?) (\S+?):(\d+)(?::\d+)?$')
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude|_deps)/')


def findings_of(lines, root):
    """The distinct (sanitizer, text) findings of one log."""
    seen = set()
    pending = None  # (sanitizer, kind) of a report whose stack is being read
    fallback = None
    for raw in lines:
        line = raw.strip()
        m = RUNTIME_ERROR.match(line)
        if m:
            seen.add(('UBSan', '%s:%s: %s' % (m.group(1).replace(root, ''), m.group(2), m.group(3))))
            continue
        m = ASAN_ERROR.search(line)
        if m or LEAK.match(line):
            pending = ('ASan', m.group(1)) if m else ('LSan', 'leak')
            fallback = None
            continue
        if pending is None:
            continue
        m = FRAME.match(line)
        if m:
            where = '%s at %s:%s' % (m.group(1), m.group(2).replace(root, ''), m.group(3))
            fallback = fallback or where
            if m.group(2).startswith(root) and not THIRD_PARTY.search(m.group(2)[len(root):]):
                seen.add((pending[0], '%s: %s' % (pending[1], where)))
                pending = None
        elif not line:
            # the stack ended without a frame in our code
            seen.add((pending[0], '%s: %s' % (pending[1], fallback or 'no stack')))
            pending = None
    if pending is not None:
        seen.add((pending[0], '%s: %s' % (pending[1], fallback or 'no stack')))
    return seen


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('logs')
    ap.add_argument('--repo', default=os.getcwd(), help='checkout root, cut from the paths')
    args = ap.parse_args()

    root = args.repo.replace('\\', '/').rstrip('/') + '/'
    found = collections.Counter()
    names = sorted(os.listdir(args.logs)) if os.path.isdir(args.logs) else []
    for name in names:
        with open(os.path.join(args.logs, name), encoding='utf-8', errors='replace') as f:
            found.update(findings_of(f, root))

    if not found:
        print('No sanitizer findings (%d log files).' % len(names))
        return 0
    print('%d distinct sanitizer findings in %d log files:\n' % (len(found), len(names)))
    print('| Logs | Sanitizer | Finding |')
    print('|---:|---|---|')
    for (kind, text), count in sorted(found.items(), key=lambda kv: (-kv[1], kv[0])):
        print('| %d | %s | `%s` |' % (count, kind, text.replace('|', '/').replace('`', "'")))
    return 1


if __name__ == '__main__':
    sys.exit(main())
