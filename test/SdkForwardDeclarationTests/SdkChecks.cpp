#include <type_traits>
namespace PSDK {}
#include "win32/base.h"
#include "win32/impl/Windows.Win32.Devices.DeviceAndDriverInstallation.0.h"
#include "win32/impl/Windows.Win32.Devices.Bluetooth.0.h"
#include "win32/impl/Windows.Win32.System.Com.0.h"
#include "win32/impl/Windows.Win32.Graphics.Direct2D.Common.0.h"
#include "win32/impl/Windows.Win32.Graphics.Direct3D.0.h"
#include "win32/impl/Windows.Win32.Graphics.Direct2D.0.h"
#include "win32/impl/Windows.Win32.Graphics.DirectWrite.0.h"
#include "win32/impl/Windows.Win32.Graphics.Direct3D.Dxc.0.h"
#include "win32/impl/Windows.Win32.Data.Xml.XmlLite.0.h"
#include "win32/impl/Windows.Win32.Networking.Ldap.0.h"
#include "win32/impl/Windows.Win32.System.Threading.0.h"
static_assert(std::is_same_v<PEB, _PEB>);
static_assert(std::is_same_v<PPEB, PEB*>);
#include "NativeLdapPrototypes.h"
using ldap_sasl_callback = INT(__cdecl*)(LDAP*, PCSTR, PCSTR, LDAP_BERVAL const*, PLDAPControlA*, PLDAPControlA*, int*);
static_assert(std::is_same_v<decltype(&LdapProjectionCheck::ldap_sasl_bindA), ldap_sasl_callback>);

