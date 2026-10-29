param([string]$Headers = 'Z:\TYPEDEF')
$ErrorActionPreference = 'Stop'
$headersRoot = (Resolve-Path -LiteralPath $Headers).Path
$header = Join-Path $headersRoot 'win32\impl\Windows.Win32.Devices.DeviceAndDriverInstallation.0.h'
if (-not (Test-Path -LiteralPath $header)) { throw "找不到 SDK 投影头文件：$header" }

$bluetooth = Join-Path $headersRoot 'win32\impl\Windows.Win32.Devices.Bluetooth.0.h'
$declaration = [regex]::Match([IO.File]::ReadAllText($bluetooth), '(?m)^\s*typedef[^\r\n]*\bPFN_DEVICE_CALLBACK\)\s*\([^\r\n]*;').Value
if ($declaration -notmatch '\bconst BLUETOOTH_DEVICE_INFO\* pDevice\)')
{
    throw 'PFN_DEVICE_CALLBACK 必须保留 SDK 的 const BLUETOOTH_DEVICE_INFO* pDevice 写法'
}

$bluetoothText = [IO.File]::ReadAllText($bluetooth)
foreach ($name in 'BLUETOOTH_ADDRESS', 'BLUETOOTH_LOCAL_SERVICE_INFO', 'BLUETOOTH_DEVICE_INFO')
{
    if ($bluetoothText -notmatch "(?m)^#define $name ${name}_STRUCT\r?`$") { throw "$name 必须保留 SDK 宏定义" }
    if ($bluetoothText -match "(?m)^#ifndef $name\r?`$") { throw "$name 的类型宏不应使用条件保护屏蔽冲突定义" }
    if ($bluetoothText -notmatch "typedef struct _$name ${name}_STRUCT;") { throw "$name 必须保留真实 tag 与 typedef 关系" }
}

$output = Join-Path $PSScriptRoot 'obj'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$structs = [IO.File]::ReadAllText((Join-Path $headersRoot 'win32\impl\complex_structs.h'))
$ldap = [IO.File]::ReadAllText((Join-Path $headersRoot 'win32\Windows.Win32.Networking.Ldap.h'))
$ldapDeclarations = [Collections.Generic.List[string]]::new()
$nativeInt = [regex]::Match($structs, '(?m)^ *typedef int INT;')
if (-not $nativeInt.Success) { throw '缺少 INT 的实际导出定义' }
$ldapDeclarations.Add($nativeInt.Value)
$ldapDeclarations.Add('namespace LdapProjectionCheck {')
foreach ($name in 'ldap_sasl_bindA', 'ldap_sasl_bindW', 'ldap_sasl_bind_sA', 'ldap_sasl_bind_sW')
{
    $declaration = [regex]::Match($ldap, "(?m)^ *INT __cdecl $name\([^\r\n]*;").Value
    if (-not $declaration) { throw "$name 的返回类型必须保留 INT" }
    if ($name -notlike '*_s?' -and $declaration -notmatch 'int\* MessageNumber\)') { throw "$name 的 MessageNumber 必须保留 int*" }
    $ldapDeclarations.Add($declaration)
}
$ldapDeclarations.Add('}')
[IO.File]::WriteAllLines((Join-Path $output 'NativeLdapPrototypes.h'), $ldapDeclarations)
foreach ($name in 'LDAP_API_FEATURE_VIRTUAL_LIST_VIEW', 'LDAP_VLVINFO_VERSION')
{
    if ($ldap -notmatch "inline constexpr unsigned int $name =") { throw "$name 必须使用 C++ 基础整数类型" }
}

