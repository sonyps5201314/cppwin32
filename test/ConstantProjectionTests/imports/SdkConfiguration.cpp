#pragma warning(error: 4005)
#include <sdkddkver.h>
#include <winapifamily.h>
namespace PSDK {}
#include <win32/Windows.Win32.Foundation.h>
#include <win32/Windows.Win32.System.SystemInformation.h>
static_assert(NTDDI_WIN10 == 0x0A000000);
static_assert(_WIN32_WINNT_WIN10 == 0x0A00);
static_assert(WINAPI_FAMILY_DESKTOP_APP == 100);
