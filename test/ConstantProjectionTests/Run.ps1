param([string]$Emitter = 'L:\Open_Code\win32metadata\bin\Debug\net10.0\ClangSharpSourceToWinmd.exe')
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$output = Join-Path $PSScriptRoot 'obj'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$winmd = Join-Path $output 'Windows.Win32.winmd'
& $Emitter --sourceDir (Join-Path $PSScriptRoot 'input') --arch x64 --outputFileName $winmd "@$(Join-Path $PSScriptRoot 'input\emitter.settings.rsp')"
if ($LASTEXITCODE -ne 0) { throw '测试 winmd 生成失败' }

Push-Location (Join-Path $repo 'cppwin32')
try
{
    $lines = @(& '.\x64\Debug\cppwin32.exe' -input $winmd -output (Join-Path $output 'headers') 2>&1)
    $lines | Write-Output
    if ($LASTEXITCODE -ne 0 -or ($lines | Select-String 'cppwin32 : error')) { throw '测试头文件导出失败' }
}
finally { Pop-Location }

$nativeDefinitions = [IO.File]::ReadAllText((Join-Path $output 'headers\win32\impl\complex_structs.h'))
if ($nativeDefinitions -notmatch 'SDK_OTHER_ALIAS\* NamedAlias;') { throw 'SDK record alias spelling was lost' }

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
