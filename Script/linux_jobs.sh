#!/bin/bash
# The Linux jobs of the workflows, run on a Linux machine or in a WSL distro (the stages linux,
# sanitizers and coverage of Script/prepush.py; docs/refactor/PLAN.md, 1.5b):
#
#   Script/linux_jobs.sh gcc          verify.yml "Linux (GCC)": build with warnings as errors, header
#                                     check, ctest (clang-tidy is prepush's tidy stage; the auto-test
#                                     game, which the workflow runs too, is left to it and to the
#                                     sanitizers job below: it would add two minutes to every run)
#   Script/linux_jobs.sh sanitizers   analysis.yml "Sanitizers (ASan, UBSan)": unit tests and the
#                                     auto-test game; exit code 1 if there is any finding
#   Script/linux_jobs.sh coverage     analysis.yml "Coverage": the same runs, a table per directory
#
# Keep the commands in step with the workflows. Needs the packages their "Set up ..." steps install
# (clang and llvm for coverage) and, for the jobs that run the game, tap.py (on PATH or in ~/venv).
#
# AGS_JOBS_WORK   build trees, logs and reports (default: ~/build)
# AGS_AUTO_TEST   a checkout of the auto-test project (default: cloned into the work directory at
#                 the tag analysis.yml names)
set -uo pipefail

SRC=$(cd "$(dirname "$0")/.." && pwd)
WORK=${AGS_JOBS_WORK:-$HOME/build}
JOBS=$(nproc)
SAN_FLAGS="-fsanitize=address,undefined -fsanitize-recover=address -fno-omit-frame-pointer -g"
COV_FLAGS="-fprofile-instr-generate -fcoverage-mapping"
TOOLS="agsbuild agscc agf2dta agf2dlgasc agfexport agspak crmbuild crmpak po2tra spriteimport spritepak"
TESTS="common_test engine_test engine_full_test compiler_test tools_test"

if [ -d "$HOME/venv/bin" ]; then
    PATH="$HOME/venv/bin:$PATH"
fi
# Like a CI runner: no display and no sound server. WSLg offers both, the game window would open on
# the Windows desktop, and SDL finds Wayland through XDG_RUNTIME_DIR even without WAYLAND_DISPLAY.
unset DISPLAY WAYLAND_DISPLAY XDG_RUNTIME_DIR PULSE_SERVER

# step <name> <command...>: runs the command, reports its result and time, returns its status
step() {
    local name="$1" start=$SECONDS result=0
    shift
    echo "=== $name"
    "$@" || result=$?
    if [ $result -eq 0 ]; then
        echo "=== $name: ok ($((SECONDS - start)) s)"
    else
        echo "=== $name: FAILED ($((SECONDS - start)) s)"
    fi
    return $result
}

workflow_value() {
    sed -n "s/^ *$1: *//p" "$SRC/.github/workflows/analysis.yml" | head -n 1
}

auto_test_source() {
    if [ -n "${AGS_AUTO_TEST:-}" ]; then
        return 0
    fi
    local tag
    tag=$(workflow_value AUTO_TEST_TAG)
    AGS_AUTO_TEST="$WORK/auto-test-source"
    if [ ! -d "$AGS_AUTO_TEST/.git" ]; then
        git clone -q --no-checkout "https://github.com/$(workflow_value AUTO_TEST_REPO)" "$AGS_AUTO_TEST" || return 1
    fi
    git -C "$AGS_AUTO_TEST" rev-parse -q --verify "refs/tags/$tag" > /dev/null \
        || git -C "$AGS_AUTO_TEST" fetch -q origin "refs/tags/$tag:refs/tags/$tag" || return 1
    git -C "$AGS_AUTO_TEST" checkout -q -f --detach "$tag"
}

auto_test() {
    "$SRC/Script/agsbuild_autotest.sh" "$1" "$AGS_AUTO_TEST/ags4/auto-test" "$2"
}

job_gcc() {
    local build="$WORK/gcc"
    step configure cmake -S "$SRC" -B "$build" -DAGS_BUILD_TOOLS=1 -DAGS_TESTS=1 -DAGS_HEADER_CHECK=1 \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DAGS_WARNINGS_AS_ERRORS=ON -DCMAKE_BUILD_TYPE=Release || return 1
    # -k: a warning that became an error should not hide the next ones
    step build cmake --build "$build" --parallel "$JOBS" -- -k || return 1
    step "header check" cmake --build "$build" --target header_check --parallel "$JOBS" -- -k || return 1
    step "unit tests" ctest --test-dir "$build" --output-on-failure
}

