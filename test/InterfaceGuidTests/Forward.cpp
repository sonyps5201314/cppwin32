namespace PSDK {}
#include "InterfaceDeclarations.h"
#include "InterfaceGuidChecks.h"

static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(__uuidof(IUnknown))>>, IID>);
static_assert(std::is_same_v<decltype(&IID_IUnknown), IID const*>);
static_assert(InterfaceGuidMatches(__uuidof(IUnknown*), IID_IUnknown));
static_assert(InterfaceGuidMatches(__uuidof(*static_cast<IUnknown*>(nullptr)), IID_IUnknown));
static_assert(InterfaceGuidMatches(__uuidof(ID3D10Blob), __uuidof(ID3DBlob)));

struct __declspec(uuid("12345678-9ABC-DEF0-8123-456789ABCDEF")) UserInterface;
static_assert(InterfaceGuidMatches(win32::guid_of<UserInterface>(), IID{ 0x12345678, 0x9ABC, 0xDEF0, { 0x81, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } }));
