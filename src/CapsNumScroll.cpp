#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

#define APPLICATION_NAME "CapsNumScroll"

#define WINDOW_SIZE 75

#define HIDE_WINDOW_TIMER_EVENT 0x1010
#define HIDE_WINDOW_TIMER_INTERVAL 1000

#define NOTIFY_ICON_EVENT WM_USER + 0x1011
#define MENU_QUIT WM_USER + 0x1012

static BOOL running;
static HWND mainWindow;
static RECT primaryRect;
static HMENU mainMenu;

// TODO Show images corresponding to keyboard state
// TODO Show system tray icon

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK
LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

VOID DisplayKeyState(DWORD);

BOOL CALLBACK
MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

VOID CALLBACK
HideWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

enum { CAPS_LOCK = 20, NUM_LOCK, SCROLL_LOCK };

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    mainMenu = CreatePopupMenu();
    AppendMenu(mainMenu, MF_ENABLED | MF_STRING, MENU_QUIT, "&Quit");

    WNDCLASSEX wndcls = {};
    wndcls.cbSize = sizeof(WNDCLASSEX);
    wndcls.style = CS_NOCLOSE;
    wndcls.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wndcls.hInstance = hInstance;
    wndcls.lpfnWndProc = WindowProc;
    wndcls.lpszClassName = APPLICATION_NAME "Window";

    RegisterClassEx(&wndcls);

    int margin = (int)(1.5 * WINDOW_SIZE);
    mainWindow = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
                                APPLICATION_NAME "Window",
                                APPLICATION_NAME,
                                WS_POPUP | WS_BORDER,
                                primaryRect.right - margin,
                                primaryRect.bottom - margin,
                                WINDOW_SIZE,
                                WINDOW_SIZE,
                                NULL,
                                NULL,
                                hInstance,
                                (LPVOID)NULL);

    NOTIFYICONDATA notifyIconData = {};
    notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifyIconData.hWnd = mainWindow;
    strcpy(notifyIconData.szTip, APPLICATION_NAME);
    notifyIconData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    notifyIconData.uCallbackMessage = NOTIFY_ICON_EVENT;
    notifyIconData.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    Shell_NotifyIcon(NIM_ADD, &notifyIconData);

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    ShowWindow(mainWindow, SW_HIDE);
    UpdateWindow(mainWindow);

    running = TRUE;
    while (running)
    {
        MSG messages;
        GetMessage(&messages, mainWindow, 0, 0);

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
    switch (uMsg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        running = FALSE;
        break;
    case WM_COMMAND:
        running = LOWORD(wParam) != MENU_QUIT;
        break;
    case NOTIFY_ICON_EVENT:
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            POINT clickPosition;
            GetCursorPos(&clickPosition);
            TrackPopupMenuEx(mainMenu,
                             TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
                             clickPosition.x, clickPosition.y,
                             hwnd,
                             NULL);
        }
        break;
    default:
        DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return TRUE;
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
        if (keyState)
        {
            KillTimer(mainWindow, HIDE_WINDOW_TIMER_EVENT);
            ShowWindow(mainWindow, SW_SHOW);
        }
        else
        {
            SetTimer(mainWindow, HIDE_WINDOW_TIMER_EVENT, HIDE_WINDOW_TIMER_INTERVAL, HideWindowTimerProc);
        }
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

#if 0
    printf("%s: left|right: %d|%d, top|bottom: %d|%d\n",
           info.szDevice, tmp.left, tmp.right, tmp.top, tmp.bottom);
#endif

    return TRUE;
}

VOID CALLBACK
HideWindowTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD)
{
    KillTimer(hwnd, HIDE_WINDOW_TIMER_EVENT);
    ShowWindow(hwnd, SW_HIDE);
}

