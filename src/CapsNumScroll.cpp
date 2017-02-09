#include <windows.h>
#include "resource.h"

#define APPLICATION_NAME "CapsNumScroll"

#define WINDOW_SIZE 75

#define HIDE_WINDOW_TIMER_EVENT 0x1010
#define HIDE_WINDOW_TIMER_INTERVAL 0x5A0

#define NOTIFY_ICON_EVENT WM_USER + 0x1011
#define MENU_QUIT WM_USER + 0x1012

static HINSTANCE hMainInstance;
static HWND hMainWindow;
static NOTIFYICONDATA notifyIconData;
static RECT primaryRect;
static HMENU hMainMenu;
static HBITMAP hBitmap;
static DWORD resourceId;

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK
LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

VOID DisplayKeyState(DWORD);

BOOL CALLBACK
MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

VOID CALLBACK
HideWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

enum { CAPS_LOCK = 20, NUM_LOCK = 144, SCROLL_LOCK };

VOID 
RegisterMainWindowClass()
{
    WNDCLASSEX wndcls = { 0 };
    wndcls.cbSize = sizeof(WNDCLASSEX);
    wndcls.hIcon = LoadIcon(hMainInstance, MAKEINTRESOURCE(ICON128));
    wndcls.hInstance = hMainInstance;
    wndcls.lpfnWndProc = WindowProc;
    wndcls.lpszClassName = APPLICATION_NAME "Window";

    RegisterClassEx(&wndcls);
}

VOID
CreateApplicationMenu()
{
    hMainMenu = CreatePopupMenu();
    AppendMenu(hMainMenu, MF_ENABLED | MF_STRING, MENU_QUIT, "E&xit");
}

VOID
CreateNotificationIcon()
{
    notifyIconData = { 0 };
    notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifyIconData.hWnd = hMainWindow;
    strcpy(notifyIconData.szTip, APPLICATION_NAME);
    notifyIconData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    notifyIconData.uCallbackMessage = NOTIFY_ICON_EVENT;
    notifyIconData.hIcon = LoadIcon(hMainInstance, MAKEINTRESOURCE(ICON128));
    Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

VOID
CreateMainWindow()
{
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    int margin = (int)(1.5 * WINDOW_SIZE);
#if 1
    hMainWindow = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST,
#else
    //hMainWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
#endif
                                APPLICATION_NAME "Window",
                                APPLICATION_NAME,
                                WS_POPUP | WS_BORDER,
                                primaryRect.right - margin,
                                primaryRect.bottom - margin,
                                WINDOW_SIZE,
                                WINDOW_SIZE,
                                NULL,
                                NULL,
                                hMainInstance,
                                (LPVOID)NULL);
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    hMainInstance = hInstance;

    RegisterMainWindowClass();
    CreateMainWindow();

    ShowWindow(hMainWindow, SW_HIDE);
    UpdateWindow(hMainWindow);

    CreateApplicationMenu();
    CreateNotificationIcon();

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    MSG messages;
    while (GetMessage(&messages, hMainWindow, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    UnhookWindowsHookEx(hook);
    Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
    UnregisterClass(APPLICATION_NAME "Window", hInstance);

    return 0;
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = TRUE;

    switch (uMsg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COMMAND: 
        if (LOWORD(wParam) == MENU_QUIT) PostQuitMessage(0);
        break;
    case WM_PAINT:
        {
            hBitmap = LoadBitmap(hMainInstance, MAKEINTRESOURCE(resourceId)); 

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hMainWindow, &ps);

            HDC hdcMem = CreateCompatibleDC(hdc);
            HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);

            BITMAP bitmap;
            GetObject(hBitmap, sizeof(BITMAP), &bitmap);

            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc, 
                       0, 0, 
                       WINDOW_SIZE, WINDOW_SIZE, 
                       hdcMem, 
                       0, 0, bitmap.bmWidth, bitmap.bmHeight, 
                       SRCCOPY);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            EndPaint(hMainWindow, &ps);

            SetWindowLong(hMainWindow, 
                    GWL_EXSTYLE, 
                    GetWindowLong(hMainWindow, GWL_EXSTYLE) | WS_EX_LAYERED);

            SetLayeredWindowAttributes(hMainWindow, 0, 255*0.9, LWA_ALPHA);
        } 
        break;
    case NOTIFY_ICON_EVENT:
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            POINT clickPosition;
            GetCursorPos(&clickPosition);
            TrackPopupMenuEx(hMainMenu,
                             TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
                             clickPosition.x, clickPosition.y,
                             hwnd,
                             NULL);
        }
        break;
    default:
        result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return result;
}

LRESULT CALLBACK
LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) goto nextHook;

    if (wParam == WM_KEYUP)
    {
        KBDLLHOOKSTRUCT *kbdhook = (KBDLLHOOKSTRUCT *) lParam;
        DisplayKeyState(kbdhook->vkCode);
    }

nextHook:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

VOID
DisplayKeyState(DWORD keyCode)
{
    SHORT keyState = GetKeyState(keyCode) & 0x3;
    switch (keyCode)
    {
    case CAPS_LOCK:
        KillTimer(hMainWindow, HIDE_WINDOW_TIMER_EVENT);
        InvalidateRgn(hMainWindow, NULL, TRUE);
        if (keyState)
        {
            resourceId = CAPS_ON;
            ShowWindow(hMainWindow, SW_SHOW);
        }
        else
        {
            resourceId = CAPS_OFF;
            SetTimer(hMainWindow, HIDE_WINDOW_TIMER_EVENT, HIDE_WINDOW_TIMER_INTERVAL, HideWindowTimerProc);
        }
        UpdateWindow(hMainWindow);
        break;
    }
}

BOOL CALLBACK
MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM)
{
    MONITORINFOEX info;
    info.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &info);

    RECT tmp = info.rcWork;
    if (info.dwFlags) primaryRect = tmp;

    return TRUE;
}

VOID CALLBACK
HideWindowTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD)
{
    KillTimer(hwnd, HIDE_WINDOW_TIMER_EVENT);
    ShowWindow(hwnd, SW_HIDE);
}
