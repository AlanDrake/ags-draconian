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
#include <memory>
#include <utility>
#include <vector>

#include "font/wfnfont.h"
#include "gtest/gtest.h"
#include "util/memorystream.h"
#include "util/stream.h"

using AGS::Common::kStream_Write;
using AGS::Common::Stream;
using AGS::Common::VectorStream;

namespace
{

// A character table entry of the extended format: character code, glyph offset.
using CharEntry = std::pair<uint32_t, uint32_t>;

// Builds an extended ("WGT Font File 2") font: the 32-byte header, then the
// glyph table, then the character table.
std::vector<uint8_t> MakeExtendedFont(uint32_t bits_per_pixel, uint32_t base_shade, const std::vector<uint8_t> &glyphs,
                                      const std::vector<CharEntry> &chars, uint32_t flags = 0)
{
    const uint32_t header_size = 8 * sizeof(uint32_t);
    const uint32_t glyph_table_at = 15 + header_size;
    const uint32_t char_table_at = glyph_table_at + static_cast<uint32_t>(glyphs.size());

    std::vector<uint8_t> data;
    Stream out(std::make_unique<VectorStream>(data, kStream_Write));
    out.Write("WGT Font File 2", 15);
    out.WriteInt32(header_size);
    out.WriteInt32(flags);
    out.WriteInt32(bits_per_pixel);
    out.WriteInt32(glyph_table_at);
    out.WriteInt32(static_cast<int32_t>(glyphs.size()));
    out.WriteInt32(char_table_at);
    out.WriteInt32(static_cast<int32_t>(chars.size() * 2 * sizeof(uint32_t)));
    out.WriteInt32(base_shade);
    out.Write(glyphs.data(), glyphs.size());
    for (const CharEntry &entry : chars)
    {
        out.WriteInt32(entry.first);
        out.WriteInt32(entry.second);
    }
    return data;
}

// Two 4-bit glyphs: a 2x2 one at offset 0 (shades 8 and 15, then 0 and 1) and
// a 1x1 one at offset 6 (shade 8).
const std::vector<uint8_t> kFourBitGlyphs = {2, 0, 2, 0, 0x8F, 0x01, 1, 0, 1, 0, 0x80};

WFNError ReadFont(WFNFont &font, const std::vector<uint8_t> &data)
{
    Stream in(std::make_unique<VectorStream>(data));
    return font.ReadFromFile(&in, static_cast<soff_t>(data.size()));
}

} // namespace

TEST(WFNFont, ExtendedFontReadsGlyphsSharedByCharacters)
{
    // The character table is not in code order and two characters share a glyph.
    const std::vector<uint8_t> data = MakeExtendedFont(4, 8, kFourBitGlyphs, {{'C', 0}, {'A', 0}, {'B', 6}});
    WFNFont font;
    ASSERT_EQ(kWFNErr_NoError, ReadFont(font, data));

    EXPECT_EQ(4, font.GetBitsPerPixel());
    EXPECT_EQ(8u, font.GetBaseShadeIndex());
    EXPECT_EQ(static_cast<size_t>('C') + 1, font.GetCharCount());
    EXPECT_EQ(2, font.GetHeight());
    EXPECT_EQ(1, font.GetBBox().Right);
    EXPECT_EQ(1, font.GetBBox().Bottom);

    const WFNChar &a = font.GetChar('A');
    ASSERT_TRUE(a.IsValid());
    EXPECT_EQ(2, a.Width);
    EXPECT_EQ(2, a.Height);
    EXPECT_EQ(4, a.BitsPerPixel);
    EXPECT_EQ(1u, a.GetRowByteCount()); // two 4-bit pixels per row
    EXPECT_EQ(0x8F, a.Data[0]);
    EXPECT_EQ(0x01, a.Data[1]);

    const WFNChar &b = font.GetChar('B');
    ASSERT_TRUE(b.IsValid());
    EXPECT_EQ(1, b.Width);
    EXPECT_EQ(1, b.Height);
    EXPECT_EQ(0x80, b.Data[0]);

    EXPECT_EQ(a.Data, font.GetChar('C').Data);
    EXPECT_FALSE(font.GetChar('D').IsValid());
    EXPECT_FALSE(font.GetChar(0).IsValid());
}

