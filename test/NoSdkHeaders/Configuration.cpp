#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#ifdef CPPWIN32_PROBE_WINNT
#define _WIN32_WINNT CPPWIN32_PROBE_WINNT
#endif
#ifdef CPPWIN32_PROBE_WINVER
#define WINVER CPPWIN32_PROBE_WINVER
#endif
#ifdef CPPWIN32_PROBE_NTDDI
#define NTDDI_VERSION CPPWIN32_PROBE_NTDDI
#endif
#ifdef CPPWIN32_PROBE_IE
#define _WIN32_IE CPPWIN32_PROBE_IE
#endif
#ifdef CPPWIN32_PROBE_FAMILY
#define WINAPI_FAMILY CPPWIN32_PROBE_FAMILY
#endif
#ifdef CPPWIN32_PROBE_PARTITION
#define WINAPI_PARTITION_DESKTOP 0
#define WINAPI_PARTITION_PKG_WINMGMT 1
#endif
#include <win32/base.h>

#ifndef CPPWIN32_PROBE_WINNT
static_assert(_WIN32_WINNT == 0x0A00);
#else
static_assert(_WIN32_WINNT == CPPWIN32_PROBE_WINNT);
#endif
#ifndef CPPWIN32_PROBE_WINVER
static_assert(WINVER == _WIN32_WINNT);
#else
static_assert(WINVER == CPPWIN32_PROBE_WINVER);
#endif
#ifdef CPPWIN32_PROBE_NTDDI
static_assert(NTDDI_VERSION == CPPWIN32_PROBE_NTDDI);
#elif _WIN32_WINNT == 0x0601
static_assert(NTDDI_VERSION == 0x06010000);
#else
static_assert(NTDDI_VERSION == WDK_NTDDI_VERSION);
#endif
#ifdef CPPWIN32_PROBE_IE
static_assert(_WIN32_IE == CPPWIN32_PROBE_IE);
#elif _WIN32_WINNT == 0x0601
static_assert(_WIN32_IE == 0x0800);
#endif
constexpr unsigned partitions =
    (WINAPI_PARTITION_DESKTOP ? 0x001 : 0) |
    (WINAPI_PARTITION_APP ? 0x002 : 0) |
    (WINAPI_PARTITION_PC_APP ? 0x004 : 0) |
    (WINAPI_PARTITION_PHONE_APP ? 0x008 : 0) |
    (WINAPI_PARTITION_SYSTEM ? 0x010 : 0) |
    (WINAPI_PARTITION_SERVER ? 0x020 : 0) |
    (WINAPI_PARTITION_GAMES ? 0x040 : 0) |
    (WINAPI_PARTITION_PKG_WINMGMT ? 0x080 : 0) |
    (WINAPI_PARTITION_PKG_PERFCOUNTER ? 0x100 : 0);
#ifdef CPPWIN32_PROBE_PARTITION
static_assert(partitions == 0x0C6);
#elif WINAPI_FAMILY == 100
static_assert(partitions == 0x047);
#elif WINAPI_FAMILY == 2
static_assert(partitions == 0x006);
#elif WINAPI_FAMILY == 3
static_assert(partitions == 0x00A);
#elif WINAPI_FAMILY == 4
static_assert(partitions == 0x010);
#elif WINAPI_FAMILY == 5
static_assert(partitions == 0x1B0);
#elif WINAPI_FAMILY == 6
static_assert(partitions == 0x040);
#else
#error Unrecognized_configuration_probe_family
#endif
static_assert((WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_APP) != 0) == ((partitions & 0x003) != 0));
static_assert(OSVER(0x06010203) == 0x06010000);
static_assert(SPVER(0x06010203) == 2);
static_assert(SUBVER(0x06010203) == 3);
