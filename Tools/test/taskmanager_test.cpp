//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
// Tests of the agsbuild task manager: a failing task, and cancelling a build
// (docs/refactor/PLAN.md, 7.1b and 7.1h); what a finished task reports (6.7d-6).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <thread>
#include <utility>
#include <vector>

#include "agsbuild/diagnostics.h"
#include "agsbuild/pipedprocess.h"
#include "agsbuild/taskmanager.h"
#include "gtest/gtest.h"

using namespace AGS::Common;
using namespace AGSBuild;

namespace
{

// A command that runs for a minute
PipedProcessParams SleepCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("ping", "-n 60 127.0.0.1");
#else
    return PipedProcessParams("sleep", "60");
#endif
}

// A command that prints an error in the form of the tools, and fails
PipedProcessParams FailingCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("cmd", "/c echo Error: compile failed at a.asc, line 3 : broken& exit 1");
#else
    return PipedProcessParams("sh", "-c 'echo \"Error: compile failed at a.asc, line 3 : broken\"; exit 1'");
#endif
}

PipedProcessParams QuickCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("cmd", "/c echo done");
#else
    return PipedProcessParams("sh", "-c 'echo done'");
#endif
}

// A command that prints an error, and succeeds anyway (as spriteimport did for a missing image)
PipedProcessParams SucceedingCommandWithError()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("cmd", "/c echo Error: failed to import a sprite, left it empty& exit 0");
#else
    return PipedProcessParams("sh", "-c 'echo \"Error: failed to import a sprite, left it empty\"; exit 0'");
#endif
}

// A command that fails without a word
PipedProcessParams SilentFailingCommand()
{
#if AGS_PLATFORM_OS_WINDOWS
    return PipedProcessParams("cmd", "/c exit 3");
#else
    return PipedProcessParams("sh", "-c 'exit 3'");
#endif
}

// Runs the tasks, and returns the diagnostics they gave
std::vector<Diag::Diagnostic> RunAndCollect(TaskManager &mgr, const char *what)
{
    std::vector<Diag::Diagnostic> seen;
    Diag::SetDiagnosticObserver([&seen](const Diag::Diagnostic &d) { seen.push_back(d); });
    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    if (run.wait_for(std::chrono::seconds(60)) != std::future_status::ready)
    {
        std::fprintf(stderr, "%s: did not finish in time\n", what);
        std::abort();
    }
    run.get();
    Diag::SetDiagnosticObserver(nullptr);
    return seen;
}

template <class T>
T WaitOrAbort(std::future<T> &f, int seconds, const char *what)
{
    if (f.wait_for(std::chrono::seconds(seconds)) != std::future_status::ready)
    {
        // A blocked thread can't be stopped, so end the test run instead of hanging it
        std::fprintf(stderr, "%s: did not finish in time\n", what);
        std::abort();
    }
    return f.get();
}

} // namespace

// The tasks which depend on a failed one are cancelled, the others finish, and RunAll returns
TEST(TaskManager, FailedTaskCancelsDependentsOnly)
{
    const int errors_before = Diag::ErrorCount();
    TaskManager mgr(2);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("fail", &mgr, FailingCommand()));
    mgr.AddTask(
        std::make_unique<TaskPipedProcess>("after-fail", &mgr, SleepCommand(), std::vector<TaskInput>{{"fail"}}));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("independent", &mgr, QuickCommand()));

    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    HError err = WaitOrAbort(run, 60, "FailedTaskCancelsDependentsOnly");
    EXPECT_FALSE(err);
    // The error the tool printed was read from its output
    EXPECT_EQ(errors_before + 1, Diag::ErrorCount());
}

