; Auto-generated installer script
#define MyAppName "ScreenCast"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "ZhangFeng"
#define MyAppURL "https://github.com/frankzhangv5/ScreenCast"
#define MyAppExeName "ScreenCast.exe"
#define SourceDir "E:\QtProjects\GhScreenCast\pack\release"
#define MyOutputDir "E:\QtProjects\GhScreenCast\pack\installer"
#define MyAppIcon "E:\QtProjects\GhScreenCast\res\app_icons\windows_icon.ico"
#define MyLanguageDir "E:\QtProjects\GhScreenCast\pack\Languages"
#define MyInstallerName "ScreenCast-v1.0.0-windows-x86_64"

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
