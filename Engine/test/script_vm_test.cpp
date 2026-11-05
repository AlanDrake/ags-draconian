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
//
// The script VM (ScriptExecutor) running scripts compiled in memory by the
// script compiler (Compiler/script): results, calls between functions and
// scripts, calls into the engine, and the run-time errors a game script can
// cause (division by zero, array bounds, null pointers, runaway recursion and
// loops).
//
//=============================================================================
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ac/dynobj/managedobjectpool.h"
#include "gtest/gtest.h"
#include "script/cc_common.h"
#include "script/cc_reflect.h"
#include "script/cs_compiler.h"
#include "script/runtimescript.h"
#include "script/script_runtime.h"
#include "script/scriptexecutor.h"
#include "script/systemimports.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern std::unique_ptr<ScriptExecutor> scriptExecutor;

namespace
{

// The options the game build uses (Compiler/compiler.cpp), minus the debug-only script TOC
const uint64_t kCompileOptions = SCOPT_EXPORTALL | SCOPT_LINENUMBERS | SCOPT_UTF8 | SCOPT_RTTI | SCOPT_RTTIOPS;

// Engine functions that the test scripts import
int ImportCalls = 0;

RuntimeScriptValue TestTwice(const RuntimeScriptValue *params, int32_t param_count)
{
    ++ImportCalls;
    return RuntimeScriptValue().SetInt32(param_count > 0 ? params[0].IValue * 2 : 0);
}

RuntimeScriptValue TestAbort(const RuntimeScriptValue *, int32_t)
{
    ++ImportCalls;
    scriptExecutor->Abort();
    return RuntimeScriptValue().SetInt32(0);
}

// What the engine does on every game update (for example in Wait)
RuntimeScriptValue TestUpdate(const RuntimeScriptValue *, int32_t)
{
    ++ImportCalls;
    ccNotifyScriptStillAlive();
    return RuntimeScriptValue().SetInt32(0);
}

// Runs another script function on a second thread while the first one waits, like
// repeatedly_execute_always during a blocking call
const RuntimeScript *NestedScript = nullptr;
ScriptThread *NestedThread = nullptr;

RuntimeScriptValue TestRunNested(const RuntimeScriptValue *params, int32_t param_count)
{
    ++ImportCalls;
    const ScriptExecError err = scriptExecutor->Run(NestedThread, NestedScript, "Nested", params, param_count);
    return RuntimeScriptValue().SetInt32(err == kScExecErr_None ? scriptExecutor->GetReturnValue() : -1000);
}

// Records the pool handle of a managed object passed by the script
std::vector<int> KeptHandles;

RuntimeScriptValue TestKeepHandle(const RuntimeScriptValue *params, int32_t param_count)
{
    ++ImportCalls;
    if (param_count > 0)
        KeptHandles.push_back(pool.AddressToHandle(params[0].Ptr));
    return RuntimeScriptValue().SetInt32(0);
}

const char *const kTestImports[] = {"TestTwice^1", "TestAbort^0", "TestUpdate^0", "TestRunNested^1",
                                    "TestKeepHandle^1"};

} // namespace

