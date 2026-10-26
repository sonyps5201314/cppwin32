param([string]$Emitter = 'L:\Open_Code\win32metadata\bin\Debug\net10.0\ClangSharpSourceToWinmd.exe')
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$output = Join-Path $PSScriptRoot 'obj'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$winmd = Join-Path $output 'Windows.Win32.winmd'
& $Emitter --sourceDir (Join-Path $PSScriptRoot 'input') --arch x64 --outputFileName $winmd
if ($LASTEXITCODE -ne 0) { throw '测试 winmd 生成失败' }

Push-Location (Join-Path $repo 'cppwin32')
try
{
    $lines = @(& '.\x64\Debug\cppwin32.exe' -input $winmd -output (Join-Path $output 'headers') 2>&1)
    $lines | Write-Output
    if ($LASTEXITCODE -ne 0 -or ($lines | Select-String 'cppwin32 : error')) { throw '测试头文件导出失败' }
}
finally { Pop-Location }

foreach ($platform in 'x64', 'x86')
{
    & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
        (Join-Path $PSScriptRoot 'ConstantProjectionTests.sln') /Build "Debug|$platform" `
        /Out (Join-Path $output "$platform-build.log")
    if ($LASTEXITCODE -ne 0) { throw "$platform 编译验证失败" }
    $binaryPlatform = if ($platform -eq 'x86') { 'Win32' } else { 'x64' }
    & (Join-Path $output "$binaryPlatform\ConstantProjectionTests.exe")
    if ($LASTEXITCODE -ne 0) { throw "$platform 运行验证失败" }
}
