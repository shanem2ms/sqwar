#include "Player.h"
#include <fstream>
#include <filesystem>
#include "DepthProps.h"
#include "FFmpeg.h"

namespace sam
{
    void ConvertYUVToDepth(uint8_t* ydata, uint8_t* udata, uint8_t* vdata, int width, int height, float maxDepth, float* depthData);

    void ReadBlock(std::fstream& fs, std::vector<uint8_t>& data)
    {
        size_t sz;
        fs.read((char*)&sz, sizeof(sz));
        data.resize(sz);
        fs.read((char*)data.data(), sz);
    }

    template <typename T> void ReadBlock(std::fstream& fs, std::vector<T>& data)
    {
        size_t sz;
        fs.read((char*)&sz, sizeof(sz));
        data.resize(sz / sizeof(T));
        fs.read((char*)data.data(), sz);
    }

    Player::Player(const std::string& path) :
        m_documentsPath(path),
        m_notfound(false)
    {

    }

    void Player::Initialize()
    {
        std::filesystem::path depthfile = std::filesystem::path(m_documentsPath);
        depthfile.append("depth.mp4");
        if (!std::filesystem::exists(depthfile))
        {
            m_notfound = true;
            return;
        }
        {
            m_faceReader = std::make_shared<std::fstream>(m_documentsPath + "/face.bin", std::ios::in | std::ios::binary);
            size_t val;
            std::vector<size_t> data;
            ReadBlock(*m_faceReader, data);
            ReadBlock(*m_faceReader, m_depthVals);
            ReadBlock(*m_faceReader, m_indices);
        }
        {
            m_depthReader = std::make_shared<FFmpegFileReader>(depthfile.string());
            m_depthWidth = m_depthReader->GetWidth();
            m_depthHeight = m_depthReader->GetHeight();
        }
        {
            std::filesystem::path vidfile = std::filesystem::path(m_documentsPath);
            vidfile.append("vid.mp4");
            m_vidReader = std::make_shared<FFmpegFileReader>(vidfile.string());
            int nFrames = 0;
            m_vidWidth = m_vidReader->GetWidth();
            m_vidHeight = m_vidReader->GetHeight();           
        }
    }

    bool Player::GetNextFrame(DepthData& data)
    {
        if (m_notfound)
            return false;
        if (m_vidReader == nullptr)
            Initialize();
        if (m_vidReader == nullptr)
            return false;
        data.props.depthHeight = m_depthHeight;
        data.props.depthWidth = m_depthWidth;
        data.props.vidHeight = m_vidHeight;
        data.props.vidWidth = m_vidWidth;

        if (false && !m_faceReader->eof())
        {
            std::vector<size_t> data;
            ReadBlock(*m_faceReader, data);
            std::vector<FaceDataProps> fdp;
            ReadBlock(*m_faceReader, fdp);
            std::vector<float> vertices;
            ReadBlock(*m_faceReader, vertices);
        }


        bool hasFrames = true;
        {
            std::vector<uint8_t> ydata(m_depthWidth * m_depthHeight), udata(m_depthWidth * m_depthHeight / 4),
                vdata(m_depthWidth * m_depthHeight / 4);
            data.depthData.resize(m_depthWidth * m_depthHeight + 16);
            memcpy(data.depthData.data(), m_depthVals.data(), sizeof(float) * 16);
            if (m_depthReader->ReadFrameYUV420(ydata.data(), udata.data(), vdata.data()))
            {
                ConvertYUVToDepth(ydata.data(), udata.data(), vdata.data(), m_depthWidth, m_depthHeight, 10.0f, data.depthData.data() + 16);
            }
            else
                hasFrames = false;
        }
        {
            size_t ysize = m_vidWidth * m_vidHeight;
            size_t usize = m_vidWidth * m_vidHeight / 4;
            data.vidData.resize(ysize + usize * 2 + sizeof(YCrCbData));
            YCrCbData* vidProps = (YCrCbData*)data.vidData.data();
            vidProps->yOffset = sizeof(YCrCbData);
            vidProps->cbCrOffset = vidProps->yOffset + ysize;
            vidProps->yRowBytes = m_vidWidth;
            vidProps->cbCrRowBytes = m_vidWidth;

            std::vector<uint8_t> udata(m_vidWidth * m_vidHeight / 4),
                vdata(m_vidWidth * m_vidHeight / 4);
            uint8_t* outydata = data.vidData.data() + vidProps->yOffset;
            uint8_t* ydata = data.vidData.data() + vidProps->yOffset;
            uint8_t* outuvdata = data.vidData.data() + vidProps->cbCrOffset;
            if (!m_vidReader->ReadFrameYUV420(outydata, udata.data(), vdata.data()))
                    hasFrames = false;

            uint8_t* curOutUVRow = outuvdata;
            const uint8_t* curInURow = udata.data();
            const uint8_t* curInVRow = vdata.data();
            for (size_t y = 0; y < m_vidHeight / 2; ++y)
            {
                uint8_t* curOutUVPtr = curOutUVRow;
                const uint8_t* curInUPtr = curInURow;
                const uint8_t* curInVPtr = curInVRow;
                for (size_t x = 0; x < m_vidWidth / 2; ++x)
                {
                    *curOutUVPtr = *curInUPtr;
                    curOutUVPtr++;
                    curInUPtr++;
                    *curOutUVPtr = *curInVPtr;
                    curOutUVPtr++;
                    curInVPtr++;
                }
                curOutUVRow += m_vidWidth;
                curInURow += m_vidWidth / 2;
                curInVRow += m_vidWidth / 2;
            }
        }
        return hasFrames;
    }    
}
