#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "x64\\Debug\\WinNTL-9_8_1.lib")
#else
#pragma comment(lib, "x64\\Release\\WinNTL-9_8_1.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "Debug\\WinNTL-9_8_1.lib")
#else
#pragma comment(lib, "Release\\WinNTL-9_8_1.lib")
#endif
#endif

#include "WinNTL\include\NTL\ZZ.h"
#include <windows.h>

using namespace NTL;

TCHAR szClassName[] = TEXT("Window");

long witness(const ZZ& n, const ZZ& x)
{
	ZZ m, y, z;
	long j, k;
	if (x == 0)return 0;
	m = n - 1;
	k = MakeOdd(m);
	z = PowerMod(x, m, n);//z=x^m%n
	if (z == 1)return 0;
	j = 0;
	do
	{
		y = z;
		z = (y*y) % n;
		j++;
	} while (j<k&&z != 1);
	return z != 1 || y != n - 1;
}

long PrimeTest(const ZZ& n, long t)
{
	if (n <= 1)return 0;
	PrimeSeq s;
	long p;
	p = s.next();
	while (p&&p<10000)
	{// 小さい素数で割ってみる
		if ((n%p) == 0)return(n == p);
		p = s.next();
	}
	// Miller-Rabinテストをt回実行
	ZZ x;
	long i;
	for (i = 0; i<t; i++)
	{
		x = RandomBnd(n);//0からn-1までのランダムな数
		if (witness(n, x))
			return 0;
	}
	return 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit1;
	static HWND hButton;
	switch (msg)
	{
	case WM_CREATE:
		hEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"),
			TEXT("2519590847565789349402718324004839857142928212")
			TEXT("6204032027777137836043662020707595556264018525")
			TEXT("8807844069182906412495150821892985591491761845")
			TEXT("0280848912007284499268739280728777673597141834")
			TEXT("7270261896375014971824691165077613379859095700")
			TEXT("0973304597488084284017974291006424586918171951")
			TEXT("1874612151517265463228221686998754918242243363")
			TEXT("7259085141865462043576798423387184774447920739")
			TEXT("9342365848238242811981638150106748104516603773")
			TEXT("0605620161967625613384414360383390441495263443")
			TEXT("2190114657544454178424020924616515723350778707")
			TEXT("7498171257724679629263863563732899121548314381")
			TEXT("6789988504044536402352738195137863656439121201")
			TEXT("0397122822120720357"),
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_NUMBER | ES_MULTILINE | ES_AUTOVSCROLL,
			80, 10, 512, 64, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("計算"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit1, 10, 10, 512, 512, TRUE);
		MoveWindow(hButton, 10, 20 + 512, 256, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			ZZ n;
			int len = GetWindowTextLength(hEdit1) + 1;
			LPSTR str = (LPSTR)GlobalAlloc(GMEM_FIXED, len);
			GetWindowTextA(hEdit1, str, len);
			conv(n, str);
			GlobalFree(str);
			if (PrimeTest(n, 10))
			{
				MessageBox(
					hWnd,
					TEXT("素数の可能性があります。"),
					TEXT("確認"),
					0);
			}
			else
			{
				MessageBox(
					hWnd,
					TEXT("合成数です。"),
					TEXT("確認"),
					0);
			}
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