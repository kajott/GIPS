<#
.synopsis
    GIPS build helper
.description
    Locally install all required tools to build GIPS (except Visual Studio)
    and start the build process.
.parameter NoBuild
    Prepare everything, but don't start the build.
.parameter Package
    Create a distribution package after building.
.parameter PackageOnly
    Don't build, only create a distribution package.
.parameter Tagged
    Create the package with a version tag in the name.
.parameter ShellOnly
    Don't build, just create a shell where the Visual Studio Build Tools
    are available.
.parameter BuildType
    Set CMAKE_BUILD_TYPE to Debug / Release / RelWithDebInfo / MinSizeRel.
#>
param(
    [switch] $ShellOnly,
    [switch] $NoBuild,
    [switch] $Package,
    [switch] $PackageOnly,
    [switch] $Tagged,
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

# build steps
if ((-not $PackageOnly) -or $ShellOnly) {
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
        Write-Host -ForegroundColor Cyan "starting main build process"
        & cmd /c _build\build.cmd
        if ($?) {
            Write-Host -ForegroundColor Green "main build process succeeded"
        } else {
            Write-Host -ForegroundColor Red "main build process failed"
            exit 1
        }
    }
}
# build steps completed

# packaging steps
if ($PackageOnly -or ($Package -and (-not $NoBuild))) {
    # build archive name
    $archive = "GIPS"
    if ($Tagged) {
        $version = (Get-Content src/gips_version.h) -join '' -replace '(?s).*"([^"]+)".*','$1'
        $archive += "-$version"
        try { $rev = &git rev-parse HEAD 2>$null }
        catch { $rev = $null }
        try { $branch = &git rev-parse --abbrev-ref HEAD 2>$null }
        catch { $branch = $null }
        try { $tag = &git describe --exact-match 2>$null }
        catch { $tag = $null }
        if (-not "$tag") {
            if ("$branch" -and (-not ("$branch" -eq "$rev"))) {
                $archive += "-" + ($branch -replace '/','-')
            }
            $archive += "-g" + $rev.Substring(0, 7)
        }
    }
    $archive += "-win32.zip"

    # check for existing Pandoc
    $pandoc = (Get-Item "pandoc-*/pandoc.exe" | Select-Object -ExpandProperty FullName)

    if (-not $pandoc) {
        # download Pandoc
        GitHubDownload "Pandoc" -repo jgm/pandoc -pattern "[._-]win.*zip$" -filename pandoc.zip
        Expand-Archive -Path pandoc.zip -DestinationPath .
        Remove-Item pandoc.zip
        $pandoc = (Get-Item "pandoc-*/pandoc.exe" | Select-Object -ExpandProperty FullName)
    }

    # convert the two documentation files
    Write-Host -ForegroundColor Cyan "converting documentation to HTML"
    # change two things in README.md:
    # - replace the link to the ShaderFormat.md with a link to ~.html
    # - remove the build instructions section
    $lines = @()
    $ok = $true
    foreach ($line in (Get-Content README.md)) {
        $line = $line -replace '\.md','.html'
        if ($line -match '^## ') { $ok = -not ($line -match '(?i)build') }
        if ($ok) { $lines += $line }
    }
    # Pandoc will create a (totally harmless) warning here,
    # and it's nearly impossible to make Powershell ignore it :(
    $oldEAP = $ErrorActionPreference
    $ErrorActionPreference = "SilentlyContinue"
    $lines | & $pandoc -f markdown -t html5 -s -T "About GIPS" -o README.html 2>&1 >$null
    & $pandoc -f markdown -t html5 -s -T "GIPS Shader Format" ShaderFormat.md -o ShaderFormat.html 2>&1 >$null
    $ErrorActionPreference = $oldEAP

    # create archive
    Write-Host -ForegroundColor Cyan "creating archive"
    Compress-Archive -DestinationPath "$archive" -Path gips.exe, LICENSE.txt, README.html, ShaderFormat.html, shaders -Force
}
