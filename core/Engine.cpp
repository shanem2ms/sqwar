#include "StdIncludes.h"
#include "Engine.h"
#include <bx/readerwriter.h>
#include <bx/file.h>
#include "Hud.h"

namespace sam
{
    static Engine* sEngine = nullptr;
    Engine::Engine() :
        m_h(0),
        m_w(0),
        m_root(std::make_shared<SceneGroup>())
    {
        sEngine = this;
        m_hud = std::make_shared<Hud>();
    }

    void Engine::Resize(int w, int h)
    {
        m_h = h;
        m_w = w;
    }

    void Engine::Tick(float time)
    {
        m_camera.Update(m_w, m_h);

        for (auto itAnim = m_animations.begin();
            itAnim != m_animations.end();)
        {
            if ((*itAnim)->ProcessTick(time))
                itAnim++;
            else
                itAnim = m_animations.erase(itAnim);
        }
    }

    void Engine::Draw(DrawContext& dc)
    {
        bgfx::setViewName(0, "farstuff");
        bgfx::setViewFrameBuffer(0, BGFX_INVALID_HANDLE);
        dc.m_texture = m_texture;
        dc.m_gradient = m_gradient;
        gmtl::identity(dc.m_mat);
        gmtl::Matrix44f view = Cam().ViewMatrix();
        
        bgfx::setViewClear(0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            0x000000ff,
            1.0f,
            0
        );
        gmtl::Matrix44f proj0 = Cam().ProjectionMatrix();
        bgfx::setViewTransform(0, view.getData(), proj0.getData());
        dc.m_curviewIdx = 0;
        dc.m_nearfarpassIdx = 0;
        m_root->DoDraw(dc);
        
        m_hud->DoDraw(dc);
    }


    void Engine::AddAnimation(const std::shared_ptr<Animation>& anim)
    {
        m_animations.push_back(anim);
    }

    Animation::Animation() :
        m_startTime(-1)
    {}

    bool Animation::ProcessTick(float fullTime)
    {
        if (m_startTime < 0)
            m_startTime = fullTime;

        return Tick(fullTime - m_startTime);
    }

    static const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath)
    {
        if (bx::open(_reader, _filePath))
        {
            uint32_t size = (uint32_t)bx::getSize(_reader);
            const bgfx::Memory* mem = bgfx::alloc(size + 1);
            bx::read(_reader, mem->data, size);
            bx::close(_reader);
            mem->data[mem->size - 1] = '\0';
            return mem;
        }

        return NULL;
    }

    bgfx::ProgramHandle Engine::LoadShader(const std::string& vtx, const std::string& px)
    {
        std::string key = vtx + ":" + px;
        auto itshd = m_shaders.find(key);
        if (itshd != m_shaders.end())
            return itshd->second;
        bx::FileReader fileReader;
        bgfx::ShaderHandle vtxShader = bgfx::createShader(loadMem(&fileReader, vtx.c_str()));
        bgfx::ShaderHandle fragShader = bgfx::createShader(loadMem(&fileReader, px.c_str()));
        bgfx::ProgramHandle pgm = bgfx::createProgram(vtxShader, fragShader, true);
        m_shaders.insert(std::make_pair(key, pgm));
        return pgm;
    }

    bgfx::ProgramHandle Engine::LoadShader(const std::string& cs)
    {
        auto itshd = m_shaders.find(cs);
        if (itshd != m_shaders.end())
            return itshd->second;
        bx::FileReader fileReader;
        bgfx::ShaderHandle csShader = bgfx::createShader(loadMem(&fileReader, cs.c_str()));
        bgfx::ProgramHandle pgm = bgfx::createProgram(csShader, true);
        m_shaders.insert(std::make_pair(cs, pgm));
        return pgm;
    }

    Engine& Engine::Inst()
    {
        return *sEngine;
    }
}
