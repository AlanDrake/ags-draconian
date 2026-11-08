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
#include "data/data_helpers.h"

#include <vector>

namespace AGS
{
namespace Common
{

void ScanMacroTokens(const String &text, std::function<bool(const String &macro)> &fn_parse, bool allow_unclosed_macros)
{
    for (size_t scan_at = text.FindChar('@'); scan_at != String::NoIndex;)
    {
        const size_t macro_at = scan_at + 1;
        size_t macro_end = scan_at = text.FindChar('@', scan_at + 1);
        if (macro_end == String::NoIndex)
        {
            // Malformed macro string
            if (!allow_unclosed_macros)
                break;

            macro_end = text.GetLength();
        }
        if (macro_end == macro_at)
        {
            // Zero-length substring, ignore and continue from the last '@'
            continue;
        }

        // Pass macro to callback
        // TODO: this is where a string view class would come handy
        const size_t macro_len = macro_end - macro_at;
        if (!fn_parse(text.Mid(macro_at, macro_len)))
            continue; // no matching macro, ignore and continue from the last '@'

        // If macro was resolved, then search for the next @...@ pair
        scan_at = text.FindChar('@', macro_end + 1);
    }
}

String ResolveMacroTokens(const String &text, std::function<bool(const String &macro, String &result)> &fn_parse,
                          bool allow_unclosed_macros)
{
    String resolved_text;
    size_t text_at = 0u;
    for (size_t scan_at = text.FindChar('@'); scan_at != String::NoIndex;)
    {
        const size_t macro_at = scan_at + 1;
        size_t macro_end = scan_at = text.FindChar('@', scan_at + 1);
        if (macro_end == String::NoIndex)
        {
            // Malformed macro string
            if (!allow_unclosed_macros)
                break;

            macro_end = text.GetLength();
        }
        if (macro_end == macro_at)
        {
            // Zero-length substring, ignore and continue from the last '@'
            continue;
        }

        // Copy literal text (if there's any between macros)
        resolved_text.Append(text.Mid(text_at, macro_at - 1 - text_at));
        text_at = macro_at - 1;

        // Test which macro is it
        // TODO: this is where a string view class would come handy
        const size_t macro_len = macro_end - macro_at;
        String result;
        if (!fn_parse(text.Mid(macro_at, macro_len), result))
            continue; // no matching macro, continue from the last '@'

        // If macro was resolved, then append result and search for the next @...@ pair
        resolved_text.Append(result);
        text_at = macro_end + 1;
        scan_at = text.FindChar('@', macro_end + 1);
    }

    // Copy trailing literal text (if there's any)
    resolved_text.Append(text.Mid(text_at));
    return resolved_text;
}

const char *EncryptPassword = "Avis Durgan";
// The inversed password used to emulate a opposite math operation ('+' vs '-') when en/de-crypting
const char EncryptPasswordInversed[12] = {(char)(-'A'), (char)(-'v'), (char)(-'i'), (char)(-'s'),
                                          (char)(-' '), (char)(-'D'), (char)(-'u'), (char)(-'r'),
                                          (char)(-'g'), (char)(-'a'), (char)(-'n'), 0};

void DecryptText(char *buf, size_t buf_sz, bool inverse)
{
    const char *password = inverse ? EncryptPasswordInversed : EncryptPassword;
    for (size_t i = 0, i_psw = 0; i < buf_sz; ++i, i_psw = (i_psw == 10 ? 0 : i_psw + 1))
    {
        buf[i] -= password[i_psw];
        if (buf[i] == 0)
            break;
    }
}

String ReadStringDecrypt(Stream *in, bool inverse)
{
    std::vector<char> dec_buf;
    return ReadStringDecrypt(in, dec_buf, inverse);
}

String ReadStringDecrypt(Stream *in, std::vector<char> &dec_buf, bool inverse)
{
    size_t len = in->ReadInt32();
    dec_buf.resize(len + 1);
    in->Read(dec_buf.data(), len);
    DecryptText(dec_buf.data(), len, inverse);
    dec_buf.back() = 0; // null terminate in case read string does not have one
    return String(dec_buf.data());
}

void SkipStringDecrypt(Stream *in)
{
    size_t len = in->ReadInt32();
    in->Seek(len);
}

void EncryptText(char *buf, size_t buf_sz, bool inverse)
{
    const char *password = inverse ? EncryptPasswordInversed : EncryptPassword;
    for (size_t i = 0, i_psw = 0; i < buf_sz; ++i, i_psw = (i_psw == 10 ? 0 : i_psw + 1))
    {
        const char src_c = buf[i];
        buf[i] += password[i_psw];
        if (src_c == 0)
            break;
    }
}

void WriteStringEncrypt(Stream *out, const char *s, bool inverse)
{
    size_t len = strlen(s) + 1;
    out->WriteInt32(len);
    std::vector<char> buf(len);
    std::copy(s, s + len, buf.data());
    EncryptText(buf.data(), buf.size(), inverse);
    out->Write(buf.data(), len);
}

} // namespace Common
} // namespace AGS
