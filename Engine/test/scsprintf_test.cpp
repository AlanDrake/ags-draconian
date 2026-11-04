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
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "ac/game_version.h"
#include "gtest/gtest.h"
#include "script/runtimescriptvalue.h"
#include "script/script_api.h"

using namespace AGS::Common;

void cc_error(const char *, ...)
{
    // do nothing
}

void debug_script_event(MessageType mt, const String &msg)
{
    // do nothing
}

size_t ScriptVSprintf__(char *buffer, size_t buf_length, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t res = ScriptVSprintf(buffer, buf_length, format, args);
    va_end(args);
    return res;
}

const size_t STD_BUFFER_SIZE = 1024;

TEST(ScSprintf, ScSprintf)
{
    const int argi = 123;
    const float argf = 0.456F;
    const char *argcc = "string literal";
    RuntimeScriptValue params[10];
    params[0].SetInt32(argi);
    params[1].SetFloat(argf);
    params[2].SetStringLiteral(argcc);

    char ScSfBuffer[STD_BUFFER_SIZE];
    //
    // Called-from-script variant
    //
    // Correct format, extra placeholder
    size_t result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE,
                                  "testing ScriptSprintf:\nThis is int: %10d\nThis is float: %.4f\nThis is string: "
                                  "'%s'\nThis placeholder will be ignored: %d",
                                  params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "testing ScriptSprintf:\nThis is int:        123\nThis is float: 0.4560\nThis is "
                                   "string: 'string literal'\nThis placeholder will be ignored: %d") == 0);
    ASSERT_EQ(result, 138);

    // Literal percent sign
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%d%%", params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "123%") == 0);
    ASSERT_EQ(result, 4);
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "123%%", nullptr, 0);
    ASSERT_TRUE(strcmp(ScSfBuffer, "123%") == 0);
    ASSERT_EQ(result, 4);
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%%5d%%0.5f%%s", params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "%5d%0.5f%s") == 0);
    ASSERT_EQ(result, 10);

    // Invalid format
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%zzzzzz", params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "%zzzzzz") == 0);
    ASSERT_EQ(result, 7);
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%12.34%d", params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "%12.34%d") == 0);
    ASSERT_EQ(result, 8);

    // Not enough arguments
    result = ScriptSprintf(ScSfBuffer, STD_BUFFER_SIZE, "%5d%0.5f%s", params, 0);
    ASSERT_TRUE(strcmp(ScSfBuffer, "%5d%0.5f%s") == 0);
    ASSERT_EQ(result, 10);

    // Not enough buffer space
    result = ScriptSprintf(ScSfBuffer, 9, "12345678%d", params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "12345678") == 0);
    ASSERT_EQ(result, 11);
    result = ScriptSprintf(ScSfBuffer, 11, "12345678%d", params, 3);
    ASSERT_TRUE(strcmp(ScSfBuffer, "1234567812") == 0);
    ASSERT_EQ(result, 11);

    // Not enough buffer space and not enough params
    result = ScriptSprintf(ScSfBuffer, 10, "12345678%d", params, 0);
    ASSERT_TRUE(strcmp(ScSfBuffer, "12345678%") == 0);
    ASSERT_EQ(result, 10);
    result = ScriptSprintf(ScSfBuffer, 11, "12345678%d", params, 0);
    ASSERT_TRUE(strcmp(ScSfBuffer, "12345678%d") == 0);
    ASSERT_EQ(result, 10);

    // Test null string pointer
    params[0].SetStringLiteral(nullptr);
    result = ScriptSprintf(ScSfBuffer, 10, "A%sB", params, 1);
    ASSERT_TRUE(strcmp(ScSfBuffer, "A(null)B") == 0);
    ASSERT_EQ(result, 8);

    // Test invalid argument type
    params[0].SetInt32(1);
    result = ScriptSprintf(ScSfBuffer, 14, "A%sB", params, 1);
    ASSERT_TRUE(strcmp(ScSfBuffer, "A(undefined)B") == 0);
    ASSERT_EQ(result, 13);
    //
    // Called-from-plugin variant
    // Note that since this is variadic function, number of parameters must
    // always be equal or higher than number of placeholders in buffer string.
    //
    // Correct format, matching number of arguments
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE,
                              "testing ScriptVSprintf:\nThis is int: %10d\nThis is float: %.4f\nThis is string: '%s'\n",
                              argi, argf, argcc);
    ASSERT_TRUE(strcmp(ScSfBuffer, "testing ScriptVSprintf:\nThis is int:        123\nThis is float: 0.4560\nThis is "
                                   "string: 'string literal'\n") == 0);
    ASSERT_EQ(result, 103);

    // Literal percent sign
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%d%%", argi);
    ASSERT_TRUE(strcmp(ScSfBuffer, "123%") == 0);
    ASSERT_EQ(result, 4);
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "123%%");
    ASSERT_TRUE(strcmp(ScSfBuffer, "123%") == 0);
    ASSERT_EQ(result, 4);
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%%5d%%0.5f%%s");
    ASSERT_TRUE(strcmp(ScSfBuffer, "%5d%0.5f%s") == 0);
    ASSERT_EQ(result, 10);

    // Invalid format
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%zzzzzz", argi, argf, argcc);
    ASSERT_TRUE(strcmp(ScSfBuffer, "%zzzzzz") == 0);
    ASSERT_EQ(result, 7);
    result = ScriptVSprintf__(ScSfBuffer, STD_BUFFER_SIZE, "%12.34%d", argi, argf, argcc);
    ASSERT_TRUE(strcmp(ScSfBuffer, "%12.34%d") == 0);
    ASSERT_EQ(result, 8);

    // Not enough buffer space
    result = ScriptVSprintf__(ScSfBuffer, 9, "12345678%d", argi, argf, argcc);
    ASSERT_TRUE(strcmp(ScSfBuffer, "12345678") == 0);
    ASSERT_EQ(result, 11);
    result = ScriptVSprintf__(ScSfBuffer, 11, "12345678%d", argi, argf, argcc);
    ASSERT_TRUE(strcmp(ScSfBuffer, "1234567812") == 0);
    ASSERT_EQ(result, 11);
    // Test null string pointer
    result = ScriptVSprintf__(ScSfBuffer, 10, "A%sB", NULL);
    ASSERT_TRUE(strcmp(ScSfBuffer, "A(null)B") == 0);
    ASSERT_EQ(result, 8);
}

