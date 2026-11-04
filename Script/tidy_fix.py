"""
Runs one clang-tidy check over all first-party translation units in parallel, merges and
de-duplicates the exported fixes, and applies them to the working tree in one go.
(Running clang-tidy --fix on files in parallel corrupts shared headers.)

    python Script/tidy_fix.py <check> [extra clang-tidy args]

Needs: pyyaml, clang-tidy 19 (CLANG_TIDY env var, or on PATH), and a compile database from
    cmake -S . -B .tidy/build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ...
Afterwards run clang-format on the changed files and check with Script/compare_objects.py.
Not every fix is safe: see the commit messages of the [nullptr]/[override]/[headers]/[void] steps.
"""
import sys, os, re, json, subprocess, tempfile, collections
from concurrent.futures import ThreadPoolExecutor
import yaml
sys.path.insert(0, 'Script')
import compare_objects as c
env = c.msvc_env(); env.pop('CFLAGS', None); env.pop('CXXFLAGS', None)
T = os.environ.get('CLANG_TIDY', 'clang-tidy')
check = sys.argv[1]; extra = sys.argv[2:]
root = os.getcwd().replace(chr(92), '/') + '/'
BAD = r'/(libsrc|libinclude|_deps|agslua|\.tidy|\.compare|Editor)/'
db = json.load(open('.tidy/build/compile_commands.json'))
tus = sorted({e['file'].replace(chr(92), '/') for e in db})
tus = [f for f in tus if f.startswith(root) and not re.search(BAD, f) and f.endswith('.cpp')]
tmp = tempfile.mkdtemp()
def run(i_f):
    i, f = i_f
    out = os.path.join(tmp, '%d.yaml' % i)
    subprocess.run([T, '-p', '.tidy/build', '--checks=-*,' + check, '--quiet', '--header-filter=.*', '--export-fixes=' + out, *extra, f], env=env, capture_output=True, text=True)
    return out
with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
    outs = list(ex.map(run, enumerate(tus)))
reps = collections.defaultdict(set)
for o in outs:
    if not os.path.exists(o): continue
    d = yaml.safe_load(open(o)) or {}
    for diag in d.get('Diagnostics', []):
        for r in diag.get('DiagnosticMessage', {}).get('Replacements', []):
            p = os.path.normpath(r['FilePath']).replace(chr(92), '/')
            if p.startswith(root) and not re.search(BAD, p):
                reps[p[len(root):]].add((r['Offset'], r['Length'], r['ReplacementText']))
changed = 0; conflicts = 0
for f, rs in reps.items():
    data = open(f, 'rb').read()
    rs = sorted(rs, reverse=True)
    last = None
    for off, ln, txt in rs:
        if last is not None and off + ln > last:
            conflicts += 1; continue
        data = data[:off] + txt.encode('utf-8') + data[off + ln:]
        last = off; changed += 1
    open(f, 'wb').write(data)
print(len(tus), 'TUs;', changed, 'replacements in', len(reps), 'files;', conflicts, 'conflicts')
