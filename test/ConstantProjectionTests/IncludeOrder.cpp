#include <type_traits>
namespace PSDK {}
#include "win32/Windows.Win32.Test.OtherLegacy.h"
#include "win32/Windows.Win32.Test.UnicodeAliases.h"
#include "win32/Windows.Win32.Test.NativeOrigins.h"
namespace first = win32::Windows::Win32::Test::OtherLegacy;
namespace second = win32::Windows::Win32::Test::UnicodeAliases;
static_assert(std::is_same_v<decltype(&first::OtherLegacy), _LEGACY*(__stdcall*)(_LEGACY*)>);
static_assert(std::is_same_v<decltype(&second::LegacyFirst), int(__stdcall*)(_LEGACY*)>);
static_assert(std::is_same_v<decltype(&first::TagConsumer), COLLISION_STRUCT*(__stdcall*)(COLLISION_STRUCT*)>);
#ifndef COLLISION
#error COLLISION_alias_is_missing
#endif
#ifdef UNICODE
static_assert(std::is_same_v<PLEGACY, LEGACYW*>);
#else
static_assert(std::is_same_v<PLEGACY, _LEGACY*>);
#endif
