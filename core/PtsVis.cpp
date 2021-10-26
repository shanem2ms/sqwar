#include "StdIncludes.h"
#include "PtsVis.h"
#include "DepthPts.h"
#include "Mesh.h"

namespace sam
{

    AABoxf PtsVis::GetBounds() const
    {
        return AABoxf();
    }

    void PtsVis::Initialize(DrawContext& nvg)
    {
        m_uparams = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 1);
        m_vtexture = bgfx::createUniform("s_vid", bgfx::UniformType::Sampler);
        m_dtexture = bgfx::createUniform("s_depth", bgfx::UniformType::Sampler);
    }

    void PtsVis::SetDepthData(const unsigned char* vdata, size_t vsize, const std::vector<float> &depthData)
    {
        std::vector<gmtl::Vec4f> pts;
        GetDepthPointsWithColor(depthData, vdata, pts, 640, 480, 10000.0f);
        m_ptsmtx.lock();
        std::swap(m_pts, pts);
        m_ptsmtx.unlock();
    }

    void PtsVis::Draw(DrawContext& ctx)
    {
        if (m_pts.size() == 0)
            return;
        m_voxelinst = std::make_shared<VoxCube>();
        m_ptsmtx.lock();
        m_voxelinst->Create(m_pts);
        m_ptsmtx.unlock();
        /*
        size_t dsize = (depthData.size() - 16) * sizeof(float);
        const bgfx::Memory* m = bgfx::alloc(dsize);
        memcpy(m->data, depthData.data() + 16, dsize);
        m_depthtex =
            bgfx::createTexture2D(
                640, 480, false,
                1,
                bgfx::TextureFormat::Enum::R32F,
                BGFX_TEXTURE_NONE,
                m
            );
        const bgfx::Memory* mv = bgfx::alloc(vsize);
        memcpy(mv->data, vdata, vsize);
        m_vidtex =
            bgfx::createTexture2D(
                640, 480, false,
                1,
                bgfx::TextureFormat::Enum::RGBA8,
                BGFX_TEXTURE_NONE,
                mv
            );
            
        if (!bgfx::isValid(m_depthtex) || !bgfx::isValid(m_vidtex))
            return;*/
        Cube::init();
        Matrix44f m = ctx.m_mat * CalcMat();
        bgfx::setTransform(m.getData());
        // Set vertex and index buffer.
        bgfx::setVertexBuffer(0, Cube::vbh);
        bgfx::setIndexBuffer(Cube::ibh);
        
        //bgfx::setTexture(0, m_dtexture, m_depthtex);
        //bgfx::setTexture(1, m_vtexture, m_vidtex);
        Vec4f color = Vec4f(0.4f, 0.4f, 0.4f, 1);
        bgfx::setUniform(m_uparams, &color, 1);
        m_voxelinst->Use();
        bgfx::setInstanceDataBuffer(m_voxelinst->vbh, 0, m_voxelinst->verticesSize);

        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_MSAA
            | BGFX_STATE_BLEND_ALPHA;
        // Set render states.l
        bgfx::setState(state);
        bgfx::submit(ctx.m_curviewIdx, ctx.m_pgm);
    }

}
