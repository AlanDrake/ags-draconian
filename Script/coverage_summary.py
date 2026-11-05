#!/usr/bin/env python3
"""
Line coverage per first-party directory, as a Markdown table, from the JSON of
"llvm-cov export -summary-only" (docs/refactor/PLAN.md, 1.5).

    python Script/coverage_summary.py <coverage.json> [--repo <checkout root>]

Files are grouped by module and directory (Common/util, Engine/ac, ...). Third-party code (libsrc,
libinclude, the downloaded dependencies) and the tests themselves are left out.
"""
import argparse
import collections
import json
import os
import sys

MODULES = ('Common', 'Engine', 'Compiler', 'Tools', 'Plugins')


def directory_of(path, root):
    """'Engine/ac' for <root>/Engine/ac/x.cpp; None for what is not counted."""
    path = path.replace('\\', '/')
    if not path.startswith(root):
        return None
    parts = path[len(root):].split('/')
    if parts[0] not in MODULES or 'libsrc' in parts or 'libinclude' in parts or 'test' in parts[1:-1]:
        return None
    return '/'.join(parts[:2]) if len(parts) > 2 else parts[0] + '/.'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('json')
    ap.add_argument('--repo', default=os.getcwd(), help='checkout root, cut from the paths')
    args = ap.parse_args()

    root = args.repo.replace('\\', '/').rstrip('/') + '/'
    with open(args.json, encoding='utf-8') as f:
        export = json.load(f)
    lines = collections.Counter()
    covered = collections.Counter()
    for data in export['data']:
        for entry in data['files']:
            directory = directory_of(entry['filename'], root)
            if directory is None:
                continue
            for key in (directory, directory.split('/')[0], 'Total'):
                lines[key] += entry['summary']['lines']['count']
                covered[key] += entry['summary']['lines']['covered']
    if not lines:
        print('No first-party file in %s' % args.json)
        return 1

    def row(name, bold=False):
        percent = 100.0 * covered[name] / lines[name] if lines[name] else 0.0
        mark = '**' if bold else ''
        return '| %s%s%s | %d | %d | %s%.1f%%%s |' % (mark, name, mark, lines[name], covered[name], mark, percent, mark)

    print('| Directory | Lines | Covered | Coverage |')
    print('|---|---:|---:|---:|')
    print(row('Total', bold=True))
    for module in MODULES:
        if module not in lines:
            continue
        print(row(module, bold=True))
        for name in sorted(n for n in lines if n.startswith(module + '/')):
            print(row(name))
    return 0


if __name__ == '__main__':
    sys.exit(main())
