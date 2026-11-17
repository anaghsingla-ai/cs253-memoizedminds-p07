param(
    [ValidateSet("Demo", "Menu", "Tests")]
    [string]$Mode = "Demo"
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$buildDir = Join-Path $root "build"
$cacheFile = Join-Path $buildDir "CMakeCache.txt"
$exe = Join-Path $buildDir "calibration.exe"
$workload = Join-Path $root "tests/cases/large_demo.txt"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake was not found. Install CMake 3.16+ and add it to PATH."
}

if (-not (Get-Command g++ -ErrorAction SilentlyContinue) -and
    -not (Get-Command clang++ -ErrorAction SilentlyContinue) -and
    -not (Get-Command cl -ErrorAction SilentlyContinue)) {
    throw "No C++ compiler was found. Install GCC/MinGW, Clang, or Visual Studio C++ Build Tools."
}

# If there is no build directory yet, choose a generator available on Windows.
# Also discard a stale NMake configuration when MinGW Make is available.
$generatorArgs = @()
$needsConfigure = -not (Test-Path $cacheFile)
if ($needsConfigure) {
    if (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
        $generatorArgs = @("-G", "MinGW Makefiles")
    } elseif (Get-Command make -ErrorAction SilentlyContinue) {
        $generatorArgs = @("-G", "Unix Makefiles")
    }
} else {
    $cacheText = Get-Content $cacheFile -Raw
    if ($cacheText -match "CMAKE_GENERATOR:INTERNAL=NMake Makefiles" -and
        (Get-Command mingw32-make -ErrorAction SilentlyContinue)) {
        Remove-Item $buildDir -Recurse -Force
        $needsConfigure = $true
        $generatorArgs = @("-G", "MinGW Makefiles")
    }
}

if ($needsConfigure) {
    Write-Host "[1/3] Configuring project..." -ForegroundColor Cyan
    & cmake -S $root -B $buildDir @generatorArgs -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
} else {
    Write-Host "[1/3] Existing CMake configuration found." -ForegroundColor Cyan
}

Write-Host "[2/3] Building calibration application..." -ForegroundColor Cyan
& cmake --build $buildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

if (-not (Test-Path $exe)) {
    # Multi-configuration generators may place the executable under Release.
    $releaseExe = Join-Path $buildDir "Release/calibration.exe"
    if (Test-Path $releaseExe) { $exe = $releaseExe }
}
if (-not (Test-Path $exe)) { throw "Could not find the calibration executable after building." }

switch ($Mode) {
    "Demo" {
        Write-Host "[3/3] Running the 100,000-element A/B/Oracle demonstration..." -ForegroundColor Cyan
        & $exe --run $workload --structure both
        if ($LASTEXITCODE -ne 0) { throw "Demo run failed." }
    }
    "Menu" {
        Write-Host "[3/3] Opening the interactive calibration workbench..." -ForegroundColor Cyan
        & $exe --menu
        if ($LASTEXITCODE -ne 0) { throw "Application exited with an error." }
    }
    "Tests" {
        Write-Host "[3/3] Running the automated test suite..." -ForegroundColor Cyan
        & ctest --test-dir $buildDir -C Release --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw "One or more tests failed." }
    }
}
