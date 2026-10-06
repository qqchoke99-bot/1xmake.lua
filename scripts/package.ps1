param(
    [ValidateSet("arm64-v8a", "all")]
    [string]$Abi = "arm64-v8a",
    [string]$NdkPath = $env:ANDROID_NDK_HOME,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if (-not $NdkPath) {
    Write-Error "Set ANDROID_NDK_HOME or pass -NdkPath"
}

$Abis = if ($Abi -eq "all") { @("arm64-v8a") } else { @($Abi) }

foreach ($a in $Abis) {
    $BuildDir = Join-Path $Root "build-$a"
    if ($Clean -and (Test-Path $BuildDir)) {
        Remove-Item -Recurse -Force $BuildDir
    }

    $Toolchain = Join-Path $NdkPath "build/cmake/android.toolchain.cmake"

    cmake -S $Root -B $BuildDir `
        -G Ninja `
        -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
        -DANDROID_ABI=$a `
        -DANDROID_PLATFORM=android-28 `
        -DANDROID_STL=c++_shared `
        -DCMAKE_BUILD_TYPE=Release `
        -DMOD_ID=realistic_headbob `
        -DMOD_NAME="Realistic Head Bob" `
        -DMOD_AUTHOR=port `
        -DMOD_VERSION=1.0.0 `
        -DMOD_LIBRARY_NAME=realistic_headbob `
        -DMOD_MINECRAFT_VERSIONS='[]'

    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    cmake --build $BuildDir --target levi_package
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "Output: $BuildDir/realistic_headbob.levipack"
}
