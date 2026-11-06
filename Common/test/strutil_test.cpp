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
