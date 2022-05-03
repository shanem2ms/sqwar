#include "StdIncludes.h"
#include "FFmpeg.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

struct YCrCbData
{
    int yOffset;
    int yRowBytes;
    int cbCrOffset;
    int cbCrRowBytes;
};

FFmpegFileWriter::FFmpegFileWriter(const std::string& outname, uint32_t w, uint32_t h) :
m_file(outname),
m_w(w),
m_h(h)
{
    StartWrite(w, h);
}

int FFmpegFileWriter::StartWrite(uint32_t width, uint32_t height)
{
    av_register_all();
    avcodec_register_all();

    oformat = av_guess_format(nullptr, m_file.c_str(), nullptr);
    if (!oformat) {
        std::cout << "can't create output format" << std::endl;
        return -1;
    }
    //oformat->video_codec = AV_CODEC_ID_H265;

    int err = avformat_alloc_output_context2(&ofctx, oformat, nullptr, m_file.c_str());

    if (err) {
        std::cout << "can't create output context" << std::endl;
        return -1;
    }

    AVCodec* codec = nullptr;

    codec = avcodec_find_encoder(oformat->video_codec);
    if (!codec) {
        std::cout << "can't create codec" << std::endl;
        return -1;
    }

    stream = avformat_new_stream(ofctx, codec);

    if (!stream) {
        std::cout << "can't find format" << std::endl;
        return -1;
    }

    cctx = avcodec_alloc_context3(codec);

    if (!cctx) {
        std::cout << "can't create codec context" << std::endl;
        return -1;
    }

    stream->codecpar->codec_id = oformat->video_codec;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = width;
    stream->codecpar->height = height;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->codecpar->bit_rate = bitrate * 1000;
    stream->avg_frame_rate = AVRational{ fps, 1 };
    avcodec_parameters_to_context(cctx, stream->codecpar);
    cctx->time_base = AVRational{ 1, 1 };
    cctx->max_b_frames = 2;
    cctx->gop_size = 12;
    cctx->framerate = AVRational{ fps, 1 };

    if (stream->codecpar->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(cctx, "preset", "ultrafast", 0);
    }
    else if (stream->codecpar->codec_id == AV_CODEC_ID_H265) {
        av_opt_set(cctx, "preset", "ultrafast", 0);
    }
    else
    {
        av_opt_set_int(cctx, "lossless", 1, 0);
    }

    avcodec_parameters_from_context(stream->codecpar, cctx);

    if ((err = avcodec_open2(cctx, codec, NULL)) < 0) {
        std::cout << "Failed to open codec" << err << std::endl;
        return -1;
    }

    if (!(oformat->flags & AVFMT_NOFILE)) {
        if ((err = avio_open(&ofctx->pb, m_file.c_str(), AVIO_FLAG_WRITE)) < 0) {
            std::cout << "Failed to open file" << err << std::endl;
            return -1;
        }
    }

    if ((err = avformat_write_header(ofctx, NULL)) < 0) {
        std::cout << "Failed to write header" << err << std::endl;
        return -1;
    }

    av_dump_format(ofctx, 0, m_file.c_str(), 1);
    std::cout << stream->time_base.den << " " << stream->time_base.num << std::endl;
}

