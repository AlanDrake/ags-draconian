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
// Hand-assembled bytecode for the script VM: the checks that protect the
// engine from broken or malicious scripts, which the compiler never emits.
// Loading (RuntimeScript::Create) refuses bad fixups; running (ScriptExecutor)
// stops at stack overflow and underflow, unknown instructions, a bad start
// address, an unbalanced stack, invalid object handles and bad call targets.
//
// Some checks exist only in debug builds (DEBUG_CC_EXEC): an opcode beyond the
// instruction table and code running past its end are not checked in release
// builds, where they read out of bounds. The tests for them are skipped there.
//
//=============================================================================
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "script/cc_common.h"
#include "script/cc_internal.h"
#include "script/runtimescript.h"
#include "script/script_runtime.h"
#include "script/scriptexecutor.h"
#include "script/systemimports.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern std::unique_ptr<ScriptExecutor> scriptExecutor;

namespace
{

struct Fixup
{
    int32_t CodeIndex;
    uint8_t Type;
};

// A script with one exported function "f" (no declared arguments) starting at func_start
std::unique_ptr<ccScript> Assemble(const std::vector<int32_t> &code, const std::vector<Fixup> &fixups = {},
                                   const std::vector<uint8_t> &globaldata = {}, int32_t func_start = 0)
{
    auto script = std::make_unique<ccScript>();
    script->scriptname = "bytecode";
    script->code = code;
    for (const auto &fixup : fixups)
    {
        script->fixups.push_back(fixup.CodeIndex);
        script->fixuptypes.push_back(fixup.Type);
    }
    script->globaldata = globaldata;
    script->exports.push_back("f");
    script->export_addr.push_back((EXPORT_FUNCTION << 24) | func_start);
    script->sectionNames.push_back("bytecode");
    script->sectionOffsets.push_back(0);
    return script;
}

bool Contains(const String &text, const char *part)
{
    return std::string(text.GetCStr()).find(part) != std::string::npos;
}

} // namespace

// Fixture for running hand-assembled scripts. The engine globals it touches, all restored in TearDown:
// - scriptExecutor: the engine's executor, replaced by a fresh one for each test
// - the script error state (cc_get_error), cleared before and after each test
// - RuntimeScript's link slots: each created script takes one until it is destroyed
class ScriptBytecode : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cc_clear_error();
        scriptExecutor = std::make_unique<ScriptExecutor>();
        scriptExecutor->SetExecTimeout(60000u, 0u, 0u);
    }

    void TearDown() override
    {
        _scripts.clear();
        scriptExecutor.reset();
        cc_clear_error();
    }

    const RuntimeScript *Create(const ccScript &compiled)
    {
        std::unique_ptr<RuntimeScript> script = RuntimeScript::Create(&compiled);
        if (!script)
            return nullptr;
        _scripts.push_back(std::move(script));
        return _scripts.back().get();
    }

    ScriptExecError Run(const std::vector<int32_t> &code, const std::vector<Fixup> &fixups = {},
                        const std::vector<uint8_t> &globaldata = {})
    {
        const RuntimeScript *script = Create(*Assemble(code, fixups, globaldata));
        if (!script)
        {
            ADD_FAILURE() << "RuntimeScript::Create: " << cc_get_error().ErrorString.GetCStr();
            return kScExecErr_Generic;
        }
        return scriptExecutor->Run(&_thread, script, "f", nullptr, 0);
    }

    // Runs code that must fail, and returns the error text
    String RunFails(const std::vector<int32_t> &code, const std::vector<Fixup> &fixups = {},
                    const std::vector<uint8_t> &globaldata = {})
    {
        EXPECT_EQ(kScExecErr_Generic, Run(code, fixups, globaldata));
        EXPECT_TRUE(cc_has_error());
        EXPECT_FALSE(scriptExecutor->IsRunning());
        return cc_get_error().ErrorString;
    }

    ScriptThread _thread{"Test"};
    std::vector<std::unique_ptr<RuntimeScript>> _scripts;
};

TEST_F(ScriptBytecode, ReturnsTheValueOfAx)
{
    ASSERT_EQ(kScExecErr_None, Run({SCMD_LITTOREG, SREG_AX, 42, SCMD_RET}));
    EXPECT_EQ(42, scriptExecutor->GetReturnValue());
}

