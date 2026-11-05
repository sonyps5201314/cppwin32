namespace PSDK {}
#include "InterfaceDeclarations.h"
#include "win32/impl/complex_interfaces.h"
#include "InterfaceGuidChecks.h"

static_assert(InterfaceGuidMatches(__uuidof(IUnknown), IID_IUnknown));
static_assert(InterfaceGuidMatches(__uuidof(ID3D12Device), IID_ID3D12Device));
static_assert(InterfaceGuidMatches(win32::guid_of<ID3D12Device>(), IID_ID3D12Device));
