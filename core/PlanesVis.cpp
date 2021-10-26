#include "StdIncludes.h"
#include "PlanesVis.h"
#include "DepthPts.h"
#include "Mesh.h"
#include "Engine.h"

namespace sam
{

    PlanesVis::PlanesVis()
    {}
    PlanesVis::~PlanesVis()
    {}

    AABoxf PlanesVis::GetBounds() const
    {
        return AABoxf();
    }

    void PlanesVis::Initialize(DrawContext& nvg)
    {
        m_uparams = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 1);
    }

    void PlanesVis::SetDepthData(const unsigned char* vdata, size_t vsize, const std::vector<float> &depthData)
    {
        std::vector<Point3f> outCoords, outTexCoords;
        outCoords.resize(1 << 16);
        outTexCoords.resize(1 << 16);
        int count = 0;
        std::vector<gmtl::Point3f> pts3;
        {
            std::vector<gmtl::Vec4f> pts;
            GetDepthPoints(depthData, vdata, pts, 640, 480);
            pts3.resize(pts.size());
            auto itpt1 = pts.begin();
            auto itpt2 = pts3.begin();
            for (; itpt1 != pts.end(); ++itpt1, ++itpt2)
            {
                memcpy(&(*itpt2), &(*itpt1), sizeof(gmtl::Point3f));
            }
        }
        DepthMakePlanes(pts3.data(), outCoords.data(), outTexCoords.data(), outTexCoords.size(), &count,
            640, 480);
        std::vector<PosTexcoordVertex> postx;
        postx.resize(count);
        auto itcoords = outCoords.begin();
        auto ittex = outTexCoords.begin();
        auto itptx = postx.begin();
        for (; itptx != postx.end(); ++itcoords, ++ittex, ++itptx)
        {
            memcpy(&(*itptx), &(*itcoords), sizeof(float) * 3);
            memcpy(&(itptx->m_u), &(*ittex), sizeof(float) * 2);
        }
        m_ptsmtx.lock();
        std::swap(postx, m_pts);
        m_ptsmtx.unlock();
    }

    static void ReleaseFn(void* ptr, void* pThis)
    {
        delete[]ptr;

    }

    void PlanesVis::Draw(DrawContext& ctx)
    {
        static bgfxh<bgfx::ProgramHandle> sShader;
        if (!bgfx::isValid(sShader))
            sShader = Engine::Inst().LoadShader("vs_planes.bin", "fs_planes.bin");

        if (m_pts.size() == 0)
            return;
        m_ptsmtx.lock();
        PosTexcoordVertex* pvtx = new PosTexcoordVertex[m_pts.size()];
        memcpy(pvtx, m_pts.data(), m_pts.size());
        m_vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(pvtx, m_pts.size() * sizeof(PosTexcoordVertex), ReleaseFn)
            , PosTexcoordVertex::ms_layout
        );
        m_ptsmtx.unlock();
      
        Cube::init();
        Matrix44f m = ctx.m_mat * CalcMat();
        bgfx::setTransform(m.getData());
        // Set vertex and index buffer.
        bgfx::setVertexBuffer(0, m_vbh);
        //bgfx::setIndexBuffer(Cube::ibh);
        
        //bgfx::setTexture(0, m_dtexture, m_depthtex);
        //bgfx::setTexture(1, m_vtexture, m_vidtex);
        Vec4f color = Vec4f(0.4f, 0.4f, 0.4f, 1);
        bgfx::setUniform(m_uparams, &color, 1);

        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_MSAA
            | BGFX_STATE_BLEND_ALPHA;
        // Set render states.l
        bgfx::setState(state);
        bgfx::submit(ctx.m_curviewIdx, sShader);
    }

}