TEST_F(ScriptBytecode, JumpsAndConditions)
{
    // ax = 3; bx = 0; loop: bx += 10; ax -= 1; if (ax != 0) goto loop; ax = bx
    const std::vector<int32_t> code = {
        SCMD_LITTOREG, SREG_AX, 3,       // 0
        SCMD_LITTOREG, SREG_BX, 0,       // 3
        SCMD_ADD,      SREG_BX, 10,      // 6: loop
        SCMD_SUB,      SREG_AX, 1,       // 9
        SCMD_JNZ,      -8,               // 12: back to 6 (pc after the op is 14)
        SCMD_REGTOREG, SREG_BX, SREG_AX, // 14
        SCMD_RET,                        // 17
    };
    ASSERT_EQ(kScExecErr_None, Run(code)) << cc_get_error().ErrorString.GetCStr();
    EXPECT_EQ(30, scriptExecutor->GetReturnValue());
}

TEST_F(ScriptBytecode, StackOverflowIsAnError)
{
    // push ax forever
    const String err = RunFails({SCMD_PUSHREG, SREG_AX, SCMD_JMP, -4});
    EXPECT_TRUE(Contains(err, "stack overflow")) << err.GetCStr();
}

TEST_F(ScriptBytecode, StackDataOverflowIsAnError)
{
    // a local variable larger than the whole stack
    const String err = RunFails({SCMD_ADD, SREG_SP, 8192, SCMD_RET});
    EXPECT_TRUE(Contains(err, "stack overflow, attempted to grow from 4 by 8192 bytes")) << err.GetCStr();
}

TEST_F(ScriptBytecode, StackUnderflowIsAnError)
{
    // pop the return address, then return
    const String err = RunFails({SCMD_POPREG, SREG_BX, SCMD_RET});
    EXPECT_TRUE(Contains(err, "stack underflow")) << err.GetCStr();
}

TEST_F(ScriptBytecode, UnbalancedStackIsAnError)
{
    // push a zero, which the return takes for the return address: the real one is left on the stack
    const String err = RunFails({SCMD_LITTOREG, SREG_AX, 0, SCMD_PUSHREG, SREG_AX, SCMD_RET});
    EXPECT_TRUE(Contains(err, "stack is not unwinded after function call, 4 bytes remain")) << err.GetCStr();
}

TEST_F(ScriptBytecode, OpcodeZeroIsAnError)
{
    const String err = RunFails({SCMD_LITTOREG, SREG_AX, 1, 0, SCMD_RET});
    EXPECT_TRUE(Contains(err, "instruction 0 is not implemented")) << err.GetCStr();
}

TEST_F(ScriptBytecode, OpcodeBeyondTheTableIsAnError)
{
#if DEBUG_CC_EXEC
    const String err = RunFails({CC_NUM_SCCMDS, 0, 0, 0, SCMD_RET});
    EXPECT_TRUE(Contains(err, "invalid instruction 77 found in code stream")) << err.GetCStr();
#else
    GTEST_SKIP() << "release builds do not check opcodes (DEBUG_CC_EXEC is off)";
#endif
}

TEST_F(ScriptBytecode, CodeEndingInsideAnInstructionIsAnError)
{
#if DEBUG_CC_EXEC
    const String err = RunFails({SCMD_LITTOREG, SREG_AX});
    EXPECT_TRUE(Contains(err, "unexpected end of code data")) << err.GetCStr();
#else
    GTEST_SKIP() << "release builds do not check the end of the code (DEBUG_CC_EXEC is off)";
#endif
}

TEST_F(ScriptBytecode, StartBeyondTheCodeIsAnError)
{
    const RuntimeScript *script = Create(*Assemble({SCMD_RET}, {}, {}, 5));
    ASSERT_NE(nullptr, script);
    EXPECT_EQ(kScExecErr_Generic, scriptExecutor->Run(&_thread, script, "f", nullptr, 0));
    EXPECT_TRUE(Contains(cc_get_error().ErrorString, "specified code offset is not valid"))
        << cc_get_error().ErrorString.GetCStr();
}

TEST_F(ScriptBytecode, ZeroMemoryAwayFromTheStackTailIsAnError)
{
    // MAR holds a plain number, not the stack tail
    const String err = RunFails({SCMD_LITTOREG, SREG_MAR, 0, SCMD_ZEROMEMORY, 4, SCMD_RET});
    EXPECT_TRUE(Contains(err, "stack tail address expected on SCMD_ZEROMEMORY")) << err.GetCStr();
}