// The exit code tells whether a task failed: the errors a succeeding tool prints are warnings, so
// that a build which succeeds has no errors
TEST(TaskManager, SucceededTaskReportsErrorsAsWarnings)
{
    EXPECT_EQ(Diag::Severity::Warning, SeverityForTaskState(Diag::Severity::Error, TaskState::Success));
    EXPECT_EQ(Diag::Severity::Error, SeverityForTaskState(Diag::Severity::Error, TaskState::Failure));
    EXPECT_EQ(Diag::Severity::Warning, SeverityForTaskState(Diag::Severity::Warning, TaskState::Failure));
    EXPECT_EQ(Diag::Severity::Info, SeverityForTaskState(Diag::Severity::Info, TaskState::Success));

    const int errors_before = Diag::ErrorCount();
    const int warnings_before = Diag::WarningCount();
    TaskManager mgr(1);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("spriteimport", &mgr, SucceedingCommandWithError()));
    const auto seen = RunAndCollect(mgr, "SucceededTaskReportsErrorsAsWarnings");
    EXPECT_EQ(errors_before, Diag::ErrorCount());
    EXPECT_EQ(warnings_before + 1, Diag::WarningCount());
    ASSERT_EQ(1u, seen.size());
    EXPECT_EQ(Diag::Severity::Warning, seen[0].Level);
    EXPECT_EQ("failed to import a sprite, left it empty", seen[0].Message);
}

// A task that fails without an error line of its own is reported with its result: which file,
// what failed (the copy of a missing room gave an empty message)
TEST(TaskManager, FailedTaskWithoutErrorLineHasAMessage)
{
    TaskManager mgr(1);
    const std::vector<std::pair<String, String>> files = {{"no_such_dir/room7.crm", "no_such_dir/copy.crm"}};
    auto copy = std::make_unique<TaskMoveFiles>("copy-rooms", &mgr, files, FileMoveOp::Copy, false);
    const Task *copy_task = copy.get();
    mgr.AddTask(std::move(copy));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("silent", &mgr, SilentFailingCommand()));
    const auto seen = RunAndCollect(mgr, "FailedTaskWithoutErrorLineHasAMessage");

    EXPECT_TRUE(copy_task->GetResult().HasError());
    ASSERT_EQ(2u, seen.size());
    for (const auto &d : seen)
    {
        EXPECT_EQ(Diag::Severity::Error, d.Level);
        if (d.Task == "copy-rooms")
        {
            EXPECT_EQ("agsbuild", d.Tool);
            EXPECT_EQ("Source file no_such_dir/room7.crm does not exist", d.Message);
        }
        else
        {
            EXPECT_EQ("silent", d.Task);
            EXPECT_EQ("Process exited with error code 3", d.Message);
        }
    }
}

// The running tools are killed and the waiting tasks do not start: RunAll returns long before
// the minute that the tools would run
TEST(TaskManager, CancelStopsRunningAndWaitingTasks)
{
    Diag::ResetCancel();
    TaskManager mgr(2);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("first", &mgr, SleepCommand()));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("second", &mgr, SleepCommand()));
    mgr.AddTask(std::make_unique<TaskPipedProcess>("waiting", &mgr, SleepCommand(),
                                                   std::vector<TaskInput>{{"first"}, {"second"}}));

    const auto start = std::chrono::steady_clock::now();
    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(700)); // let the processes start
    Diag::RequestCancel();
    HError err = WaitOrAbort(run, 40, "CancelStopsRunningAndWaitingTasks");
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start);
    Diag::ResetCancel();

    EXPECT_FALSE(err);
    EXPECT_LT(seconds.count(), 30);
}

// A build cancelled before any task has started runs nothing
TEST(TaskManager, CancelBeforeStart)
{
    Diag::ResetCancel();
    Diag::RequestCancel();
    TaskManager mgr(2);
    mgr.AddTask(std::make_unique<TaskPipedProcess>("only", &mgr, SleepCommand()));
    auto run = std::async(std::launch::async, [&mgr]() { return mgr.RunAll(); });
    HError err = WaitOrAbort(run, 40, "CancelBeforeStart");
    Diag::ResetCancel();
    EXPECT_FALSE(err);
}
