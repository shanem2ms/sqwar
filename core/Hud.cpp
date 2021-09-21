#include "StdIncludes.h"
#include "World.h"
#include "Application.h"
#include "Engine.h"
#include <numeric>
#include "Mesh.h"
#include "Hud.h"
#define NOMINMAX

namespace sam
{
    void Hud::Initialize(DrawContext& nvg)
    {
        m_shader = Engine::Inst().LoadShader("vs_hud.bin", "fs_hud.bin");
    }

	void Hud::Draw(DrawContext& ctx)
	{        
        Matrix44f m =
            ctx.m_mat * CalcMat();

        bgfx::dbgTextClear();

        Engine& e = Engine::Inst();
        Camera::Fly la = e.Cam().GetFly();
        bgfx::dbgTextPrintf(0, 4, 0x0f, "Cam [%f %f %f]", la.pos[0], la.pos[1], la.pos[2]);
        bgfx::dbgTextPrintf(0, 5, 0x0f, "    [%f %f]", la.dir[0], la.dir[1]);

        //bgfx::setTransform(m.getData());
        Quad::init();

        // Set vertex and index buffer.
        bgfx::setVertexBuffer(0, Quad::vbh);
        bgfx::setIndexBuffer(Quad::ibh);
        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_MSAA
            | BGFX_STATE_BLEND_ALPHA;
        // Set render states.l
        bgfx::setState(state);
        bgfx::submit(0, m_shader);
    }

	AABoxf Hud::GetBounds() const
	{
		return AABoxf();
	}
}
