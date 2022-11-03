#include <Windows.h>
#include <WindowsX.h>

#pragma comment(lib, "Msimg32.lib")

HDC hdcBack;
HANDLE hbmBack;
RECT clientRect;

BITMAP bitmap;
HANDLE hBmp;

POINT rectCoordinates = { 0 };
POINT mousePosOnRect;
int mousePressed = 0;
int offset = 13;

int rectHeight;
int rectWidth;
int state = 0;

enum WorkingMode {
	Auto, Manual
};
enum WorkingMode workingMode = Manual;

enum TimerStatus {
	Timer, WaitTimer
};
enum TimerStatus currentTimer = Timer;
HANDLE waitTimer;

HBRUSH CurrentBackColor;
HBRUSH GreenBR;
HBRUSH GrayBR;

int durationX = 10;
int durationY = 10;

void AutoMode(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	int newXPos = rectCoordinates.x + durationX;
	int newYPos = rectCoordinates.y + durationY;

	if (newXPos < 0 || newXPos + rectWidth > clientRect.right)
		durationX = -durationX;
	else if (newYPos < 0 || newYPos + rectHeight > clientRect.bottom)
		durationY = -durationY;
	
	rectCoordinates.x += durationX;
	rectCoordinates.y += durationY;
	
	InvalidateRect(hWnd, NULL, FALSE);
}




HANDLE threadId;

void ChangeMode(HWND hWnd)
{
	if (workingMode == Manual)
	{
		SetTimer(hWnd, 0, 10, AutoMode);

		currentTimer = Timer;
		workingMode = Auto;
	}
	else if (workingMode == Auto)
	{
		if (currentTimer == Timer)
			KillTimer(hWnd, 0);
		else
		{
			CancelWaitableTimer(waitTimer);			
			WaitForSingleObject(threadId, INFINITE);
		}

		CurrentBackColor = GreenBR;
		workingMode = Manual;
	}
}

VOID CALLBACK TimerAPCProc(LPVOID lpArg, DWORD dwTimerLowValue,	DWORD dwTimerHighValue)
{
	SendMessage(lpArg, WM_USER, 0, 0);
}

DWORD WINAPI TimerThread(LPVOID lpParam)
{
	__int64         qwDueTime;
	LARGE_INTEGER   liDueTime;
	qwDueTime = -1 * 10;

	liDueTime.LowPart = (DWORD)(qwDueTime & 0xFFFFFFFF);
	liDueTime.HighPart = (LONG)(qwDueTime >> 32);

	SetWaitableTimer(waitTimer, &liDueTime, 20, TimerAPCProc, lpParam, FALSE);

	while (!(WaitForSingleObject(waitTimer, 100) != WAIT_OBJECT_0))
	{
		SleepEx(INFINITE, TRUE);
	}

	return 0;
}

void ChangeTypeOfTimer(HWND hWnd)
{
	if (workingMode != Auto)
		return;

	if (currentTimer == Timer)
	{
		KillTimer(hWnd, 0);
		
		threadId = CreateThread(NULL, 0, TimerThread, hWnd, 0, 0);

		CurrentBackColor = GreenBR;
		currentTimer = WaitTimer;
	}
	else
	{		
		CancelWaitableTimer(waitTimer);
		WaitForSingleObject(threadId, INFINITE);

		SetTimer(hWnd, 0, 10, AutoMode);

		CurrentBackColor = GreenBR;
		currentTimer = Timer;
	}
}

void InitializeBackBuffer(HWND hWnd, int width, int height)
{
	HDC hdcWindow;
	hdcWindow = GetDC(hWnd);
	hdcBack = CreateCompatibleDC(hdcWindow);
	hbmBack = CreateCompatibleBitmap(hdcWindow, width, height);
	SaveDC(hdcBack);
	SelectObject(hdcBack, hbmBack);
	ReleaseDC(hWnd, hdcWindow);	
}

void FinalizeBackBuffer()
{
	if (hdcBack)
	{
		RestoreDC(hdcBack, -1);
		DeleteObject(hbmBack);
		DeleteObject(hdcBack);
	}
}

BOOL OnSizeChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GetClientRect(hWnd, &clientRect);
	FinalizeBackBuffer();
	InitializeBackBuffer(hWnd, clientRect.right, clientRect.bottom);	
	InvalidateRect(hWnd, NULL, FALSE);
}

