#include <type_traits>
namespace PSDK {}
#include <win32/Windows.Win32.Web.MsHtml.h>
#include <win32/Windows.Win32.System.Diagnostics.ToolHelp.h>
#include <win32/Windows.Win32.System.SystemInformation.h>
#include <win32/Windows.Win32.Devices.Bluetooth.h>
#include <win32/Windows.Win32.Networking.Ldap.h>
#if !defined(_INC_SDKDDKVER) || !defined(_INC_WINAPIFAMILY) || !defined(_INC_WINPACKAGEFAMILY)
#error Required_SDK_configuration_header_was_not_included
#endif

namespace toolhelp = win32::Windows::Win32::System::Diagnostics::ToolHelp;
static_assert(std::is_same_v<decltype(&toolhelp::Module32First), BOOL(__stdcall*)(HANDLE, tagMODULEENTRY32*)>);
#ifdef UNICODE
static_assert(std::is_same_v<LPMODULEENTRY32, MODULEENTRY32W*>);
#else
static_assert(std::is_same_v<LPMODULEENTRY32, tagMODULEENTRY32*>);
#endif
