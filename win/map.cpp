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

bool ReadNextBlock(std::vector<unsigned char>& data)
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
    std::vector<unsigned char> vidData;
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
    std::vector<unsigned char> data;
    while (ReadNextBlock(data))
    {
        s_binfile.read((char*)data.data(), sz);
        if (data.size() == 8)
        {
            size_t val = *(size_t*)data.data();
            if (val == 1234)
            {
                DepthFrame df;
                ReadNextBlock(data);
                memcpy(&df.props, data.data(), data.size());
                ReadNextBlock(df.vidData);
                std::vector<unsigned char> ddata;
                ReadNextBlock(ddata);
                df.depthData.resize(ddata.size() / sizeof(float));
                memcpy(df.depthData.data(), ddata.data(), ddata.size());
                depthFrames.push_back(df);
            }
            else if (val == 5678)
            {
                FaceFrame ff;
                ReadNextBlock(data);
                memcpy(&ff.props, data.data(), sizeof(ff.props));

                ReadNextBlock(data);
                ff.vertices.resize(data.size() / sizeof(float));
                memcpy(ff.vertices.data(), data.data(), data.size());
                ReadNextBlock(data);
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

void Tick()
{
    if (!bgfxInit)
        return;

#ifdef SENDFRAMES

    if (firsttick)
    {
        ReadAllBlocks();
        sCurtime = sStartTime;
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
        app.OnDepthBuffer(depthFrames[curDepthIdx].vidData, depthFrames[curDepthIdx].depthData, depthFrames[curDepthIdx].props);
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
    app.Tick(elapsedTime);

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

