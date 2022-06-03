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

namespace sam
{

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
        cctx->time_base = AVRational{ 1, fps };
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
        const uint8_t* linedata[3] = { ydata, udata, vdata };

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
            av_interleaved_write_frame(ofctx, &pkt);
        }
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
            uint8_t* size = ((uint8_t*)pkt.data);
            av_interleaved_write_frame(ofctx, &pkt);
        }
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
#define INBUF_SIZE 4096

    static int open_input_file(const char* filename);
    FFmpegFileReader::FFmpegFileReader(const std::string& outname)
    {        
        av_register_all();
        avcodec_register_all();

        open_input_file(outname.c_str());

    }


    static AVFormatContext* fmt_ctx;
    static AVCodecContext* dec_ctx;
    static int video_stream_index = -1;
    static int64_t last_pts = AV_NOPTS_VALUE;
    void readframes();
    static int open_input_file(const char* filename)
    {
        AVCodec* dec;
        int ret;

        if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
            return ret;
        }

        if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
            return ret;
        }

        /* select the video stream */
        ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
            return ret;
        }
        video_stream_index = ret;

        /* create decoding context */
        dec_ctx = avcodec_alloc_context3(dec);
        if (!dec_ctx)
            return AVERROR(ENOMEM);
        avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);

        /* init the video decoder */
        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
            return ret;
        }

        readframes();
        return 0;
    }


    void readframes()
    {
        int ret;
        AVPacket* packet;
        AVFrame* frame;
        AVFrame* filt_frame;
        frame = av_frame_alloc();
        filt_frame = av_frame_alloc();
        packet = av_packet_alloc();
        /* read all packets */
        while (1) {
            if ((ret = av_read_frame(fmt_ctx, packet)) < 0)
                break;

            if (packet->stream_index == video_stream_index) {
                ret = avcodec_send_packet(dec_ctx, packet);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(dec_ctx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    }
                    else if (ret < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                        return;
                    }

                    frame->pts = frame->best_effort_timestamp;

                    av_frame_unref(frame);
                }
            }
            av_packet_unref(packet);
        }
    }
}
#if 0
    static void video_decode_example(const char* outfilename, const char* filename)
    {
        AVCodec* codec;
        AVCodecContext* c = NULL;
        int frame_count;
        FILE* f;
        AVFrame* frame;
        uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
        AVPacket avpkt;

        av_init_packet(&avpkt);

        /* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
        memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

        printf("Decode video file %s to %s\n", filename, outfilename);

        /* find the mpeg1 video decoder */
        codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
        if (!codec) {
            fprintf(stderr, "Codec not found\n");
            exit(1);
        }

        c = avcodec_alloc_context3(codec);
        if (!c) {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }

        if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
            c->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames

        /* For some codecs, such as msmpeg4 and mpeg4, width and height
           MUST be initialized there because this information is not
           available in the bitstream. */

           /* open it */
        if (avcodec_open2(c, codec, NULL) < 0) {
            fprintf(stderr, "Could not open codec\n");
            exit(1);
        }

        f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "Could not open %s\n", filename);
            exit(1);
        }

        frame = av_frame_alloc();
        if (!frame) {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }

        frame_count = 0;
        for (;;) {
            avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
            if (avpkt.size == 0)
                break;

            /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
               and this is the only method to use them because you cannot
               know the compressed data size before analysing it.

               BUT some other codecs (msmpeg4, mpeg4) are inherently frame
               based, so you must call them with all the data for one
               frame exactly. You must also initialize 'width' and
               'height' before initializing them. */

               /* NOTE2: some codecs allow the raw parameters (frame size,
                  sample rate) to be changed at any frame. We handle this, so
                  you should also take care of it */

                  /* here, we use a stream based decoder (mpeg1video), so we
                     feed decoder and see if it could decode a frame */
            avpkt.data = inbuf;
            while (avpkt.size > 0)
                if (decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 0) < 0)
                    exit(1);
        }

        /* some codecs, such as MPEG, transmit the I and P frame with a
           latency of one frame. You must do the following to have a
           chance to get the last frame of the video */
        avpkt.data = NULL;
        avpkt.size = 0;
        decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 1);

        fclose(f);

        avcodec_close(c);
        av_free(c);
        av_frame_free(&frame);
        printf("\n");
    }



    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return;
    }

    FILE* f = fopen(outname.c_str(), "rb");
    AVFrame* frame = av_frame_alloc();

    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;

    av_init_packet(&avpkt);
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    int frame_count = 0;
    for (;;) {
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        if (avpkt.size == 0)
            break;

        /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
           and this is the only method to use them because you cannot
           know the compressed data size before analysing it.

           BUT some other codecs (msmpeg4, mpeg4) are inherently frame
           based, so you must call them with all the data for one
           frame exactly. You must also initialize 'width' and
           'height' before initializing them. */

           /* NOTE2: some codecs allow the raw parameters (frame size,
              sample rate) to be changed at any frame. We handle this, so
              you should also take care of it */

              /* here, we use a stream based decoder (mpeg1video), so we
                 feed decoder and see if it could decode a frame */
        avpkt.data = inbuf;
        while (avpkt.size > 0)
        {
            int len, got_frame;
            char buf[1024];

            len = avcodec_decode_video2(c, frame, &got_frame, &avpkt);
            if (len < 0) {
                break;
            }
            if (avpkt.data) {
                avpkt.size -= len;
                avpkt.data += len;
            }
        }
    }
#endif
