#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "WinNTL-9_8_1\\x64\\Debug\\WinNTL-9_8_1.lib")
#else
#pragma comment(lib, "WinNTL-9_8_1\\x64\\Release\\WinNTL-9_8_1.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "WinNTL-9_8_1\\Debug\\WinNTL-9_8_1.lib")
#else
#pragma comment(lib, "WinNTL-9_8_1\\Release\\WinNTL-9_8_1.lib")
#endif
#endif

#include "WinNTL-9_8_1\include\NTL\ZZ.h"
#include <windows.h>

using namespace NTL;

TCHAR szClassName[] = TEXT("Window");

long len(const ZZ& n)
{
	long i;
	ZZ num;
	num = abs(n);
	for (i = 0; num != 0; i++)num /= 10;
	return (n > 0) ? i : i + 1;
}

void zztostr(const ZZ& n, char* str, int len)
{
	ZZ num;
	num = abs(n);
	str[len--] = '\0';
	do {
		str[len--] = '0' + (char)(num % 10);
		num /= 10;
	} while (num != 0);
	if (n < 0)str[0] = '-';
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	switch (msg)
	{
	case WM_CREATE:
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("計算"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_SIZE:
		MoveWindow(hButton, 10, 10, 256, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			ZZ a = to_ZZ("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");
			ZZ b = to_ZZ("112233445566778899001122334455667788990011223344556677889900112233445566778899001122334455667788990011223344556677889900");
			char str[1000];
			a *= b;
			zztostr(a, str, len(a));
			MessageBoxA(hWnd, str, 0, 0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("NTLを使ったサンプルプログラム"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
