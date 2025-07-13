# Auto-build Qt Windows application deployment script
$build_dir = "$PSScriptRoot/../build"
$release_dir = "$PSScriptRoot/release"
$installerDir = "$PSScriptRoot/installer"
$languagesDir = "$PSScriptRoot/Languages"
# Get Qt path from environment variable or use default
if ($env:Qt6_DIR) {
    $qt_home = "$env:Qt6_DIR"
} elseif ($env:QT_ROOT_DIR) {
    $qt_home = "$env:QT_ROOT_DIR"
} else {
    # Fallback for local development
    $qt_home = "E:/Qt/6.9.1/mingw_64"
}

$qmake_path = Join-Path $qt_home "bin\qmake.exe"
$deploy_tool = Join-Path $qt_home "bin\windeployqt6.exe"

Write-Host "Qt home: $qt_home"
Write-Host "QMake path: $qmake_path"
Write-Host "Deploy tool path: $deploy_tool"

# Read version from version.pri
$versionFile = "$PSScriptRoot/../version.pri"
$versionMajor = (Select-String "VER_MAJOR = " $versionFile | ForEach-Object { $_.Line.Split('=')[1].Trim() })
$versionMinor = (Select-String "VER_MINOR = " $versionFile | ForEach-Object { $_.Line.Split('=')[1].Trim() })
$versionPatch = (Select-String "VER_PATCH = " $versionFile | ForEach-Object { $_.Line.Split('=')[1].Trim() })
$version = "$versionMajor.$versionMinor.$versionPatch"

# Get CPU architecture
$arch = $env:PROCESSOR_ARCHITECTURE
switch ($arch) {
    "AMD64" { $archName = "x86_64" }
    "ARM64" { $archName = "arm64" }
    default { $archName = $arch }
}

<#
.SYNOPSIS
Safely clean build directory and handle file lock issues
#>
function SafeRemove-BuildDir {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string]$path
    )
    
    # Terminate processes that may occupy files
    $processes = @("qtcreator", "ScreenCast")
    foreach ($proc in $processes) {
        Get-Process -Name $proc -ErrorAction SilentlyContinue | 
            Where-Object { $_.Path -like "*$path*" } |
            Stop-Process -Force -ErrorAction SilentlyContinue
    }
    
    $retryCount = 3
    $delay = 2
    
    if (Test-Path $path) {
        for ($i = 1; $i -le $retryCount; $i++) {
            try {
                Remove-Item $path -Recurse -Force -ErrorAction Stop
                Write-Host "[SUCCESS] Build directory cleaned: $path"
                return
            }
            catch {
                Write-Warning "[ATTEMPT $i] Clean failed: $_"
                if ($i -lt $retryCount) {
                    Write-Host "Waiting $delay seconds..."
                    Start-Sleep -Seconds $delay
                    $delay *= 2
                }
            }
        }
        Write-Error "Failed to clean build directory after $retryCount attempts"
        exit 1
    }
    else {
        Write-Host "Build directory not found: $path"
    }
}

# ========== Main script execution starts ==========
Write-Host "Starting build process..."
Write-Host "Using Qt home: $qt_home"
Write-Host "Version: $version"
Write-Host "Architecture: $archName"

# Clean build directories
SafeRemove-BuildDir -path $build_dir
SafeRemove-BuildDir -path $release_dir
SafeRemove-BuildDir -path $installerDir

# Initialize build environment
[void](New-Item -Path $build_dir -ItemType Directory -Force)

# Generate Release version
Set-Location $build_dir
Write-Host "Generating Makefiles..."
& $qmake_path "$PSScriptRoot/../ScreenCast.pro" -spec win32-g++ "CONFIG+=release"

if (-not $?) {
    Write-Error "QMake failed! Check project configuration"
    exit 1
}

Write-Host "Building with mingw32-make..."
mingw32-make

if (-not $?) {
    Write-Error "Build failed! Check compiler output"
    exit 1
}

# Create release directory
if (-not (Test-Path $release_dir)) {
    New-Item -Path $release_dir -ItemType Directory | Out-Null
}

# Copy executable file
$exePath = "$build_dir/release/ScreenCast.exe"
if (Test-Path $exePath) {
    Copy-Item $exePath $release_dir
}
else {
    Write-Error "Executable not found: $exePath"
    exit 1
}

