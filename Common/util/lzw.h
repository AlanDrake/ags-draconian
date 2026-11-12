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
// LZW (un)compression functions.
//
//=============================================================================
#pragma once

#include "util/stream.h"

// Compresses all data read from lzw_in until its end, and writes the result to out.
// Returns false if the working memory could not be allocated.
bool lzwcompress(AGS::Common::Stream *lzw_in, AGS::Common::Stream *out);
// Expands lzw-compressed data from src to dst.
// the dst buffer should be large enough, or the uncompression will not be complete.
bool lzwexpand(const uint8_t *src, size_t src_sz, uint8_t *dst, size_t dst_sz);
