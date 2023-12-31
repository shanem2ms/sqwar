#include "Record.h"
#include "DepthPts.h"
#include "FFmpeg.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>


namespace sam
{
    Recorder::Recorder() :
        m_terminate(false),
        m_writeFaceHeader(false),
        m_hasHeaderDepthVals(false)
    {
        std::thread t1(BackgroundThreadF, this);
        m_backThread.swap(t1);
    }

    Recorder::~Recorder()
    {
        m_terminate = true;
        m_backThread.join();
    }

    void Recorder::BackgroundThreadF(Recorder* pThis)
    {
        pThis->BackgroundThread();
    }

    std::atomic<int> g_framesPushed;
    std::atomic<int> g_framesWritten;
    bool streammode = true;

    void Recorder::BackgroundThread()
    {
        while (!m_terminate)
        {
            std::unique_lock<std::mutex> mlock(m_hasFramesMtx);
            m_hasFramesCv.wait(mlock, std::bind(&Recorder::m_hasFrames, this));
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
                    StreamFrameBkg(frame);
                    g_framesWritten++;
                }
            }
            if (streammode)
            {
            }
            else
            {
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
    }

    void Recorder::StartRecording(const std::string& outputPath)
    {
        m_outputPath = outputPath;
    }

    void Recorder::StopRecording()
    {
        std::lock_guard<std::mutex> guard(m_hasFramesMtx);
        DepthData stopFrame;
        stopFrame.props.timestamp = -1;
        m_depthDataQueue.push_back(stopFrame);
        m_hasFrames = true;
        m_hasFramesCv.notify_one();
    }

    void Recorder::WriteFrame(DepthData& frame)
    {
        std::lock_guard<std::mutex> guard(m_hasFramesMtx);
        m_depthDataQueue.push_back(frame);
        m_hasFrames = true;
        g_framesPushed++;
        m_hasFramesCv.notify_one();
    }

    void Recorder::StreamFrameBkg(DepthData& frame)
    {
        if (m_depthVidStream == nullptr) {
            m_depthVidStream = std::make_shared<FFmpegOutputStreamer>("udp://shanemor.ddns.net:23000", frame.props.depthWidth,
                frame.props.depthHeight * 2, true);
        }        
        float avgavg = 0;
        float avgmax = 0;
        size_t ySize = (frame.props.depthWidth * frame.props.depthHeight);
        size_t uvSize = (frame.props.depthWidth * frame.props.depthHeight) / 4;
        std::vector<uint8_t> depthYData(ySize * 2);
        std::vector<uint8_t> depthUData(uvSize * 2);
        std::vector<uint8_t> depthVData(uvSize * 2);
        sam::ConvertDepthToYUV(frame.depthData.data() + 16, frame.props.depthWidth,
            frame.props.depthHeight, 10.0f, depthYData.data(), depthUData.data(), depthVData.data());

        {
            uint8_t *data = frame.vidData.data();
            YCrCbData* yuvData = (YCrCbData*)data;
            yuvData->cbCrOffset -= yuvData->yOffset;
            yuvData->yOffset = 0;

            int yPitch = yuvData->yRowBytes;
            uint8_t* ydata = data + sizeof(YCrCbData);
            uint8_t* yDst = depthYData.data() + ySize;
            for (int y = 0; y < frame.props.depthHeight; ++y)
            {
                int srcY = (y * frame.props.vidHeight) / frame.props.depthHeight;
                uint8_t* ySrc = ydata + srcY * yuvData->yRowBytes;
                for (int x = 0; x < frame.props.depthWidth; ++x)
                {
                    int srcX = (x * frame.props.vidWidth) / frame.props.depthWidth;
                    yDst[x] = ySrc[srcX];
                }

                yDst += frame.props.depthWidth;
            }

            int dstlineIn = frame.props.vidWidth / 2;
            int uvheightIn = frame.props.vidHeight / 2;
            int dstline = frame.props.depthWidth / 2;
            int uvheight = frame.props.depthHeight / 2;
            uint8_t* udata = new uint8_t[dstline * uvheight];
            uint8_t* vdata = new uint8_t[dstline * uvheight];
            uint8_t* uvData = ydata + yuvData->cbCrOffset;

            uint8_t* uDstPtr = depthUData.data() + uvSize;
            uint8_t* vDstPtr = depthVData.data() + uvSize;
            uint8_t* uvSrcPtr = uvData;
            for (int y = 0; y < uvheight; ++y)
            {
                int srcY = (y * uvheightIn) / uvheight;
                uvSrcPtr = uvData + (yuvData->cbCrRowBytes * srcY);
                for (int idx = 0; idx < dstline; ++idx)
                {
                    int idxSrc = (idx * dstlineIn) / dstline;
                    uDstPtr[idx] = uvSrcPtr[idxSrc * 2];
                    vDstPtr[idx] = uvSrcPtr[idxSrc * 2 + 1];
                }
                uDstPtr += dstline;
                vDstPtr += dstline;
            }
        }
        m_depthVidStream->WriteFrameYUV420(depthYData.data(), depthUData.data(), depthVData.data(),
            frame.props.timestamp);
        //m_depthVidStream->WriteFrameYCbCr(frame.vidData.data());
    }

    void Recorder::FinishFFmpeg()
    {
    }

    void Recorder::WriteFaceFrame(const FaceDataProps& props, const std::vector<float>& vertices, const std::vector<int16_t>& indices)
    {
        std::lock_guard<std::mutex> guard(m_hasFramesMtx);
        m_faceDataQueue.push_back(FaceData());
        FaceData& fd = m_faceDataQueue.back();
        fd.props = props;
        fd.vertices = vertices;
        fd.indices = indices;
        m_hasFrames = true;
        g_framesPushed++;
        m_hasFramesCv.notify_one();
    }


    void Recorder::WriteFileData(const char* data, size_t sz)
    {
        if (!m_fs.is_open())
            m_fs = std::fstream(m_outputPath + "/face.bin", std::ios::out | std::ios::binary);
        m_fs.write((const char*)&sz, sizeof(size_t));
        m_fs.write((const char*)data, sz);
    }

}
