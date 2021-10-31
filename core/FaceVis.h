#pragma once

#include "SceneItem.h"
#include <mutex>

struct PosTexcoordVertex;
namespace sam
{
    struct DrawContext;
    struct FaceDataProps;

    class FaceVis : public SceneItem
    {
        AABoxf GetBounds() const override;
        bgfxh<bgfx::UniformHandle> m_uparams;
        bgfxh<bgfx::VertexBufferHandle> m_vbh;
        bgfxh<bgfx::IndexBufferHandle> m_ibh;
        std::mutex m_ptsmtx;
        std::vector<float> m_vertices;
        std::vector<int16_t> m_indices;
        gmtl::Matrix44f  m_worldMat;
    public:
        FaceVis();
        ~FaceVis();
        void OnFaceData(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t> indices);
    protected:

        void Initialize(DrawContext& nvg) override;
        void Draw(DrawContext& ctx) override;
    };
}
