param([string]$Emitter = 'L:\Open_Code\win32metadata\bin\Release\net10.0\ClangSharpSourceToWinmd.exe', [string]$SdkHeaders = '', [ValidateSet('Ansi','Unicode')][string]$CharacterSet = 'Ansi', [string]$NativeContracts = '')
$ErrorActionPreference = 'Stop'
$previousPostCL = $env:_CL_
try
{
    $env:_CL_ = "$previousPostCL " + $(if ($CharacterSet -eq 'Unicode') { '/DUNICODE /D_UNICODE' } else { '/UUNICODE /U_UNICODE' })
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$output = Join-Path $PSScriptRoot 'obj'
New-Item -ItemType Directory -Path $output -Force | Out-Null
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
    (Join-Path $PSScriptRoot 'imports\ImportCollisions.sln') /Build 'Debug|Any CPU' `
    /Out (Join-Path $output 'import-fixture-build.log')
if ($LASTEXITCODE -ne 0) { throw '导入冲突元数据编译失败' }
$imports = Join-Path $output 'imports\ImportCollisions.dll'
$winmd = Join-Path $output 'Windows.Win32.winmd'
& $Emitter --sourceDir (Join-Path $PSScriptRoot 'input') --arch x64 --outputFileName $winmd "@$(Join-Path $PSScriptRoot 'input\emitter.settings.rsp')"
if ($LASTEXITCODE -ne 0) { throw '测试 winmd 生成失败' }

Push-Location (Join-Path $repo 'cppwin32')
try
{
    $lines = @(& '.\x64\Debug\cppwin32.exe' -input $winmd $imports -output (Join-Path $output 'headers') 2>&1)
    $lines | Write-Output
    if ($LASTEXITCODE -ne 0 -or ($lines | Select-String 'cppwin32 : error')) { throw '测试头文件导出失败' }
}
finally { Pop-Location }

& python (Join-Path $PSScriptRoot '..\AuditForwardDeclarations.py') --headers (Join-Path $output 'headers\win32') --output (Join-Path $output 'forward-declarations.json')
if ($LASTEXITCODE -ne 0) { throw '测试头文件存在同一条件和有效架构内的重复前置声明或 typedef' }

foreach ($header in Get-ChildItem -LiteralPath (Join-Path $output 'headers\win32\impl') -Filter '*.0.h' -File)
{
    if ([IO.File]::ReadAllText($header.FullName) -match '#include "win32/base\.h"\s*WIN32_EXPORT namespace [A-Za-z_][A-Za-z_0-9:]*\s*\{\s*\}') { throw "没有配置常量时不应输出空命名空间：$($header.Name)" }
}

$nativeDefinitions = [IO.File]::ReadAllText((Join-Path $output 'headers\win32\impl\complex_structs.h'))
if ($nativeDefinitions -notmatch 'SDK_OTHER_ALIAS\* NamedAlias;') { throw 'SDK record alias spelling was lost' }
foreach ($pair in @(@('_SDK_RECORD','SDK_RECORD_STRUCT'), @('_MethodRecord','MethodRecord')))
{
    if ($nativeDefinitions -notmatch ('(?s)typedef struct ' + $pair[0] + '\s*\{[^{}]*\}\s*' + $pair[1] + '(?:,|;)')) { throw "结构定义没有保留相邻的真实 typedef：$($pair[1])" }
}
if ($nativeDefinitions -match 'KIND_MACRO_ENUM|SharedNativeName_ENUM') { throw '不应新增合成的 _ENUM 类型' }

$legacyApis = [IO.File]::ReadAllText((Join-Path $output 'headers\win32\Windows.Win32.Test.UnicodeAliases.h'))
foreach ($declaration in 'int __stdcall LegacyFirst(PLEGACY value);', 'PLEGACY __stdcall LegacyReturn(LEGACY* value);')
{
    if (-not $legacyApis.Contains($declaration)) { throw "选择宏导致 API 的真实 typedef 拼写丢失：$declaration" }
}
foreach ($declaration in 'LEGACY Value;', 'LEGACY* Pointer;', 'PLEGACY Alias;')
{
    if (-not $nativeDefinitions.Contains($declaration)) { throw "选择宏导致字段的真实 typedef 拼写丢失：$declaration" }
}
$legacyCallbacks = [IO.File]::ReadAllText((Join-Path $output 'headers\win32\impl\Windows.Win32.Test.UnicodeAliases.0.h'))
if ($legacyCallbacks -notmatch 'typedef PLEGACY \(__stdcall \* LegacyCallback\) \(PLEGACY alias, LEGACY\* pointer\);') { throw '选择宏导致回调的真实 typedef 拼写丢失' }
$legacyInterfaces = [IO.File]::ReadAllText((Join-Path $output 'headers\win32\impl\complex_interfaces.h'))
if ($legacyInterfaces -notmatch 'virtual PLEGACY __stdcall Get\(PLEGACY alias, LEGACY\* pointer\) noexcept = 0;') { throw '选择宏导致接口的真实 typedef 拼写丢失' }
$nativeAliases = $nativeDefinitions
foreach ($file in 'native_declarations.h','native_aliases.h')
{
    if (Test-Path -LiteralPath (Join-Path $output "headers\win32\impl\$file")) { throw "不应继续生成集中声明头：$file" }
}
$macroProtections = @(Get-ChildItem -LiteralPath (Join-Path $output 'headers\win32') -Filter '*.h' -File -Recurse | Select-String -Pattern '#pragma (?:push_macro|pop_macro)')
if ($macroProtections.Count) { throw '不应通过宏保护序列补偿自身的声明顺序' }
if ($nativeDefinitions -notmatch '(?s)typedef struct _LEGACY\s*\{[^{}]*\}\s*LEGACY;\s*typedef LEGACY\* PLEGACY;\s*typedef LEGACY \* QLEGACY;') { throw '结构体及其真实指针别名没有相邻输出' }
if ($nativeDefinitions -notmatch '(?s)typedef struct _ZRECORD\s*\{[^{}]*\}\s*ZRECORD;\s*typedef ZRECORD\* PZRECORD;\s*typedef PZRECORD PALIAS;') { throw '元数据 typedef 与补充 typedef 没有按依赖相邻输出' }
if ($legacyApis.IndexOf('#define PLEGACY ') -lt $legacyApis.IndexOf('LegacyFirst(')) { throw '选择宏早于原始 ANSI API 声明' }
foreach ($declaration in 'typedef LEGACY * QLEGACY;', 'typedef LEGACYW * QLEGACYW;')
{
    if (-not $nativeAliases.Contains($declaration)) { throw "补充 typedef 没有保留 SDK 定义：$declaration" }
}
if ($SdkHeaders)
{
    $toolHelp = [IO.File]::ReadAllText((Join-Path $SdkHeaders 'win32\Windows.Win32.System.Diagnostics.ToolHelp.h'))
    foreach ($declaration in 'Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme);', 'Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme);', 'Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);', 'Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);')
    {
        if (-not $toolHelp.Contains($declaration)) { throw "真实 SDK API typedef 拼写回退：$declaration" }
    }
}


foreach ($declaration in 'UINT8 First : 4;', 'BYTE Second : 4;', 'UINT64 StartupSuspend : 1;', 'int Signed : 4;', 'unsigned int Unsigned : 28;', 'unsigned int Value : 32;')
{
    if (-not $nativeDefinitions.Contains($declaration)) { throw "位域类型拼写丢失：$declaration" }
}

$primitives = [IO.File]::ReadAllText((Join-Path $output 'headers\win32\Windows.Win32.Test.Primitives.h'))
foreach ($declaration in 'long NativeLong;', 'unsigned long NativeUnsignedLong;', 'const unsigned short* Words;', 'int PlainInt;')
{
    if (-not $nativeDefinitions.Contains($declaration)) { throw "基础类型拼写丢失：$declaration" }
}
if ($primitives -notmatch 'long __cdecl PrimitiveMethod\(long\* value, int plain\);') { throw '函数原生 long 返回值或参数丢失' }
if ($primitives -notmatch 'void __stdcall PrimitiveVoid\(long\* first, int second\);') { throw 'void 返回值属性导致参数错位' }
if ($primitives -notmatch 'inline constexpr unsigned int PLAIN_UNSIGNED =') { throw '普通无符号常量未使用 C++ 基础类型' }

foreach ($platform in 'x64', 'x86')
{
    $conflictLog = Join-Path $output "$platform-macro-conflict-build.log"
    if (Test-Path -LiteralPath $conflictLog) { Remove-Item -LiteralPath $conflictLog }
    $previousCL = $env:CL
    try
    {
        $env:CL = "$previousCL /DCPPWIN32_VERIFY_MACRO_CONFLICT"
        & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
            (Join-Path $PSScriptRoot 'ConstantProjectionTests.sln') /Rebuild "Debug|$platform" /Out $conflictLog
        $conflictExitCode = $LASTEXITCODE
    }
    finally { $env:CL = $previousCL }
    if ($conflictExitCode -eq 0 -or [IO.File]::ReadAllText($conflictLog) -notmatch 'error C4005:[^\r\n]*SDK_RECORD')
    {
        throw "$platform SDK_RECORD 的冲突宏定义未触发 C4005"
    }

    & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
        (Join-Path $PSScriptRoot 'ConstantProjectionTests.sln') /Rebuild "Debug|$platform" `
        /Out (Join-Path $output "$platform-build.log")
    if ($LASTEXITCODE -ne 0) { throw "$platform 编译验证失败" }
    $binaryPlatform = if ($platform -eq 'x86') { 'Win32' } else { 'x64' }
    & (Join-Path $output "$binaryPlatform\ConstantProjectionTests.exe")
    if ($LASTEXITCODE -ne 0) { throw "$platform 运行验证失败" }
}

