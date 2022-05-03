#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include "map.h"
#include <chrono>
#include <stdio.h>
#include "shellscalingapi.h"
#include <Windows.h>
#include <ShlObj.h>
#include <windowsx.h>
#include <bx/bx.h>
#include <bx/spscqueue.h>
#include <bx/thread.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "Application.h"
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BYTE prevKeys[256];


LRESULT KeyboardHookproc(
    int code,
    WPARAM wParam,
    LPARAM lParam
);

int StartWrite(uint32_t w, uint32_t h);
void FinishWrite();
static void pushVidFrame(uint8_t* data);
static void pushYUV420Frame(uint8_t* ydata, uint8_t* udata, uint8_t* vdata);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    memset(prevKeys, 0, sizeof(prevKeys));

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }
   
    //SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookproc, hInstance, 0);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAP));

    MSG msg;
    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MAP);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void Tick();

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

LARGE_INTEGER frequency;        // ticks per second
float timePeriod;
LARGE_INTEGER startTime;

RECT curWindowRect;
HWND hWnd;
bool bgfxInit = false;
sam::Application app;

void WriteDbgMessage(const char* str)
{
    OutputDebugString(str);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    int pro12maxW = 1284;
    int pro12maxH = 2778;
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, pro12maxW, pro12maxH, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    RECT rect;
    GetClientRect(hWnd, &rect);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, 0, 17, nullptr);
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);

    timePeriod = 1.0f / frequency.QuadPart;

    bgfx::renderFrame();
    bgfx::Init init;
    init.platformData.nwh = hWnd;
    GetClientRect(hWnd, &curWindowRect);
    init.resolution.width = (uint32_t)curWindowRect.right;
    init.resolution.height = (uint32_t)curWindowRect.bottom;
    init.resolution.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init))
        return 1;
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    // Set view 0 to the same dimensions as the window and to clear the color buffer.
    const bgfx::ViewId kClearView = 0;
    bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);
    bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
    bgfxInit = true;

    CHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

    app.SetDebugMsgFunc(WriteDbgMessage);
    GetCurrentDirectory(MAX_PATH, my_documents);
    app.Initialize(my_documents);
    app.Resize(rect.right, rect.bottom);
    return TRUE;
}

int blowup = 0;
int screenshot = 0;
int premult = 0;
int xm = 0;
int ym = 0;
int xWin = 0;
int yWin = 0;

float prevt = 0, cpuTime = 0;

const char* pszWindowClass = "WindowClass";


