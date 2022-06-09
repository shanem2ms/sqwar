#include "StdIncludes.h"
#include "Application.h"
#include "DepthProps.h"
#include "DepthPts.h"
#include "Record.h"
#include <bgfx/bgfx.h>
#include "Engine.h"
#include "World.h"
#include "Network.h"
#include "Player.h"
#include "imgui.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <atomic>
#include <condition_variable>
#include <filesystem>
//#include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FFmpeg.h"


namespace sam
{
    void CalcDepthError(const std::vector<float>& d1, const std::vector<float>& d2,
        float& outAvgErr, float& outMaxErr);
    void ConvertYUVToDepth(uint8_t* ydata, uint8_t* udata, uint8_t* vdata, int width, int height, float maxDepth, float* depthData);

    void (*Application::m_dbgFunc)(const char*) = nullptr;
    void Application::SetDebugMsgFunc(void (*dbgfunc)(const char*))
    {
        m_dbgFunc = dbgfunc;
    }
    void Application::DebugMsg(const std::string& str)
    {
        if (m_dbgFunc != nullptr)
            m_dbgFunc(str.c_str());
    }

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
        m_buttonDown(false),
        m_clientInit(false),
        m_isrecording(true),
        m_wasrecording(false)
    {
        s_pInst = this;
        m_engine = std::make_unique<Engine>();
        m_world = std::make_unique<World>();
#ifdef _WIN32
        m_server = std::make_unique<Server>(
            [this](const std::vector<unsigned char>& data)
            {
                //if (m_world->GetSquare())
                //    m_world->GetSquare()->SetDepthData(data.data(), data.size(),
                ///        std::vector<float>());
            }
        );
#endif
        m_bkgWriter = std::make_shared<Recorder>();
    }

    Application& Application::Inst()
    {
        return *s_pInst;
    }

    void Application::TouchDown(float x, float y, int touchId)
    {
        m_touchDown = gmtl::Vec2f(x, y);
        m_touchPos = gmtl::Vec2f(x, y);
        m_buttonDown = 1;
        m_world->TouchDown(x, y, touchId);
    }

