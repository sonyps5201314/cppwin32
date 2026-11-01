param([Parameter(Mandatory)][string]$Headers, [string]$NativeContracts='', [string]$Logs=(Join-Path $PSScriptRoot 'obj\logs'), [switch]$ConfigurationsOnly)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Path $Logs -Force | Out-Null
$previous=@{}
foreach($name in 'ProjectionHeaders','NativeContractsHeader','ConfigurationProbe','ConfigOnly','Cppwin32CrtHeaders','CL','_CL_') {$previous[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
function Build-Probe([string]$Name,[string]$Platform,[string]$Probe,[bool]$OnlyConfig,[bool]$Unicode) {
    $env:ConfigurationProbe=$Probe
    $env:ConfigOnly=if($OnlyConfig){'true'}else{'false'}
    $env:_CL_=if($Unicode){'/DUNICODE /D_UNICODE'}else{'/UUNICODE /U_UNICODE'}
    $log=Join-Path $Logs "$Name-$Platform.log"
    if(Test-Path -LiteralPath $log){Remove-Item -LiteralPath $log}
    & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' (Join-Path $PSScriptRoot 'NoSdkHeaders.sln') /Rebuild "Debug|$Platform" /Out $log
    if($LASTEXITCODE -ne 0){throw "无 SDK 平台头编译失败：$Name/$Platform"}
    $text=[IO.File]::ReadAllText($log)
    if($text -match '(?i)Windows Kits[\\/]\d+[\\/]Include[\\/][^\\/\r\n]+[\\/](shared|um|winrt|cppwinrt)[\\/]'){throw "编译仍引入 Windows SDK 平台头：$log"}
    if($text -notmatch '(?i)[\\/]ucrt[\\/]corecrt.h'){throw "缺少标准运行库 include 轨迹：$log"}
}
try {
    $env:ProjectionHeaders=(Resolve-Path -LiteralPath $Headers).Path
    $runtime=Get-ChildItem -LiteralPath 'C:\Program Files (x86)\Windows Kits\10\Include' -Directory | Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt\corecrt.h')} | Sort-Object {[version]$_.Name} -Descending | Select-Object -First 1
    if(-not $runtime){throw '未找到 C/C++ 标准运行库头文件'}
    $env:Cppwin32CrtHeaders=Join-Path $runtime.FullName 'ucrt'
    $env:NativeContractsHeader=if($NativeContracts){(Resolve-Path -LiteralPath $NativeContracts).Path}else{''}
    $env:CL=''
    if(-not $ConfigurationsOnly){foreach($platform in 'x64','x86'){Build-Probe 'Ansi' $platform '' $false $false; Build-Probe 'Unicode' $platform '' $false $true}}
    $probes=[ordered]@{
        Win7='CPPWIN32_PROBE_WINNT=0x0601'
        Win7Symbol='CPPWIN32_PROBE_WINNT=_WIN32_WINNT_WIN7'
        Overrides='CPPWIN32_PROBE_WINNT=0x0601;CPPWIN32_PROBE_WINVER=0x0501;CPPWIN32_PROBE_NTDDI=0x06010203;CPPWIN32_PROBE_IE=0x0700'
        PcApp='CPPWIN32_PROBE_FAMILY=2'
        PcAppSymbol='CPPWIN32_PROBE_FAMILY=WINAPI_FAMILY_APP'
        Phone='CPPWIN32_PROBE_FAMILY=3'
        System='CPPWIN32_PROBE_FAMILY=4'
        Server='CPPWIN32_PROBE_FAMILY=5'
        Games='CPPWIN32_PROBE_FAMILY=6'
        PartitionOverride='CPPWIN32_PROBE_PARTITION=1'
    }
    foreach($entry in $probes.GetEnumerator()){foreach($platform in 'x64','x86'){Build-Probe $entry.Key $platform $entry.Value $true $false}}
    [ordered]@{Headers=$env:ProjectionHeaders;NativeContracts=$env:NativeContractsHeader;FullBuilds=$(if($ConfigurationsOnly){0}else{4});ConfigurationBuilds=($probes.Count*2);ForbiddenPlatformIncludes=0} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Logs 'verification.json') -Encoding utf8
} finally {foreach($name in $previous.Keys){[Environment]::SetEnvironmentVariable($name,$previous[$name],'Process')}}
