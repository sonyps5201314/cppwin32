#include "../../SampleD3DApp/SampleD3DApp/pch.h"

auto sdk_first_import_symbol = &win32::Windows::Win32::Media::Audio::DirectSound::GetDeviceID;
auto sdk_second_import_symbol = &win32::Windows::Win32::System::TpmBaseServices::tbs_GetDeviceID;

static_assert(std::is_same_v<decltype(sdk_first_import_symbol), HRESULT(__stdcall*)(const LPCGUID, LPGUID)>);
static_assert(std::is_same_v<decltype(sdk_second_import_symbol), HRESULT(__stdcall*)(PBYTE, UINT32, PUINT32, BOOL*)>);
static_assert(std::is_same_v<LSA_UNICODE_STRING, _LSA_UNICODE_STRING>);
static_assert(std::is_same_v<PLSA_UNICODE_STRING, LSA_UNICODE_STRING*>);

static_assert(sizeof(::_LSA_UNICODE_STRING) == (sizeof(void*) == 8 ? 16 : 8));
static_assert(std::is_same_v<::LSA_UNICODE_STRING, ::_LSA_UNICODE_STRING>);

static_assert(std::is_same_v<MI_PropertyDecl, _MI_PropertyDecl>);
static_assert(sizeof(MI_PropertyDecl) == (sizeof(void*) == 8 ? 72 : 48));
static_assert(std::is_same_v<decltype(SEC_WINNT_AUTH_IDENTITY_A::Flags), SEC_WINNT_AUTH_IDENTITY_FLAGS>);
static_assert(std::is_same_v<decltype(SEC_WINNT_AUTH_IDENTITY_W::Flags), SEC_WINNT_AUTH_IDENTITY_FLAGS>);
template<typename R, typename First, typename Second, typename Third>
constexpr bool NativeNotifyMaskMatches(R(__stdcall*)(First, Second, Third)) { return std::is_same_v<Second, SERVICE_NOTIFY_FLAGS>; }
static_assert(NativeNotifyMaskMatches(&win32::Windows::Win32::System::Services::NotifyServiceStatusChangeA));

#if defined(UNICODE)
static_assert(std::is_same_v<SEC_WINNT_AUTH_IDENTITY, SEC_WINNT_AUTH_IDENTITY_W>);
static_assert(std::is_same_v<PSEC_WINNT_AUTH_IDENTITY, SEC_WINNT_AUTH_IDENTITY_W*>);
static_assert(std::is_same_v<SERVICE_NOTIFY, SERVICE_NOTIFY_2W>);
static_assert(std::is_same_v<PSERVICE_NOTIFY, SERVICE_NOTIFY_2W*>);
#else
static_assert(std::is_same_v<SEC_WINNT_AUTH_IDENTITY, SEC_WINNT_AUTH_IDENTITY_A>);
static_assert(std::is_same_v<PSEC_WINNT_AUTH_IDENTITY, SEC_WINNT_AUTH_IDENTITY_A*>);
static_assert(std::is_same_v<SERVICE_NOTIFY, SERVICE_NOTIFY_2A>);
static_assert(std::is_same_v<PSERVICE_NOTIFY, SERVICE_NOTIFY_2A*>);
#endif
static_assert(std::is_enum_v<SEC_WINNT_AUTH_IDENTITY_FLAGS>);
static_assert(std::is_enum_v<SERVICE_NOTIFY_FLAGS>);

namespace toolhelp = win32::Windows::Win32::System::Diagnostics::ToolHelp;
static_assert(std::is_same_v<decltype(&toolhelp::Module32First), BOOL(__stdcall*)(HANDLE, tagMODULEENTRY32*)>);
static_assert(std::is_same_v<decltype(&toolhelp::Module32Next), BOOL(__stdcall*)(HANDLE, tagMODULEENTRY32*)>);
static_assert(std::is_same_v<decltype(&toolhelp::Process32First), BOOL(__stdcall*)(HANDLE, tagPROCESSENTRY32*)>);
static_assert(std::is_same_v<decltype(&toolhelp::Process32Next), BOOL(__stdcall*)(HANDLE, tagPROCESSENTRY32*)>);
#if defined(UNICODE)
static_assert(std::is_same_v<LPMODULEENTRY32, MODULEENTRY32W*>);
static_assert(std::is_same_v<LPPROCESSENTRY32, PROCESSENTRY32W*>);
static_assert(std::is_same_v<PMODULEENTRY32, MODULEENTRY32W*>);
static_assert(std::is_same_v<PPROCESSENTRY32, PROCESSENTRY32W*>);
#else
static_assert(std::is_same_v<LPMODULEENTRY32, tagMODULEENTRY32*>);
static_assert(std::is_same_v<LPPROCESSENTRY32, tagPROCESSENTRY32*>);
static_assert(std::is_same_v<PMODULEENTRY32, tagMODULEENTRY32*>);
static_assert(std::is_same_v<PPROCESSENTRY32, tagPROCESSENTRY32*>);
#endif
