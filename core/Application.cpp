#include "StdIncludes.h"
#include "Application.h"
#include <bgfx/bgfx.h>
#include "Engine.h"
#include "World.h"
#include "Network.h"
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
    void ConvertDepthToYUV(float* data, int width, int height, float maxDepth, uint8_t* ydata, uint8_t* udata, uint8_t* vdata);
    void ConvertYUVToDepth(uint8_t* ydata, uint8_t* udata, uint8_t* vdata, int width, int height, float maxDepth, float* depthData);

    void CalcDepthError(const std::vector<float>& d1, const std::vector<float>& d2,
        float& outAvgErr, float& outMaxErr);

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

    std::shared_ptr<BackgroundFFMpegWriter> CreateBackgroundFFMpegWriter();
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
        m_isrecording(false),
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
        m_bkgWriter = CreateBackgroundFFMpegWriter();
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
    void Application::Tick(float time, double deviceTimestamp)
    {
        m_deviceTimestamp = deviceTimestamp;
        //DoPlayback();
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
        //encode_test("shane.out", "libx264");
        //testwrite();
    }

    void Application::DoPlayback()
    {
        if (m_vidReader == nullptr)
        {
            {
                std::filesystem::path depthfile = std::filesystem::path(m_documentsPath);
                depthfile.append("depth.mp4");
                m_depthReader = std::make_shared<FFmpegFileReader>(depthfile.string());
                int nFrames = 0;
                int width = m_depthReader->GetWidth();
                int height = m_depthReader->GetHeight();
                std::vector<uint8_t> ydata(width * height), udata(width * height / 4), vdata(width * height / 4);
                std::vector<float> depth(width * height);
                while (m_depthReader->ReadFrameYUV420(ydata.data(), udata.data(), vdata.data()))
                {
                    ConvertYUVToDepth(ydata.data(), udata.data(), vdata.data(), width, height, 10.0f, depth.data());
                    nFrames++;
                };
            }
            {
                std::filesystem::path vidfile = std::filesystem::path(m_documentsPath);
                vidfile.append("vid.mp4");
                m_vidReader = std::make_shared<FFmpegFileReader>(vidfile.string());
                int nFrames = 0;
                int width = m_vidReader->GetWidth();
                int height = m_vidReader->GetHeight();
                std::vector<uint8_t> ydata(width * height), udata(width * height / 4), vdata(width * height / 4);
                while (m_vidReader->ReadFrameYUV420(ydata.data(), udata.data(), vdata.data()))
                {
                    nFrames++;
                };
            }
        }
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
    struct FaceData
    {
        FaceDataProps props;
        std::vector<float> vertices;
        std::vector<int16_t> indices;
    };

    class BackgroundFFMpegWriter
    {
        std::shared_ptr<FFmpegFileWriter> m_depthWriter;
        std::shared_ptr<FFmpegFileWriter> m_vidWriter;
        std::vector<DepthData> m_depthDataQueue;
        std::vector<FaceData> m_faceDataQueue;
        std::fstream m_fs;

        float m_depthVals[16];
        bool m_hasHeaderDepthVals;
        std::string m_outputPath;
        std::mutex m_datamutex;
        std::thread m_backThread;
        bool m_terminate;
        std::condition_variable m_hasFramesCv;
        std::mutex m_hasFramesMtx;
        bool m_hasFrames;
        bool m_writeFaceHeader;
        void WriteFrameBkg(DepthData& frame);
        static void BackgroundThreadF(BackgroundFFMpegWriter *pThis);
        void BackgroundThread();
    public:
        BackgroundFFMpegWriter();
        ~BackgroundFFMpegWriter();
        void StartRecording(const std::string& outputPath);
        void StopRecording();
        void WriteFrame(DepthData& frame);
        void WriteFaceFrame(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t>& indices);
        void FinishFFmpeg();

        void WriteFileData(const char* data, size_t sz)
        {
            if (!m_fs.is_open())
                m_fs = std::fstream(m_outputPath + "/face.bin", std::ios::out | std::ios::binary);
            m_fs.write((const char*)&sz, sizeof(size_t));
            m_fs.write((const char*)data, sz);
        }
        template <typename T> void WriteFileData(const T* data, size_t sz)
        {
            WriteFileData((const char*)data, sz);
        }
        template<typename T> void WriteFileData(const std::vector<T>& data)
        {
            size_t sz = data.size() * sizeof(T);
            WriteFileData((const char*)data.data(), sz);
        }
    };
    


    BackgroundFFMpegWriter::BackgroundFFMpegWriter() :
        m_terminate(false),
        m_writeFaceHeader(false),
        m_hasHeaderDepthVals(false)
    {
        std::thread t1(BackgroundThreadF, this);
        m_backThread.swap(t1);
    }

    BackgroundFFMpegWriter::~BackgroundFFMpegWriter()
    {
        m_terminate = true;
        m_backThread.join();
    }

    void BackgroundFFMpegWriter::BackgroundThreadF(BackgroundFFMpegWriter* pThis)
    {
        pThis->BackgroundThread();
    }

    std::atomic<int> g_framesPushed;
    std::atomic<int> g_framesWritten;

    void BackgroundFFMpegWriter::BackgroundThread()
    {
        while (!m_terminate)
        {
            std::unique_lock<std::mutex> mlock(m_hasFramesMtx);
            m_hasFramesCv.wait(mlock, std::bind(&BackgroundFFMpegWriter::m_hasFrames, this));
            std::vector<DepthData> depthFrames;
            std::swap(depthFrames, m_depthDataQueue);
            std::vector<FaceData> faceFrames;
            std::swap(faceFrames, m_faceDataQueue);

            for (DepthData& frame : depthFrames)
            {
                if (!m_hasHeaderDepthVals)
                {
                    memcpy(m_depthVals, frame.depthData.data(), sizeof(m_depthVals));
                    m_hasHeaderDepthVals = true;
                }

                if (frame.props.timestamp < 0)
                    FinishFFmpeg();
                else
                {
                    WriteFrameBkg(frame);
                    g_framesWritten++;
                }
            }

            if (m_hasHeaderDepthVals && !m_writeFaceHeader &&
                faceFrames.size() > 0)
            {
                FaceData& fd = faceFrames[0];
                size_t val = 1234;
                WriteFileData(&val, sizeof(val));
                WriteFileData(m_depthVals, sizeof(m_depthVals));
                WriteFileData(fd.indices);
                m_writeFaceHeader = true;
            }
            if (m_writeFaceHeader)
            {
                for (FaceData& fd : faceFrames)
                {
                    size_t val = 5678;
                    WriteFileData(&val, sizeof(val));
                    WriteFileData(&fd.props, sizeof(fd.props));
                    WriteFileData(fd.vertices);
                }
            }
        }
    }

    void BackgroundFFMpegWriter::StartRecording(const std::string& outputPath)
    {
        m_outputPath = outputPath;
    }

    void BackgroundFFMpegWriter::StopRecording()
    {
        std::lock_guard<std::mutex> guard(m_hasFramesMtx);
        DepthData stopFrame;
        stopFrame.props.timestamp = -1;
        m_depthDataQueue.push_back(stopFrame);
        m_hasFrames = true;
        m_hasFramesCv.notify_one();
    }
    
    void BackgroundFFMpegWriter::WriteFrame(DepthData& frame)
    {
        std::lock_guard<std::mutex> guard(m_hasFramesMtx);
        m_depthDataQueue.push_back(frame);
        m_hasFrames = true;
        g_framesPushed++;
        m_hasFramesCv.notify_one();
    }

    void BackgroundFFMpegWriter::WriteFrameBkg(DepthData& frame)
    {
        if (m_depthWriter == nullptr)
        {
            m_depthWriter = std::make_shared<FFmpegFileWriter>(m_outputPath + "/depth.mp4", frame.props.depthWidth,
                frame.props.depthHeight);
            m_vidWriter = std::make_shared<FFmpegFileWriter>(m_outputPath + "/vid.mp4", frame.props.vidWidth,
                frame.props.vidHeight);
        }
        float avgavg = 0;
        float avgmax = 0;
        std::vector<uint8_t> depthYData(frame.props.depthWidth * frame.props.depthHeight);
        std::vector<uint8_t> depthUData((frame.props.depthWidth * frame.props.depthHeight) / 4);
        std::vector<uint8_t> depthVData((frame.props.depthWidth * frame.props.depthHeight) / 4);
        sam::ConvertDepthToYUV(frame.depthData.data() + 16, frame.props.depthWidth,
            frame.props.depthHeight, 10.0f, depthYData.data(), depthUData.data(), depthVData.data());

        m_depthWriter->WriteFrameYUV420(depthYData.data(), depthUData.data(), depthVData.data());
        m_vidWriter->WriteFrameYCbCr(frame.vidData.data());
    }

    void BackgroundFFMpegWriter::FinishFFmpeg()
    {
        if (m_depthWriter != nullptr)
        {
            m_depthWriter->FinishWrite();
            m_depthWriter = nullptr;
        }
        if (m_vidWriter != nullptr)
        {
            m_vidWriter->FinishWrite();
            m_vidWriter = nullptr;
        }
    }

    void BackgroundFFMpegWriter::WriteFaceFrame(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t>& indices)
    {
        std::lock_guard<std::mutex> guard(m_hasFramesMtx);
        m_faceDataQueue.push_back(FaceData());
        FaceData &fd = m_faceDataQueue.back();
        fd.props = props;
        fd.vertices = vertices;
        fd.indices = indices;
        m_hasFrames = true;
        g_framesPushed++;
        m_hasFramesCv.notify_one();
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

    std::shared_ptr<BackgroundFFMpegWriter> CreateBackgroundFFMpegWriter()
    {
        return std::make_shared<BackgroundFFMpegWriter>();
    }
}
