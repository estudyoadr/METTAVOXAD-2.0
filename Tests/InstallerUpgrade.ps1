param(
    [Parameter(Mandatory=$true)][ValidateSet('x86','x64')][string]$Architecture,
    [Parameter(Mandatory=$true)][string]$Iscc
)
# Ephemeral CI integration test. Never install fixtures on a user's computer.
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Este teste de instalacao so pode rodar no GitHub Actions.' }
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path $env:RUNNER_TEMP "mettavoxad_upgrade_$Architecture"
New-Item -ItemType Directory -Path $work -Force | Out-Null
$dll = Join-Path $root "build_$Architecture\mettavoxad_artefacts\Release\VST\METTAVOXAD21.dll"
$common = if ($Architecture -eq 'x86') { [Environment]::GetFolderPath('CommonProgramFilesX86') } else { [Environment]::GetFolderPath('CommonProgramFiles') }
$app = Join-Path $common "VST2\mettavoxad_ci_upgrade_$Architecture"
$archSetup = if ($Architecture -eq 'x64') { "ArchitecturesAllowed=x64compatible`nArchitecturesInstallIn64BitMode=x64compatible" } else { '' }
function Install-Silent([string]$Exe, [string]$LogName) {
    $log = Join-Path $work $LogName
    $process = Start-Process -FilePath $Exe -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$log`"") -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Instalador falhou: $Exe, codigo $($process.ExitCode). Log: $log" }
}
function Compile-Fixture([string]$Name, [string]$Id, [string]$Version, [string]$Destination, [string]$Files) {
    $source = @"
[Setup]
AppId=$Id
AppName=METTAVOXAD Upgrade Fixture $Version
AppVersion=$Version
DefaultDirName=$Destination
DisableDirPage=yes
PrivilegesRequired=admin
$archSetup
OutputDir=$work
OutputBaseFilename=$Name
[Files]
$Files
"@
    $script = Join-Path $work "$Name.iss"
    Set-Content -Path $script -Value $source -Encoding UTF8
    & $Iscc $script
    if ($LASTEXITCODE -ne 0) { throw "Compilacao da fixture falhou: $Name" }
    Install-Silent (Join-Path $work "$Name.exe") "$Name.log"
}
$oldBundle = Join-Path $common 'VST3\mettavoxad.vst3'
$oldFiles = @"
Source: "$dll"; DestDir: "{app}"; DestName: "mettavoxad 2.0.dll"
Source: "$dll"; DestDir: "$oldBundle"; DestName: "previous.vst3"
"@
Compile-Fixture 'previous_201' "mettavoxad-2-$Architecture" '2.0.1' $app $oldFiles
if ($Architecture -eq 'x64') {
    $old200 = Join-Path $common 'VST3\mettavoxad 2.0.vst3'
    Compile-Fixture 'previous_200' '{{7E4C9A31-2F8B-4D60-9C1A-58B3E0A7D2F4}' '2.0.0' $old200 "Source: `"$dll`"; DestDir: `"{app}`"; DestName: `"previous.vst3`""
}
# Also reproduce BAT 1.1 copies and an unrelated neighboring plugin.
$batFolder = Join-Path $common 'VST2'
$batFiles = @('mettavoxad.dll','mettavoxad_Mono.dll','mettavoxad_Sliders.dll')
foreach ($name in $batFiles) { Copy-Item $dll (Join-Path $batFolder $name) -Force }
$neighbor = Join-Path $batFolder 'OUTRO_PLUGIN_NAO_APAGAR.dll'
Copy-Item $dll $neighbor -Force
$installer = Join-Path $root "Installer\Windows\Output\mettavoxad_2.1.0_${Architecture}_Setup.exe"
Install-Silent $installer 'upgrade_210.log'
if (!(Test-Path (Join-Path $app 'METTAVOXAD21.dll'))) { throw 'A atualizacao nao reutilizou o diretorio da 2.0.1.' }
if (Test-Path (Join-Path $app 'mettavoxad 2.0.dll')) { throw 'DLL 2.0.1 antiga nao foi removida.' }
if (Test-Path $oldBundle) { throw 'Bundle VST3 antigo nao foi removido.' }
foreach ($name in $batFiles) { if (Test-Path (Join-Path $batFolder $name)) { throw "Copia do BAT permaneceu: $name" } }
if (!(Test-Path $neighbor)) { throw 'A atualizacao apagou um plugin diferente.' }
if (!(Test-Path (Join-Path $common 'VST3\METTAVOXAD21.vst3'))) { throw 'VST3 novo nao foi instalado.' }
$view = if ($Architecture -eq 'x64') { [Microsoft.Win32.RegistryView]::Registry64 } else { [Microsoft.Win32.RegistryView]::Registry32 }
$registry = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,$view)
try {
    $key = $registry.OpenSubKey("Software\Microsoft\Windows\CurrentVersion\Uninstall\mettavoxad-2-${Architecture}_is1")
    if (!$key) { throw 'Registro de atualizacao ausente.' }
    try { if ($key.GetValue('DisplayVersion') -ne '2.1.0') { throw 'Registro da versao anterior nao foi atualizado.' } } finally { $key.Dispose() }
    if ($Architecture -eq 'x64') {
        $oldKey = $registry.OpenSubKey('Software\Microsoft\Windows\CurrentVersion\Uninstall\{7E4C9A31-2F8B-4D60-9C1A-58B3E0A7D2F4}_is1')
        if ($oldKey) { $oldKey.Dispose();throw 'Registro 2.0.0 permaneceu apos desinstalar.' }
    }
} finally { $registry.Dispose() }
Write-Host "PASS: upgrade $Architecture, arquivos antigos removidos, nova DLL/VST3 instalados, plugin vizinho preservado."
