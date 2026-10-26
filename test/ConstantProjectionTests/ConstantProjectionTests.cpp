#include <type_traits>

namespace PSDK {}

#include "win32/Windows.Win32.Test.Constants.h"
#include "win32/impl/EnumFromUdtConst.h"

namespace constants = win32::Windows::Win32::Test::Constants;

static_assert(constants::MAX_VALUE == 0xffffffffu);
static_assert(constants::CHAIN_VALUE == 42);
static_assert(constants::POINTER_INTEGER_VALUE == 12);
static_assert(constants::FLOAT_CONSTANT == 1.5f);
static_assert(constants::TEXT[0] == L'C');
static_assert(constants::UNTYPED == 7);
static_assert(std::is_same_v<std::remove_cv_t<decltype(constants::MAX_VALUE)>, DWORD>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(constants::CHAIN_VALUE)>, COUNT>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(constants::POINTER_INTEGER_VALUE)>, POINTER_INTEGER>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(constants::FLOAT_CONSTANT)>, FLOAT_VALUE>);
static_assert(std::is_same_v<decltype(constants::TEXT), wchar_t const* const>);
static_assert(::LEGACY_FAILURE == -1);
static_assert(::LEGACY_TREE == -1);
static_assert(::LEGACY_RESOURCE == 1);
static_assert(std::is_enum_v<decltype(::LEGACY_FAILURE)>);
static_assert(std::is_enum_v<decltype(::LEGACY_TREE)>);
static_assert(std::is_enum_v<decltype(::LEGACY_RESOURCE)>);

int main() { return 0; }
