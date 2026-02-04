; Seko Screenshot Tool Installer Script
; Inno Setup 6.x

#define MyAppName "Seko"
#define MyAppVersion "3.0.0"
#define MyAppPublisher "Seko"
#define MyAppURL "https://nekoo.ru"
#define MyAppExeName "seko.exe"

[Setup]
AppId={{8B5CF6A0-1234-5678-9ABC-DEF012345678}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\LICENSE
OutputDir=.
OutputBaseFilename=seko-setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startup"; Description: "Start with Windows"; GroupDescription: "Additional options:"; Flags: checkedonce

[Files]
Source: "..\build\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "NekooScreenshot"; ValueData: """{app}\{#MyAppExeName}"""; Flags: uninsdeletevalue; Tasks: startup

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  // Check if app is running and close it
  if CheckForMutexes('NekooScreenshotMutex') then
  begin
    if MsgBox('Seko is currently running. Setup must close it to continue. Close now?', mbConfirmation, MB_YESNO) = IDYES then
    begin
      Exec('taskkill', '/F /IM seko.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    end
    else
    begin
      Result := False;
      Exit;
    end;
  end;
  Result := True;
end;
