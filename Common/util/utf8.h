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
// UTF-8 utilities.
// Based on utf8 code from https://c9x.me/git/irc.git/tree/irc.c
//
//=============================================================================
#pragma once

#include <algorithm>
#include <cstring>

#include "platform/types.h"

namespace Utf8
{

typedef int32_t Rune;
const size_t UtfSz = 4;
const Rune RuneInvalid = 0xFFFD;

const unsigned char utfbyte[UtfSz + 1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
const unsigned char utfmask[UtfSz + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
const Rune utfmin[UtfSz + 1] = {0, 0, 0x80, 0x800, 0x10000};
const Rune utfmax[UtfSz + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};


inline size_t Validate(Rune *u, size_t i)
{
    if (*u < utfmin[i] || *u > utfmax[i] || (0xD800 <= *u && *u <= 0xDFFF))
        *u = RuneInvalid;
    for (i = 1; *u > utfmax[i]; ++i)
        ;
    return i;
}

inline Rune DecodeByte(unsigned char c, size_t *i)
{
    for (*i = 0; *i < UtfSz + 1; ++(*i))
        if ((c & utfmask[*i]) == utfbyte[*i])
            return c & ~utfmask[*i];
    return 0;
}

inline char EncodeByte(Rune u, size_t i)
{
    return utfbyte[i] | (u & ~utfmask[i]);
}

// Read a single utf8 codepoint from the c-string;
// returns codepoint's size in bytes (may be used to advance string pos)
inline size_t GetChar(const char *c, size_t clen, Rune *u)
{
    size_t i, j, len, type;
    Rune udecoded;
    if (!clen || !*c)
    {
        *u = 0;
        return 0;
    }
    *u = RuneInvalid;
    udecoded = DecodeByte(c[0], &len);
    if (len < 1 || len > UtfSz)
        return 1;
    for (i = 1, j = 1; i < clen && j < len; ++i, ++j)
    {
        udecoded = (udecoded << 6) | DecodeByte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    Validate(u, len);
    return len;
}

// Convert utf8 codepoint to the string representation and write to the buffer
inline size_t SetChar(Rune u, char *c, size_t clen)
{
    size_t len, i;
    len = Validate(&u, 0);
    if (len > UtfSz || len > clen)
        return 0;
    for (i = len - 1; i != 0; --i)
    {
        c[i] = EncodeByte(u, 0);
        u >>= 6;
    }
    c[0] = EncodeByte(u, len);
    return len;
}

// Takes a utf8 string pointer and advances it one char forward, unless it hits the end of the string
inline const char *ForwardOneChar(const char *c, const char *end)
{
    for (; c < end && ((*(++c) & 0xC0) == 0x80);)
        ;
    return c;
}

// Takes a utf8 string pointer and rolls it back one char, unless it hits the head of same string
inline const char *BackOneChar(const char *c, const char *front)
{
    for (; c > front && ((*(--c) & 0xC0) == 0x80);)
        ;
    return c;
}

// FIXME: move ToLower into the cpp file, it's likely that it is too big to inline

// Unicode-aware version of the ANSI tolower() function.
inline Rune ToLower(Rune c)
{
    if ((c >= 65 && c <= 90) || (c >= 192 && c <= 214) || (c >= 216 && c <= 222) || (c >= 913 && c <= 929) ||
        (c >= 931 && c <= 939) || (c >= 1040 && c <= 1071))
        return c + 32;
    if ((c >= 393 && c <= 394))
        return c + 205;
    if ((c >= 433 && c <= 434))
        return c + 217;
    if ((c >= 904 && c <= 906))
        return c + 37;
    if ((c >= 910 && c <= 911))
        return c + 63;
    if ((c >= 1025 && c <= 1036) || (c >= 1038 && c <= 1039))
        return c + 80;
    if ((c >= 1329 && c <= 1366) || (c >= 4256 && c <= 4293))
        return c + 48;
    if ((c >= 7944 && c <= 7951) || (c >= 7960 && c <= 7965) || (c >= 7976 && c <= 7983) || (c >= 7992 && c <= 7999) ||
        (c >= 8008 && c <= 8013) || (c >= 8040 && c <= 8047) || (c >= 8072 && c <= 8079) || (c >= 8088 && c <= 8095) ||
        (c >= 8104 && c <= 8111) || (c >= 8120 && c <= 8121) || (c >= 8152 && c <= 8153) || (c >= 8168 && c <= 8169))
        return c + -8;
    if ((c >= 8122 && c <= 8123))
        return c + -74;
    if ((c >= 8136 && c <= 8139))
        return c + -86;
    if ((c >= 8154 && c <= 8155))
        return c + -100;
    if ((c >= 8170 && c <= 8171))
        return c + -112;
    if ((c >= 8184 && c <= 8185))
        return c + -128;
    if ((c >= 8186 && c <= 8187))
        return c + -126;
    if ((c >= 8544 && c <= 8559))
        return c + 16;
    if ((c >= 9398 && c <= 9423))
        return c + 26;

    switch (c)
    {
    case 256:
    case 258:
    case 260:
    case 262:
    case 264:
    case 266:
    case 268:
    case 270:
    case 272:
    case 274:
    case 276:
    case 278:
    case 280:
    case 282:
    case 284:
    case 286:
    case 288:
    case 290:
    case 292:
    case 294:
    case 296:
    case 298:
    case 300:
    case 302:
    case 306:
    case 308:
    case 310:
    case 313:
    case 315:
    case 317:
    case 319:
    case 321:
    case 323:
    case 325:
    case 327:
    case 330:
    case 332:
    case 334:
    case 336:
    case 338:
    case 340:
    case 342:
    case 344:
    case 346:
    case 348:
    case 350:
    case 352:
    case 354:
    case 356:
    case 358:
    case 360:
    case 362:
    case 364:
    case 366:
    case 368:
    case 370:
    case 372:
    case 374:
    case 377:
    case 379:
    case 381:
    case 386:
    case 388:
    case 391:
    case 395:
    case 401:
    case 408:
    case 416:
    case 418:
    case 420:
    case 423:
    case 428:
    case 431:
    case 435:
    case 437:
    case 440:
    case 444:
    case 453:
    case 456:
    case 459:
    case 461:
    case 463:
    case 465:
    case 467:
    case 469:
    case 471:
    case 473:
    case 475:
    case 478:
    case 480:
    case 482:
    case 484:
    case 486:
    case 488:
    case 490:
    case 492:
    case 494:
    case 498:
    case 500:
    case 506:
    case 508:
    case 510:
    case 512:
    case 514:
    case 516:
    case 518:
    case 520:
    case 522:
    case 524:
    case 526:
    case 528:
    case 530:
    case 532:
    case 534:
    case 994:
    case 996:
    case 998:
    case 1000:
    case 1002:
    case 1004:
    case 1006:
    case 1120:
    case 1122:
    case 1124:
    case 1126:
    case 1128:
    case 1130:
    case 1132:
    case 1134:
    case 1136:
    case 1138:
    case 1140:
    case 1142:
    case 1144:
    case 1146:
    case 1148:
    case 1150:
    case 1152:
    case 1168:
    case 1170:
    case 1172:
    case 1174:
    case 1176:
    case 1178:
    case 1180:
    case 1182:
    case 1184:
    case 1186:
    case 1188:
    case 1190:
    case 1192:
    case 1194:
    case 1196:
    case 1198:
    case 1200:
    case 1202:
    case 1204:
    case 1206:
    case 1208:
    case 1210:
    case 1212:
    case 1214:
    case 1217:
    case 1219:
    case 1223:
    case 1227:
    case 1232:
    case 1234:
    case 1236:
    case 1238:
    case 1240:
    case 1242:
    case 1244:
    case 1246:
    case 1248:
    case 1250:
    case 1252:
    case 1254:
    case 1256:
    case 1258:
    case 1262:
    case 1264:
    case 1266:
    case 1268:
    case 1272:
    case 7680:
    case 7682:
    case 7684:
    case 7686:
    case 7688:
    case 7690:
    case 7692:
    case 7694:
    case 7696:
    case 7698:
    case 7700:
    case 7702:
    case 7704:
    case 7706:
    case 7708:
    case 7710:
    case 7712:
    case 7714:
    case 7716:
    case 7718:
    case 7720:
    case 7722:
    case 7724:
    case 7726:
    case 7728:
    case 7730:
    case 7732:
    case 7734:
    case 7736:
    case 7738:
    case 7740:
    case 7742:
    case 7744:
    case 7746:
    case 7748:
    case 7750:
    case 7752:
    case 7754:
    case 7756:
    case 7758:
    case 7760:
    case 7762:
    case 7764:
    case 7766:
    case 7768:
    case 7770:
    case 7772:
    case 7774:
    case 7776:
    case 7778:
    case 7780:
    case 7782:
    case 7784:
    case 7786:
    case 7788:
    case 7790:
    case 7792:
    case 7794:
    case 7796:
    case 7798:
    case 7800:
    case 7802:
    case 7804:
    case 7806:
    case 7808:
    case 7810:
    case 7812:
    case 7814:
    case 7816:
    case 7818:
    case 7820:
    case 7822:
    case 7824:
    case 7826:
    case 7828:
    case 7840:
    case 7842:
    case 7844:
    case 7846:
    case 7848:
    case 7850:
    case 7852:
    case 7854:
    case 7856:
    case 7858:
    case 7860:
    case 7862:
    case 7864:
    case 7866:
    case 7868:
    case 7870:
    case 7872:
    case 7874:
    case 7876:
    case 7878:
    case 7880:
    case 7882:
    case 7884:
    case 7886:
    case 7888:
    case 7890:
    case 7892:
    case 7894:
    case 7896:
    case 7898:
    case 7900:
    case 7902:
    case 7904:
    case 7906:
    case 7908:
    case 7910:
    case 7912:
    case 7914:
    case 7916:
    case 7918:
    case 7920:
    case 7922:
    case 7924:
    case 7926:
    case 7928: return c + 1;
    case 304: return c + -199;
    case 376: return c + -121;
    case 385: return c + 210;
    case 390: return c + 206;
    case 398: return c + 79;
    case 399: return c + 202;
    case 400: return c + 203;
    case 403: return c + 205;
    case 404: return c + 207;
    case 406:
    case 412: return c + 211;
    case 407: return c + 209;
    case 413: return c + 213;
    case 415: return c + 214;
    case 422:
    case 425:
    case 430: return c + 218;
    case 439: return c + 219;
    case 452:
    case 455:
    case 458:
    case 497: return c + 2;
    case 902: return c + 38;
    case 908: return c + 64;
    case 8025:
    case 8027:
    case 8029:
    case 8031: return c + -8;
    case 8124:
    case 8140:
    case 8188: return c + -9;
    case 8172: return c + -7;
    default: return c;
    }
}

inline int CStrCompareNoCase(const char *cstr1, const char *cstr2)
{
    if (!cstr1 && cstr2)
        return -1;
    else if (cstr1 && !cstr2)
        return 1;
    else if (!cstr1 && !cstr2)
        return 0;

    Rune r1, r2;
    do
    {
        cstr1 += GetChar(cstr1, Utf8::UtfSz, &r1);
        cstr2 += GetChar(cstr2, Utf8::UtfSz, &r2);
        int diff = ToLower(r1) - ToLower(r2);
        if (diff != 0)
            return diff;
    } while (r1 && r2);
    return 0;
}

// FIXME: move CStrToLower into the cpp file, it's unlikely that it may be inlined

// Convert a UTF-8 c-string to lowercase, writing a result into the provided buffer.
// If a nullptr passed instead of a buffer, then this function does not do any writing,
// only calculates the required destination buffer size.
inline size_t CStrToLower(const char *src, char *dst, size_t dst_sz)
{
    Rune r;
    if (dst)
    {
        const char *dst_begin = dst;
        do
        {
            src += GetChar(src, UtfSz, &r);
            r = ToLower(r);
            size_t clen = SetChar(r, dst, dst_sz);
            dst += clen;
            dst_sz -= clen;
        } while (r > 0 && r != RuneInvalid && dst_sz > 0u);
        return dst - dst_begin;
    }
    else
    {
        size_t dst_len = 0u;
        do
        {
            src += GetChar(src, UtfSz, &r);
            r = ToLower(r);
            dst_len += Validate(&r, 0u);
        } while (r > 0 && r != RuneInvalid);
        return dst_len;
    }
}

inline int StrCmpNoCase(const char *cstr1, const char *cstr2)
{
    for (;;)
    {
        Rune c1, c2;
        cstr1 += GetChar(cstr1, UtfSz, &c1);
        cstr2 += GetChar(cstr2, UtfSz, &c2);
        c1 = ToLower(c1);
        c2 = ToLower(c2);

        if (c1 != c2)
            return c1 - c2;

        if (!c1)
            return 0;
    }
}

} // namespace Utf8
