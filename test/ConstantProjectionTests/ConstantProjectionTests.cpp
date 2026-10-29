#pragma warning(error: 4005)

#if defined(CPPWIN32_VERIFY_MACRO_CONFLICT)
#define SDK_RECORD SDK_OTHER
#else
#define SDK_RECORD SDK_RECORD_STRUCT
#endif

#include <type_traits>

namespace PSDK {}

#include "win32/impl/Windows.Win32.Test.ForwardDeclarations.0.h"
#include "win32/Windows.Win32.Test.NativeOrigins.h"
#include "win32/Windows.Win32.Test.Bitfields.h"
#include "win32/Windows.Win32.Test.Primitives.h"
#include "win32/Windows.Win32.Test.NativeCallbacks.h"
#include "win32/Windows.Win32.Test.Constants.h"
#include "win32/Windows.Win32.Test.Platform.h"
#include "win32/Windows.Win32.Test.InterfaceProjection.h"
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

static_assert(sizeof(Boolean) == 2);
static_assert(std::is_same_v<Boolean, int16_t>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Transactional), Boolean*>);
static_assert(std::is_same_v<decltype(ProjectionProbe::ManagedBoolean), bool>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Format), PixelFormat*>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Hints), InterpolationMode>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Encoders), EncoderParameters**>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Frames), Image**>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Frame), Image*>);
static_assert(sizeof(HANDLE_PTR) == sizeof(void*));
static_assert(std::is_same_v<decltype(ProjectionProbe::PortableHandle), HANDLE_PTR>);
static_assert(std::is_same_v<LPVOID, void*>);
static_assert(std::is_same_v<decltype(ProjectionProbe::Mode), MODE_ALIAS>);
static_assert(std::extent_v<decltype(ProjectionProbe::Transform)> == 12);
static_assert(sizeof(ProjectionProbe::Transform) == sizeof(float) * 12);
static_assert(std::is_same_v<std::remove_extent_t<decltype(ProjectionProbe::Transform)>, FLOAT>);
namespace projection = win32::Windows::Win32::Test::InterfaceProjection;
static_assert(projection::MODE_ONE == NativeMode::One);
static_assert(std::is_same_v<std::remove_cv_t<decltype(projection::MODE_ONE)>, MODE_ALIAS>);
using name_map_function = int32_t(__stdcall*)(wchar_t const***);
static_assert(std::is_same_v<decltype(&projection::GetNameIdMap), name_map_function>);
static_assert(sizeof(DOT11_MAC_ADDRESS) == 6);
static_assert(sizeof(SpatialAudioHrtfOrientation) == sizeof(float) * 9);
static_assert(std::is_same_v<PDOT11_MAC_ADDRESS, uint8_t(*)[6]>);
static_assert(std::is_same_v<decltype(ProjectionProbe::MacPointer), PDOT11_MAC_ADDRESS>);
static_assert(std::extent_v<decltype(ProjectionProbe::MacAddresses), 0> == 2);
static_assert(std::extent_v<decltype(ProjectionProbe::MacAddresses), 1> == 6);
static_assert(std::is_same_v<decltype(ProjectionProbe::Orientation), float(*)[9]>);

#if defined(_M_IX86)
static_assert(sizeof(FLOATOBJ) == 8);
static_assert(sizeof(FLOATOBJ_XFORM) == 48);
#else
static_assert(std::is_same_v<FLOATOBJ, float>);
static_assert(sizeof(FLOATOBJ_XFORM) == 24);
#endif

static_assert(std::is_same_v<PPACKET, PACKET*>);
static_assert(std::is_same_v<MPACKET, PACKET*>);
static_assert(std::is_same_v<PPPACKET, PACKET**>);
#if defined(_M_IX86)
static_assert(sizeof(PACKET) == 8);
static_assert(alignof(PACKET) == 1);
#else
static_assert(sizeof(PACKET) == 16);
static_assert(alignof(PACKET) == 8);
#endif

