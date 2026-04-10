#ifdef PICA_ON_DESKTOP

#include <stdio.h>
#include <windows.h>

#include "OurDisplay.h"
#include "gles2/gl2.h"
#include "system/pod/dmpgl.h"

// Pointeurs de fonctions WGL basiques
typedef HGLRC(WINAPI* PFWGLCREATECONTEXT)(HDC);
typedef BOOL(WINAPI* PFWGLMAKECURRENT)(HDC, HGLRC);
typedef BOOL(WINAPI* PFWGLDELETECONTEXT)(HGLRC);

PFWGLCREATECONTEXT wglCreateContext;
PFWGLMAKECURRENT wglMakeCurrent;
PFWGLDELETECONTEXT wglDeleteContext;

static HMODULE hOpenGL_lib = 0;
static HWND hWnd = 0;
static HDC hDC_wnd = 0;
static HGLRC hGLRC_wnd = 0;

static int enter_drawloop = 0;
int (*draw_func)(void) = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int endFlag = 0;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        if (draw_func && !endFlag && enter_drawloop)
        {
            if (draw_func() == 0)
            {
                PostQuitMessage(0);
                endFlag = 1;
            }
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static void create_native_window(const unsigned int width, const unsigned int height, const char* name)
{
    WNDCLASS wc = { 0 };
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT WindowRect = { 0, 0, (long)width, (long)height };
    HINSTANCE hInstance = GetModuleHandle(NULL);

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "OpenGL";

    if (!RegisterClass(&wc)) return;

    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

    hWnd = CreateWindowEx(dwExStyle, "OpenGL", name,
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL, NULL, hInstance, NULL);

    hDC_wnd = GetDC(hWnd);
}

void init_display(const unsigned int width, const unsigned int height, const char* name, int (*drawfunc)(void))
{
    PIXELFORMATDESCRIPTOR pfd_wnd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
        24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    int pidx;

    // Création de la fenêtre
    create_native_window(width, height, name);

    hOpenGL_lib = LoadLibrary("opengl32.dll");

    pidx = ChoosePixelFormat(hDC_wnd, &pfd_wnd);
    SetPixelFormat(hDC_wnd, pidx, &pfd_wnd);

    wglCreateContext = (PFWGLCREATECONTEXT)GetProcAddress(hOpenGL_lib, "wglCreateContext");
    wglMakeCurrent = (PFWGLMAKECURRENT)GetProcAddress(hOpenGL_lib, "wglMakeCurrent");
    wglDeleteContext = (PFWGLDELETECONTEXT)GetProcAddress(hOpenGL_lib, "wglDeleteContext");

    hGLRC_wnd = wglCreateContext(hDC_wnd);
    wglMakeCurrent(hDC_wnd, hGLRC_wnd);

    dmpglInitializePicaOnDesktop();

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

    draw_func = drawfunc;
}

void shutdown_display(void)
{
    dmpglFinalizePicaOnDesktop();

    if (hGLRC_wnd) wglDeleteContext(hGLRC_wnd);
    if (hOpenGL_lib) FreeLibrary(hOpenGL_lib);
    if (hDC_wnd) ReleaseDC(hWnd, hDC_wnd);
    if (hWnd) DestroyWindow(hWnd);
}

void swap_buffer(void)
{
    // Plus de bidouille de texture, on affiche directement le framebuffer !
    if (hDC_wnd) SwapBuffers(hDC_wnd);
}

void draw_loop(void)
{
    MSG msg;
    enter_drawloop = 1;

    while (1)
    {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT) break;
    }
}

#endif /* PICA_ON_DESKTOP */