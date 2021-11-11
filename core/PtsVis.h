#pragma once

#include "SceneItem.h"
#include <mutex>

class VoxCube;
namespace sam
{
    struct DrawContext;
    struct DepthDataProps;
    struct DepthData;

    class PtsVis : public SceneItem
    {
        AABoxf GetBounds() const override;
        bgfxh<bgfx::UniformHandle> m_uparams;
        bgfx::UniformHandle m_vtexture;
        bgfx::UniformHandle m_dtexture;
        bgfxh<bgfx::TextureHandle> m_depthtex;
        bgfxh<bgfx::TextureHandle> m_vidtex;
        std::shared_ptr<VoxCube> m_voxelinst;
        std::vector<gmtl::Vec4f> m_pts;
        std::mutex m_ptsmtx;
        gmtl::Matrix44f m_alignMtx;
    public:
        PtsVis() {}
        void SetDepthData(const DepthData& depth);
    protected:

        void Initialize(DrawContext& nvg) override;
        void Draw(DrawContext& ctx) override;
    };
}