TEST_F(ScriptBytecode, InvalidObjectHandleIsAnError)
{
    // a global variable holds a handle that no managed object has
    const std::vector<uint8_t> globals = {0x39, 0x30, 0, 0}; // 12345
    const String err =
        RunFails({SCMD_LITTOREG, SREG_MAR, 0, SCMD_MEMREADPTR, SREG_AX, SCMD_RET}, {{2, FIXUP_GLOBALDATA}}, globals);
    EXPECT_TRUE(Contains(err, "invalid handle 12345")) << err.GetCStr();
}

TEST_F(ScriptBytecode, CallingANumberIsAnError)
{
    const String err = RunFails({SCMD_LITTOREG, SREG_BX, 5, SCMD_CALLEXT, SREG_BX, SCMD_RET});
    EXPECT_TRUE(Contains(err, "invalid pointer type for function call")) << err.GetCStr();
}

TEST_F(ScriptBytecode, ExternalCallStackUnderflowIsAnError)
{
    const String err = RunFails({SCMD_SUBREALSTACK, 1, SCMD_RET});
    EXPECT_TRUE(Contains(err, "function callstack underflow")) << err.GetCStr();
}

TEST_F(ScriptBytecode, FixupOutsideTheCodeIsRefused)
{
    const auto compiled = Assemble({SCMD_LITTOREG, SREG_AX, 0, SCMD_RET}, {{100, FIXUP_GLOBALDATA}}, {0, 0, 0, 0});
    EXPECT_EQ(nullptr, Create(*compiled));
    EXPECT_TRUE(Contains(cc_get_error().ErrorString, "bad fixup at 0 (fixup type 1, bytecode pos 100"))
        << cc_get_error().ErrorString.GetCStr();
}

TEST_F(ScriptBytecode, UnknownFixupTypeIsRefused)
{
    const auto compiled = Assemble({SCMD_LITTOREG, SREG_AX, 0, SCMD_RET}, {{2, 42}});
    EXPECT_EQ(nullptr, Create(*compiled));
    EXPECT_TRUE(Contains(cc_get_error().ErrorString, "unknown fixup type: 42")) << cc_get_error().ErrorString.GetCStr();
}

TEST_F(ScriptBytecode, StringFixupPointsIntoTheStrings)
{
    // ax = "abc" (a literal); compare the first byte
    auto compiled = Assemble({SCMD_LITTOREG, SREG_MAR, 1, SCMD_MEMREADB, SREG_AX, SCMD_RET}, {{2, FIXUP_STRING}});
    compiled->strings = {'x', 'a', 'b', 'c', 0};
    const RuntimeScript *script = Create(*compiled);
    ASSERT_NE(nullptr, script);
    ASSERT_EQ(kScExecErr_None, scriptExecutor->Run(&_thread, script, "f", nullptr, 0))
        << cc_get_error().ErrorString.GetCStr();
    EXPECT_EQ('a', scriptExecutor->GetReturnValue());
}

TEST_F(ScriptBytecode, DestroyedScriptFreesItsLinkSlot)
{
    // no script outlives a test, so every link slot is free, the first one too
    ASSERT_EQ(nullptr, RuntimeScript::GetLinkedScript(0));
    const auto compiled = Assemble({SCMD_RET});
    std::unique_ptr<RuntimeScript> first = RuntimeScript::Create(compiled.get());
    ASSERT_TRUE(first);
    const int slot = first->GetLinkIndex();
    EXPECT_EQ(0, slot);
    EXPECT_EQ(first.get(), RuntimeScript::GetLinkedScript(slot));
    first.reset();
    EXPECT_EQ(nullptr, RuntimeScript::GetLinkedScript(slot));
    // the next script takes the lowest free slot, which is the same one
    std::unique_ptr<RuntimeScript> second = RuntimeScript::Create(compiled.get());
    ASSERT_TRUE(second);
    EXPECT_EQ(slot, second->GetLinkIndex());
}

TEST_F(ScriptBytecode, EveryLinkSlotIsAddressable)
{
    // the instance id in a CALLAS instruction has 10 bits, so 1024 scripts can be linked at once
    const auto compiled = Assemble({SCMD_RET});
    std::vector<std::unique_ptr<RuntimeScript>> scripts;
    for (int i = 0; i < 300; ++i)
    {
        scripts.push_back(RuntimeScript::Create(compiled.get()));
        ASSERT_TRUE(scripts.back());
        ASSERT_EQ(scripts.back().get(), RuntimeScript::GetLinkedScript(scripts.back()->GetLinkIndex())) << i;
    }
    EXPECT_EQ(299, scripts.back()->GetLinkIndex());
}
