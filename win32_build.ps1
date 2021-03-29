<#
.synopsis
    GIPS build helper
.description
    Locally install all required tools to build GIPS (except Visual Studio)
    and start the build process.
.parameter NoBuild
    Prepare everything, but don't start the build.
.parameter ShellOnly
    Don't build, just create a shell where the Visual Studio Build Tools
    are available.
.parameter BuildType
    Set CMAKE_BUILD_TYPE to Debug / Release / RelWithDebInfo / MinSizeRel.
#>
param(
    [switch] $ShellOnly,
    [switch] $NoBuild,
    [string] $BuildType = "Release"
)

# switch into this script's base directory
Set-Location (Split-Path -Parent $PSCommandPath)

# helper function to download the latest release of something from GitHub
function GitHubDownload($description, $repo, $pattern, $filename) {
    if (Test-Path -LiteralPath $filename) { return }
    Write-Host -ForegroundColor Cyan "downloading prerequisite: $description"
    $metaUrl = "https://api.github.com/repos/$repo/releases/latest"
    Write-Host -ForegroundColor Gray "$metaUrl"
    $url = (Invoke-WebRequest $metaUrl `
        | Select-Object -ExpandProperty Content `
        | ConvertFrom-Json `
        | Select-Object -ExpandProperty assets `
        | Where-Object browser_download_url -Match $pattern `
        | Select-Object -ExpandProperty browser_download_url)
    if (-not $url) {
        Write-Host -ForegroundColor Red "no download URL for $description found"
        exit 1
    }
    Write-Host -ForegroundColor Gray "$url"
    Invoke-WebRequest $url -OutFile $filename
}

# get vswhere.exe
GitHubDownload "vswhere" -repo microsoft/vswhere -pattern "vswhere.exe$" -filename vswhere.exe

# detect VS Path
Write-Host -ForegroundColor Cyan "searching for Visual Studio installation"
$vcvars = &./vswhere -products * -latest -find VC/Auxiliary/Build/vcvars64.bat
if (-not $vcvars) {
    Write-Host -ForegroundColor Red "no suitable Visual Studio installation found"
    exit 1
}
Write-Host -ForegroundColor Green "Visual Studio found: $vcvars"

# shell mode?
if ($ShellOnly) {
    cmd /k "$vcvars"
    exit
}

# get Ninja
if (-not (Test-Path -LiteralPath ninja.exe)) {
    GitHubDownload "Ninja" -repo ninja-build/ninja -pattern "[._-]win.*zip$" -filename ninja.zip
    Expand-Archive -Path ninja.zip -DestinationPath .
    Remove-Item ninja.zip
}

# get CMake
$cmake = ""
if (-not $cmake) {
    # search for local ZIP installation
    $cmake = (Get-Item "cmake-*/bin/cmake.exe" | Select-Object -ExpandProperty FullName)
}
if (-not $cmake) {
    # search for a system-wide installation
    $dir = (Get-ItemProperty -Path HKLM:/SOFTWARE/Kitware/CMake -ErrorAction SilentlyContinue `
        | Select-Object -ExpandProperty InstallDir -ErrorAction SilentlyContinue)
    if ($dir) {
        $f = Join-Path $dir bin/cmake.exe
        if (Test-Path -LiteralPath $f) { $cmake = $f }
    }
}
if (-not $cmake) {
    # download a local ZIP installation
    GitHubDownload "CMake" -repo Kitware/CMake -pattern "[._-]win.*x(86_)?64.*zip$" -filename cmake.zip
    Expand-Archive -Path "cmake.zip" -DestinationPath .
    Remove-Item "cmake.zip"
    $cmake = (Get-Item "cmake-*/bin/cmake.exe" | Select-Object -ExpandProperty FullName)
}
if (-not $cmake) {
    # nothing worked, giving up
    Write-Host -ForegroundColor Red "no suitable CMake installation found"
    exit 1
}
Write-Host -ForegroundColor Green "CMake found: $cmake"

# create the build directory and script
Write-Host -ForegroundColor Cyan "creating build directory and script"
if (-not (Test-Path _build)) {
    mkdir _build >$null
}

$script = @"
@cd /d "%~dp0"

"@
$script += "call `"$vcvars`"`n"
$script += "`"$cmake`" -GNinja -DCMAKE_BUILD_TYPE=$BuildType -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe ..`n"
$script += @"
@if errorlevel 1 goto end
..\ninja
:end
"@
$script | Set-Content -Path _build/build.cmd -Encoding ascii

# finally, run the build
if (-not $NoBuild) {
    cmd /c _build\build.cmd
}