static float tick = 0;

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static std::vector<char> keystate(256);

    switch (message)
    {
    case WM_TIMER:
    case WM_PAINT:
    {
        Tick();
    }
    break;
    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        app.Resize(rect.right, rect.bottom);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN:
        app.TouchDown((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), 0);
        break;
    case WM_LBUTTONUP:
        app.TouchUp(0);
        break;
    case WM_MOUSEMOVE:
        app.TouchMove((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), 0);
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (keystate[wParam] == 0)
        {
            app.KeyDown(wParam);
            keystate[wParam] = 1;
        }
        break;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (keystate[wParam] != 0)
        {
            app.KeyUp(wParam);
            keystate[wParam] = 0;
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#define SENDFRAMES 1


static std::fstream s_binfile;

bool ReadNextBlock(std::vector<uint8_t>& data)
{
    if (!s_binfile.is_open())
        s_binfile = std::fstream("C:\\homep4\\\sqwar\\file.binary", std::ios::binary | std::ios::in);
    size_t sz = 0;
    s_binfile.read((char*)&sz, sizeof(sz));
    data.resize(sz);
    s_binfile.read((char*)data.data(), sz);

    if (s_binfile.eof())
    {        
        s_binfile.clear();
        s_binfile.seekg(0, s_binfile.beg);
        return false;
    }

    return true;
}

struct DepthFrame
{
    sam::DepthDataProps props;
    std::vector<uint8_t> vidData;
    std::vector<float> depthData;
};

struct FaceFrame
{
    sam::FaceDataProps props;
    std::vector<float> vertices;
    std::vector<int16_t> indices;
};
std::vector<DepthFrame> depthFrames;
std::vector<FaceFrame> faceFrames;

static double sStartTime, sEndTime;

void ReadAllBlocks()
{
    size_t sz = 0;
    std::vector<uint8_t> data;
    bool moreData = true;
    while (moreData && ReadNextBlock(data))
    {
        s_binfile.read((char*)data.data(), sz);
        size_t streampos = s_binfile.tellg();
        if (data.size() == 8)
        {
            size_t val = *(size_t*)data.data();

            if (val == 1234)
            {
                DepthFrame df;
                ReadNextBlock(data);
                memcpy(&df.props, data.data(), data.size());
                moreData &= ReadNextBlock(df.vidData);
                std::vector<uint8_t> ddata;
                moreData &= ReadNextBlock(ddata);
                df.depthData.resize(ddata.size() / sizeof(float));
                memcpy(df.depthData.data(), ddata.data(), ddata.size());
                depthFrames.push_back(df);
            }
            else if (val == 5678)
            {
                FaceFrame ff;
                ReadNextBlock(data);
                memcpy(&ff.props, data.data(), sizeof(ff.props));

                moreData &= ReadNextBlock(data);
                ff.vertices.resize(data.size() / sizeof(float));
                memcpy(ff.vertices.data(), data.data(), data.size());
                moreData &= ReadNextBlock(data);
                ff.indices.resize(data.size() / sizeof(int16_t));
                memcpy(ff.indices.data(), data.data(), data.size());
                faceFrames.push_back(ff);
            }
        }
    }

    std::sort(depthFrames.begin(), depthFrames.end(), [](const DepthFrame& lhs, const DepthFrame& rhs)
        {
            return lhs.props.timestamp < rhs.props.timestamp;
        });
    
    std::sort(faceFrames.begin(), faceFrames.end(), [](const FaceFrame& lhs, const FaceFrame& rhs)
        {
            return lhs.props.timestamp < rhs.props.timestamp;
        });

    sStartTime = std::min(depthFrames[0].props.timestamp, faceFrames[0].props.timestamp);
    sEndTime = std::max(depthFrames.back().props.timestamp, faceFrames.back().props.timestamp);
}

static bool firsttick = true;
static double sCurtime = sStartTime;
static size_t curDepthIdx = 0;
static size_t curFaceIdx = 0;

namespace sam
{
    void ConvertDepthToYUV(float* data, int width, int height, float maxDepth, uint8_t* ydata, uint8_t* udata, uint8_t* vdata);
}
void Tick()
{
    if (!bgfxInit)
        return;

#ifdef SENDFRAMES

    if (firsttick)
    {
        ReadAllBlocks();
        sCurtime = sStartTime;
        StartWrite(depthFrames[0].props.depthWidth,
            depthFrames[0].props.depthHeight);
        for (auto& frame : depthFrames)
        {
            std::vector<uint8_t> depthYData(frame.props.depthWidth * frame.props.depthHeight);
            std::vector<uint8_t> depthUData((frame.props.depthWidth * frame.props.depthHeight) / 4);
            std::vector<uint8_t> depthVData((frame.props.depthWidth * frame.props.depthHeight) / 4);
            sam::ConvertDepthToYUV(frame.depthData.data() + 16, frame.props.depthWidth,
                frame.props.depthHeight, 10.0f, depthYData.data(), depthUData.data(), depthVData.data());
            pushYUV420Frame(depthYData.data(), depthUData.data(), depthVData.data());
        }
        FinishWrite();
    }

    size_t prevDepth = curDepthIdx;
    size_t prevFace = curFaceIdx;
    while (curDepthIdx < (depthFrames.size() - 1) &&
        depthFrames[curDepthIdx + 1].props.timestamp <= sCurtime)
        curDepthIdx++;

    while (curFaceIdx < (faceFrames.size() - 1) &&
        faceFrames[curFaceIdx + 1].props.timestamp <= sCurtime)
        curFaceIdx++;
    
    

    if (firsttick || curDepthIdx != prevDepth)
    {
        sam::DepthData dd;
        dd.vidData = depthFrames[curDepthIdx].vidData;
        dd.depthData = depthFrames[curDepthIdx].depthData;
        dd.props = depthFrames[curDepthIdx].props;
        app.OnDepthBuffer(dd);
    }
    if (firsttick || curFaceIdx != prevFace)
        app.OnFaceData(faceFrames[curFaceIdx].props, faceFrames[curFaceIdx].vertices, faceFrames[curFaceIdx].indices);
   
    firsttick = false;
    sCurtime += 1 / 60.0f;
    if (sCurtime > sEndTime)
    {
        sCurtime = sStartTime;
        curDepthIdx = curFaceIdx = 0;
    }

#endif
    LARGE_INTEGER cur;
    QueryPerformanceCounter(&cur);

    float elapsedTime = (float)(cur.QuadPart - startTime.QuadPart) * timePeriod;
    app.Tick(elapsedTime, sCurtime);

    RECT r;
    GetClientRect(hWnd, &r);

    int width = r.right;
    int height = r.bottom;
    if (r.right != curWindowRect.right || r.bottom != curWindowRect.bottom) {
        curWindowRect = r;
        bgfx::reset((uint32_t)r.right, (uint32_t)r.bottom, BGFX_RESET_VSYNC);
        bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
        bgfx::setViewRect(1, 0, 0, bgfx::BackbufferRatio::Equal);
    }
    // This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
    bgfx::touch(0);
    bgfx::touch(1);
    app.Draw();
}

LRESULT KeyboardHookproc(
    int code,
    WPARAM wParam,
    LPARAM lParam
)
{
    static std::vector<char> keystate(256);
    KBDLLHOOKSTRUCT* pKbb = (KBDLLHOOKSTRUCT*)lParam;
    switch (wParam)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (keystate[pKbb->vkCode] == 0)
        {
            app.KeyDown(pKbb->vkCode);
            keystate[pKbb->vkCode] = 1;
        }
        break;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (keystate[pKbb->vkCode] != 0)
        {
            app.KeyUp(pKbb->vkCode);
            keystate[pKbb->vkCode] = 0;
        }
        break;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}
#include <iostream>
AVFrame* videoFrame = nullptr;
AVCodecContext* cctx = nullptr;
SwsContext* swsCtx = nullptr;
int frameCounter = 0;
AVFormatContext* ofctx = nullptr;
AVOutputFormat* oformat = nullptr;
AVStream* stream = nullptr;
int fps = 30;
int bitrate = 2000;

struct YCrCbData
{
    int yOffset;
    int yRowBytes;
    int cbCrOffset;
    int cbCrRowBytes;
};


static void pushVidFrame(uint8_t* data)
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
    const uint8_t const * linedata[3] = { ydata, udata, vdata};

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

static void pushYUV420Frame(uint8_t* ydata, uint8_t* udata, uint8_t* vdata)
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

    int inLinesize[3] = { cctx->width, cctx->width/2, cctx->width/2};
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
static void finish()
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
}

static void free()
{
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

int StartWrite(uint32_t width, uint32_t height)
{
    av_register_all();
    avcodec_register_all();

    oformat = av_guess_format(nullptr, "test.mp4", nullptr);
    if (!oformat) {
        std::cout << "can't create output format" << std::endl;
        return -1;
    }
    //oformat->video_codec = AV_CODEC_ID_H265;

    int err = avformat_alloc_output_context2(&ofctx, oformat, nullptr, "test.mp4");

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
        if ((err = avio_open(&ofctx->pb, "test.mp4", AVIO_FLAG_WRITE)) < 0) {
            std::cout << "Failed to open file" << err << std::endl;
            return -1;
        }
    }

    if ((err = avformat_write_header(ofctx, NULL)) < 0) {
        std::cout << "Failed to write header" << err << std::endl;
        return -1;
    }

    av_dump_format(ofctx, 0, "test.mp4", 1);
    std::cout << stream->time_base.den << " " << stream->time_base.num << std::endl;
}

void FinishWrite()
{
    finish();
    free();
}