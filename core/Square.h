#pragma once

#include "SceneItem.h"

class VoxCube;
namespace sam
{
    struct DrawContext;


    class Square : public SceneItem
    {
        AABoxf GetBounds() const override;
        bgfxh<bgfx::UniformHandle> m_uparams;
        bgfx::UniformHandle m_vtexture;
        bgfx::UniformHandle m_dtexture;
        bgfxh<bgfx::TextureHandle> m_depthtex;
        bgfxh<bgfx::TextureHandle> m_vidtex;
        std::shared_ptr<VoxCube> m_voxelinst;
    public:
        Square() {}
        void SetDepthData(const unsigned char* vdata, size_t vsize,
            const std::vector<float>& depthData);
    protected:

        void Initialize(DrawContext& nvg) override;
        void Draw(DrawContext& ctx) override;
    };
}
