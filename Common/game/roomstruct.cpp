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
#include "game/roomstruct.h"

namespace AGS
{
namespace Common
{

RoomStruct::RoomStruct(const RoomData &room_data)
{
    *this = room_data;
}

RoomStruct::RoomStruct(RoomData &&room_data)
{
    *this = std::move(room_data);
}

RoomStruct::~RoomStruct()
{
    Free();
}

RoomStruct &RoomStruct::operator=(const RoomData &room_data)
{
    static_cast<RoomData &>(*this) = room_data;
    InitBitmaps();
    return *this;
}

RoomStruct &RoomStruct::operator=(RoomData &&room_data)
{
    static_cast<RoomData &>(*this) = std::move(room_data);
    InitBitmaps();
    return *this;
}

void RoomStruct::InitBitmaps()
{
    BgImages.resize(BgFrames.size());
    for (size_t i = 0; i < BgFrames.size(); ++i)
        if (BgFrames[i].GraphicBuf)
            BgImages[i].reset(new Bitmap(std::move(BgFrames[i].GraphicBuf)));
    if (HotspotMaskBuf)
        HotspotMask.reset(new Bitmap(std::move(HotspotMaskBuf)));
    if (RegionMaskBuf)
        RegionMask.reset(new Bitmap(std::move(RegionMaskBuf)));
    if (WalkAreaMaskBuf)
        WalkAreaMask.reset(new Bitmap(std::move(WalkAreaMaskBuf)));
    if (WalkBehindMaskBuf)
        WalkBehindMask.reset(new Bitmap(std::move(WalkBehindMaskBuf)));
}

void RoomStruct::Free()
{
    RoomData::Free();

    BgImages = {};
    HotspotMask.reset();
    RegionMask.reset();
    WalkAreaMask.reset();
    WalkBehindMask.reset();
}

Bitmap *RoomStruct::GetBackgroundImage(int index) const
{
    assert(index >= 0 && static_cast<uint8_t>(index) < BgImages.size());
    if (index < 0 || static_cast<uint8_t>(index) >= BgImages.size())
        return nullptr;

    return BgImages[index].get();
}

void RoomStruct::SetBackgroundImage(int index, std::unique_ptr<Bitmap> &&bmp, PALETTE *bgpal)
{
    assert(index >= 0 && index < MAX_ROOM_BGFRAMES);
    if (index < 0 || index >= MAX_ROOM_BGFRAMES)
        return;

    BgFrameCount = std::max<uint8_t>(BgFrameCount, static_cast<uint8_t>(index) + 1);
    if (BgFrames.size() < BgFrameCount)
        BgFrames.resize(BgFrameCount);
    if (BgImages.size() < BgFrameCount)
        BgImages.resize(BgFrameCount);
    BgImages[index] = std::move(bmp);
    if (bgpal)
    {
        BgFrames[index].IsPaletteShared = false;
        memcpy(BgFrames[index].Palette, *bgpal, sizeof(*bgpal));
    }
}

Bitmap *RoomStruct::GetMask(RoomAreaMask mask) const
{
    switch (mask)
    {
    case kRoomAreaHotspot: return HotspotMask.get();
    case kRoomAreaWalkBehind: return WalkBehindMask.get();
    case kRoomAreaWalkable: return WalkAreaMask.get();
    case kRoomAreaRegion: return RegionMask.get();
    default: return nullptr;
    }
}

void RoomStruct::SetMask(RoomAreaMask mask, std::unique_ptr<Bitmap> &&bmp)
{
    switch (mask)
    {
    case kRoomAreaHotspot: HotspotMask.reset(bmp.release()); break;
    case kRoomAreaWalkBehind: WalkBehindMask.reset(bmp.release()); break;
    case kRoomAreaWalkable: WalkAreaMask.reset(bmp.release()); break;
    case kRoomAreaRegion: RegionMask.reset(bmp.release()); break;
    default: assert(0); break;
    }
}

void RoomStruct::CopyMask(RoomAreaMask mask, const Bitmap *bitmap)
{
    Bitmap *room_mask = GetMask(mask);
    if (room_mask)
    {
        room_mask->Clear(0); // in case sizes are different
        room_mask->Blit(bitmap);
    }
}

} // namespace Common
} // namespace AGS