BOOL OnWindowPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	FillRect(hdcBack, &clientRect, CurrentBackColor);

	PaintRectWithPicture(hWnd, hdcBack);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcBack, 0, 0, SRCCOPY);
	EndPaint(hWnd, &ps);
}



BOOL PaintRectWithPicture(HWND hWnd, HDC hdc)
{
	if (state == 0)
		hBmp = LoadImage(NULL, L"onoshko.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (state == 1)
		hBmp = LoadImage(NULL, L"onoshkoR.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (state == 2)
		hBmp = LoadImage(NULL, L"onoshkoD.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (state == 3)
		hBmp = LoadImage(NULL, L"onoshkoL.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	GetObject(hBmp, sizeof(bitmap), &bitmap);
	rectHeight = 80;
	rectWidth = 90;
	HDC hMemDC = CreateCompatibleDC(hdc);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);
	
	if (hOldBmp)
	{
		SetMapMode(hMemDC, GetMapMode(hdc));
		TransparentBlt(hdc, rectCoordinates.x, rectCoordinates.y, rectWidth, rectHeight, hMemDC, 0, 0, bitmap.bmWidth, bitmap.bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC, hOldBmp);
	}
	DeleteDC(hMemDC);
	
	
}

BOOL LeftMouseButtonPressed(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);

	
	mousePosOnRect.x = x - rectCoordinates.x;
	mousePosOnRect.y = y - rectCoordinates.y;

	if ((mousePosOnRect.x >= 0) && (mousePosOnRect.x <= rectWidth) && (mousePosOnRect.y >= 0) && (mousePosOnRect.y <= rectHeight))
		mousePressed = 1;
}

BOOL MouseMove(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!mousePressed)
		return;  

	int x = LOWORD(lParam);
	int y = HIWORD(lParam);

	if ((x - mousePosOnRect.x >= clientRect.left) && (x - mousePosOnRect.x <= clientRect.right - rectWidth))
		rectCoordinates.x = x - mousePosOnRect.x;
	else
		if (x - mousePosOnRect.x < clientRect.left)
			rectCoordinates.x = 0;
		else
			rectCoordinates.x = clientRect.right - rectWidth;

	/*if ((y - mousePosOnRect.y >= clientRect.top) && (y - mousePosOnRect.y <= clientRect.bottom - rectHeight))
		rectCoordinates.y = y - mousePosOnRect.y;
	else
		if (y - mousePosOnRect.y < clientRect.top)
			rectCoordinates.y = 0;
		else
			rectCoordinates.y = clientRect.bottom - rectHeight;*/
	InvalidateRect(hWnd, NULL, FALSE);
}

BOOL LeftMouseButtonUp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	mousePressed = 0;
}

BOOL MouseWheel(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (mousePressed)
		return;

	int fwKey = GET_KEYSTATE_WPARAM(wParam);
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

	if (fwKey == MK_SHIFT)
		if ((rectCoordinates.x - (zDelta / 25) >= clientRect.left) && (rectCoordinates.x - (zDelta / 25) <= clientRect.right - rectWidth))
			rectCoordinates.x -= (zDelta / 25);
		else
			rectCoordinates.x += (zDelta / 25);
	else
		if ((rectCoordinates.y + (zDelta / 25) >= clientRect.top) && (rectCoordinates.y + (zDelta / 25) <= clientRect.bottom - rectHeight))
			rectCoordinates.y += (zDelta / 25);
		else
			rectCoordinates.y -= (zDelta / 25);

	InvalidateRect(hWnd, NULL, FALSE);
}




