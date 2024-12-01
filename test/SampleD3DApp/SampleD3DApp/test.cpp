#include "pch.h"

//typedef wchar_t* PWSTR;
enum INF_STYLE
{
	INF_STYLE_NONE = 0x0,
	INF_STYLE_OLDNT = 0x1,
	INF_STYLE_WIN4 = 0x2,
	INF_STYLE_CACHE_ENABLE = 0x10,
	INF_STYLE_CACHE_DISABLE = 0x20,
	INF_STYLE_CACHE_IGNORE = 0x40,
};
extern "C" void* __stdcall SetupOpenInfFileW(PWSTR FileName, PWSTR InfClass, INF_STYLE InfStyle, unsigned int* ErrorLine);
#pragma comment(lib,"Setupapi.lib")
extern "C" int    __cdecl rand(void);
extern "C" int __cdecl printf(const char* _Format, ...);
int main()
{
	auto pfnSetupOpenInfFileW = SetupOpenInfFileW;
	int x = (int)pfnSetupOpenInfFileW + rand();
	printf("%p %p", x,pfnSetupOpenInfFileW);
	return x;
}