#include "StdIncludes.h"
#include "Application.h"
#include <bgfx/bgfx.h>
#include "Engine.h"
#include "World.h"
#include "Network.h"
#include "imgui.h"
//#include 


namespace sam
{
#ifdef SAM_COROUTINES
    co::static_thread_pool g_threadPool(8);
#endif
    static Application* s_pInst = nullptr;

    Application::Application() :
        m_height(0),
        m_touchDown(0, 0),
        m_touchPos(0, 0),
        m_width(0),
        m_frameIdx(0),
        m_buttonDown(false)
    {
        s_pInst = this;
        m_engine = std::make_unique<Engine>();
        m_world = std::make_unique<World>();
        m_server = std::make_unique<Server>();
    }

    Application& Application::Inst()
    {
        return *s_pInst;
    }

    void Application::TouchDown(float x, float y, int touchId)
    {
        m_touchDown = gmtl::Vec2f(x, y);
        m_buttonDown = 1;
        m_world->TouchDown(x, y, touchId);
    }

    void Application::TouchMove(float x, float y, int touchId)
    {
        m_touchPos = gmtl::Vec2f(x, y);
        m_world->TouchDrag(x, y, touchId);
    }

    void Application::TouchUp(int touchId)
    {
        m_buttonDown = 0;
        m_world->TouchUp(touchId);
    }

    void Application::KeyDown(int keyId)
    {
        m_world->KeyDown(keyId);
    }

    void Application::KeyUp(int keyId)
    {
        m_world->KeyUp(keyId);
    }

    void Application::Resize(int w, int h)
    {
        m_width = w;
        m_height = h;
        m_engine->Resize(w, h);
        m_world->Layout(w, h);
    }
    void Application::Tick(float time)
    {
        m_engine->Tick(time);
    }

    void Application::Initialize(const char* folder)
    {
        m_documentsPath = folder;
        std::string dbPath = m_documentsPath + "/testlvl";
        m_world->Open(dbPath);
        imguiCreate();

        _sleep(100);
        char* msg = "Hello world";
        Client c;
        c.SendData((unsigned char *)msg, strlen(msg));

    }

    const float Pi = 3.1415297;

    void Application::Draw()
    {
        sam::DrawContext ctx;
        ctx.m_nearfar[0] = 0.1f;
        ctx.m_nearfar[1] = 25.0f;
        ctx.m_nearfar[2] = 100.0f;
        ctx.m_frameIdx = m_frameIdx;
        ctx.m_pWorld = m_world.get();
        m_world->Update(*m_engine, ctx);

        bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
        bgfx::setViewRect(1, 0, 0, uint16_t(m_width), uint16_t(m_height));
        m_engine->Draw(ctx);

        imguiBeginFrame(m_touchPos[0]
            , m_touchPos[1]
            , m_buttonDown
            , 0
            , uint16_t(m_width)
            , uint16_t(m_height)
        );

        ImGui::PushStyleColor(ImGuiCol_Text
            , false
            ? ImVec4(1.0, 0.0, 0.0, 1.0)
            : ImVec4(1.0, 1.0, 1.0, 1.0)
        );
        ImGui::TextWrapped("%s", "What is going on here");
        ImGui::Separator();
        ImGui::PopStyleColor();

        imguiEndFrame();
        m_frameIdx = bgfx::frame() + 1;
    }

    void Application::OnDepthBuffer(const std::vector<float>& pixelData)
    {

    }
    Application::~Application()
    {
    }

}