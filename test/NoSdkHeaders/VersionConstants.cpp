namespace PSDK {}
#include <win32/base.h>
#undef WINVER
#undef NTDDI_MAXVER
#undef _WIN32_IE_MAXVER
#undef _WIN32_WINNT_MAXVER
#undef NTDDI_WIN2K
#undef NTDDI_WINXP
#undef NTDDI_WINXPSP2
#undef NTDDI_WS03SP1
#undef NTDDI_VISTA
#undef NTDDI_VISTASP1
#undef NTDDI_WIN7
#undef NTDDI_WIN8
#undef NTDDI_WINBLUE
#undef NTDDI_WINTHRESHOLD
#undef _WIN32_WINNT_NT4
#undef _WIN32_WINNT_WIN2K
#undef _WIN32_WINNT_WINXP
#undef _WIN32_WINNT_WS03
#undef _WIN32_WINNT_WIN6
#undef _WIN32_WINNT_VISTA
#undef _WIN32_WINNT_WS08
#undef _WIN32_WINNT_LONGHORN
#undef _WIN32_WINNT_WIN7
#undef _WIN32_WINNT_WIN8
#undef _WIN32_WINNT_WINBLUE
#undef _WIN32_WINNT_WINTHRESHOLD
#undef _WIN32_WINNT_WIN10
#undef _WIN32_IE_IE20
#undef _WIN32_IE_IE30
#undef _WIN32_IE_IE302
#undef _WIN32_IE_IE40
#undef _WIN32_IE_IE401
#undef _WIN32_IE_IE50
#undef _WIN32_IE_IE501
#undef _WIN32_IE_IE55
#undef _WIN32_IE_IE60
#undef _WIN32_IE_IE60SP1
#undef _WIN32_IE_IE60SP2
#undef _WIN32_IE_IE70
#undef _WIN32_IE_IE80
#undef _WIN32_IE_IE90
#undef _WIN32_IE_IE100
#undef _WIN32_IE_IE110
#undef _WIN32_IE_NT4
#undef _WIN32_IE_NT4SP1
#undef _WIN32_IE_NT4SP2
#undef _WIN32_IE_NT4SP3
#undef _WIN32_IE_NT4SP4
#undef _WIN32_IE_NT4SP5
#undef _WIN32_IE_NT4SP6
#undef _WIN32_IE_WIN98
#undef _WIN32_IE_WIN98SE
#undef _WIN32_IE_WINME
#undef _WIN32_IE_WIN2K
#undef _WIN32_IE_WIN2KSP1
#undef _WIN32_IE_WIN2KSP2
#undef _WIN32_IE_WIN2KSP3
#undef _WIN32_IE_WIN2KSP4
#undef _WIN32_IE_XP
#undef _WIN32_IE_XPSP1
#undef _WIN32_IE_XPSP2
#undef _WIN32_IE_WS03
#undef _WIN32_IE_WS03SP1
#undef _WIN32_IE_WIN6
#undef _WIN32_IE_LONGHORN
#undef _WIN32_IE_WIN7
#undef _WIN32_IE_WIN8
#undef _WIN32_IE_WINBLUE
#undef _WIN32_IE_WINTHRESHOLD
#undef _WIN32_IE_WIN10
#undef NTDDI_WIN4
#undef NTDDI_WIN2KSP1
#undef NTDDI_WIN2KSP2
#undef NTDDI_WIN2KSP3
#undef NTDDI_WIN2KSP4
#undef NTDDI_WINXPSP1
#undef NTDDI_WINXPSP3
#undef NTDDI_WINXPSP4
#undef NTDDI_WS03
#undef NTDDI_WS03SP2
#undef NTDDI_WS03SP3
#undef NTDDI_WS03SP4
#undef NTDDI_WIN6
#undef NTDDI_WIN6SP1
#undef NTDDI_WIN6SP2
#undef NTDDI_WIN6SP3
#undef NTDDI_WIN6SP4
#undef NTDDI_VISTASP2
#undef NTDDI_VISTASP3
#undef NTDDI_VISTASP4
#undef NTDDI_LONGHORN
#undef NTDDI_WS08
#undef NTDDI_WS08SP2
#undef NTDDI_WS08SP3
#undef NTDDI_WS08SP4
#undef NTDDI_WIN10
#undef NTDDI_WIN10_TH2
#undef NTDDI_WIN10_RS1
#undef NTDDI_WIN10_RS2
#undef NTDDI_WIN10_RS3
#undef NTDDI_WIN10_RS4
#undef NTDDI_WIN10_RS5
#undef NTDDI_WIN10_19H1
#undef NTDDI_WIN10_VB
#undef NTDDI_WIN10_MN
#undef NTDDI_WIN10_FE
#undef NTDDI_WIN10_CO
#undef NTDDI_WIN10_NI
#undef NTDDI_WIN10_CU
#undef NTDDI_WIN11_ZN
#undef NTDDI_WIN11_GA
#undef NTDDI_WIN11_GE
#undef NTDDI_WIN11_DT
#undef NTDDI_WIN11_BR
#undef WDK_NTDDI_VERSION
#undef OSVERSION_MASK
#undef SPVERSION_MASK
#undef SUBVERSION_MASK
#undef NTDDI_VERSION
static_assert(win32::Windows::Win32::Foundation::WINVER == 0x500);
static_assert(win32::Windows::Win32::Foundation::NTDDI_MAXVER == 0xa00);
static_assert(win32::Windows::Win32::Foundation::_WIN32_IE_MAXVER == 0xa00);
static_assert(win32::Windows::Win32::Foundation::_WIN32_WINNT_MAXVER == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN2K == 0x5000000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINXP == 0x5010000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINXPSP2 == 0x5010200);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS03SP1 == 0x5020100);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_VISTA == 0x6000000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_VISTASP1 == 0x6000100);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN7 == 0x6010000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN8 == 0x6020000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINBLUE == 0x6030000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINTHRESHOLD == 0xa000000);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_NT4 == 0x400);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WIN2K == 0x500);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WINXP == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WS03 == 0x502);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WIN6 == 0x600);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_VISTA == 0x600);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WS08 == 0x600);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_LONGHORN == 0x600);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WIN7 == 0x601);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WIN8 == 0x602);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WINBLUE == 0x603);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WINTHRESHOLD == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_WINNT_WIN10 == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE20 == 0x200);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE30 == 0x300);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE302 == 0x302);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE40 == 0x400);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE401 == 0x401);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE50 == 0x500);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE501 == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE55 == 0x550);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE60 == 0x600);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE60SP1 == 0x601);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE60SP2 == 0x603);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE70 == 0x700);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE80 == 0x800);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE90 == 0x900);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE100 == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_IE110 == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4 == 0x200);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4SP1 == 0x200);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4SP2 == 0x200);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4SP3 == 0x302);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4SP4 == 0x401);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4SP5 == 0x401);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_NT4SP6 == 0x500);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN98 == 0x401);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN98SE == 0x500);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WINME == 0x550);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN2K == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN2KSP1 == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN2KSP2 == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN2KSP3 == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN2KSP4 == 0x501);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_XP == 0x600);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_XPSP1 == 0x601);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_XPSP2 == 0x603);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WS03 == 0x602);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WS03SP1 == 0x603);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN6 == 0x700);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_LONGHORN == 0x700);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN7 == 0x800);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN8 == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WINBLUE == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WINTHRESHOLD == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::_WIN32_IE_WIN10 == 0xa00);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN4 == 0x4000000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN2KSP1 == 0x5000100);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN2KSP2 == 0x5000200);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN2KSP3 == 0x5000300);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN2KSP4 == 0x5000400);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINXPSP1 == 0x5010100);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINXPSP3 == 0x5010300);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WINXPSP4 == 0x5010400);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS03 == 0x5020000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS03SP2 == 0x5020200);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS03SP3 == 0x5020300);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS03SP4 == 0x5020400);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN6 == 0x6000000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN6SP1 == 0x6000100);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN6SP2 == 0x6000200);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN6SP3 == 0x6000300);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN6SP4 == 0x6000400);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_VISTASP2 == 0x6000200);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_VISTASP3 == 0x6000300);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_VISTASP4 == 0x6000400);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_LONGHORN == 0x6000000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS08 == 0x6000100);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS08SP2 == 0x6000200);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS08SP3 == 0x6000300);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WS08SP4 == 0x6000400);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10 == 0xa000000);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_TH2 == 0xa000001);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_RS1 == 0xa000002);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_RS2 == 0xa000003);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_RS3 == 0xa000004);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_RS4 == 0xa000005);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_RS5 == 0xa000006);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_19H1 == 0xa000007);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_VB == 0xa000008);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_MN == 0xa000009);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_FE == 0xa00000a);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_CO == 0xa00000b);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_NI == 0xa00000c);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN10_CU == 0xa00000d);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN11_ZN == 0xa00000e);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN11_GA == 0xa00000f);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN11_GE == 0xa000010);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN11_DT == 0xa000011);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_WIN11_BR == 0xa000012);
static_assert(win32::Windows::Win32::System::SystemInformation::WDK_NTDDI_VERSION == 0xa000012);
static_assert(win32::Windows::Win32::System::SystemInformation::OSVERSION_MASK == 0xffff0000);
static_assert(win32::Windows::Win32::System::SystemInformation::SPVERSION_MASK == 0xff00);
static_assert(win32::Windows::Win32::System::SystemInformation::SUBVERSION_MASK == 0xff);
static_assert(win32::Windows::Win32::System::SystemInformation::NTDDI_VERSION == 0xa000012);
