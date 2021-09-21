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

void Tick()
{
    if (!bgfxInit)
        return;

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