void FFmpegFileWriter::WriteFrameYCbCr(uint8_t* data)
{
    YCrCbData* yuvData = (YCrCbData*)data;
    yuvData->cbCrOffset -= yuvData->yOffset;
    yuvData->yOffset = 0;

    int yPitch = yuvData->yRowBytes;
    uint8_t* ydata = data + sizeof(YCrCbData);
    int dstline = cctx->width / 2;
    int uvheight = cctx->height / 2;
    uint8_t* udata = new uint8_t[dstline * uvheight];
    uint8_t* vdata = new uint8_t[dstline * uvheight];
    uint8_t* uvData = ydata + yuvData->cbCrOffset;

    uint8_t* uDstPtr = udata;
    uint8_t* vDstPtr = vdata;
    uint8_t* uvSrcPtr = uvData;
    for (int y = 0; y < uvheight; ++y)
    {
        for (int idx = 0; idx < dstline; ++idx)
        {
            uDstPtr[idx] = uvSrcPtr[idx * 2];
            vDstPtr[idx] = uvSrcPtr[idx * 2 + 1];
        }
        uDstPtr += dstline;
        vDstPtr += dstline;
        uvSrcPtr += yuvData->cbCrRowBytes;
    }

    int err;
    if (!videoFrame) {
        videoFrame = av_frame_alloc();
        videoFrame->format = AV_PIX_FMT_YUV420P;
        videoFrame->width = cctx->width;
        videoFrame->height = cctx->height;

        if ((err = av_frame_get_buffer(videoFrame, 32)) < 0) {
            std::cout << "Failed to allocate picture" << err << std::endl;
            return;
        }
    }


    if (!swsCtx) {
        swsCtx = sws_getContext(cctx->width, cctx->height, AV_PIX_FMT_YUV420P, cctx->width, cctx->height,
            AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
    }

    int inLinesize[3] = { yuvData->yRowBytes, dstline, dstline };
    const uint8_t * linedata[3] = { ydata, udata, vdata };

    // From RGB to YUV
    sws_scale(swsCtx, linedata, inLinesize, 0, cctx->height, videoFrame->data,
        videoFrame->linesize);
    //90k
    videoFrame->pts = (frameCounter++) * stream->time_base.den / (stream->time_base.num * fps);


    //std::cout << videoFrame->pts << " " << cctx->time_base.num << " " << cctx->time_base.den << " " << frameCounter
      //        << std::endl;

    if ((err = avcodec_send_frame(cctx, videoFrame)) < 0) {
        std::cout << "Failed to send frame" << err << std::endl;
        return;
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    int ret = 0;
    if ((ret = avcodec_receive_packet(cctx, &pkt)) == 0) {
        static int counter = 0;
        std::cout << "pkt key: " << (pkt.flags & AV_PKT_FLAG_KEY) << " " << pkt.size << " " << (counter++) << std::endl;
        uint8_t* size = ((uint8_t*)pkt.data);
        std::cout << "first: " << (int)size[0] << " " << (int)size[1] << " " << (int)size[2] << " " << (int)size[3]
            << " " << (int)size[4] << " " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7]
            << std::endl;

        av_interleaved_write_frame(ofctx, &pkt);
    }
    std::cout << "push: " << ret << std::endl;
    av_packet_unref(&pkt);

    delete[]udata;
    delete[]vdata;
}

void FFmpegFileWriter::WriteFrameYUV420(uint8_t* ydata, uint8_t* udata, uint8_t* vdata)
{
    int err;
    if (!videoFrame) {
        videoFrame = av_frame_alloc();
        videoFrame->format = AV_PIX_FMT_YUV420P;
        videoFrame->width = cctx->width;
        videoFrame->height = cctx->height;

        if ((err = av_frame_get_buffer(videoFrame, 32)) < 0) {
            std::cout << "Failed to allocate picture" << err << std::endl;
            return;
        }
    }

    int inLinesize[3] = { cctx->width, cctx->width / 2, cctx->width / 2 };
    const uint8_t const* linedata[3] = { ydata, udata, vdata };

    memcpy(videoFrame->data, linedata, sizeof(linedata));
    memcpy(videoFrame->linesize, inLinesize, sizeof(inLinesize));
    //90k
    videoFrame->pts = (frameCounter++) * stream->time_base.den / (stream->time_base.num * fps);


    //std::cout << videoFrame->pts << " " << cctx->time_base.num << " " << cctx->time_base.den << " " << frameCounter
      //        << std::endl;

    if ((err = avcodec_send_frame(cctx, videoFrame)) < 0) {
        std::cout << "Failed to send frame" << err << std::endl;
        return;
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    int ret = 0;
    if ((ret = avcodec_receive_packet(cctx, &pkt)) == 0) {
        static int counter = 0;
        std::cout << "pkt key: " << (pkt.flags & AV_PKT_FLAG_KEY) << " " << pkt.size << " " << (counter++) << std::endl;
        uint8_t* size = ((uint8_t*)pkt.data);
        std::cout << "first: " << (int)size[0] << " " << (int)size[1] << " " << (int)size[2] << " " << (int)size[3]
            << " " << (int)size[4] << " " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7]
            << std::endl;

        av_interleaved_write_frame(ofctx, &pkt);
    }
    std::cout << "push: " << ret << std::endl;
    av_packet_unref(&pkt);
}

void FFmpegFileWriter::FinishWrite()
{
    // DELAYED FRAMES
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (;;) {
        avcodec_send_frame(cctx, NULL);
        if (avcodec_receive_packet(cctx, &pkt) == 0) {
            av_interleaved_write_frame(ofctx, &pkt);
            std::cout << "final push: " << std::endl;
        }
        else {
            break;
        }
    }

    av_packet_unref(&pkt);

    av_write_trailer(ofctx);
    if (!(oformat->flags & AVFMT_NOFILE)) {
        int err = avio_close(ofctx->pb);
        if (err < 0) {
            std::cout << "Failed to close file" << err << std::endl;
        }
    }

    if (videoFrame) {
        av_frame_free(&videoFrame);
    }

    if (cctx) {
        avcodec_free_context(&cctx);
    }
    if (ofctx) {
        avformat_free_context(ofctx);
    }
    if (swsCtx) {
        sws_freeContext(swsCtx);
    }

}