    void Application::TouchMove(float x, float y, int touchId)
    {
        if (!m_buttonDown)
            return;
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

    static bool playback = false;
    void Application::Tick(float time, double deviceTimestamp)
    {
        m_deviceTimestamp = deviceTimestamp;
        if (playback)
        {
            if (m_player == nullptr)
                m_player = std::make_shared<Player>(m_documentsPath);

            DepthData depthData;
            if (m_player->GetNextFrame(depthData))
            {
                m_world->OnDepthBuffer(depthData);
            }
        }
        m_engine->Tick(time);
    }

    void Application::Initialize(const char* folder)
    {
        m_documentsPath = folder;
        std::string dbPath = m_documentsPath + "/testlvl";
        m_world->Open(dbPath);
        imguiCreate();
        m_client = std::make_unique<Client>();
        m_clientInit = true;
    }

    const float Pi = 3.1415297;

    void Application::Draw()
    {
        sam::DrawContext ctx;
        ctx.m_deviceTimestamp = m_deviceTimestamp;
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

        const int btnSize = 80;
        const int btnSpace = 10;
        ImGui::SetNextWindowPos(
            ImVec2(btnSize, btnSize)
            , ImGuiCond_Always
        );

        int buttonsThisFrame[256];
        memset(buttonsThisFrame, 0, sizeof(buttonsThisFrame));
        ImGui::Begin("make window", nullptr,
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove);

        ImGui::SetCursorPos(ImVec2(0, 0));
        if (ImGui::Button(ICON_FA_CHEVRON_UP, ImVec2(btnSize, btnSize)))
        {
            m_world->SetMode(
                (m_world->GetMode() + 1) & 7);
        }

        ImGui::SetCursorPos(ImVec2(100, 0));
        if (ImGui::Button(m_isrecording ? ICON_FA_SQUARE : ICON_FA_CIRCLE, ImVec2(btnSize, btnSize)))
        {
            m_isrecording = !m_isrecording;
        }

        ImGui::End();

        imguiEndFrame();
        m_frameIdx = bgfx::frame() + 1;
    }

    void WriteFileData(const std::string& path, const char* data, size_t sz)
    {
        static std::fstream fs;
        if (!fs.is_open())
            fs = std::fstream(path + "/file.binary", std::ios::out | std::ios::binary);
        fs.write((const char*)&sz, sizeof(size_t));
        fs.write((const char*)data, sz);
    }
    template <typename T> void WriteFileData(const std::string& path, const T* data, size_t sz)
    {
        WriteFileData(path, (const char*)data, sz);
    }
    template<typename T> void WriteFileData(const std::string& path, const std::vector<T>& data)
    {
        size_t sz = data.size() * sizeof(T);
        WriteFileData(path, (const char*)data.data(), sz);
    }

    void Application::WriteDepthDataToFile(DepthData& depthData)
    {
        m_filemtx.lock();
        size_t val = 1234;
        WriteFileData(m_documentsPath, &val, sizeof(val));
        WriteFileData(m_documentsPath, &depthData.props, sizeof(depthData.props));
        WriteFileData(m_documentsPath, depthData.vidData);
        WriteFileData(m_documentsPath, depthData.depthData);
        m_filemtx.unlock();
    }

    void Application::WriteFaceDataToFile(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t> indices)
    {
        m_filemtx.lock();
        size_t val = 5678;
        WriteFileData(m_documentsPath, &val, sizeof(val));
        WriteFileData(m_documentsPath, &props, sizeof(props));
        WriteFileData(m_documentsPath, vertices);
        WriteFileData(m_documentsPath, indices);
        m_filemtx.unlock();
    }

    //#define DOWRITEDATA 1

    void Application::OnDepthBuffer(DepthData& depth)
    {
        s_pInst->OnDepthBufferInst(depth);
    }

    void Application::OnDepthBufferInst(DepthData& depth)
    {
#ifdef DOSENDDATA
        static Client c;
        c.SendData((const unsigned char*)depthData.data(), depthData.size() *
            sizeof(float));
#endif

        if (m_isrecording && !m_wasrecording)
            m_bkgWriter->StartRecording(m_documentsPath);
        else if (!m_isrecording && m_wasrecording)
            m_bkgWriter->StopRecording();

        if (m_isrecording)
            m_bkgWriter->WriteFrame(depth);
        /*
        std::vector<uint8_t> depthYData(depth.props.depthWidth * depth.props.depthHeight);
        std::vector<uint8_t> depthUData((depth.props.depthWidth * depth.props.depthHeight) / 4);
        std::vector<uint8_t> depthVData((depth.props.depthWidth * depth.props.depthHeight) / 4);
        sam::ConvertDepthToYUV(depth.depthData.data() + 16, depth.props.depthWidth,
                               depth.props.depthHeight, 10.0f, depthYData.data(), depthUData.data(), depthVData.data());
        sam::ConvertYUVToDepth(depthYData.data(), depthUData.data(), depthVData.data(),
                               depth.props.depthWidth, depth.props.depthHeight, 10.0f,
                               depth.depthData.data() + 16);
                               */
        m_world->OnDepthBuffer(depth);
        m_wasrecording = m_isrecording;
    }

    void Application::OnFaceData(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t> &indices)
    {
        s_pInst->OnFaceDataInst(props, vertices, indices);
    }
    void Application::OnFaceDataInst(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t>& indices)
    {
        if (m_isrecording && !m_wasrecording)
            m_bkgWriter->StartRecording(m_documentsPath);
        else if (!m_isrecording && m_wasrecording)
            m_bkgWriter->StopRecording();

        if (m_isrecording)
            m_bkgWriter->WriteFaceFrame(props, vertices, indices);
        m_world->OnFaceData(props, vertices, indices);
    }
    Application::~Application()
    {
    }

}
