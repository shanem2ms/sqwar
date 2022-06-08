#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include "gmtl/Vec.h"
#include "gmtl/Matrix.h"

namespace sam
{

    class FFmpegFileWriter;
    class FFmpegFileReader;
    struct DepthData;

    class Player
    {
        std::shared_ptr<FFmpegFileReader> m_vidReader;
        std::shared_ptr<FFmpegFileReader> m_depthReader;
        std::shared_ptr<std::fstream> m_faceReader;
        std::string m_documentsPath;
        std::vector<float> m_depthVals;        
        std::vector<uint16_t> m_indices;
        int m_depthWidth;
        int m_depthHeight;
        int m_depthFrames;
        int m_vidWidth;
        int m_vidHeight;
        int m_vidFrames;
        bool m_notfound;
    public:
        Player(const std::string &path);
        void Initialize();
        bool GetNextFrame(DepthData& data);
    };
}