sanitizers_unit_tests() {
    # UBSan reports to stderr whatever log_path says, and ctest shows the output of failed tests only:
    # -V keeps all of it, in a file next to the sanitizer logs for the summary to read
    (cd "$1" && ctest -V -j 4 > "$2/ctest-output.txt" 2>&1) || {
        grep -E "^[0-9]+/[0-9]+ Test|tests failed|\*\*\*Failed" "$2/ctest-output.txt" | grep -v Passed | tail -n 60
        return 1
    }
    tail -n 3 "$2/ctest-output.txt"
}

sanitizers_auto_test() {
    auto_test "$1" "$WORK/san-autotest" 2>&1 | tee "$2/autotest-output.txt" | tail -n 12
}

sanitizers_findings() {
    local result=0
    python3 "$SRC/Script/sanitizer_summary.py" "$1" --repo "$SRC" > "$1/findings.md" || result=$?
    cat "$1/findings.md"
    return $result
}

job_sanitizers() {
    local build="$WORK/san" logs="$WORK/san-logs" result=0
    rm -rf "$logs"
    mkdir -p "$logs"
    step "auto-test project" auto_test_source || return 1
    # RelWithDebInfo: optimized, so the game runs in reasonable time, with the line numbers the reports
    # need. The sanitizer run-time must not report while CMake probes the compiler.
    step configure env ASAN_OPTIONS=detect_leaks=0 cmake -S "$SRC" -B "$build" -DAGS_BUILD_TOOLS=1 \
        -DAGS_TESTS=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DCMAKE_C_FLAGS=$SAN_FLAGS" \
        "-DCMAKE_CXX_FLAGS=$SAN_FLAGS" "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined" || return 1
    step build cmake --build "$build" --parallel "$JOBS" || return 1

    export ASAN_OPTIONS="halt_on_error=0:detect_leaks=1:log_path=$logs/san"
    # The suppressions list the reports whose cause is in third-party code, each with its reason
    export UBSAN_OPTIONS="print_stacktrace=1:log_path=$logs/san:suppressions=$SRC/ci/sanitizers/ubsan.supp"
    # A leak is a finding in the log like the others, not an exit code that hides the next ones
    export LSAN_OPTIONS="exitcode=0:suppressions=$SRC/ci/sanitizers/lsan.supp"
    step "unit tests" sanitizers_unit_tests "$build" "$logs" || result=1
    step "auto-test game" sanitizers_auto_test "$build" "$logs" || result=1
    step findings sanitizers_findings "$logs" || result=1
    return $result
}

coverage_report() {
    local build="$1" profiles="$2" version profdata cov objects="" name path
    # The tools of the clang that compiled the code: a newer or older llvm-cov cannot read its profiles
    version=$(clang -dumpversion | cut -d. -f1)
    profdata=$(command -v "llvm-profdata-$version" || command -v llvm-profdata)
    cov=$(command -v "llvm-cov-$version" || command -v llvm-cov)
    "$profdata" merge -sparse "$profiles"/*.profraw -o "$WORK/coverage.profdata" || return 1
    for name in ags $TOOLS $TESTS; do
        path=$(find "$build" -type f -name "$name" -perm -u+x -not -path '*/_deps/*' | head -n 1)
        if [ -n "$path" ]; then
            objects="$objects -object $path"
        fi
    done
    # the first binary is given without -object
    # shellcheck disable=SC2086
    "$cov" export -summary-only -instr-profile "$WORK/coverage.profdata" ${objects#" -object "} \
        > "$WORK/coverage.json" || return 1
    python3 "$SRC/Script/coverage_summary.py" "$WORK/coverage.json" --repo "$SRC" > "$WORK/coverage.md" || return 1
    cat "$WORK/coverage.md"
}

job_coverage() {
    local build="$WORK/cov" profiles="$WORK/cov-profiles"
    export CC=clang CXX=clang++
    # %m: one file per binary, merged in place by every process that runs it
    export LLVM_PROFILE_FILE="$profiles/%m.profraw"
    rm -rf "$profiles"
    step "auto-test project" auto_test_source || return 1
    step configure cmake -S "$SRC" -B "$build" -DAGS_BUILD_TOOLS=1 -DAGS_TESTS=1 -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_C_FLAGS=$COV_FLAGS" "-DCMAKE_CXX_FLAGS=$COV_FLAGS" \
        "-DCMAKE_EXE_LINKER_FLAGS=-fprofile-instr-generate" || return 1
    step build cmake --build "$build" --parallel "$JOBS" || return 1
    step "unit tests" ctest --test-dir "$build" --output-on-failure -j 4 || return 1
    step "auto-test game" auto_test "$build" "$WORK/cov-autotest" || return 1
    step coverage coverage_report "$build" "$profiles"
}

mkdir -p "$WORK"
case "${1:-}" in
    gcc) job_gcc ;;
    sanitizers) job_sanitizers ;;
    coverage) job_coverage ;;
    *)
        echo "usage: $0 gcc|sanitizers|coverage" >&2
        exit 2
        ;;
esac
