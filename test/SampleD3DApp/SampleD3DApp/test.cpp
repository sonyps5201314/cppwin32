#include "pch.h"


using namespace win32::Windows::Win32::UI::WindowsAndMessaging;
using namespace win32::Windows::Win32::UI::Input::KeyboardAndMouse;
using namespace win32::Windows::Win32::System::Console;
int main()
{
	char buf[1000];
	wsprintfA(buf, "%p %p", 0x123, main);
	auto hWnd = GetActiveWindow();
	hWnd = nullptr;
	auto hConsoleWnd = GetConsoleWindow();
	SetConsoleTitleW(L"Hello World!");
	SetWindowPos(hConsoleWnd, { (void*)MACRO_HWND::HWND_TOPMOST }, 0, 0, 100, 100, (SET_WINDOW_POS_FLAGS)((uint32_t)SET_WINDOW_POS_FLAGS::SWP_NOMOVE | (uint32_t)SET_WINDOW_POS_FLAGS::SWP_NOACTIVATE));
	return 0;
}