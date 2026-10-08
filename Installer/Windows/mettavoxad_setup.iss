#ifndef PluginArch
  #define PluginArch "x64"
#endif
#if PluginArch == "x86"
  #define CommonFiles "{commoncf32}"
  #define ProgramFiles "{autopf32}"
#else
  #define CommonFiles "{commoncf64}"
  #define ProgramFiles "{autopf64}"
#endif
[Setup]
AppId=mettavoxad-2-{#PluginArch}
AppName=METTAVOXAD 2.1 ({#PluginArch})
AppVersion=2.1.0
AppPublisher=mettavoxad Audio Engineering
DefaultDirName={#CommonFiles}\VST2\mettavoxad
DefaultGroupName=METTAVOXAD 2.1 ({#PluginArch})
#if PluginArch == "x64"
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#endif
MinVersion=6.1sp1
OutputDir=Output
OutputBaseFilename=mettavoxad_2.1.0_{#PluginArch}_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
UsePreviousAppDir=yes
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
[Files]
Source: "..\..\build_{#PluginArch}\mettavoxad_artefacts\Release\VST\METTAVOXAD21.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\build_{#PluginArch}\mettavoxad_artefacts\Release\VST3\METTAVOXAD21.vst3\*"; DestDir: "{#CommonFiles}\VST3\METTAVOXAD21.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\Docs\COMPATIBILIDADE_E_BUILD.md"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\Instrucoes"; Filename: "{app}\COMPATIBILIDADE_E_BUILD.md"

; Keep AppId mettavoxad-2-ARCH: 2.0.1 is upgraded in place, one uninstall entry.
; Remove only known old plugin files of this architecture, never other plugins.
[InstallDelete]
Type: files; Name: "{app}\mettavoxad 2.0.dll"
Type: files; Name: "{app}\mettavoxad.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad\mettavoxad 2.0.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad\mettavoxad.dll"
Type: filesandordirs; Name: "{#CommonFiles}\VST3\mettavoxad.vst3"
#if PluginArch == "x64"
Type: filesandordirs; Name: "{#CommonFiles}\VST3\mettavoxad 2.0.vst3"
#endif

; Known 1.1 BAT destinations; delete exact METTAVOXAD names only.
Type: files; Name: "{#ProgramFiles}\MAGIX\VSTPlugins\mettavoxad.dll"
Type: files; Name: "{#ProgramFiles}\MAGIX\VSTPlugins\mettavoxad_Mono.dll"
Type: files; Name: "{#ProgramFiles}\MAGIX\VSTPlugins\mettavoxad_Sliders.dll"
Type: files; Name: "{#ProgramFiles}\MAGIX\VSTPlugins\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\MAGIX\VSTPlugins\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Steinberg\VstPlugins\mettavoxad.dll"
Type: files; Name: "{#ProgramFiles}\Steinberg\VstPlugins\mettavoxad_Mono.dll"
Type: files; Name: "{#ProgramFiles}\Steinberg\VstPlugins\mettavoxad_Sliders.dll"
Type: files; Name: "{#ProgramFiles}\Steinberg\VstPlugins\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Steinberg\VstPlugins\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\VSTPlugins\mettavoxad.dll"
Type: files; Name: "{#ProgramFiles}\VSTPlugins\mettavoxad_Mono.dll"
Type: files; Name: "{#ProgramFiles}\VSTPlugins\mettavoxad_Sliders.dll"
Type: files; Name: "{#ProgramFiles}\VSTPlugins\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\VSTPlugins\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad_Mono.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad_Sliders.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#CommonFiles}\VST2\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\VstPlugins\mettavoxad.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\VstPlugins\mettavoxad_Mono.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\VstPlugins\mettavoxad_Sliders.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\VstPlugins\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\VstPlugins\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{code:LegacyRegisteredFolder}\mettavoxad.dll"
Type: files; Name: "{code:LegacyRegisteredFolder}\mettavoxad_Mono.dll"
Type: files; Name: "{code:LegacyRegisteredFolder}\mettavoxad_Sliders.dll"
Type: files; Name: "{code:LegacyRegisteredFolder}\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{code:LegacyRegisteredFolder}\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{code:LegacyDesktopFolder}\mettavoxad.dll"
Type: files; Name: "{code:LegacyDesktopFolder}\mettavoxad_Mono.dll"
Type: files; Name: "{code:LegacyDesktopFolder}\mettavoxad_Sliders.dll"
Type: files; Name: "{code:LegacyDesktopFolder}\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{code:LegacyDesktopFolder}\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Sonic Foundry\Shared Plug-Ins\VstPlugins\mettavoxad.dll"
Type: files; Name: "{#ProgramFiles}\Sonic Foundry\Shared Plug-Ins\VstPlugins\mettavoxad_Mono.dll"
Type: files; Name: "{#ProgramFiles}\Sonic Foundry\Shared Plug-Ins\VstPlugins\mettavoxad_Sliders.dll"
Type: files; Name: "{#ProgramFiles}\Sonic Foundry\Shared Plug-Ins\VstPlugins\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Sonic Foundry\Shared Plug-Ins\VstPlugins\mettavoxad_Mono_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\mettavoxad.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\mettavoxad_Mono.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\mettavoxad_Sliders.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\mettavoxad_{#PluginArch}.dll"
Type: files; Name: "{#ProgramFiles}\Sony\Shared Plug-Ins\mettavoxad_Mono_{#PluginArch}.dll"

[Code]
function LegacyRegisteredFolder(Param: String): String;
begin
#if PluginArch == "x64"
  if not RegQueryStringValue(HKLM64, 'Software\VST', 'VSTPluginsPath', Result) then
#else
  if not RegQueryStringValue(HKLM32, 'Software\VST', 'VSTPluginsPath', Result) then
#endif
    Result := '';
  if (Result = '') or (not DirExists(Result)) then
    Result := ExpandConstant('{app}\__no_legacy_location__');
end;

function LegacyDesktopFolder(Param: String): String;
begin
  Result := ExpandConstant('{userdesktop}\mettavoxad_VST_Plugins');
#if PluginArch == "x86"
  if IsWin64 then Result := Result + '\32-bit (Sound Forge 7, 8, 9, 10)';
#endif
end;

#if PluginArch == "x64"
// Earlier 2.0.0 used a different Inno AppId. Run its registered uninstaller.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Location, Uninstaller, Key: String;
  ResultCode: Integer;
begin
  Result := '';
  Key := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{7E4C9A31-2F8B-4D60-9C1A-58B3E0A7D2F4}_is1';
  if not RegQueryStringValue(HKLM64, Key, 'InstallLocation', Location) then
    RegQueryStringValue(HKLM32, Key, 'InstallLocation', Location);
  if Location <> '' then begin
    Uninstaller := AddBackslash(Location) + 'unins000.exe';
    if not FileExists(Uninstaller) then begin
      Result := 'A instalacao 2.0.0 foi encontrada, mas seu desinstalador esta ausente. Remova essa versao em Aplicativos do Windows antes de continuar.';
      exit;
    end;
    if not Exec(Uninstaller, '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
      Result := 'Nao foi possivel remover a versao 2.0.0. Feche o Sound Forge e tente novamente.'
    else if ResultCode <> 0 then
      Result := 'A remocao da versao anterior falhou. Feche os programas de audio e tente novamente.';
  end;
end;
#endif
