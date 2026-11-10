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
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "util/string_utils.h"

using namespace AGS::Common;

TEST(StrUtil, Unescape)
{
    String s = "Text\\\\Text";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\\Text");

    s = "Text\\\\nText";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\\nText");

    s = "Text\\nText";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\nText");

    s = "Text\\rText";
    ASSERT_TRUE(StrUtil::Unescape(s) == "Text\rText");
}

TEST(StrUtil, UnescapeNumericSequences)
{
    // up to three octal digits, up to two hex digits, \e
    EXPECT_STREQ("AA\x1B!", StrUtil::Unescape("\\x41\\101\\e\\41").GetCStr());
    // the digits that would pass 255 are text
    EXPECT_STREQ(" 0 A1", StrUtil::Unescape("\\400 \\x411").GetCStr());
    // a code above 127 is written as UTF-8
    EXPECT_STREQ("caf\xC3\xA9 \xC3\xA9", StrUtil::Unescape("caf\\xe9 \\351").GetCStr());
    // a code of 0 and an x without digits are not sequences
    EXPECT_STREQ("0 x", StrUtil::Unescape("\\0 \\x").GetCStr());
    EXPECT_STREQ("\\0 \\x", StrUtil::Unescape("\\0 \\x", true).GetCStr());
}

TEST(StrUtil, UnescapeUnknownSequences)
{
    // an unknown sequence loses its backslash, or stays as it is when asked
    EXPECT_STREQ("a[b %q", StrUtil::Unescape("a\\[b \\%q").GetCStr());
    EXPECT_STREQ("a\\[b \\%q\n", StrUtil::Unescape("a\\[b \\%q\\n", true).GetCStr());
    // a backslash at the end stays
    EXPECT_STREQ("end\\", StrUtil::Unescape("end\\").GetCStr());
    EXPECT_STREQ("\n\\", StrUtil::Unescape("\\n\\", true).GetCStr());
}

TEST(StrUtil, Escape)
{
    EXPECT_STREQ("plain", StrUtil::Escape("plain").GetCStr());
    EXPECT_STREQ("Back\\\\slash \\\"quoted\\\"\\r\\n\\t\\001 end",
                 StrUtil::Escape("Back\\slash \"quoted\"\r\n\t\x01 end").GetCStr());
    // bytes of UTF-8 text stay as they are
    EXPECT_STREQ("caf\xC3\xA9", StrUtil::Escape("caf\xC3\xA9").GetCStr());
    // Unescape is its reverse
    const String text = "a\\b \"c\"\a\b\f\n\r\t\v\x01\x1F d";
    EXPECT_STREQ(text.GetCStr(), StrUtil::Unescape(StrUtil::Escape(text)).GetCStr());
}

TEST(StrUtil, EscapeUnescapedQuotes)
{
    EXPECT_STREQ("no quotes", StrUtil::EscapeUnescapedQuotes("no quotes").GetCStr());
    EXPECT_STREQ("a \\\"b\\\" c", StrUtil::EscapeUnescapedQuotes("a \"b\" c").GetCStr());
    // an escaped quote stays, a quote after an escaped backslash is escaped
    EXPECT_STREQ("a \\\"b\\\\\\\" c", StrUtil::EscapeUnescapedQuotes("a \\\"b\\\\\" c").GetCStr());
}

TEST(StrUtil, GetKeyValue)
{
    auto kv = StrUtil::GetKeyValue(" Font1 = Size=12 ");
    EXPECT_STREQ("Font1", kv.first.GetCStr());
    EXPECT_STREQ("Size=12", kv.second.GetCStr());
    // the given separator, not '='
    kv = StrUtil::GetKeyValue("PARSERWORD:5", ':');
    EXPECT_STREQ("PARSERWORD", kv.first.GetCStr());
    EXPECT_STREQ("5", kv.second.GetCStr());
    kv = StrUtil::GetKeyValue("NoSeparator");
    EXPECT_STREQ("NoSeparator", kv.first.GetCStr());
    EXPECT_TRUE(kv.second.IsEmpty());
}

// A wide string is UTF-16 where wchar_t has 16 bits (Windows): a character above U+FFFF is a pair
// of surrogates there, and one wchar elsewhere; the compiler encodes the wide literal the same way
TEST(StrUtil, ConvertsUtf8ToWideStringAndBack)
{
    const char *utf8 = "proj\xC3\xA9 \xD0\xB6\xD0\xB4 \xF0\x9F\x98\x80.ogg";
    const std::wstring wide = L"proj\u00E9 \u0436\u0434 \U0001F600.ogg";

    std::vector<wchar_t> wide_buf;
    EXPECT_EQ(StrUtil::ConvertUtf8ToWstr(utf8, wide_buf), wide.size());
    EXPECT_TRUE(wide == wide_buf.data());

    std::vector<char> utf8_buf;
    EXPECT_EQ(StrUtil::ConvertWstrToUtf8(wide.c_str(), utf8_buf), strlen(utf8));
    EXPECT_STREQ(utf8_buf.data(), utf8);
}

// The buffer's size includes the terminator; a character that does not fit is left out whole
TEST(StrUtil, WideStringConversionsStayInsideTheBuffer)
{
    wchar_t wide_buf[4] = {L'x', L'x', L'x', L'x'};
    EXPECT_EQ(StrUtil::ConvertUtf8ToWstr("abcdef", wide_buf, 3), 2u);
    EXPECT_TRUE(std::wstring(L"ab") == wide_buf);
    EXPECT_EQ(wide_buf[3], L'x');

    char utf8_buf[4] = {'x', 'x', 'x', 'x'};
    EXPECT_EQ(StrUtil::ConvertWstrToUtf8(L"a\u0436\u0436", utf8_buf, 3), 1u);
    EXPECT_STREQ(utf8_buf, "a");
    EXPECT_EQ(utf8_buf[3], 'x');
}

TEST(StrUtil, SplitPreservingDoublequoted)
{
    String s =
        "\"path with, comma\":\"value with, comma\",\"second path\":\"second value\",\"third, comma\":\"third value,\"";
    auto result = StrUtil::SplitPreservingDoublequoted(s, ',');
    EXPECT_EQ(3u, result.size());
    EXPECT_STREQ("\"path with, comma\":\"value with, comma\"", result[0].GetCStr());
    EXPECT_STREQ("\"second path\":\"second value\"", result[1].GetCStr());
    EXPECT_STREQ("\"third, comma\":\"third value,\"", result[2].GetCStr());

    // Additional test with extra commas
    s = ",,\"path with, comma\":\"value with, comma\",\"second path\":\"second value\",,\"third, comma\":\"third "
        "value,\",,";
    result = StrUtil::SplitPreservingDoublequoted(s, ',');
    EXPECT_EQ(8u, result.size());
    EXPECT_STREQ("", result[0].GetCStr());
    EXPECT_STREQ("", result[1].GetCStr());
    EXPECT_STREQ("\"path with, comma\":\"value with, comma\"", result[2].GetCStr());
    EXPECT_STREQ("\"second path\":\"second value\"", result[3].GetCStr());
    EXPECT_STREQ("", result[4].GetCStr());
    EXPECT_STREQ("\"third, comma\":\"third value,\"", result[5].GetCStr());
    EXPECT_STREQ("", result[6].GetCStr());
    EXPECT_STREQ("", result[7].GetCStr());
}
