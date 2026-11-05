param(
    [string]$Headers = 'Z:\ms',
    [string]$MetadataRoot = 'L:\Open_Code\win32metadata',
    [string]$Output = 'Z:\sdk-deep-audit-20261105',
    [ValidateSet('x86','x64','arm64')][string[]]$Architectures = @('x86','x64','arm64'),
    [ValidateSet('Ansi','Unicode')][string[]]$CharacterSets = @('Ansi','Unicode'),
    [int]$Jobs = 4,
    [switch]$CompareOnly,
    [switch]$ExtractOnly,
    [switch]$ReportOnly,
    [switch]$UseBuiltExtractor,
    [switch]$SkipSelfTests,
    [switch]$FailOnDifference
)
$ErrorActionPreference = 'Stop'
$Output = [IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Path $Output -Force | Out-Null
if (Test-Path -LiteralPath (Join-Path $Headers 'win32')) { $Headers = Join-Path $Headers 'win32' }
if ($ReportOnly)
{
    & python -B (Join-Path $PSScriptRoot 'report.py') --output $Output
    if ($LASTEXITCODE -ne 0) { throw '汇总报告生成失败' }
    return
}
$environment = @{}
foreach ($name in 'DeepAuditOutput','DeepAuditFlavor','ClangSharpBinaryRoot','DEEP_AUDIT_OUTPUT','DEEP_AUDIT_METADATA','DEEP_AUDIT_HEADERS','PYTHONUTF8') { $environment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
try
{
    $env:PYTHONUTF8 = '1'
    $env:DeepAuditOutput = $Output
    $env:ClangSharpBinaryRoot = Join-Path $MetadataRoot 'bin\Release\net10.0\win-x64'
    $env:DEEP_AUDIT_OUTPUT, $env:DEEP_AUDIT_METADATA, $env:DEEP_AUDIT_HEADERS = $Output, $MetadataRoot, $Headers
    if (-not $UseBuiltExtractor -and -not $CompareOnly)
    {
        foreach ($flavor in 'Extractor','Extractor.Inventory')
        {
            $env:DeepAuditFlavor = $flavor
            & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
                (Join-Path $PSScriptRoot 'Extractor\Extractor.sln') /Build 'Release|Any CPU' /Out (Join-Path $Output "$flavor-build.log")
            if ($LASTEXITCODE -ne 0 -and [IO.File]::ReadAllText((Join-Path $Output "$flavor-build.log")) -match 'NuGet|NETSDK1004' -and [IO.File]::ReadAllText((Join-Path $Output "$flavor-build.log")) -notmatch 'error CS\d+')
            {
                & 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\devenv.com' `
                    (Join-Path $PSScriptRoot 'Extractor\Extractor.sln') /Build 'Release|Any CPU' /Out (Join-Path $Output "$flavor-build-retry.log")
            }
            if ($LASTEXITCODE -ne 0) { throw "AST 提取器构建失败：$flavor" }
        }
    }
    if (-not $SkipSelfTests)
    {
        & python -B (Join-Path $PSScriptRoot 'selftest.py')
        if ($LASTEXITCODE -ne 0) { throw '故障注入自校验失败' }
    }
    $arguments = @('-B', (Join-Path $PSScriptRoot 'run.py'), '--metadata', $MetadataRoot, '--headers', $Headers, '--output', $Output, '--jobs', $Jobs, '--architectures') + $Architectures + @('--character-sets') + $CharacterSets
    if ($CompareOnly) { $arguments += '--compare-only' }
    if ($ExtractOnly) { $arguments += '--extract-only' }
    if ($FailOnDifference) { $arguments += '--fail-on-difference' }
    & python @arguments
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and $exitCode -ne 2) { throw "深度校验未能完成，退出码 $exitCode" }
    if (-not $ExtractOnly)
    {
        & python -B (Join-Path $PSScriptRoot 'report.py') --output $Output
        if ($LASTEXITCODE -ne 0) { throw '汇总报告生成失败' }
    }
    if ($exitCode -eq 2) { throw '校验已完成，但存在差异或未验证项' }
}
finally { foreach ($entry in $environment.GetEnumerator()) { [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process') } }
