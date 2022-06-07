#include "StdIncludes.h"
#include "PtsVis.h"
#include "DepthProps.h"
#include "DepthPts.h"
#include "Mesh.h"
#include "Application.h"

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

    void PtsVis::SetDepthData(const DepthData& depth)
    {
        m_ptsmtx.lock();
        m_pts = depth.pts;
        m_alignMtx = depth.alignMtx;
        m_ptsmtx.unlock();
    }

    void PtsVis::Draw(DrawContext& ctx)
    {
        if (m_pts.size() == 0)
            return;
        m_voxelinst = std::make_shared<VoxCube>();
        Matrix44f alignMtx;
        m_ptsmtx.lock();
        m_voxelinst->Create(m_pts);
        alignMtx = m_alignMtx;
        m_ptsmtx.unlock();
  
        Cube::init();
        Matrix44f m = ctx.m_mat * alignMtx * CalcMat();
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