// Fixture for the script VM. The engine globals it touches, all restored in TearDown:
// - scriptExecutor: the engine's executor; error call stacks are taken from it (cc_get_callstack)
// - simp: the test imports above and the exports of the loaded scripts are registered, then removed
// - ccCompOptions: SCOPT_AUTOIMPORT is on while scripts are linked, as at game start
// - the script error state (cc_get_error), cleared before and after each test
// - pool: the managed objects that scripts create; scripts release them, garbage collection runs
//   when the pool decides (after 1024 new objects, at the end of a run); what is left is removed in TearDown
// - RuntimeScript's link slots: each loaded script takes one until the test ends
// Not restored: RuntimeScript's joint RTTI (no reset exists; types only accumulate).
class ScriptVM : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cc_clear_error();
        ImportCalls = 0;
        scriptExecutor = std::make_unique<ScriptExecutor>();
        // no system polling during long loops; "hung script" after 10000 loops without an update
        scriptExecutor->SetExecTimeout(60000u, 0u, 10000u);
        ccAddExternalStaticFunction("TestTwice^1", TestTwice);
        ccAddExternalStaticFunction("TestAbort^0", TestAbort);
        ccAddExternalStaticFunction("TestUpdate^0", TestUpdate);
        ccAddExternalStaticFunction("TestRunNested^1", TestRunNested);
        ccAddExternalStaticFunction("TestKeepHandle^1", TestKeepHandle);
        KeptHandles.clear();
        NestedThread = &_nestedThread;
    }

    void TearDown() override
    {
        for (auto &script : _scripts)
            script->UnRegisterExports(simp);
        _scripts.clear();
        for (const char *name : kTestImports)
            ccRemoveExternalSymbol(name);
        NestedScript = nullptr;
        NestedThread = nullptr;
        scriptExecutor.reset();
        // what the scripts left alive (a run that ends with an error, a test that keeps its objects)
        pool.Reset();
        cc_clear_error();
    }

    // Compiles a script and links it with the test imports and the previously loaded scripts.
    // Each script gets a section name of its own, as the game build's preprocessor gives one to
    // every script: the joint RTTI, which outlives the tests, tells types apart by section and name.
    const RuntimeScript *Load(const std::string &source, const std::string &name = "test")
    {
        static int loaded_count = 0;
        const std::string section = name + std::to_string(++loaded_count);
        AGS::MessageHandler mh;
        std::unique_ptr<ccScript> compiled(
            ccCompileText("\"__NEWSCRIPTSTART_" + section + "\"\n" + source, section, kCompileOptions, mh));
        if (!compiled)
        {
            ADD_FAILURE() << "compile error: " << mh.GetError().Message;
            return nullptr;
        }
        std::unique_ptr<RuntimeScript> script = RuntimeScript::Create(compiled.get());
        if (!script)
        {
            ADD_FAILURE() << "RuntimeScript::Create: " << cc_get_error().ErrorString.GetCStr();
            return nullptr;
        }
        const int was_autoimport = ccGetOption(SCOPT_AUTOIMPORT);
        ccSetOption(SCOPT_AUTOIMPORT, 1);
        script->RegisterExports(simp);
        ccSetOption(SCOPT_AUTOIMPORT, was_autoimport);
        if (!script->ResolveImports(simp))
        {
            ADD_FAILURE() << "ResolveImports: " << cc_get_error().ErrorString.GetCStr();
            script->UnRegisterExports(simp);
            return nullptr;
        }
        _scripts.push_back(std::move(script));
        return _scripts.back().get();
    }

    ScriptExecError Run(const RuntimeScript *script, const char *func, const std::vector<int> &args = {})
    {
        std::vector<RuntimeScriptValue> params;
        for (int arg : args)
            params.push_back(RuntimeScriptValue().SetInt32(arg));
        return scriptExecutor->Run(&_thread, script, func, params.data(), params.size());
    }

    // Runs a function that must succeed, and returns its result
    int Call(const RuntimeScript *script, const char *func, const std::vector<int> &args = {})
    {
        const ScriptExecError err = Run(script, func, args);
        EXPECT_EQ(kScExecErr_None, err) << func << ": " << cc_get_error().ErrorString.GetCStr();
        return scriptExecutor->GetReturnValue();
    }

    // Runs a function that must fail with a script error, and returns the error text
    std::string CallFails(const RuntimeScript *script, const char *func, const std::vector<int> &args = {})
    {
        const ScriptExecError err = Run(script, func, args);
        EXPECT_EQ(kScExecErr_Generic, err) << func;
        EXPECT_TRUE(cc_has_error()) << func;
        return cc_get_error().ErrorString.GetCStr();
    }

    ScriptThread _thread{"Test"};
    ScriptThread _nestedThread{"Nested"};
    std::vector<std::unique_ptr<RuntimeScript>> _scripts;
};

TEST_F(ScriptVM, RunsFunctionWithArguments)
{
    const auto *script = Load("int Add(int a, int b) { return a + b; }");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(5, Call(script, "Add", {2, 3}));
    EXPECT_EQ(-7, Call(script, "Add", {-10, 3}));
    // more arguments than declared are allowed: the first ones are used
    EXPECT_EQ(3, Call(script, "Add", {1, 2, 100}));
}

