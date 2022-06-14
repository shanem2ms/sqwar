#pragma once
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include "DepthProps.h"

namespace sam
{
    struct FaceData
    {
        FaceDataProps props;
        std::vector<float> vertices;
        std::vector<int16_t> indices;
    };
    class FFmpegOutputStreamer;
    class FFmpegFileWriter;
    class FFmpegFileWriter;

    class Recorder
    {
        std::shared_ptr<FFmpegOutputStreamer> m_depthVidStream;
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
        void StreamFrameBkg(DepthData& frame);
        static void BackgroundThreadF(Recorder* pThis);
        void BackgroundThread();
    public:
        Recorder();
        ~Recorder();
        void StartRecording(const std::string& outputPath);
        void StopRecording();
        void WriteFrame(DepthData& frame);
        void WriteFaceFrame(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t>& indices);
        void FinishFFmpeg();
        void WriteFileData(const char* data, size_t sz);
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

}