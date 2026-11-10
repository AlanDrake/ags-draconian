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
#include "util/string_utils.h"

#include <cctype>
#include <cerrno>
#include <cstring>
#include <cwchar>
#include <regex>
#include <string>
#include <type_traits>

#include "platform/platform.h"
#include "util/math.h"
#include "util/stream.h"
#include "util/string_compat.h"
#include "util/utf8.h"

using namespace AGS::Common;

namespace AGS
{
namespace Common
{

int StrUtil::LexographicalCompare(const String &s1, const String &s2, const char *locale_name)
{
    try
    {
        const auto loc = (locale_name && *locale_name) ? std::locale(locale_name) : std::locale();
        const auto &fac_c = std::use_facet<std::collate<char>>(loc);
        return fac_c.compare(s1.GetCStr(), s1.GetCStr() + s1.GetLength(), s2.GetCStr(), s2.GetCStr() + s2.GetLength());
    }
    catch (const std::runtime_error &)
    {
        return s1.Compare(s2);
    }
}

int StrUtil::LexographicalCompareNoCase(const String &s1, const String &s2, const char *locale_name)
{
    try
    {
        // TODO: find out if there's a way to avoid allocating lowercase strings,
        // and do collation over original strings instead (does C++ stdlib support that?)
        const String s1lower = s1.LowerUTF8();
        const String s2lower = s2.LowerUTF8();
        const auto loc = (locale_name && *locale_name) ? std::locale(locale_name) : std::locale();
        const auto &fac_c = std::use_facet<std::collate<char>>(loc);
        return fac_c.compare(s1lower.GetCStr(), s1lower.GetCStr() + s1lower.GetLength(), s2lower.GetCStr(),
                             s2lower.GetCStr() + s2lower.GetLength());
    }
    catch (const std::runtime_error &)
    {
        return Utf8::StrCmpNoCase(s1.GetCStr(), s2.GetCStr());
    }
}

std::unique_ptr<IStrCmp> StrUtil::GetStrCmpImplFor(bool unicode, bool nocase, const char *locale_name)
{
    // NOTE: locale_aware only makes sense in unicode mode
    const bool locale_aware = locale_name != nullptr;
    if (unicode)
    {
        if (nocase)
            return std::unique_ptr<IStrCmp>(locale_aware ? (IStrCmp *)new StrCmpLexographicalNoCase(locale_name)
                                                         : new StrCmpUtf8NoCase());
        else
            return std::unique_ptr<IStrCmp>(locale_aware ? (IStrCmp *)new StrCmpLexographical(locale_name)
                                                         : new StrCmpDirect());
    }
    else
    {
        if (nocase)
            return std::unique_ptr<IStrCmp>(new StrCmpNoCase());
        else
            return std::unique_ptr<IStrCmp>(new StrCmpDirect());
    }
}

String StrUtil::IntToString(int d)
{
    return String::FromFormat("%d", d);
}

inline int StringToIntImpl(const String &s, int radix, int def_val)
{
    if (s.IsEmpty())
        return def_val;
    char *stop_ptr;
    int val = strtol(s.GetCStr(), &stop_ptr, radix);
    return (stop_ptr == s.GetCStr() + s.GetLength()) ? val : def_val;
}

int StrUtil::StringToInt(const String &s, int def_val)
{
    return StringToIntImpl(s, 0, def_val);
}

int StrUtil::StringToIntHex(const String &s, int def_val)
{
    return StringToIntImpl(s, 16, def_val);
}

StrUtil::ConversionError StrUtil::StringToInt(const String &s, int &val, int def_val)
{
    val = def_val;
    if (s.IsEmpty())
        return StrUtil::kFailed;
    char *stop_ptr;
    errno = 0;
    long lval = strtol(s.GetCStr(), &stop_ptr, 0);
    if (stop_ptr != s.GetCStr() + s.GetLength())
        return StrUtil::kFailed;
    if (lval > INT_MAX || lval < INT_MIN || errno == ERANGE)
        return StrUtil::kOutOfRange;
    val = static_cast<int>(lval);
    return StrUtil::kNoError;
}

int64_t StrUtil::StringToInt64(const String &s, int64_t def_val)
{
    if (s.IsEmpty())
        return def_val;
    char *stop_ptr;
    int64_t val = strtoll(s.GetCStr(), &stop_ptr, 0);
    return (stop_ptr == s.GetCStr() + s.GetLength()) ? val : def_val;
}

float StrUtil::StringToFloat(const String &s, float def_val)
{
    if (s.IsEmpty())
        return def_val;
    char *stop_ptr;
    float val = strtof(s.GetCStr(), &stop_ptr);
    return (stop_ptr == s.GetCStr() + s.GetLength()) ? val : def_val;
}

String StrUtil::WildcardToRegex(const String &wildcard)
{
    // https://stackoverflow.com/questions/40195412/c11-regex-search-for-exact-string-escape
    // matches any characters that need to be escaped in RegEx
    std::regex esc{R"([-[\]{}()*+?.,\^$|#\s])"};
    std::string sanitized = std::regex_replace(wildcard.GetCStr(), esc, R"(\$&)");
    // convert (now escaped) wildcard "\\*" and "\\?" into ".*" and "." respectively
    String pattern(sanitized.c_str());
    pattern.Replace("\\*", ".*");
    pattern.Replace("\\?", ".");
    return pattern;
}

// Appends a character code given by a numeric escape; codes above 127 as UTF-8
static void AppendCharCode(std::string &out, int code)
{
    char buf[Utf8::UtfSz];
    out.append(buf, Utf8::SetChar(code, buf, sizeof(buf)));
}

String StrUtil::Unescape(const String &s, bool keep_unknown)
{
    const size_t at = s.FindChar('\\');
    if (at == String::NoIndex)
    {
        return s; // no unescaping necessary, return original string
    }
    const char *text = s.GetCStr();
    const size_t len = s.GetLength();
    std::string out;
    out.reserve(len); // a sequence is never shorter than what it stands for
    out.append(text, at);
    for (size_t i = at; i < len; ++i)
    {
        char c = text[i];
        if (c != '\\' || i + 1 >= len)
        {
            out.push_back(c); // a backslash at the end stays
            continue;
        }
        c = text[++i];
        switch (c)
        {
        case 'a': out.push_back('\a'); continue;
        case 'b': out.push_back('\b'); continue;
        case 'e': out.push_back(27); continue;
        case 'f': out.push_back('\f'); continue;
        case 'n': out.push_back('\n'); continue;
        case 'r': out.push_back('\r'); continue;
        case 't': out.push_back('\t'); continue;
        case 'v': out.push_back('\v'); continue;
        case '\\':
        case '\'':
        case '\"':
        case '\?': out.push_back(c); continue;
        default: break;
        }
        const size_t seq_at = i;
        int code = 0;
        if (c >= '0' && c < '8')
        {
            // up to 3 octal digits, max value 255
            code = c - '0';
            for (int n = 0; n < 2 && i + 1 < len; ++n)
            {
                const int digit = text[i + 1] - '0';
                if (digit < 0 || digit >= 8 || code * 8 + digit > 255)
                {
                    break;
                }
                code = code * 8 + digit;
                ++i;
            }
        }
        else if (c == 'x')
        {
            // up to 2 hex digits
            for (int n = 0; n < 2 && i + 1 < len && isxdigit(static_cast<unsigned char>(text[i + 1])); ++n)
            {
                const char h = static_cast<char>(tolower(static_cast<unsigned char>(text[++i])));
                code = code * 16 + (h <= '9' ? h - '0' : h - 'a' + 10);
            }
        }
        if (code > 0)
        {
            AppendCharCode(out, code);
            continue;
        }
        // not a sequence we know
        i = seq_at;
        if (keep_unknown)
        {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return String(out.data(), out.size());
}

String StrUtil::Escape(const String &s)
{
    String result;
    for (size_t i = 0; i < s.GetLength(); ++i)
    {
        const char c = s[i];
        switch (c)
        {
        case '\\': result.Append("\\\\"); continue;
        case '"': result.Append("\\\""); continue;
        case '\a': result.Append("\\a"); continue;
        case '\b': result.Append("\\b"); continue;
        case '\f': result.Append("\\f"); continue;
        case '\n': result.Append("\\n"); continue;
        case '\r': result.Append("\\r"); continue;
        case '\t': result.Append("\\t"); continue;
        case '\v': result.Append("\\v"); continue;
        default: break;
        }
        if (static_cast<unsigned char>(c) < ' ')
        {
            result.AppendFmt("\\%03o", static_cast<unsigned char>(c));
        }
        else
        {
            result.AppendChar(c);
        }
    }
    return result;
}

String StrUtil::EscapeUnescapedQuotes(const String &s)
{
    if (s.FindChar('"') == String::NoIndex)
    {
        return s;
    }
    String result;
    int backslashes = 0;
    for (size_t i = 0; i < s.GetLength(); ++i)
    {
        const char c = s[i];
        if (c == '"' && (backslashes % 2) == 0)
        {
            result.AppendChar('\\');
        }
        backslashes = (c == '\\') ? backslashes + 1 : 0;
        result.AppendChar(c);
    }
    return result;
}

String StrUtil::ReadString(Stream *in)
{
    size_t len = in->ReadInt32();
    if (len > 0)
        return String::FromStreamCount(in, len);
    return String();
}

void StrUtil::SkipString(Stream *in)
{
    size_t len = in->ReadInt32();
    in->Seek(len);
}

void StrUtil::WriteString(const String &s, Stream *out)
{
    size_t len = s.GetLength();
    len = std::min<size_t>(len, UINT32_MAX);
    out->WriteInt32(static_cast<uint32_t>(len));
    if (len > 0)
        out->Write(s.GetCStr(), len);
}

void StrUtil::WriteString(const char *cstr, Stream *out)
{
    size_t len = strlen(cstr);
    len = std::min<size_t>(len, UINT32_MAX);
    out->WriteInt32(static_cast<uint32_t>(len));
    if (len > 0)
        out->Write(cstr, len);
}

void StrUtil::WriteString(const char *cstr, size_t len, Stream *out)
{
    len = std::min<size_t>(len, UINT32_MAX);
    out->WriteInt32(static_cast<uint32_t>(len));
    if (len > 0)
        out->Write(cstr, len);
}

void StrUtil::WriteFixedString(const String &s, size_t len, Stream *out)
{
    if (!out || len == 0)
        return;

    // truncate and pad using raw bytes
    const String tmp = s.Left(len);
    if (!tmp.IsEmpty())
        out->Write(tmp.GetCStr(), tmp.GetLength());
    out->WriteByteCount(0, len - tmp.GetLength());
}

void StrUtil::WriteFixedString(const char *cstr, size_t len, Stream *out)
{
    WriteFixedString(String::Wrapper(cstr), len, out);
}


String StrUtil::ReadCStr(Stream *in, size_t buf_limit)
{
    return String::FromStream(in, buf_limit);
}

void StrUtil::ReadCStr(char *buf, Stream *in, size_t buf_limit)
{
    if (buf_limit == 0)
    {
        while (in->ReadByte() > 0)
            ;
        return;
    }

    auto ptr = buf;
    auto last = buf + buf_limit - 1;
    for (;;)
    {
        if (ptr >= last)
        {
            *ptr = 0;
            while (in->ReadByte() > 0)
                ; // must still read until 0
            break;
        }

        auto ichar = in->ReadByte();
        if (ichar <= 0)
        {
            *ptr = 0;
            break;
        }
        *ptr = static_cast<char>(ichar);
        ptr++;
    }
}

void StrUtil::ReadCStrCount(char *buf, Stream *in, size_t count)
{
    in->Read(buf, count);
    buf[count - 1] = 0; // for safety
}

std::string StrUtil::ReadCStrAsStdString(Stream *in)
{
    char buf[1024];
    ReadCStr(buf, in, sizeof(buf));
    return buf;
}

void StrUtil::WriteCStr(const char *cstr, Stream *out)
{
    if (cstr)
        out->Write(cstr, strlen(cstr) + 1);
    else
        out->WriteByte(0);
}

void StrUtil::WriteCStr(const String &s, Stream *out)
{
    out->Write(s.GetCStr(), s.GetLength() + 1);
}

void StrUtil::ReadStringMap(StringMap &map, Stream *in)
{
    size_t count = in->ReadInt32();
    for (size_t i = 0; i < count; ++i)
    {
        String key = StrUtil::ReadString(in);
        String value = StrUtil::ReadString(in);
        map.insert(std::make_pair(key, value));
    }
}

void StrUtil::WriteStringMap(const StringMap &map, Stream *out)
{
    out->WriteInt32(static_cast<uint32_t>(map.size()));
    for (const auto &kv : map)
    {
        StrUtil::WriteString(kv.first, out);
        StrUtil::WriteString(kv.second, out);
    }
}

std::pair<String, String> StrUtil::GetKeyValue(const String &s, char key_val_separator)
{
    String key, value;
    const auto at = s.FindChar(key_val_separator);
    if (at != String::NoIndex)
    {
        key = s.Left(at);
        key.Trim();
        value = s.Mid(at + 1);
        value.Trim();
    }
    else
    {
        key = s;
        key.Trim();
    }
    return std::make_pair(key, value);
}

std::vector<String> StrUtil::SplitPreservingDoublequoted(const String &s, char separator)
{
    if (s.IsEmpty())
    {
        return std::vector<String>{""};
    }
    if (separator == '\"')
    {
        return s.Split(separator);
    }

    std::vector<String> result;
    bool inside_quotes = false;
    size_t begin_at = 0u;
    for (size_t i = 0u; i < s.GetLength(); ++i)
    {
        char c = s[i];
        if (c == '\"')
        {
            inside_quotes = !inside_quotes;
        }
        else if (c == separator)
        {
            if (!inside_quotes)
            {
                result.push_back(s.Mid(begin_at, i - begin_at));
                begin_at = i + 1u;
            }
        }
    }
    // Append last section (it may be empty if separator was a last character)
    result.push_back(s.Mid(begin_at, s.GetLength() - begin_at));
    return result;
}

// Where wchar_t has 16 bits (Windows) wide strings are UTF-16: a character above U+FFFF is a pair
// of surrogates there, while UTF-8 has it as one sequence of four bytes.
#if WCHAR_MAX <= 0xFFFF
#define AGS_WCHAR_IS_UTF16 (1)
#else
#define AGS_WCHAR_IS_UTF16 (0)
#endif

size_t StrUtil::ConvertUtf8ToWstr(const char *mbstr, wchar_t *out_wcstr, size_t out_sz)
{
    if (out_sz == 0)
    {
        return 0; // no output space
    }
    size_t len = 0;
    while (*mbstr && (len + 1 < out_sz))
    {
        Utf8::Rune r;
        const size_t mb_sz = Utf8::GetChar(mbstr, Utf8::UtfSz, &r);
        if (mb_sz == 0)
        {
            break;
        }
#if AGS_WCHAR_IS_UTF16
        if (r > 0xFFFF)
        {
            if (len + 2 >= out_sz)
            {
                break; // no space for the pair
            }
            out_wcstr[len++] = static_cast<wchar_t>(0xD800 + ((r - 0x10000) >> 10));
            r = 0xDC00 + ((r - 0x10000) & 0x3FF);
        }
#endif
        out_wcstr[len++] = static_cast<wchar_t>(r);
        mbstr += mb_sz;
    }
    out_wcstr[len] = 0;
    return len;
}

size_t StrUtil::ConvertUtf8ToWstr(const char *mbstr, std::vector<wchar_t> &out_wcstr)
{
    // No more wchars than bytes: a sequence of four bytes makes a pair in UTF-16
    out_wcstr.resize(strlen(mbstr) + 1);
    return ConvertUtf8ToWstr(mbstr, out_wcstr.data(), out_wcstr.size());
}

size_t StrUtil::ConvertWstrToUtf8(const wchar_t *wcstr, char *out_mbstr, size_t out_sz)
{
    if (out_sz == 0)
    {
        return 0; // no output space
    }
    size_t len = 0;
    while (*wcstr)
    {
        // A code unit as the unsigned number it is: wchar_t is signed where it has 32 bits
        Utf8::Rune r = static_cast<Utf8::Rune>(static_cast<std::make_unsigned<wchar_t>::type>(*wcstr++));
#if AGS_WCHAR_IS_UTF16
        if ((r >= 0xD800) && (r <= 0xDBFF) && (*wcstr >= 0xDC00) && (*wcstr <= 0xDFFF))
        {
            r = 0x10000 + ((r - 0xD800) << 10) + (static_cast<Utf8::Rune>(*wcstr++) - 0xDC00);
        }
#endif
        const size_t mb_sz = Utf8::SetChar(r, out_mbstr + len, out_sz - len - 1);
        if (mb_sz == 0)
        {
            break; // no space left
        }
        len += mb_sz;
    }
    out_mbstr[len] = 0;
    return len;
}

size_t StrUtil::ConvertWstrToUtf8(const wchar_t *wcstr, std::vector<char> &out_mbstr)
{
    out_mbstr.resize(wcslen(wcstr) * Utf8::UtfSz + 1);
    return ConvertWstrToUtf8(wcstr, out_mbstr.data(), out_mbstr.size());
}

std::unique_ptr<char[]> StrUtil::Duplicate(const char *cstr)
{
    size_t len = strlen(cstr);
    std::unique_ptr<char[]> buf(new char[len + 1]);
    memcpy(buf.get(), cstr, len + 1);
    return buf;
}

std::unique_ptr<char[]> StrUtil::Substring(const char *cstr, size_t start, size_t length)
{
    size_t src_len = strlen(cstr);
    start = std::min(start, src_len);
    length = std::min(length, src_len - start);
    std::unique_ptr<char[]> buf(new char[length + 1]);
    memcpy(buf.get(), cstr + start, length);
    buf[length] = 0;
    return buf;
}

String StrUtil::ApplyTextDirection(const String &text, bool default_rtl)
{
    const Utf8::Rune LRE = 0x202A;
    const Utf8::Rune RLE = 0x202B;

    // We may only cut control characters, but not add any new ones,
    // therefore we allocate a buffer which is at least as large as the input.
    // NOTE: we don't allocate a buffer unless meet control chars (or RTL section);
    // in case whole string is LTR, we just return original string.
    char *dst_buf = nullptr;

    bool is_rtl = default_rtl;
    const char *src_ptr = text.GetCStr();
    const char *sec_begin = src_ptr;
    char *dst_ptr = nullptr;
    for (Utf8::Rune r = Utf8::RuneInvalid; r != 0;)
    {
        size_t c_len = Utf8::GetChar(src_ptr, Utf8::UtfSz, &r);

        if (is_rtl)
        {
            // Right-to-left section goes until a LRE control char or end of string
            if (r == LRE || r == 0)
            {
                if (!dst_buf)
                {
                    dst_buf = new char[text.GetLength() + 1];
                    dst_ptr = dst_buf;
                }

                // RTL section is copied char by char in reverse direction
                // TODO: pick this out to a separate helper function?
                const char *copy_from = src_ptr, *copy_to;
                do
                {
                    copy_to = copy_from;
                    copy_from = Utf8::BackOneChar(copy_to, sec_begin);
                    size_t copy_len = copy_to - copy_from;
                    memcpy(dst_ptr, copy_from, copy_len);
                    dst_ptr += copy_len;
                } while (copy_from != sec_begin);
                is_rtl = false;
                sec_begin = src_ptr + c_len;
            }
        }
        else
        {
            // Left-to-right section goes until a LRE control char or end of string
            if (r == RLE || r == 0)
            {
                // Optimization: if we reached end of string without seeing any control chars,
                // then just return the original string as-is.
                if (r == 0 && (sec_begin == text.GetCStr()))
                    return text;

                if (!dst_buf)
                {
                    dst_buf = new char[text.GetLength() + 1];
                    dst_ptr = dst_buf;
                }

                // LTR section is copied plainly as a sequence of bytes
                size_t sec_length = src_ptr - sec_begin;
                memcpy(dst_ptr, sec_begin, sec_length);
                dst_ptr += sec_length;
                is_rtl = true;
                sec_begin = src_ptr + c_len;
            }
        }
        src_ptr += c_len;
    }
    *dst_ptr = 0;
    return String(dst_buf);
}

static bool TryUTF8LocaleName(const String &try_name, String &got_name)
{
    try
    {
        auto locale = std::locale(try_name.GetCStr());
        got_name = locale.name();
        return true;
    }
    catch (const std::runtime_error &)
    {
    }
    return false;
}

String StrUtil::FindCompatibleUTF8LocaleName(const String &lang_name)
{
    if (lang_name.IsEmpty())
        return "";

    String locale_name = lang_name;
    locale_name.Replace('-', '_');
    // Try several suffix variants which are commonly supported by the C++ runtime libs
    String try_locale = String::FromFormat("%s.utf8", locale_name.GetCStr());
    String got_locale;
    if (TryUTF8LocaleName(try_locale, got_locale))
        return got_locale;
    try_locale = String::FromFormat("%s.utf-8", locale_name.GetCStr());
    if (TryUTF8LocaleName(try_locale, got_locale))
        return got_locale;
    try_locale = String::FromFormat("%s.UTF8", locale_name.GetCStr());
    if (TryUTF8LocaleName(try_locale, got_locale))
        return got_locale;
    try_locale = String::FromFormat("%s.UTF-8", locale_name.GetCStr());
    if (TryUTF8LocaleName(try_locale, got_locale))
        return got_locale;
    return "";
}

// References:
// https://en.wikipedia.org/wiki/Base64
// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
static const char *Base64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64_encode(const void *addr, size_t input_size)
{
    const uint8_t *bytes = static_cast<const uint8_t *>(addr);
    String out;
    int val = 0, valb = -6;
    for (size_t i = 0; i < input_size; ++i)
    {
        uint8_t c = bytes[i];
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.AppendChar(Base64Alphabet[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        out.AppendChar(Base64Alphabet[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.GetLength() % 4)
    {
        out.AppendChar('=');
    }
    return out;
}

void base64_decode(const String &input, std::vector<uint8_t> &out)
{
    out.clear();
    int value = 0, bits = -8;
    for (size_t i = 0; i < input.GetLength(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        const int digit = c >= 'A' && c <= 'Z'   ? c - 'A'
                          : c >= 'a' && c <= 'z' ? c - 'a' + 26
                          : c >= '0' && c <= '9' ? c - '0' + 52
                          : c == '+'             ? 62
                          : c == '/'             ? 63
                                                 : -1;
        if (digit < 0)
        {
            continue; // padding, line breaks
        }
        value = ((value << 6) | digit) & 0xFFFFFF;
        bits += 6;
        if (bits >= 0)
        {
            out.push_back(static_cast<uint8_t>((value >> bits) & 0xff));
            bits -= 8;
        }
    }
}

} // namespace Common
} // namespace AGS
