#include "StdIncludes.h"
#include "Square.h"
#include "Mesh.h"

namespace sam
{

    AABoxf Square::GetBounds() const
    {
        return AABoxf();
    }

    void Square::Initialize(DrawContext& nvg)
    {
        m_uparams = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, 1);

    }

    void Square::Draw(DrawContext& ctx)
    {
        Cube::init();
        Matrix44f m = ctx.m_mat * CalcMat();
        bgfx::setTransform(m.getData());
        // Set vertex and index buffer.
        bgfx::setVertexBuffer(0, Cube::vbh);
        bgfx::setIndexBuffer(Cube::ibh);

        Vec4f color = Vec4f(1, 0, 1, 1);
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
        bgfx::submit(ctx.m_curviewIdx, ctx.m_pgm);
    }

}