foreach ($declaration in 'UINT8 EventPending : 1;', 'UINT8 EventType : 3;', 'UINT8 Reserved : 4;', 'UINT64 StartupSuspend : 1;', 'UINT64 HaltSuspend : 1;', 'UINT64 IdleSuspend : 1;')
{
    if (-not $structs.Contains($declaration)) { throw "Hypervisor 位域类型拼写丢失：$declaration" }
}
$definitions = [Collections.Generic.List[string]]::new()
foreach ($name in 'DWORD', 'WORD', 'BYTE', 'ULONG', 'BTH_ADDR')
{
    $match = [regex]::Match($structs, "(?m)^ *typedef[^;\r\n]+\b$name\s*;")
    if (-not $match.Success) { throw "缺少 $name 的实际导出定义" }
    $definitions.Add($match.Value)
}
$systemtimeAlias = [regex]::Match($structs, '(?m)^ *typedef struct _SYSTEMTIME SYSTEMTIME;')
if (-not $systemtimeAlias.Success) { throw 'SYSTEMTIME native typedef is missing' }
$definitions.Add($systemtimeAlias.Value)
foreach ($name in '_SYSTEMTIME', '_BLUETOOTH_ADDRESS', '_BLUETOOTH_LOCAL_SERVICE_INFO', '_BLUETOOTH_DEVICE_INFO')
{
    $match = [regex]::Match($structs, "(?m)^ *struct $name\s*\{")
    if (-not $match.Success) { throw "缺少 $name 的实际导出结构体" }
    $position = $match.Index + $match.Length
    $depth = 1
    while ($depth -gt 0 -and $position -lt $structs.Length)
    {
        if ($structs[$position] -eq '{') { $depth++ }
        elseif ($structs[$position] -eq '}') { $depth-- }
        $position++
    }
    if ($depth -ne 0 -or $structs[$position] -ne ';') { throw "$name 结构定义不完整" }
    $definitions.Add($structs.Substring($match.Index, $position + 1 - $match.Index))
}
[IO.File]::WriteAllLines((Join-Path $output 'NativeRecordLayouts.h'), $definitions)

$interfaces = [IO.File]::ReadAllText((Join-Path $headersRoot 'win32\impl\complex_interfaces.h'))
$callbackDefinitions = [Collections.Generic.List[string]]::new()
$callbackDefinitions.Add('namespace NativeCallbackCheck {')
foreach ($name in 'HRESULT','ULONG','IID','HSTRING','UINT64','PVOID','INT32','alljoyn_msgarg')
{
    $match = [regex]::Match($structs, "(?m)^ *typedef[^;\r\n]+\b$name\s*;")
    if (-not $match.Success) { throw "缺少回调依赖 $name 的实际导出定义" }
    $callbackDefinitions.Add($match.Value)
}
$winrt = [IO.File]::ReadAllText((Join-Path $headersRoot 'win32\impl\Windows.Win32.System.WinRT.0.h'))
$trust = [regex]::Match($winrt, '(?ms)^ *enum class TrustLevel[^}]+};')
if (-not $trust.Success) { throw '缺少 TrustLevel 的实际定义' }
$callbackDefinitions.Add($trust.Value)
foreach ($name in 'IUnknown','IInspectable','IWindowsDevicesAllJoynBusObjectInterop')
{
    $match = [regex]::Match($interfaces, "(?ms)^ *struct __declspec\(novtable\) $name\b[^}]+};")
    if (-not $match.Success) { throw "缺少 $name 的实际导出定义" }
    $callbackDefinitions.Add($match.Value)
}
$callbackDefinitions.Add('using Handler = INT32(__stdcall*)(PVOID,HSTRING,HSTRING,alljoyn_msgarg);')
$callbackDefinitions.Add('using Method = HRESULT(__stdcall IWindowsDevicesAllJoynBusObjectInterop::*)(PVOID,HSTRING,Handler) noexcept;')
$callbackDefinitions.Add('static_assert(std::is_same_v<decltype(&IWindowsDevicesAllJoynBusObjectInterop::AddPropertyGetHandler),Method>);')
$callbackDefinitions.Add('static_assert(std::is_same_v<decltype(&IWindowsDevicesAllJoynBusObjectInterop::AddPropertySetHandler),Method>);')
$callbackDefinitions.Add('}')
[IO.File]::WriteAllLines((Join-Path $output 'NativeCallbackInterfaces.h'),$callbackDefinitions)

$escapedRoot = [Security.SecurityElement]::Escape($headersRoot)
@"
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup><SdkHeadersRoot>$escapedRoot</SdkHeadersRoot></PropertyGroup>
</Project>
"@ | Set-Content -LiteralPath (Join-Path $output 'SdkHeaders.props') -Encoding utf8

foreach ($platform in 'x64', 'x86')
{
    & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
        (Join-Path $PSScriptRoot 'SdkForwardDeclarationTests.sln') /Rebuild "Debug|$platform" `
        /Out (Join-Path $output "$platform-build.log")
    if ($LASTEXITCODE -ne 0) { throw "$platform SDK 前置声明编译验证失败" }
    $binaryPlatform = if ($platform -eq 'x86') { 'Win32' } else { 'x64' }
    & (Join-Path $output "$binaryPlatform\SdkForwardDeclarationTests.exe")
    if ($LASTEXITCODE -ne 0) { throw "$platform SDK 前置声明运行验证失败" }
}