#ifndef SDK_RECORD
#error SDK_RECORD must remain a macro
#endif
#ifndef SDK_RECORD_CHAIN
#error SDK_RECORD_CHAIN must remain a macro
#endif
static_assert(std::is_same_v<_SDK_RECORD, SDK_RECORD_STRUCT>);
static_assert(std::is_same_v<SDK_RECORD_ALIAS, SDK_RECORD_STRUCT>);
static_assert(std::is_same_v<SDK_RECORD_CHAIN, SDK_RECORD_STRUCT>);
static_assert(std::is_same_v<decltype(OriginHolder::Record), SDK_RECORD_STRUCT*>);
static_assert(std::is_same_v<decltype(OriginHolder::Other), SDK_OTHER*>);
static_assert(sizeof(SDK_RECORD_STRUCT) == sizeof(int));

static_assert(sizeof(COLLISION_STRUCT) == sizeof(int));
static_assert(std::is_same_v<COLLISION, COLLISION_STRUCT>);
static_assert(sizeof(ANONYMOUS_STRUCT) == sizeof(int));
static_assert(std::is_same_v<ANONYMOUS, ANONYMOUS_STRUCT>);

#ifndef PUBLIC_BARE_RECORD
#error PUBLIC_BARE_RECORD must remain a macro
#endif
static_assert(std::is_same_v<PUBLIC_BARE_RECORD, BareTag>);
static_assert(sizeof(PUBLIC_BARE_RECORD) == sizeof(int));

static_assert(std::is_same_v<ManagedTagOnly, ActualBareTag>);
static_assert(sizeof(ManagedTagOnly) == sizeof(int));

static_assert(sizeof(BitfieldAliases) == 1);
static_assert(sizeof(WideBitfields) == 8);
static_assert(sizeof(SignedBitfields) == 4);
static_assert(sizeof(LegacyBitfields) == 4);
static_assert(std::is_same_v<decltype(SignedBitfields::Signed), int>);
static_assert(std::is_same_v<decltype(SignedBitfields::Unsigned), unsigned int>);

static_assert(std::is_same_v<decltype(PrimitiveProbe::NativeLong), long>);
static_assert(std::is_same_v<decltype(PrimitiveProbe::NativeUnsignedLong), unsigned long>);
static_assert(std::is_same_v<decltype(PrimitiveProbe::Words), unsigned short const*>);
static_assert(std::is_same_v<PrimitiveCallback, unsigned long(__stdcall*)(long*)>);
namespace primitives = win32::Windows::Win32::Test::Primitives;
static_assert(std::is_same_v<decltype(&primitives::PrimitiveMethod), long(__cdecl*)(long*, int)>);
static_assert(std::is_same_v<decltype(&primitives::PrimitiveVoid), void(__stdcall*)(long*, int)>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(primitives::PLAIN_UNSIGNED)>, unsigned int>);

using native_callback = int(__stdcall*)(int);
using native_cdecl_callback = int(__cdecl*)(int);
using native_nested_callback = int(__cdecl*)(void(__stdcall*)(int));
static_assert(std::is_same_v<decltype(InlineCallbacks::First), native_cdecl_callback>);
static_assert(std::is_same_v<decltype(InlineCallbacks::Second), native_callback>);
static_assert(std::is_same_v<decltype(InlineCallbacks::Nested), native_nested_callback>);
static_assert(std::is_same_v<decltype(InlineCallbacks::Array), int(*)[4]>);
static_assert(std::is_same_v<CdeclCallback, native_cdecl_callback>);
namespace native_callbacks = win32::Windows::Win32::Test::NativeCallbacks;
static_assert(std::is_same_v<decltype(&native_callbacks::Register), void(__stdcall*)(native_callback)>);
static_assert(std::is_same_v<decltype(&native_callbacks::GetCallback), native_callback(__stdcall*)()>);

int main() { return 0; }
