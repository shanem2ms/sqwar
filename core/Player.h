#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include "gmtl/Vec.h"
#include "gmtl/Matrix.h"

namespace sam
{

    class FFmpegFileWriter;
    class FFmpegFileReader;
    class FFmpegInputStreamer;
    struct DepthData;

    class Player
    {
        std::shared_ptr<FFmpegFileReader> m_vidReader;
        std::shared_ptr<FFmpegFileReader> m_depthReader;
        std::shared_ptr<FFmpegInputStreamer> m_depthStreamer;
        std::shared_ptr<std::fstream> m_faceReader;
        std::string m_documentsPath;
        std::vector<float> m_depthVals;        
        std::vector<uint16_t> m_indices;
        std::thread m_backThread;
        std::mutex m_datamutex;
        std::queue<DepthData> m_frames;
        bool m_terminate;
        std::condition_variable m_hasFramesCv;
        std::mutex m_hasFramesMtx;
        int m_depthWidth;
        int m_depthHeight;
        int m_depthFrames;
        int m_vidWidth;
        int m_vidHeight;
        int m_vidFrames;
        bool m_notfound;
        bool m_streaming;
        void BackgroundThread();
        static void BackgroundThreadF(Player* pThis);
    public:
        Player(const std::string &path, bool streaming);
        void Initialize();
        bool GetNextFrame(DepthData& data);
    };
}
