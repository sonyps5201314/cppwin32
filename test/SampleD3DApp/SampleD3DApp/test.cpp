#include "pch.h"


using namespace win32::Windows::Win32::UI::WindowsAndMessaging;
using namespace win32::Windows::Win32::UI::Input::KeyboardAndMouse;
using namespace win32::Windows::Win32::System::Console;
int main()
{
	char buf[1000];
	wsprintfA({ (uint8_t*)buf }, { (uint8_t*)"%p %p" }, (void*)0x123, main);
	auto hWnd = GetActiveWindow();
	auto hConsoleWnd = GetConsoleWindow();
	SetConsoleTitleW({ (char*)L"Hello World!" });
	SetWindowPos(hConsoleWnd, { (void*)HWND::HWND_TOPMOST }, 0, 0, 100, 100, (SET_WINDOW_POS_FLAGS)((uint32_t)SET_WINDOW_POS_FLAGS::SWP_NOMOVE | (uint32_t)SET_WINDOW_POS_FLAGS::SWP_NOACTIVATE));
	return 0;
}