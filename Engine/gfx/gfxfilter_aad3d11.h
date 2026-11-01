#pragma once

#include "gfx/gfxfilter_d3d11.h"

namespace AGS
{
namespace Engine
{
namespace D3D11
{

class AAD3D11GfxFilter : public D3D11GfxFilter
{
public:
    static const GfxFilterInfo FilterInfo;

    const GfxFilterInfo &GetInfo() const override;
    int GetSamplerStateForStandardSprite();
    void SetSamplerStateForStandardSprite(void *direct3dcontext);
    bool NeedToColourEdgeLines();
};

} // namespace D3D11
} // namespace Engine
} // namespace AGS