TEST_F(ScriptVM, IntegerArithmeticAndBitOperations)
{
    const auto *script = Load(R"(
int Arith(int a, int b)
{
    return (a * b) - (a / b) + (a % b);
}
int Bits(int a, int b)
{
    return ((a & b) | (a ^ b)) + (a << 2) + (a >> 1);
}
int Logic(int a, int b)
{
    int r = 0;
    if (a > b && b >= 0) r += 1;
    if (a < b || b == 0) r += 10;
    if (!(a != b)) r += 100;
    if (a <= b) r += 1000;
    return r;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(17 * 5 - 17 / 5 + 17 % 5, Call(script, "Arith", {17, 5}));
    EXPECT_EQ(-17 * 5 - (-17 / 5) + (-17 % 5), Call(script, "Arith", {-17, 5}));
    EXPECT_EQ(((12 & 10) | (12 ^ 10)) + (12 << 2) + (12 >> 1), Call(script, "Bits", {12, 10}));
    EXPECT_EQ(1, Call(script, "Logic", {5, 3}));
    EXPECT_EQ(11, Call(script, "Logic", {5, 0}));
    EXPECT_EQ(1100, Call(script, "Logic", {4, 4}));
    EXPECT_EQ(1010, Call(script, "Logic", {-1, 2}));
}

TEST_F(ScriptVM, IntegerOverflowWraps)
{
    const auto *script = Load("int Mul(int a, int b) { return a * b; } int Inc(int a) { return a + 1; }");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(INT32_MIN, Call(script, "Inc", {INT32_MAX}));
    EXPECT_EQ(static_cast<int>(0x10000u * 0x8000u), Call(script, "Mul", {0x10000, 0x8000}));
}

TEST_F(ScriptVM, FloatArithmetic)
{
    const auto *script = Load(R"(
int FloatChecks()
{
    float a = 1.5;
    float b = a * 4.0;
    int r = 0;
    if (b == 6.0) r += 1;
    if (b / 4.0 == a) r += 10;
    if (a - 2.0 < 0.0) r += 100;
    if (b >= 6.0 && b <= 6.0) r += 1000;
    return r;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(1111, Call(script, "FloatChecks"));
}

TEST_F(ScriptVM, LoopsAndLocalArrays)
{
    const auto *script = Load(R"(
int SumTo(int n)
{
    int s = 0;
    for (int i = 1; i <= n; i++)
        s += i;
    return s;
}
int WhileDown(int n)
{
    int steps = 0;
    while (n > 1)
    {
        if (n % 2 == 0) n = n / 2; else n = 3 * n + 1;
        steps++;
    }
    return steps;
}
int LocalArray()
{
    int a[10];
    for (int i = 0; i < 10; i++)
        a[i] = i * i;
    int s = 0;
    for (int i = 0; i < 10; i++)
        s += a[i];
    return s;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(5050, Call(script, "SumTo", {100}));
    EXPECT_EQ(0, Call(script, "SumTo", {0}));
    EXPECT_EQ(111, Call(script, "WhileDown", {27}));
    EXPECT_EQ(285, Call(script, "LocalArray"));
}

TEST_F(ScriptVM, RecursiveCalls)
{
    const auto *script = Load(R"(
int Fib(int n)
{
    if (n < 2)
        return n;
    return Fib(n - 1) + Fib(n - 2);
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(610, Call(script, "Fib", {15}));
}

TEST_F(ScriptVM, GlobalVariablesKeepTheirValuesBetweenRuns)
{
    const auto *script = Load(R"(
int counter;
int table[3];
void Bump(int by)
{
    counter += by;
    table[by % 3] += 1;
}
int GetCounter() { return counter; }
int GetTable(int i) { return table[i]; }
)");
    ASSERT_NE(nullptr, script);
    Call(script, "Bump", {4});
    Call(script, "Bump", {5});
    Call(script, "Bump", {7});
    EXPECT_EQ(16, Call(script, "GetCounter"));
    EXPECT_EQ(0, Call(script, "GetTable", {0}));
    EXPECT_EQ(2, Call(script, "GetTable", {1}));
    EXPECT_EQ(1, Call(script, "GetTable", {2}));
    // the global data is the script's own memory, little-endian
    const auto &data = script->GetGlobalData();
    ASSERT_GE(data.size(), 4u);
    EXPECT_EQ(16, data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
}

TEST_F(ScriptVM, DynamicArraysAndManagedStructs)
{
    const auto *script = Load(R"(
managed struct Node
{
    int Value;
    Node *Next;
};
int Squares(int n)
{
    int a[] = new int[n];
    for (int i = 0; i < n; i++)
        a[i] = i * i;
    return a[n - 1] + a[1];
}
int ListSum(int n)
{
    Node *head;
    for (int i = 1; i <= n; i++)
    {
        Node *node = new Node;
        node.Value = i;
        node.Next = head;
        head = node;
    }
    int s = 0;
    for (Node *it = head; it != null; it = it.Next)
        s += it.Value;
    return s;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(81 + 1, Call(script, "Squares", {10}));
    EXPECT_EQ(55, Call(script, "ListSum", {10}));
}

TEST_F(ScriptVM, CallsEngineFunctions)
{
    const auto *script = Load(R"(
import int TestTwice(int x);
int TwicePlusOne(int v)
{
    return TestTwice(v) + 1;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(43, Call(script, "TwicePlusOne", {21}));
    EXPECT_EQ(1, ImportCalls);
}

TEST_F(ScriptVM, CallsFunctionsOfAnotherScript)
{
    const auto *lib = Load("int Triple(int x) { return x * 3; }", "lib");
    ASSERT_NE(nullptr, lib);
    const auto *user = Load(R"(
import int Triple(int x);
int UseTriple(int v)
{
    return Triple(v) + Triple(1);
}
)",
                            "user");
    ASSERT_NE(nullptr, user);
    EXPECT_EQ(33, Call(user, "UseTriple", {10}));
}

// Joining adds new locations, which grows the joint string table, and then looks the next ones
// up among the existing locations: their names must not be read through pointers into the
// table's old buffer (a sanitizer reports it as a use after free)
TEST(JointRTTI, FindsExistingLocationAfterTheStringTableGrew)
{
    AGS::JointRTTI joint;
    std::unordered_map<uint32_t, uint32_t> loc_l2g, type_l2g;
    AGS::RTTIBuilder first;
    first.AddLocation("shared", 0u, 0u);
    first.AddLocation("only_in_first", 1u, 0u);
    joint.Join(first.Finalize(), loc_l2g, type_l2g);
    ASSERT_EQ(2u, joint.GetLocations().size());

    loc_l2g.clear();
    AGS::RTTIBuilder second;
    const uint32_t new_locs = 8u;
    for (uint32_t i = 0; i < new_locs; ++i)
    {
        second.AddLocation("a_new_location_with_a_long_name_" + std::to_string(i) + std::string(64u, 'x'), i, 0u);
    }
    second.AddLocation("shared", new_locs, 0u);
    second.AddLocation("only_in_first", new_locs + 1u, 0u);
    joint.Join(second.Finalize(), loc_l2g, type_l2g);

    EXPECT_EQ(2u + new_locs, joint.GetLocations().size());
    EXPECT_EQ(0u, loc_l2g[new_locs]);
    EXPECT_EQ(1u, loc_l2g[new_locs + 1u]);
}

TEST_F(ScriptVM, UnresolvedImportIsRefusedAtLink)
{
    AGS::MessageHandler mh;
    std::unique_ptr<ccScript> compiled(ccCompileText(
        "import int NoSuchFunction(int x); int F() { return NoSuchFunction(1); }", "t", kCompileOptions, mh));
    ASSERT_TRUE(compiled) << mh.GetError().Message;
    std::unique_ptr<RuntimeScript> script = RuntimeScript::Create(compiled.get());
    ASSERT_TRUE(script);
    EXPECT_FALSE(script->ResolveImports(simp));
    EXPECT_NE(std::string::npos, std::string(cc_get_error().ErrorString.GetCStr()).find("NoSuchFunction"));
}

TEST_F(ScriptVM, UnknownFunctionAndMissingArguments)
{
    const auto *script = Load("int Add(int a, int b) { return a + b; }");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(kScExecErr_FuncNotFound, Run(script, "Sub", {1, 2}));
    EXPECT_EQ(kScExecErr_InvalidArgNum, Run(script, "Add", {1}));
    EXPECT_FALSE(scriptExecutor->IsRunning());
    // the executor is usable after a refused call
    EXPECT_EQ(3, Call(script, "Add", {1, 2}));
}

TEST_F(ScriptVM, IntegerDivisionByZeroIsAnError)
{
    const auto *script = Load(R"(
int Div(int a, int b)
{
    return a / b;
}
int Mod(int a, int b)
{
    return a % b;
}
)");
    ASSERT_NE(nullptr, script);
    std::string err = CallFails(script, "Div", {1, 0});
    EXPECT_NE(std::string::npos, err.find("Integer divide by zero")) << err;
    EXPECT_NE(std::string::npos, err.find("line 4")) << err;
    EXPECT_TRUE(cc_get_error().IsUserError);
    err = CallFails(script, "Mod", {1, 0});
    EXPECT_NE(std::string::npos, err.find("Integer divide by zero")) << err;
    EXPECT_NE(std::string::npos, err.find("line 8")) << err;
    // a failed run leaves the executor ready for the next one
    EXPECT_FALSE(scriptExecutor->IsRunning());
    EXPECT_EQ(3, Call(script, "Div", {7, 2}));
}

TEST_F(ScriptVM, FloatDivisionByZeroIsAnError)
{
    const auto *script = Load("float FDiv(float a, float b) { return a / b; } int F() { FDiv(1.0, 0.0); return 0; }");
    ASSERT_NE(nullptr, script);
    const std::string err = CallFails(script, "F");
    EXPECT_NE(std::string::npos, err.find("Floating point divide by zero")) << err;
}

TEST_F(ScriptVM, ArrayIndexOutOfBoundsIsAnError)
{
    const auto *script = Load(R"(
int table[4];
int Static(int i)
{
    return table[i];
}
int Dynamic(int n, int i)
{
    int a[] = new int[n];
    return a[i];
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(0, Call(script, "Static", {3}));
    std::string err = CallFails(script, "Static", {4});
    EXPECT_NE(std::string::npos, err.find("Array index out of bounds (index: 4, bounds: 0..3)")) << err;
    err = CallFails(script, "Static", {-1});
    EXPECT_NE(std::string::npos, err.find("Array index out of bounds (index: -1, bounds: 0..3)")) << err;

    EXPECT_EQ(0, Call(script, "Dynamic", {3, 2}));
    err = CallFails(script, "Dynamic", {3, 3});
    EXPECT_NE(std::string::npos, err.find("Array index out of bounds (index: 3, bounds: 0..2)")) << err;
    err = CallFails(script, "Dynamic", {3, -1});
    EXPECT_NE(std::string::npos, err.find("Array index out of bounds")) << err;
    err = CallFails(script, "Dynamic", {-1, 0});
    EXPECT_NE(std::string::npos, err.find("Invalid size for dynamic array")) << err;
}

TEST_F(ScriptVM, NullPointerIsAnError)
{
    const auto *script = Load(R"(
managed struct Node
{
    int Value;
};
int ReadNull()
{
    Node *node;
    return node.Value;
}
int NullArray()
{
    int a[];
    return a[0];
}
)");
    ASSERT_NE(nullptr, script);
    std::string err = CallFails(script, "ReadNull");
    EXPECT_NE(std::string::npos, err.find("Null pointer referenced")) << err;
    EXPECT_NE(std::string::npos, err.find("line 9")) << err;
    err = CallFails(script, "NullArray");
    EXPECT_NE(std::string::npos, err.find("Null pointer referenced")) << err;
}

TEST_F(ScriptVM, RunawayRecursionIsAnError)
{
    const auto *script = Load("int Down(int n) { return Down(n + 1); }");
    ASSERT_NE(nullptr, script);
    const std::string err = CallFails(script, "Down", {0});
    EXPECT_NE(std::string::npos, err.find("call stack overflow")) << err;
    EXPECT_FALSE(scriptExecutor->IsRunning());
}

TEST_F(ScriptVM, HungLoopIsAborted)
{
    const auto *script = Load(R"(
int Spin()
{
    int i = 0;
    while (i >= 0)
    {
        i = 1;
    }
    return i;
}
)");
    ASSERT_NE(nullptr, script);
    const std::string err = CallFails(script, "Spin");
    EXPECT_NE(std::string::npos, err.find("Script appears to be hung (a while loop ran 10001 times)")) << err;
}

TEST_F(ScriptVM, LoopCheckCountsIterationsWithinOneRun)
{
    // the limit is 10000 iterations of loops without an engine update in between,
    // counted from the start of each run
    const auto *script = Load(R"(
int Count(int n)
{
    int i = 0;
    while (i < n)
        i++;
    return i;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(9999, Call(script, "Count", {9999}));
    const std::string err = CallFails(script, "Count", {20000});
    EXPECT_NE(std::string::npos, err.find("Script appears to be hung")) << err;
}

TEST_F(ScriptVM, AbortStopsTheScript)
{
    const auto *script = Load(R"(
import int TestAbort();
import int TestTwice(int x);
int counter;
int AbortMidway()
{
    counter = 1;
    TestAbort();
    counter = 2;
    TestTwice(1);
    return 5;
}
int GetCounter() { return counter; }
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(kScExecErr_Aborted, Run(script, "AbortMidway"));
    EXPECT_EQ(1, ImportCalls); // TestTwice was not called
    EXPECT_FALSE(scriptExecutor->IsRunning());
    EXPECT_EQ(1, Call(script, "GetCounter"));
}

TEST_F(ScriptVM, ErrorReportsTheCallStack)
{
    const auto *script = Load(R"(
int Inner(int b)
{
    return 10 / b;
}
int Outer(int b)
{
    return Inner(b) + 1;
}
)",
                              "callstack");
    ASSERT_NE(nullptr, script);
    CallFails(script, "Outer", {0});
    const std::string stack = cc_get_error().CallStack.GetCStr();
    EXPECT_NE(std::string::npos, stack.find("line 4")) << stack;
    EXPECT_NE(std::string::npos, stack.find("line 8")) << stack;
    EXPECT_EQ(4, cc_get_error().Line);
}

TEST_F(ScriptVM, EngineUpdatesResetTheLoopCheck)
{
    const auto *script = Load(R"(
import int TestUpdate();
int CountWithUpdates(int n)
{
    int i = 0;
    while (i < n)
    {
        i++;
        if (i % 1000 == 0)
            TestUpdate();
    }
    return i;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(30000, Call(script, "CountWithUpdates", {30000}));
    EXPECT_EQ(30, ImportCalls);
}

TEST_F(ScriptVM, Strings)
{
    const auto *script = Load(R"(
internalstring autoptr builtin managed struct String
{
};
int Compare(int which)
{
    String a = "abc";
    String b = "abc";
    String c = "abd";
    String none;
    if (which == 0) return a == b;
    if (which == 1) return a != c;
    if (which == 2) return a == c;
    return a == none;
}
)");
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(1, Call(script, "Compare", {0}));
    EXPECT_EQ(1, Call(script, "Compare", {1}));
    EXPECT_EQ(0, Call(script, "Compare", {2}));
    const std::string err = CallFails(script, "Compare", {3});
    EXPECT_NE(std::string::npos, err.find("Null pointer referenced")) << err;
}

TEST_F(ScriptVM, NestedRunOnAnotherThread)
{
    const auto *script = Load(R"(
import int TestRunNested(int x);
int depth;
int Nested(int x)
{
    depth += 1;
    return x * 10;
}
int Outer(int x)
{
    int local = x + 1;
    int nested = TestRunNested(x);
    return local + nested + depth;
}
)");
    ASSERT_NE(nullptr, script);
    NestedScript = script;
    // the outer function's locals survive the nested run on the other thread
    EXPECT_EQ(5 + 40 + 1, Call(script, "Outer", {4}));
    EXPECT_FALSE(_thread.IsBusy());
    EXPECT_FALSE(_nestedThread.IsBusy());
    EXPECT_FALSE(scriptExecutor->IsRunning());
}

TEST_F(ScriptVM, ReferenceCycleIsCollected)
{
    const auto *script = Load(R"(
managed struct Node
{
    Node *Other;
};
import void TestKeepHandle(Node *n);
void MakeCycle()
{
    Node *a = new Node;
    Node *b = new Node;
    a.Other = b;
    b.Other = a;
    TestKeepHandle(a);
    TestKeepHandle(b);
}
void MakeGarbage(int n)
{
    for (int i = 0; i < n; i++)
    {
        Node *temp = new Node;
    }
}
)");
    ASSERT_NE(nullptr, script);
    Call(script, "MakeCycle");
    ASSERT_EQ(2u, KeptHandles.size());
    // the two nodes hold each other: no reference count drops to zero
    EXPECT_NE(nullptr, pool.HandleToAddress(KeptHandles[0]));
    EXPECT_NE(nullptr, pool.HandleToAddress(KeptHandles[1]));
    // enough new objects for the pool to run the garbage collection after the run
    Call(script, "MakeGarbage", {1100});
    EXPECT_EQ(nullptr, pool.HandleToAddress(KeptHandles[0]));
    EXPECT_EQ(nullptr, pool.HandleToAddress(KeptHandles[1]));
}
