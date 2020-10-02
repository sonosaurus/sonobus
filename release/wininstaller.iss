[Setup]
AppName=SonoBus
AppVersion={#SBVERSION}
MinVersion=6.1
WizardStyle=modern
DefaultDirName={autopf}\SonoBus
DefaultGroupName=SonoBus
UninstallDisplayIcon={app}\SonoBus.exe
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=SonoBus Installer
;LicenseFile=license.rtf
SetupLogging=yes

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom


[Components]
Name: "app"; Description: "Standalone application (.exe)"; Types: full custom;
;Name: "vst2_32"; Description: "32-bit VST2 Plugin (.dll)"; Types: full custom;
Name: "vst2_64"; Description: "64-bit VST2 Plugin (.dll)"; Types: full custom; Check: Is64BitInstallMode;
;Name: "vst3_32"; Description: "32-bit VST3 Plugin (.vst3)"; Types: full custom;
Name: "vst3_64"; Description: "64-bit VST3 Plugin (.vst3)"; Types: full custom; Check: Is64BitInstallMode;
;Name: "rtas_32"; Description: "32-bit RTAS Plugin (.dpm)"; Types: full custom;
;Name: "aax_32"; Description: "32-bit AAX Plugin (.aaxplugin)"; Types: full custom;
;Name: "aax_64"; Description: "64-bit AAX Plugin (.aaxplugin)"; Types: full custom; Check: Is64BitInstallMode;
;Name: "manual"; Description: "User guide"; Types: full custom; Flags: fixed



[Files]
Source: "SonoBus\SonoBus.exe";  DestDir: "{app}" ; Check: Is64BitInstallMode; Components:app; Flags: ignoreversion;
;Source: "SonoBus\Plugins\SonoBus.dll";  DestDir: {code:GetVST2Dir_64}; Check: Is64BitInstallMode; Components:vst2_64; Flags: ignoreversion;
Source: "SonoBus\Plugins\SonoBus.dll";  DestDir: "{autopf64}\Steinberg\VSTPlugins\"; Check: Is64BitInstallMode; Components:vst2_64; Flags: ignoreversion;
Source: "SonoBus\Plugins\SonoBus.vst3"; DestDir: "{commoncf64}\VST3\"; Check: Is64BitInstallMode; Components:vst3_64; Flags: ignoreversion;
Source: "SonoBus\README.txt"; DestDir: "{app}"; DestName: "README.txt"; Flags: isreadme


[Icons]
Name: "{group}\SonoBus"; Filename: "{app}\SonoBus.exe"
Name: "{group}\README"; Filename: "{app}\README.txt"
Name: "{group}\Uninstall SonoBus"; Filename: "{app}\unins000.exe"

[Code]
var
  OkToCopyLog : Boolean;
  //VST2DirPage_32: TInputDirWizardPage;
  //VST2DirPage_64: TInputDirWizardPage;

procedure InitializeWizard;
begin

//  if IsWin64 then begin
//    VST2DirPage_64 := CreateInputDirPage(wpSelectDir,
//    'Confirm 64-Bit VST2 Plugin Directory', '',
//    'Select the folder in which setup should install the 64-bit VST2 Plugin, then click Next.',
//    False, '');
//    VST2DirPage_64.Add('');
//    VST2DirPage_64.Values[0] := ExpandConstant('{reg:HKLM\SOFTWARE\VST,VSTPluginsPath|{pf}\Steinberg\VSTPlugins}\');
//
//  end;
  
end;


//function GetVST2Dir_64(Param: String): String;
//begin
//  Result := VST2DirPage_64.Values[0]
//end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssDone then
    OkToCopyLog := True;
end;

procedure DeinitializeSetup();
begin
  if OkToCopyLog then
    FileCopy (ExpandConstant ('{log}'), ExpandConstant ('{app}\InstallationLogFile.log'), FALSE);
  RestartReplace (ExpandConstant ('{log}'), '');
end;

[UninstallDelete]
Type: files; Name: "{app}\InstallationLogFile.log"
