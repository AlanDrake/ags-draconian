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
// The script binder (script/script_binder.h, game/script_api/bindings.h): each conversion of a script parameter
// and of a result that the function's signature selects, and each tag that overrides one.
#include <cstdint>
#include <string>

#include "game/script_api/bindings.h"
#include "gtest/gtest.h"
#include "script/dynobj/cc_ags_dynamic_object.h"
#include "script/runtimescriptvalue.h"
#include "script/script_binder.h"
#include "util/string.h"

namespace
{

// A type with a fixed manager, and a type that is its own manager
struct TestHandle
{
    int Id = 0;
};

struct TestObject final : CCBasicObject
{
    const char *GetType() override { return "TestObject"; }
    int Id = 0;
};

struct TestManager final : CCBasicObject
{
    const char *GetType() override { return "TestManager"; }
};

TestManager test_manager;
TestManager other_manager;
TestHandle test_handle;
TestObject test_object;

enum TestEnum
{
    kTestEnum_A = 3,
    kTestEnum_B = 7
};

// What the last bound function received
struct Received
{
    void *Self = nullptr;
    int Int = 0;
    float Float = 0.F;
    double Double = 0.0;
    bool Bool = false;
    uint8_t Byte = 0;
    TestEnum Enum = kTestEnum_A;
    const char *Text = nullptr;
    std::string Str;
    TestHandle *Handle = nullptr;
    int Calls = 0;
};
Received received;

RuntimeScriptValue Int(int32_t value)
{
    return RuntimeScriptValue().SetInt32(value);
}

RuntimeScriptValue Float(float value)
{
    return RuntimeScriptValue().SetFloat(value);
}

RuntimeScriptValue Pointer(void *ptr)
{
    return RuntimeScriptValue().SetScriptObject(ptr, &test_manager);
}

void TakeInts(int a, uint8_t b, TestEnum c)
{
    received.Int = a;
    received.Byte = b;
    received.Enum = c;
    ++received.Calls;
}

void TakeFloats(float a, double b)
{
    received.Float = a;
    received.Double = b;
}

void TakeBool(bool a)
{
    received.Bool = a;
}

void TakeInt(int a)
{
    received.Int = a;
}

void TakeText(const char *text, TestHandle *handle)
{
    received.Text = text;
    received.Handle = handle;
}

void TakeString(const AGS::Common::String &text)
{
    received.Str = text.GetCStr();
}

int ReturnInt()
{
    return -42;
}

bool ReturnTrue()
{
    return true;
}

int ReturnFive()
{
    return 5;
}

float ReturnFloat()
{
    return 2.5F;
}

TestHandle *ReturnHandle()
{
    return &test_handle;
}

TestObject *ReturnObject()
{
    return &test_object;
}

void *ReturnVoidPointer()
{
    return &test_object;
}

const char *ReturnText()
{
    return "text";
}

void Nothing()
{
    ++received.Calls;
}

int Method(TestHandle *self, int a)
{
    received.Self = self;
    received.Int = a;
    return a * 2;
}

void VoidMethod(TestObject *self)
{
    received.Self = self;
}

void SayAt(TestHandle *self, int x, const char *text)
{
    received.Self = self;
    received.Int = x;
    received.Str = text;
}

TestObject *Create(int id, const char *text)
{
    test_object.Id = id;
    received.Str = text;
    return &test_object;
}

} // namespace

template <>
struct ScriptManagerOf<TestHandle>
{
    static IScriptObject *Get() { return &test_manager; }
};

TEST(ScriptBinder, ReadsParametersAsTheirTypesSay)
{
    received = Received();
    const RuntimeScriptValue params[] = {Int(-7), Int(258), Int(kTestEnum_B)};
    const RuntimeScriptValue result = ScStaticWrapper<&TakeInts>()(params, 3);
    EXPECT_EQ(received.Int, -7);
    EXPECT_EQ(received.Byte, 2); // the int converted to uint8_t, as a cast does
    EXPECT_EQ(received.Enum, kTestEnum_B);
    EXPECT_EQ(result.Type, kScValInteger);
    EXPECT_EQ(result.IValue, 0);

    const RuntimeScriptValue floats[] = {Float(1.5F), Float(-0.25F)};
    ScStaticWrapper<&TakeFloats>()(floats, 2);
    EXPECT_EQ(received.Float, 1.5F);
    EXPECT_EQ(received.Double, -0.25);
}