static_assert(std::is_same_v<ID3D10Blob, ID3DBlob>);
static_assert(std::is_same_v<ID3D10Include, ID3DInclude>);
static_assert(std::is_same_v<IDWriteGeometrySink, ID2D1SimplifiedGeometrySink>);
static_assert(std::is_same_v<IDxcBlobWide, IDxcBlobUtf16>);
static_assert(std::is_same_v<IXmlReaderInput, IUnknown>);
static_assert(std::is_same_v<IXmlWriterOutput, IUnknown>);
static_assert(std::is_same_v<win32::Windows::Win32::Devices::DeviceAndDriverInstallation::PRIORITY, uint32_t>);
static_assert(std::is_same_v<PSP_ALTPLATFORM_INFO, SP_ALTPLATFORM_INFO_V2*>);
static_assert(std::is_same_v<PSP_BACKUP_QUEUE_PARAMS_A, SP_BACKUP_QUEUE_PARAMS_V2_A*>);
static_assert(std::is_same_v<PSP_BACKUP_QUEUE_PARAMS_W, SP_BACKUP_QUEUE_PARAMS_V2_W*>);
static_assert(std::is_same_v<PSP_CLASSIMAGELIST_DATA, SP_CLASSIMAGELIST_DATA*>);
static_assert(std::is_same_v<PSP_CLASSINSTALL_HEADER, SP_CLASSINSTALL_HEADER*>);
static_assert(std::is_same_v<PSP_DEVICE_INTERFACE_DATA, SP_DEVICE_INTERFACE_DATA*>);
static_assert(std::is_same_v<PSP_DEVICE_INTERFACE_DETAIL_DATA_A, SP_DEVICE_INTERFACE_DETAIL_DATA_A*>);
static_assert(std::is_same_v<PSP_DEVICE_INTERFACE_DETAIL_DATA_W, SP_DEVICE_INTERFACE_DETAIL_DATA_W*>);
static_assert(std::is_same_v<PSP_DEVINFO_DATA, SP_DEVINFO_DATA*>);
static_assert(std::is_same_v<PSP_DEVINFO_LIST_DETAIL_DATA_A, SP_DEVINFO_LIST_DETAIL_DATA_A*>);
static_assert(std::is_same_v<PSP_DEVINFO_LIST_DETAIL_DATA_W, SP_DEVINFO_LIST_DETAIL_DATA_W*>);
static_assert(std::is_same_v<PSP_DEVINSTALL_PARAMS_A, SP_DEVINSTALL_PARAMS_A*>);
static_assert(std::is_same_v<PSP_DEVINSTALL_PARAMS_W, SP_DEVINSTALL_PARAMS_W*>);
static_assert(std::is_same_v<PSP_DRVINFO_DATA_A, SP_DRVINFO_DATA_V2_A*>);
static_assert(std::is_same_v<PSP_DRVINFO_DATA, SP_DRVINFO_DATA_V2_W*>);
static_assert(std::is_same_v<PSP_DRVINFO_DATA_W, SP_DRVINFO_DATA_V2_W*>);
static_assert(std::is_same_v<PSP_DRVINFO_DETAIL_DATA_A, SP_DRVINFO_DETAIL_DATA_A*>);
static_assert(std::is_same_v<PSP_DRVINFO_DETAIL_DATA_W, SP_DRVINFO_DETAIL_DATA_W*>);
static_assert(std::is_same_v<PSP_DRVINSTALL_PARAMS, SP_DRVINSTALL_PARAMS*>);
static_assert(std::is_same_v<PSP_FILE_COPY_PARAMS_A, SP_FILE_COPY_PARAMS_A*>);
static_assert(std::is_same_v<PSP_FILE_COPY_PARAMS_W, SP_FILE_COPY_PARAMS_W*>);
static_assert(std::is_same_v<PSP_INF_INFORMATION, SP_INF_INFORMATION*>);
static_assert(std::is_same_v<PSP_INF_SIGNER_INFO_A, SP_INF_SIGNER_INFO_V2_A*>);
static_assert(std::is_same_v<PSP_INF_SIGNER_INFO_W, SP_INF_SIGNER_INFO_V2_W*>);
static_assert(std::is_same_v<PSP_INSTALLWIZARD_DATA, SP_INSTALLWIZARD_DATA*>);
static_assert(std::is_same_v<PSP_ORIGINAL_FILE_INFO_A, SP_ORIGINAL_FILE_INFO_A*>);
static_assert(std::is_same_v<PSP_ORIGINAL_FILE_INFO_W, SP_ORIGINAL_FILE_INFO_W*>);
static_assert(std::is_same_v<PINFCONTEXT, INFCONTEXT*>);
using bluetooth_callback = BOOL(__stdcall*)(LPVOID, BLUETOOTH_DEVICE_INFO const*);
static_assert(std::is_same_v<PFN_DEVICE_CALLBACK, bluetooth_callback>);
#ifndef BLUETOOTH_ADDRESS
#error BLUETOOTH_ADDRESS must be a macro
#endif
#ifndef BLUETOOTH_LOCAL_SERVICE_INFO
#error BLUETOOTH_LOCAL_SERVICE_INFO must be a macro
#endif
#ifndef BLUETOOTH_DEVICE_INFO
#error BLUETOOTH_DEVICE_INFO must be a macro
#endif
static_assert(std::is_same_v<_BLUETOOTH_ADDRESS, BLUETOOTH_ADDRESS_STRUCT>);
static_assert(std::is_same_v<_BLUETOOTH_LOCAL_SERVICE_INFO, BLUETOOTH_LOCAL_SERVICE_INFO_STRUCT>);
static_assert(std::is_same_v<_BLUETOOTH_DEVICE_INFO, BLUETOOTH_DEVICE_INFO_STRUCT>);

#include "NativeRecordLayouts.h"
static_assert(sizeof(BLUETOOTH_ADDRESS_STRUCT) == 8);
static_assert(sizeof(BLUETOOTH_DEVICE_INFO_STRUCT) == 560);
static_assert(offsetof(BLUETOOTH_DEVICE_INFO_STRUCT, Address) == 8);
static_assert(sizeof(BLUETOOTH_LOCAL_SERVICE_INFO_STRUCT) == 1040);
static_assert(offsetof(BLUETOOTH_LOCAL_SERVICE_INFO_STRUCT, btAddr) == 8);

#include "NativeCallbackInterfaces.h"

int main() { return 0; }
