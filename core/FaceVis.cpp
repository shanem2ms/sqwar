#include "StdIncludes.h"
#include "FaceVis.h"
#include "DepthPts.h"
#include "Mesh.h"
#include "Engine.h"
#include "Application.h"

namespace sam
{
    FaceVis::FaceVis()
    {}

    FaceVis::~FaceVis()
    {}

    AABoxf FaceVis::GetBounds() const
    {
        return AABoxf();
    }

    void FaceVis::Initialize(DrawContext& nvg)
    {
        m_uparams = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 1);
    }

    static void ReleaseFn(void* ptr, void* pThis)
    {
        delete[]ptr;
    }

    void FaceVis::OnFaceData(const FaceDataProps& props, const std::vector<float>& vertices, 
        const std::vector<int16_t> indices)
    {
        std::vector<float> v = vertices;
        std::vector<int16_t> i = indices;
        Matrix44f wm, vm;
        wm.mState = Matrix44f::AFFINE;;
        vm.mState = Matrix44f::AFFINE;
        memcpy(wm.mData, props.wMatf, sizeof(props.wMatf));
        memcpy(vm.mData, props.viewMatf, sizeof(props.viewMatf));
        wm = vm* wm;

        m_faceTimestamp = props.timestamp;
        m_ptsmtx.lock();
        m_worldMat = wm;
        m_indices.swap(i);
        m_vertices.swap(v);
        m_ptsmtx.unlock();
    }

    void FaceVis::Draw(DrawContext& ctx)
    {
        static bgfxh<bgfx::ProgramHandle> sShader;
        if (!bgfx::isValid(sShader))
            sShader = Engine::Inst().LoadShader("vs_face.bin", "fs_planes.bin");

        if (m_vertices.size() == 0)
            return;
        if (fabs(m_faceTimestamp - ctx.m_deviceTimestamp) > 1 / 30.0)
            return;
        size_t ptsize;
        m_ptsmtx.lock();
        constexpr int ncomponents = sizeof(PosTexcoordVertex) / sizeof(float);
        int nvertices = m_vertices.size() / ncomponents;
        PosTexcoordVertex* pvtx = new PosTexcoordVertex[nvertices];
        memcpy(pvtx, m_vertices.data(), sizeof(PosTexcoordVertex) * nvertices);
        bgfx::setTransform(m_worldMat.getData());
        m_ptsmtx.unlock();
        m_vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(pvtx, nvertices * sizeof(PosTexcoordVertex), ReleaseFn)
            , PosTexcoordVertex::ms_layout
        );
        int16_t* pidx = new int16_t[m_indices.size()];
        memcpy(pidx, m_indices.data(), m_indices.size() * sizeof(int16_t));
        m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(pidx, m_indices.size() * sizeof(int16_t), ReleaseFn), BGFX_BUFFER_NONE);
      
        Cube::init();
        Matrix44f m = ctx.m_mat * CalcMat();
        // Set vertex and index buffer.
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);
        
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