TEST(ScriptBinder, ReadsBoolAsNotNull)
{
    received = Received();
    // GetAsBool: a value with a pointer and IValue 0 is true, which an int read would make false
    const RuntimeScriptValue pointer[] = {Pointer(&test_handle)};
    ScStaticWrapper<&TakeBool>()(pointer, 1);
    EXPECT_TRUE(received.Bool);
    const RuntimeScriptValue zero[] = {Int(0)};
    ScStaticWrapper<&TakeBool>()(zero, 1);
    EXPECT_FALSE(received.Bool);
}

TEST(ScriptBinder, ReadsPointersAndStrings)
{
    received = Received();
    const char *text = "hello";
    const RuntimeScriptValue params[] = {RuntimeScriptValue().SetStringLiteral(text), Pointer(&test_handle)};
    ScStaticWrapper<&TakeText>()(params, 2);
    EXPECT_EQ(received.Text, text);
    EXPECT_EQ(received.Handle, &test_handle);

    const RuntimeScriptValue string[] = {RuntimeScriptValue().SetStringLiteral("a String")};
    ScStaticWrapper<&TakeString>()(string, 1);
    EXPECT_EQ(received.Str, "a String");
}

TEST(ScriptBinder, ReturnsResultsAsTheirTypesSay)
{
    RuntimeScriptValue value = ScStaticWrapper<&ReturnInt>()(nullptr, 0);
    EXPECT_EQ(value.Type, kScValInteger);
    EXPECT_EQ(value.IValue, -42);

    value = ScStaticWrapper<&ReturnTrue>()(nullptr, 0);
    EXPECT_EQ(value.Type, kScValInteger);
    EXPECT_EQ(value.IValue, 1);

    value = ScStaticWrapper<&ReturnFloat>()(nullptr, 0);
    EXPECT_EQ(value.Type, kScValFloat);
    EXPECT_EQ(value.FValue, 2.5F);

    received = Received();
    value = ScStaticWrapper<&Nothing>()(nullptr, 0);
    EXPECT_EQ(received.Calls, 1);
    EXPECT_EQ(value.Type, kScValInteger);
    EXPECT_EQ(value.IValue, 0);
    EXPECT_EQ(value.Size, 4);
}

TEST(ScriptBinder, ReturnsObjectsWithTheirManager)
{
    RuntimeScriptValue value = ScStaticWrapper<&ReturnHandle>()(nullptr, 0);
    EXPECT_EQ(value.Type, kScValScriptObject);
    EXPECT_EQ(value.Ptr, &test_handle);
    EXPECT_EQ(value.ObjMgr, &test_manager);

    // An IScriptObject without a ScriptManagerOf is its own manager
    value = ScStaticWrapper<&ReturnObject>()(nullptr, 0);
    EXPECT_EQ(value.Ptr, &test_object);
    EXPECT_EQ(value.ObjMgr, static_cast<IScriptObject *>(&test_object));

    // A C string is a new script string
    value = ScStaticWrapper<&ReturnText>()(nullptr, 0);
    EXPECT_EQ(value.Type, kScValScriptObject);
    EXPECT_EQ(value.ObjMgr, static_cast<IScriptObject *>(&myScriptStringImpl));
}

TEST(ScriptBinder, TagsOverrideTheResult)
{
    received = Received();
    RuntimeScriptValue value = ScStaticWrapper<&ReturnInt, ScVoidResult>()(nullptr, 0);
    EXPECT_EQ(value.Type, kScValInteger);
    EXPECT_EQ(value.IValue, 0);

    value = ScStaticWrapper<&Nothing, ScUndefinedResult>()(nullptr, 0);
    EXPECT_EQ(received.Calls, 1);
    EXPECT_EQ(value.Type, kScValUndefined);
    EXPECT_EQ(value.Size, 0);

    value = ScStaticWrapper<&ReturnFive, ScBoolResult>()(nullptr, 0);
    EXPECT_EQ(value.IValue, 1);

    value = ScStaticWrapper<&ReturnVoidPointer, ScResultAs<TestObject>>()(nullptr, 0);
    EXPECT_EQ(value.Ptr, &test_object);
    EXPECT_EQ(value.ObjMgr, static_cast<IScriptObject *>(&test_object));

    value = ScStaticWrapper<&ReturnHandle, ScManagedBy<other_manager>>()(nullptr, 0);
    EXPECT_EQ(value.Ptr, &test_handle);
    EXPECT_EQ(value.ObjMgr, &other_manager);
}