// Formats with script arguments into a std::string, through a buffer of the given size
std::string ScFormat(const char *format, const std::vector<RuntimeScriptValue> &args, size_t buf_size = STD_BUFFER_SIZE)
{
    std::vector<char> buf(buf_size);
    ScriptSprintf(buf.data(), buf.size(), format, args.data(), static_cast<int32_t>(args.size()));
    return buf.data();
}

RuntimeScriptValue Int(int value)
{
    return RuntimeScriptValue().SetInt32(value);
}

RuntimeScriptValue Float(float value)
{
    return RuntimeScriptValue().SetFloat(value);
}

RuntimeScriptValue Str(const char *value)
{
    return RuntimeScriptValue().SetStringLiteral(value);
}

TEST(ScSprintf, IntegerConversionsAndFlags)
{
    EXPECT_EQ("42 -42 42", ScFormat("%d %i %u", {Int(42), Int(-42), Int(42)}));
    EXPECT_EQ("4294967295", ScFormat("%u", {Int(-1)}));
    EXPECT_EQ("ff FF 0xff 377", ScFormat("%x %X %#x %o", {Int(255), Int(255), Int(255), Int(255)}));
    EXPECT_EQ("[   42][42   ][00042][+42][ 42]",
              ScFormat("[%5d][%-5d][%05d][%+d][% d]", {Int(42), Int(42), Int(42), Int(42), Int(42)}));
    EXPECT_EQ("[  007]", ScFormat("[%5.3d]", {Int(7)}));
    EXPECT_EQ("-2147483648 2147483647", ScFormat("%d %d", {Int(INT32_MIN), Int(INT32_MAX)}));
}