$tools = Get-ChildItem -LiteralPath 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Tools\MSVC' -Directory | Sort-Object Name -Descending | Select-Object -First 1
$dumpbin = Join-Path $tools.FullName 'bin\Hostx64\x64\dumpbin.exe'
$previousSdkHeaders = $env:SdkHeadersRoot
$previousNativeContracts = $env:NativeContractsHeader
try
{
    $env:SdkHeadersRoot = if ($SdkHeaders) { (Resolve-Path -LiteralPath $SdkHeaders).Path } else { $null }
    $env:NativeContractsHeader = if ($NativeContracts) { (Resolve-Path -LiteralPath $NativeContracts).Path } else { $null }
    foreach ($platform in 'x64', 'x86')
    {
        $log = Join-Path $output "$platform-symbols-build.log"
        if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log }
        & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
            (Join-Path $PSScriptRoot 'imports\ImportSymbols.sln') /Rebuild "Debug|$platform" /Out $log
        if ($LASTEXITCODE -ne 0) { throw "$platform C 符号编译失败" }
        if ([IO.File]::ReadAllText($log) -notmatch 'warning CPPWIN32001') { throw '缺少重名 API 的编译警告' }
        $binaryPlatform = if ($platform -eq 'x86') { 'Win32' } else { 'x64' }
        $objects = @('ImportSymbols')
        if ($SdkHeaders) { $objects += 'SdkImportSymbols' }
        foreach ($name in $objects)
        {
            $object = Join-Path $output "symbols\$binaryPlatform\$name.obj"
            $symbols = (& $dumpbin /symbols $object | Out-String)
            if ($LASTEXITCODE -ne 0) { throw "读取符号失败：$object" }
            $symbols | Set-Content -LiteralPath (Join-Path $output "$platform-$name-symbols.txt") -Encoding utf8
            $expected = if ($name -eq 'ImportSymbols') {
                if ($platform -eq 'x86') { @('_LibraryProbe@0', '_user32_LibraryProbe@4') } else { @('LibraryProbe', 'user32_LibraryProbe') }
            } else {
                if ($platform -eq 'x86') { @('_GetDeviceID@8', '_tbs_GetDeviceID@16') } else { @('GetDeviceID', 'tbs_GetDeviceID') }
            }
            foreach ($symbol in $expected)
            {
                if ($symbols -notmatch ('(?m)\|\s*' + [regex]::Escape($symbol) + '\s*$')) { throw "缺少预期 C 符号：$symbol" }
            }
            if ($symbols -match '\?(LibraryProbe|user32_LibraryProbe|GetDeviceID|tbs_GetDeviceID)@') { throw '出现了不应有的 C++ 函数符号' }
        }
    }
}
finally { $env:SdkHeadersRoot = $previousSdkHeaders; $env:NativeContractsHeader = $previousNativeContracts }

}
finally { $env:_CL_ = $previousPostCL }
