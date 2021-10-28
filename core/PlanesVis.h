#pragma once

#include "SceneItem.h"
#include <mutex>

struct PosTexcoordVertex;
namespace sam
{
    struct DrawContext;
    struct DepthDataProps;

    class PlanesVis : public SceneItem
    {
        AABoxf GetBounds() const override;
        bgfxh<bgfx::UniformHandle> m_uparams;
        bgfxh<bgfx::VertexBufferHandle> m_vbh;
        std::vector<PosTexcoordVertex> m_pts;
        std::mutex m_ptsmtx;
    public:
        PlanesVis();
        ~PlanesVis();
        void SetDepthData(const unsigned char* vdata, size_t vsize,
            const std::vector<float>& depthData, const DepthDataProps &props);
    protected:

        void Initialize(DrawContext& nvg) override;
        void Draw(DrawContext& ctx) override;
    };
}
