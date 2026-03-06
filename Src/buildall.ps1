<#
.SYNOPSIS
Builds the Let It Rain project for all supported architectures and configurations.
#>

$solutionPath = "let-it-rain.sln"

if (!(Test-Path $solutionPath)) {
    Write-Host "Error: Could not find $solutionPath in the current directory." -ForegroundColor Red
    exit 1
}

# Determine the path to MSBuild (requires Visual Studio 2022)
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $msbuildPath = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
} else {
    Write-Host "Error: vswhere.exe not found. Is Visual Studio installed?" -ForegroundColor Red
    exit 1
}

if (-not $msbuildPath) {
    Write-Host "Error: MSBuild.exe not found." -ForegroundColor Red
    exit 1
}

Write-Host "Found MSBuild at: $msbuildPath" -ForegroundColor Green

$platforms = @("x64", "ARM64", "x86")
$configurations = @("Release", "Debug")

$successCount = 0
$failCount = 0

foreach ($config in $configurations) {
    foreach ($platform in $platforms) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Building $config | $platform" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan

        # Run MSBuild
        & $msbuildPath $solutionPath /p:Configuration=$config /p:Platform=$platform /t:Rebuild /v:m

        if ($LASTEXITCODE -eq 0) {
            Write-Host "SUCCESS: $config | $platform" -ForegroundColor Green
            $successCount++
        } else {
            Write-Host "FAILED: $config | $platform" -ForegroundColor Red
            $failCount++
        }
    }
}

Write-Host ""
Write-Host "========================================"
Write-Host "Build Summary:"
Write-Host "  Successful builds: $successCount" -ForegroundColor Green
Write-Host "  Failed builds:     $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Green" })
Write-Host "========================================"

if ($failCount -gt 0) {
    exit 1
}

exit 0