TEST(WFNFont, ExtendedFontRejectsUnknownFlags)
{
    const std::vector<uint8_t> data = MakeExtendedFont(4, 8, kFourBitGlyphs, {{'A', 0}}, 1);
    WFNFont font;
    EXPECT_EQ(kWFNErr_UnsupportedFlags, ReadFont(font, data));
}

TEST(WFNFont, ExtendedFontRejectsDuplicateCharacterCodes)
{
    const std::vector<uint8_t> data = MakeExtendedFont(4, 8, kFourBitGlyphs, {{'A', 0}, {'A', 6}});
    WFNFont font;
    EXPECT_EQ(kWFNErr_BadCharacterTable, ReadFont(font, data));
}

TEST(WFNFont, ExtendedFontSkipsGlyphOffsetPastTheTable)
{
    const std::vector<uint8_t> data = MakeExtendedFont(4, 8, kFourBitGlyphs, {{'A', 100}, {'B', 6}});
    WFNFont font;
    EXPECT_EQ(kWFNErr_HasBadCharacters, ReadFont(font, data));
    EXPECT_FALSE(font.GetChar('A').IsValid());
    EXPECT_TRUE(font.GetChar('B').IsValid());
}

TEST(WFNFont, ExtendedFontTruncatesGlyphShortOfPixels)
{
    // A 2x4 glyph needs 4 bytes of pixels, only 2 are stored.
    const std::vector<uint8_t> glyphs = {2, 0, 4, 0, 0x8F, 0x01};
    const std::vector<uint8_t> data = MakeExtendedFont(4, 8, glyphs, {{'A', 0}});
    WFNFont font;
    EXPECT_EQ(kWFNErr_HasBadCharacters, ReadFont(font, data));
    EXPECT_EQ(2, font.GetChar('A').Width);
    EXPECT_EQ(2, font.GetChar('A').Height);
}

TEST(WFNFont, ClassicFontReadsOneBitGlyphs)
{
    // Signature, 16-bit offset table address, one 3x2 glyph at offset 17,
    // then the offset table: codes 0 to 'A', only 'A' has a glyph.
    const uint16_t table_at = 17 + 6;
    std::vector<uint8_t> data;
    Stream out(std::make_unique<VectorStream>(data, kStream_Write));
    out.Write("WGT Font File  ", 15);
    out.WriteInt16(table_at);
    const uint8_t glyph[] = {3, 0, 2, 0, 0xA0, 0x40};
    out.Write(glyph, sizeof(glyph));
    for (int code = 0; code < 'A'; ++code)
        out.WriteInt16(0);
    out.WriteInt16(17);

    WFNFont font;
    ASSERT_EQ(kWFNErr_NoError, ReadFont(font, data));
    EXPECT_EQ(1, font.GetBitsPerPixel());
    EXPECT_EQ(static_cast<size_t>('A') + 1, font.GetCharCount());
    EXPECT_EQ(2, font.GetHeight());

    const WFNChar &a = font.GetChar('A');
    ASSERT_TRUE(a.IsValid());
    EXPECT_EQ(3, a.Width);
    EXPECT_EQ(2, a.Height);
    EXPECT_EQ(1, a.BitsPerPixel);
    EXPECT_EQ(1u, a.GetRowByteCount());
    EXPECT_EQ(0xA0, a.Data[0]);
    EXPECT_EQ(0x40, a.Data[1]);
    EXPECT_FALSE(font.GetChar(0).IsValid());
}

TEST(WFNFont, RejectsUnknownSignature)
{
    std::vector<uint8_t> data(64, 0);
    WFNFont font;
    EXPECT_EQ(kWFNErr_BadSignature, ReadFont(font, data));
}
