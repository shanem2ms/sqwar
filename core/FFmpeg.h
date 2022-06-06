#pragma once

typedef struct AVFrame AVFrame;
typedef struct AVCodecContext AVCodecContext;
typedef struct SwsContext SwsContext;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVOutputFormat AVOutputFormat;
typedef struct AVStream AVStream;

namespace sam
{
    class FFmpegFileWriter
    {

        std::string m_file;
        uint8_t m_w;
        uint8_t m_h;

        AVFrame* videoFrame = nullptr;
        AVCodecContext* cctx = nullptr;
        SwsContext* swsCtx = nullptr;
        int frameCounter = 0;
        AVFormatContext* ofctx = nullptr;
        AVOutputFormat* oformat = nullptr;
        AVStream* stream = nullptr;
        int fps = 30;
        int bitrate = 2000;

        int StartWrite(uint32_t width, uint32_t height);
    public:

        FFmpegFileWriter(const std::string& outname, uint32_t w, uint32_t h);
        void WriteFrameYCbCr(uint8_t* data);
        void WriteFrameYUV420(uint8_t* ydata, uint8_t* udata, uint8_t* vdata);
        void FinishWrite();
    };

    class FFmpegFileReader
    {
        AVFormatContext* m_fmt_ctx;
        AVCodecContext* m_dec_ctx;
        int m_video_stream_index;
        int64_t m_last_pts;
        std::string m_filename;
        std::vector<AVFrame*> m_decodedFrames;
        void Open();
    public:
        int GetWidth() const;
        int GetHeight() const;
        FFmpegFileReader(const std::string& inname);
        bool ReadFrameYUV420(uint8_t* ydata, uint8_t* udata, uint8_t* vdata);
    };
}