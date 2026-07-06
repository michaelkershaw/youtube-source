; YouTube Source Plugin for VirtualDJ - Inno Setup Installer
; Build with Inno Setup 6: https://jrsoftware.org/isdl.php
; Compile: ISCC.exe YouTubeSource.iss  (or use build_installer.bat)

#define AppName "YouTube Source for VirtualDJ"
#define AppVersion "3.0.0.0"
#define AppPublisher "YouTube Source"
#define SourceRoot ".."

[Setup]
AppId={{B7E8F2A1-4C6D-4E9A-9F3B-2D8C5A7E1F04}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\VirtualDJ\Plugins64\OnlineSources
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=output
OutputBaseFilename=YouTubeSource_Setup_{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile=logo.ico
WizardImageFile=wizard.bmp
WizardSmallImageFile=wizard-small.bmp
UninstallDisplayIcon={app}\logo.ico
UninstallDisplayName={#AppName}
CloseApplications=no

[Files]
; Plugin DLL
Source: "{#SourceRoot}\x64\Release\YouTubeSource64.dll"; DestDir: "{app}"; Flags: ignoreversion
; WebView2 UI
Source: "{#SourceRoot}\ui\*"; DestDir: "{app}\youtube-source\ui"; Flags: ignoreversion recursesubdirs
; Tools (in dedicated youtube-source subfolder)
Source: "{#SourceRoot}\yt-dlp.exe"; DestDir: "{app}\youtube-source"; Flags: ignoreversion
Source: "{#SourceRoot}\ffmpeg-8.0.1-essentials_build\bin\ffmpeg.exe"; DestDir: "{app}\youtube-source"; Flags: ignoreversion
; Logo for Add/Remove Programs entry
Source: "logo.ico"; DestDir: "{app}"; Flags: ignoreversion

[Run]
; Install WebView2 runtime if missing (bootstrapper downloaded in code below)
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; \
  StatusMsg: "Installing Microsoft WebView2 Runtime..."; Check: NeedsWebView2; Flags: waituntilterminated

[Code]
function NeedsWebView2: Boolean;
var
  Version: String;
begin
  // Evergreen WebView2 runtime per-machine or per-user registry keys
  Result := not (
    RegQueryStringValue(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) or
    RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) or
    RegQueryStringValue(HKCU, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version)
  ) or (Version = '') or (Version = '0.0.0.0');
end;

function IsVirtualDJRunning: Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  if Exec(ExpandConstant('{cmd}'), '/C tasklist /FI "IMAGENAME eq virtualdj.exe" | find /I "virtualdj.exe" > nul',
          '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    Result := (ResultCode = 0);
end;

function InitializeSetup: Boolean;
begin
  Result := True;
  while IsVirtualDJRunning do
  begin
    if MsgBox('VirtualDJ is currently running.' + #13#10 + #13#10 +
              'Please close VirtualDJ, then click OK to continue.',
              mbError, MB_OKCANCEL) = IDCANCEL then
    begin
      Result := False;
      Exit;
    end;
  end;
end;

procedure InitializeWizard;
begin
  // Nothing extra
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if (CurStep = ssInstall) and NeedsWebView2 then
  begin
    // Download the WebView2 Evergreen bootstrapper (~2 MB)
    if not Exec(ExpandConstant('{cmd}'),
        '/C powershell -NoProfile -Command "Invoke-WebRequest -Uri ''https://go.microsoft.com/fwlink/p/?LinkId=2124703'' -OutFile ''' +
        ExpandConstant('{tmp}\MicrosoftEdgeWebview2Setup.exe') + '''"',
        '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
    begin
      MsgBox('Could not download the WebView2 Runtime installer.' + #13#10 +
             'The plugin will fall back to the classic interface.' + #13#10 +
             'You can install WebView2 later from:' + #13#10 +
             'https://developer.microsoft.com/microsoft-edge/webview2/', mbInformation, MB_OK);
    end;
  end;
end;

[UninstallDelete]
Type: filesandordirs; Name: "{app}\youtube-source"
Type: files; Name: "{app}\YouTubeSource64.dll"
Type: files; Name: "{app}\logo.ico"

[Messages]
FinishedLabelNoIcons=Setup has finished installing [name].%n%nStart VirtualDJ and YouTube Source will appear in the browser sidebar.%nLog in via right-click on the folder > Manage License to activate.
