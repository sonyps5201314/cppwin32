param(
    [Parameter(Mandatory)][string]$Headers,
    [Parameter(Mandatory)][string]$Baseline,
    [string]$Output = 'Z:\iid-uuid-20261105\guid-tests',
    [ValidateSet('Ansi','Unicode')][string[]]$CharacterSets = @('Ansi','Unicode'),
    [switch]$NegativeControl
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$Headers = (Resolve-Path -LiteralPath $Headers).Path
$Baseline = (Resolve-Path -LiteralPath $Baseline).Path
$Output = [IO.Path]::GetFullPath($Output)
& python -B (Join-Path $repo 'test\AuditInterfaceGuids.py') --baseline (Join-Path $Baseline 'win32') --headers (Join-Path $Headers 'win32') --output $Output
if ($LASTEXITCODE -ne 0) { throw '接口 GUID 逐项检查失败' }
$previousHeaders, $previousOutput = $env:ProjectionHeaders, $env:InterfaceGuidOutput
$previousPostCL = $env:_CL_
try
{
    $env:InterfaceGuidOutput = $Output
    if ($NegativeControl)
    {
        $env:ProjectionHeaders = $Baseline
        $log = Join-Path $Output 'baseline-negative-x64.log'
        & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' (Join-Path $PSScriptRoot 'InterfaceGuidTests.sln') /Rebuild 'Debug|x64' /Out $log
        if ($LASTEXITCODE -eq 0 -or [IO.File]::ReadAllText($log) -notmatch 'error C2787') { throw '旧头文件没有触发预期的缺少 UUID 错误' }
    }
    $env:ProjectionHeaders = $Headers
    foreach ($characterSet in $CharacterSets)
    {
        $env:_CL_ = "$previousPostCL " + $(if ($characterSet -eq 'Unicode') { '/DUNICODE /D_UNICODE' } else { '/UUNICODE /U_UNICODE' })
        foreach ($platform in 'x64','x86')
        {
            $log = Join-Path $Output "$characterSet-$platform-build.log"
            & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' (Join-Path $PSScriptRoot 'InterfaceGuidTests.sln') /Rebuild "Debug|$platform" /Out $log
            if ($LASTEXITCODE -ne 0) { throw "$characterSet/$platform __uuidof 和 IID 编译检查失败" }
        }
    }
    [ordered]@{Headers=$Headers;Baseline=$Baseline;Platforms=@('x64','x86');CharacterSets=$CharacterSets;TranslationUnits=@('Forward.cpp','Complete.cpp');NegativeControl=[bool]$NegativeControl} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Output 'verification.json') -Encoding utf8
}
finally { $env:ProjectionHeaders, $env:InterfaceGuidOutput = $previousHeaders, $previousOutput; $env:_CL_ = $previousPostCL }
