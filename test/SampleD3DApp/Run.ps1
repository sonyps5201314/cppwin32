param(
    [string]$Winmd = 'L:\Open_Code\win32metadata\bin\Windows.Win32.winmd',
    [string]$Output = 'Z:\ms',
    [string[]]$Configurations = @('Debug|x64', 'Debug|x86', 'Release|x64', 'Release|x86'),
    [ValidateSet('Ansi','Unicode')][string[]]$CharacterSets = @('Ansi','Unicode'),
    [string]$NativeContracts = ''
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$devenv = 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com'
$logs = Join-Path $PSScriptRoot 'obj\full-sdk'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$Winmd = (Resolve-Path -LiteralPath $Winmd).Path
$Output = [IO.Path]::GetFullPath($Output)
$project = Join-Path $PSScriptRoot 'SampleD3DApp\SampleD3DApp.vcxproj'
$projectHash = (Get-FileHash -LiteralPath $project -Algorithm SHA256).Hash

function Build-Solution([string]$Solution, [string]$Configuration, [string]$Log)
{
    if (Test-Path -LiteralPath $Log) { Remove-Item -LiteralPath $Log }
    & $devenv $Solution /Rebuild $Configuration /Out $Log
    if ($LASTEXITCODE -ne 0) { throw "构建失败：$Solution $Configuration，日志：$Log" }
}

Build-Solution (Join-Path $repo 'cppwin32\cppwin32.sln') 'Debug|x64' (Join-Path $logs 'generator.log')
Push-Location (Join-Path $repo 'cppwin32')
try
{
    $arguments = @('-input', $Winmd, '-output', $Output)
    if ($NativeContracts) { $arguments += @('-contracts', $NativeContracts) }
    $lines = @(& '.\x64\Debug\cppwin32.exe' @arguments 2>&1)
    $lines | Tee-Object -FilePath (Join-Path $logs 'export.log')
    if ($LASTEXITCODE -ne 0 -or ($lines | Select-String 'cppwin32 : error')) { throw 'SDK 头文件导出失败' }
}
finally { Pop-Location }
foreach ($name in 'complex_structs.h','complex_interfaces.h')
{
    if (-not (Test-Path -LiteralPath (Join-Path $Output "win32\impl\$name"))) { throw "导出缺少必要头文件：$name" }
}
foreach ($name in 'native_declarations.h','native_aliases.h')
{
    if (Test-Path -LiteralPath (Join-Path $Output "win32\impl\$name")) { throw "不应继续生成集中声明头：$name" }
}
if ($NativeContracts -and -not (Test-Path -LiteralPath $NativeContracts)) { throw '导出缺少原生声明检查文件' }

$previousCL = $env:CL
$previousPostCL = $env:_CL_
try
{
    $env:CL = "/I`"$Output`" $previousCL"
    foreach ($characterSet in $CharacterSets)
    {
        $env:_CL_ = "$previousPostCL " + $(if ($characterSet -eq 'Unicode') { '/DUNICODE /D_UNICODE' } else { '/UUNICODE /U_UNICODE' })
        foreach ($configuration in $Configurations)
        {
            $log = Join-Path $logs ($characterSet + '-' + $configuration.Replace('|', '-') + '.log')
            Build-Solution (Join-Path $PSScriptRoot 'SampleD3DApp.sln') $configuration $log
        }
    }
}
finally { $env:CL = $previousCL; $env:_CL_ = $previousPostCL }

if ((Get-FileHash -LiteralPath $project -Algorithm SHA256).Hash -ne $projectHash) { throw '测试工程文件发生了意外修改' }
[ordered]@{
    Winmd = $Winmd
    WinmdSha256 = (Get-FileHash -LiteralPath $Winmd -Algorithm SHA256).Hash
    Headers = $Output
    HeaderCount = @(Get-ChildItem -LiteralPath (Join-Path $Output 'win32') -File -Recurse).Count
    ProjectSha256 = $projectHash
    Configurations = $Configurations
    CharacterSets = $CharacterSets
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logs 'verification.json') -Encoding utf8