BOOL KeyDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (mousePressed)
		return;

	if (GetKeyState(VK_UP) < 0 && GetKeyState(VK_RIGHT) < 0) {
		if (rectCoordinates.y - offset <= clientRect.top)
			rectCoordinates.y += offset;
		else {
			rectCoordinates.y -= offset;
			rectCoordinates.x += offset;
		}
	}
	else if (GetKeyState(VK_UP) < 0 && GetKeyState(VK_LEFT) < 0) {
		if (rectCoordinates.x - offset <= clientRect.left)
			rectCoordinates.x += offset;
		else {
			rectCoordinates.y -= offset;
			rectCoordinates.x -= offset;
		}
	}
	else if (GetKeyState(VK_DOWN) < 0 && GetKeyState(VK_LEFT) < 0) {
		if (rectCoordinates.y + offset >= clientRect.bottom - rectHeight)
			rectCoordinates.y -= offset;
		else {
			rectCoordinates.x -= offset;
			rectCoordinates.y += offset;
		}
	}
	else if (GetKeyState(VK_DOWN) < 0 && GetKeyState(VK_RIGHT) < 0) {
		if (rectCoordinates.y + offset >= clientRect.bottom - rectHeight)
			rectCoordinates.y -= offset;
		else {
			rectCoordinates.x += offset;
			rectCoordinates.y += offset;
		}
	}

	switch (wParam) {
	case VK_UP:
		if (rectCoordinates.y - offset <= clientRect.top)
			rectCoordinates.y += offset;
		else
		{
			rectCoordinates.y -= offset;
			switch (state) {
			case 0:
				state = 1;
				break;
			case 1:
				state = 2;
				break;
			case 2:
				state = 3;
				break;
			case 3:
				state = 0;
				break;
			}
		}
		break;
	case VK_DOWN:
		if (rectCoordinates.y + offset >= clientRect.bottom - rectHeight)
			rectCoordinates.y -= offset;	
		else
		{
			rectCoordinates.y += offset;
			switch (state) {
			case 0:
				state = 1;
				break;
			case 1:
				state = 2;
				break;
			case 2:
				state = 3;
				break;
			case 3:
				state = 0;
				break;
			}
		}
		break;
	case VK_LEFT:
		if (rectCoordinates.x - offset <= clientRect.left)
			rectCoordinates.x += offset;
		else
		{
			rectCoordinates.x -= offset;
			switch (state) {
			case 0:
				state = 1;
				break;
			case 1:
				state = 2;
				break;
			case 2:
				state = 3;
				break;
			case 3:
				state = 0;
				break;
			}
		}
		break;
	case VK_RIGHT:
		if (rectCoordinates.x + offset >= clientRect.right - rectWidth)
			rectCoordinates.x -= offset;
		else
		{
			rectCoordinates.x += offset;
			switch (state) {
			case 0:
				state = 1;
				break;
			case 1:
				state = 2;
				break;
			case 2:
				state = 3;
				break;
			case 3:
				state = 0;
				break;
			}
		}
		break;
	case VK_TAB:
		ChangeMode(hWnd);
		break;
	case VK_RETURN:
		ChangeTypeOfTimer(hWnd);
		break;
	}

	InvalidateRect(hWnd, NULL, FALSE);
}

BOOL OnWindowCreate(HWND hWnd)
{
	GreenBR = CreateSolidBrush(RGB(0, 255, 0));
	GrayBR = CreateSolidBrush(RGB(128, 128, 128));

	CurrentBackColor = GreenBR;
	waitTimer = CreateWaitableTimer(NULL, FALSE, NULL);
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		OnWindowCreate(hWnd);
		return 0;
	case WM_DESTROY:
		FinalizeBackBuffer();
		CloseHandle(waitTimer);
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		OnSizeChanged(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_PAINT:
		OnWindowPaint(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_LBUTTONDOWN:
		LeftMouseButtonPressed(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_MOUSEMOVE:
		MouseMove(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_LBUTTONUP:
		LeftMouseButtonUp(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_MOUSEWHEEL:
		MouseWheel(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_KEYDOWN:
		KeyDown(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_USER:
		AutoMode(hWnd, 0, 0, 0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

ATOM RegisterMyClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = MainWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"MainWindowClass";
	wcex.hIconSm = wcex.hIcon;

	return RegisterClassEx(&wcex);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	HWND hMainWindow;
	MSG msg;

	if (!RegisterMyClass(hInstance))
	{
		return 0;
	}

	hMainWindow = CreateWindowEx(NULL, L"MainWindowClass", L"Onoshko_window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 
		CW_USEDEFAULT, 600, 600, NULL, NULL, hInstance, NULL);

	ShowWindow(hMainWindow, nCmdShow);

	while (GetMessage(&msg, 0, 0, 0))
	{
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}