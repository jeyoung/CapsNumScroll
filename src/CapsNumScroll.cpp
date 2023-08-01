#include <windows.h>
#include "resource.h"

#define APPLICATION_NAME "CapsNumScroll"

#define WINDOW_SIZE 60

#define HIDE_WINDOW_TIMER_EVENT 0x1010
#define HIDE_WINDOW_TIMER_INTERVAL 0x5A0

#define NOTIFY_ICON_EVENT WM_USER + 0x1011
#define MENU_QUIT WM_USER + 0x1012

#define MONITORS_LIMIT 128

static HINSTANCE hMainInstance;
static NOTIFYICONDATA notifyIconData;

static HMENU hMainMenu;
static HBITMAP hBitmap;
static DWORD resourceId;

static UINT monitorcounter;
static RECT monitorrects[MONITORS_LIMIT];
static HWND hWindows[MONITORS_LIMIT];

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK
LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

VOID
DisplayKeyState(DWORD);

BOOL CALLBACK
MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

VOID CALLBACK
HideWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

enum { CAPS_LOCK = 20, NUM_LOCK = 144, SCROLL_LOCK };

VOID
RegisterWindowClass()
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

/**
 * Create a notification icon, linking it to the first window to receive
 * notifications.
 */
VOID
CreateNotificationIcon()
{
    notifyIconData = { 0 };
    notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifyIconData.hWnd = hWindows[0];
    strcpy(notifyIconData.szTip, APPLICATION_NAME);
    notifyIconData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    notifyIconData.uCallbackMessage = NOTIFY_ICON_EVENT;
    notifyIconData.hIcon = LoadIcon(hMainInstance, MAKEINTRESOURCE(ICON128));
    Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

VOID
CreateWindows()
{
    monitorcounter = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    for (UINT i=0; i<monitorcounter; ++i) {
	RECT rect = monitorrects[i];
	int width = rect.right - rect.left;
	int left = rect.left + (0.5*(width-WINDOW_SIZE));
	int top = rect.bottom - (1.5*WINDOW_SIZE);
	hWindows[i] = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST,
		APPLICATION_NAME "Window",
		APPLICATION_NAME,
		WS_POPUP | WS_BORDER,
		left,
		top,
		WINDOW_SIZE,
		WINDOW_SIZE,
		NULL,
		NULL,
		hMainInstance,
		(LPVOID)NULL);
    }
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    hMainInstance = hInstance;

    RegisterWindowClass();
    CreateWindows();

    for (int i=0; i<monitorcounter; ++i) {
	ShowWindow(hWindows[i], SW_HIDE);
	UpdateWindow(hWindows[i]);
    }

    DisplayKeyState(CAPS_LOCK);

    CreateApplicationMenu();
    CreateNotificationIcon();

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    MSG messages;
    while (GetMessage(&messages, NULL, 0, 0))
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
	    if (LOWORD(wParam) == MENU_QUIT)
		PostQuitMessage(0);
	    break;
	case WM_PAINT:
	    hBitmap = LoadBitmap(hMainInstance, MAKEINTRESOURCE(resourceId));

	    for (int i=0; i<monitorcounter; ++i) {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWindows[i], &ps);

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

		EndPaint(hWindows[i], &ps);

		SetWindowLong(hWindows[i],
			GWL_EXSTYLE,
			GetWindowLong(hWindows[i], GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hWindows[i], 0, 255*0.9, LWA_ALPHA);
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
    if (nCode < 0)
	goto nextHook;

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
	    for (int i=0; i<monitorcounter; ++i) {
		KillTimer(hWindows[i], HIDE_WINDOW_TIMER_EVENT);
		InvalidateRgn(hWindows[i], NULL, TRUE);
		if (keyState)
		{
		    resourceId = CAPS_ON;
		    ShowWindow(hWindows[i], SW_SHOW);
		}
		else
		{
		    resourceId = CAPS_OFF;
		    SetTimer(hWindows[i], HIDE_WINDOW_TIMER_EVENT, HIDE_WINDOW_TIMER_INTERVAL, HideWindowTimerProc);
		}
		UpdateWindow(hWindows[i]);
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
    monitorrects[monitorcounter++] = info.rcWork;
    return TRUE;
}

VOID CALLBACK
HideWindowTimerProc(HWND hwnd, UINT, UINT_PTR, DWORD)
{
    KillTimer(hwnd, HIDE_WINDOW_TIMER_EVENT);
    ShowWindow(hwnd, SW_HIDE);
}
