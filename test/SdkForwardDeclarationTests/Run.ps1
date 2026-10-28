param([string]$Headers = 'Z:\TYPEDEF')
$ErrorActionPreference = 'Stop'
$headersRoot = (Resolve-Path -LiteralPath $Headers).Path
$header = Join-Path $headersRoot 'win32\impl\Windows.Win32.Devices.DeviceAndDriverInstallation.0.h'
if (-not (Test-Path -LiteralPath $header)) { throw "找不到 SDK 投影头文件：$header" }

$output = Join-Path $PSScriptRoot 'obj'
New-Item -ItemType Directory -Path $output -Force | Out-Null
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