TEST(ScSprintf, FloatConversions)
{
    EXPECT_EQ("1.500000 -0.25", ScFormat("%f %.2f", {Float(1.5f), Float(-0.25f)}));
    EXPECT_EQ("1.25e+02 1.25E+02", ScFormat("%.2e %.2E", {Float(125.f), Float(125.f)}));
    EXPECT_EQ("0.0001 1e+10", ScFormat("%g %g", {Float(0.0001f), Float(1e10f)}));
    EXPECT_EQ("[   3.1]", ScFormat("[%6.1f]", {Float(3.14159f)}));
}

TEST(ScSprintf, Characters)
{
    EXPECT_EQ("[A]", ScFormat("[%c]", {Int('A')}));
    // characters above ASCII are written in UTF-8 (the engine's text mode in a Unicode game)
    EXPECT_EQ("[\xC3\xA9]", ScFormat("[%c]", {Int(0xE9)}));
    EXPECT_EQ("[\xE2\x98\xBA]", ScFormat("[%c]", {Int(0x263A)}));
}

TEST(ScSprintf, StringsWithWidthAndPrecision)
{
    EXPECT_EQ("[  abc][abc  ][ab]", ScFormat("[%5s][%-5s][%.2s]", {Str("abc"), Str("abc"), Str("abc")}));
    // UTF-8 text is copied as bytes
    EXPECT_EQ("[\xC3\xA9t\xC3\xA9]", ScFormat("[%s]", {Str("\xC3\xA9t\xC3\xA9")}));
}

TEST(ScSprintf, ExtraArgumentsAreIgnored)
{
    EXPECT_EQ("1 2", ScFormat("%d %d", {Int(1), Int(2), Int(3), Str("unused")}));
}

TEST(ScSprintf, CountingPassGivesThePrintedLength)
{
    const RuntimeScriptValue args[] = {Int(12345), Float(2.5f), Str("text")};
    const char *format = "value %d, %.3f and '%s' %%";
    const size_t counted = ScriptSprintf(nullptr, 0u, format, args, 3);
    char buf[STD_BUFFER_SIZE];
    const size_t printed = ScriptSprintf(buf, STD_BUFFER_SIZE, format, args, 3);
    EXPECT_EQ(counted, printed);
    EXPECT_EQ(strlen(buf), printed);
    EXPECT_STREQ("value 12345, 2.500 and 'text' %", buf);
}

TEST(ScSprintf, VectorVariantGrowsToTheResult)
{
    // longer than any fixed buffer used by the engine for formatting
    const std::string long_text(5000, 'x');
    const RuntimeScriptValue args[] = {Str(long_text.c_str()), Int(7)};
    std::vector<char> buf;
    const char *result = ScriptSprintf(buf, "<%s|%d>", args, 2);
    EXPECT_EQ("<" + long_text + "|7>", std::string(result));
}

TEST(ScSprintf, OverlongPlaceholderIsCopiedAsText)
{
    // the placeholder does not fit the parser's buffer: it is printed as it is, and the argument
    // is left for the next placeholder
    const std::string zeros(40, '0');
    const std::string format = "%" + zeros + "d %d";
    EXPECT_EQ(format.substr(0, format.size() - 2) + "5", ScFormat(format.c_str(), {Int(5)}));
}

TEST(ScSprintf, ArgumentPointingToTheOutputBufferIsRefused)
{
    char buf[64] = "previous";
    const RuntimeScriptValue args[] = {RuntimeScriptValue().SetStringLiteral(buf)};
    EXPECT_EQ(0u, ScriptSprintf(buf, sizeof(buf), "%s", args, 1));
}

TEST(ScSprintf, TruncatedOutputIsTerminated)
{
    char buf[6];
    const RuntimeScriptValue args[] = {Str("abcdefgh")};
    // the result length is the full one, the buffer gets what fits
    EXPECT_EQ(10u, ScriptSprintf(buf, sizeof(buf), "[%s]", args, 1));
    EXPECT_STREQ("[abcd", buf);
    EXPECT_EQ(8u, ScriptSprintf(buf, 1, "%s", args, 1));
    EXPECT_STREQ("", buf);
}