# Deploy Qt dependencies
Write-Host "Deploying Qt dependencies..."
& $deploy_tool "$release_dir/ScreenCast.exe" --no-translations --no-system-d3d-compiler --no-opengl-sw --release

if (-not $?) {
    Write-Error "Deployment failed! Check dependency paths"
    exit 1
}

# Get version information from executable
$versionFile = "$release_dir/ScreenCast.exe"
$appName = (Get-Item $versionFile).VersionInfo.ProductName
$companyName = (Get-Item $versionFile).VersionInfo.CompanyName

# Handle potential encoding issues with Chinese characters
if ([string]::IsNullOrEmpty($appName)) {
    $appName = "ScreenCast"
}
if ([string]::IsNullOrEmpty($companyName)) {
    $companyName = "ZhangFeng"
}

# Create installer package
Write-Host "`nCreating installer package..."
$isccPath = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $isccPath)) {
    $isccPath = "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
}

if (-not (Test-Path $installerDir)) {
    New-Item -ItemType Directory -Path $installerDir -Force | Out-Null
}

# Inno Setup configuration
$appIconPath = Convert-Path -Path "$release_dir\..\..\res\app_icons\windows_icon.ico"
$installerName = "${appName}-v${version}-windows-${archName}"
$issPath = Join-Path $installerDir "win_installer.iss"
$issContent = @"
; Auto-generated installer script
#define MyAppName "$appName"
#define MyAppVersion "$version"
#define MyAppPublisher "$companyName"
#define MyAppURL "https://github.com/frankzhangv5/ScreenCast"
#define MyAppExeName "${appName}.exe"
#define SourceDir "$(Convert-Path $release_dir)"
#define MyOutputDir "$(Convert-Path $installerDir)"
#define MyAppIcon "$(if ($appIconPath) { Convert-Path $appIconPath } else { '' })"
#define MyLanguageDir "$(Convert-Path $languagesDir)"
#define MyInstallerName "$installerName"

[Setup]
AppId={{08002A78-CCA5-4196-9B6A-FD8301627357}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
OutputDir={#MyOutputDir}
OutputBaseFilename={#MyInstallerName}-Setup
SetupIconFile={#MyAppIcon}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "English"; MessagesFile: "compiler:Default.isl"
Name: "ChineseSimplified"; MessagesFile: "{#MyLanguageDir}\ChineseSimplified.isl"
Name: "ChineseTraditional"; MessagesFile: "{#MyLanguageDir}\ChineseTraditional.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: *.pdb, *.ilk, *.exp, *.obj, debug

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
"@

# Write Inno Setup script
Set-Content -Path $issPath -Value $issContent -Encoding Default

# Create installer package using Inno Setup
Write-Host "Building installer with Inno Setup..."
if (Test-Path $isccPath) {
    try {
        & "$isccPath" $issPath /Q
        if ($LASTEXITCODE -eq 0) {
            $installerPath = Join-Path $installerDir ${installerName}-Setup.exe
            Write-Host "Inno Setup installer created successfully: $installerPath"
        } else {
            Write-Error "Inno Setup compilation failed with exit code: $LASTEXITCODE"
            exit 1
        }
    } catch {
        Write-Error "Inno Setup failed: $_"
        exit 1
    }
} else {
    Write-Error "Inno Setup not found at: $isccPath"
    Write-Error "Please install Inno Setup 6 to create Windows installer"
    exit 1
}

# Cleanup and output result
Write-Host "Cleaning up temporary files..."
# Remove-Item -Path $issPath -Force -ErrorAction SilentlyContinue

# Show final result
if (Test-Path $installerPath) {
    $fileInfo = Get-Item $installerPath
    $fileSize = [math]::Round($fileInfo.Length / 1MB, 2)

    Write-Host "`nBuild completed successfully!"
    Write-Host "Output file: $installerPath"
    Write-Host "File size: $fileSize MB"
    Write-Host "`nOpening output directory..."

    # Open output directory
    Start-Process explorer.exe -ArgumentList "/select,`"$installerPath`""
} else {
    Write-Error "Failed to create final installer package"
    exit 1
}