TEST(ScriptBinder, TagsOverrideAParameter)
{
    received = Received();
    // ScBoolArg: an int parameter gets GetAsBool(), 1 for the pointer
    const RuntimeScriptValue pointer[] = {Pointer(&test_handle)};
    ScStaticWrapper<&TakeInt, ScBoolArg<0>>()(pointer, 1);
    EXPECT_EQ(received.Int, 1);
    // ScIntArg: a bool parameter gets IValue != 0, false for the pointer
    ScStaticWrapper<&TakeBool, ScIntArg<0>>()(pointer, 1);
    EXPECT_FALSE(received.Bool);
    const RuntimeScriptValue two[] = {Int(2)};
    ScStaticWrapper<&TakeBool, ScIntArg<0>>()(two, 1);
    EXPECT_TRUE(received.Bool);
}

TEST(ScriptBinder, MethodGetsTheObject)
{
    received = Received();
    const RuntimeScriptValue params[] = {Int(21)};
    const RuntimeScriptValue value = ScMethodWrapper<&Method>()(&test_handle, params, 1);
    EXPECT_EQ(received.Self, &test_handle);
    EXPECT_EQ(received.Int, 21);
    EXPECT_EQ(value.IValue, 42);

    ScMethodWrapper<&VoidMethod>()(&test_object, nullptr, 0);
    EXPECT_EQ(received.Self, &test_object);
}

TEST(ScriptBinder, RowsRegisterTheWrapperAndThePluginFunction)
{
    const ScFnRegister row = ScStatic<&ReturnInt>("Test::ReturnInt");
    EXPECT_STREQ(row.Name, "Test::ReturnInt");
    EXPECT_EQ(row.Fn.Type, kScValStaticFunction);
    EXPECT_EQ(row.Fn.SPfn, ScStaticWrapper<&ReturnInt>());
    EXPECT_EQ(row.PlFn.Type, kScValPluginFunction);
    EXPECT_EQ(row.PlFn.Ptr, reinterpret_cast<void *>(&ReturnInt));

    const ScFnRegister method = ScMethod<&Method, ScVoidResult>("Test::Method^1", &ReturnFive);
    EXPECT_EQ(method.Fn.Type, kScValObjectFunction);
    EXPECT_EQ(method.Fn.ObjPfn, (ScMethodWrapper<&Method, ScVoidResult>()));
    EXPECT_EQ(method.PlFn.Ptr, reinterpret_cast<void *>(&ReturnFive));
}

TEST(ScriptBinder, FormatsTheTextParameter)
{
    received = Received();
    const RuntimeScriptValue params[] = {Int(3), RuntimeScriptValue().SetStringLiteral("n=%d, %d"), Int(4), Int(5)};
    RuntimeScriptValue value = ScMethodFormatWrapper<&SayAt>()(&test_handle, params, 4);
    EXPECT_EQ(received.Self, &test_handle);
    EXPECT_EQ(received.Int, 3);
    EXPECT_EQ(received.Str, "n=4, 5");
    EXPECT_EQ(value.Type, kScValInteger);
    EXPECT_EQ(value.IValue, 0);

    const RuntimeScriptValue create[] = {Int(9), RuntimeScriptValue().SetStringLiteral("%d%%"), Int(50)};
    value = ScStaticFormatWrapper<&Create>()(create, 3);
    EXPECT_EQ(test_object.Id, 9);
    EXPECT_EQ(received.Str, "50%");
    EXPECT_EQ(value.Ptr, &test_object);
    EXPECT_EQ(value.ObjMgr, static_cast<IScriptObject *>(&test_object));

    const ScFnRegister row = ScStaticFormat<&Create>("Test::Create^102", &ReturnObject);
    EXPECT_EQ(row.Fn.SPfn, ScStaticFormatWrapper<&Create>());
    EXPECT_EQ(row.PlFn.Ptr, reinterpret_cast<void *>(&ReturnObject));
}
