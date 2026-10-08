param(
    [ValidateSet('x86','x64','both')][string]$Architecture = 'both',
    [string]$SigningThumbprint = ''
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Invoke-Native([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable falhou com codigo $LASTEXITCODE" }
}
Push-Location $PSScriptRoot
try {
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
    Get-Command git -ErrorAction Stop | Out-Null
    $program32 = ${env:ProgramFiles(x86)}
    if (!$program32) { $program32 = $env:ProgramFiles }
    $iscc = Join-Path $program32 'Inno Setup 6\ISCC.exe'
    if (!(Test-Path $iscc)) { $iscc = Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe' }
    if (!(Test-Path $iscc)) { throw 'Instale Inno Setup 6.3 ou superior antes de compilar.' }
    $signTool = ''
    if ($SigningThumbprint) {
        $command = Get-Command signtool -ErrorAction SilentlyContinue
        if ($command) { $signTool = $command.Source }
        else {
            $tools = @(Get-ChildItem (Join-Path $program32 'Windows Kits\10\bin\*\x64\signtool.exe') -ErrorAction SilentlyContinue | Sort-Object FullName -Descending)
            if ($tools.Count -gt 0) { $signTool = $tools[0].FullName }
        }
        if (!$signTool) { throw 'SignTool nao encontrado no SDK do Windows.' }
    }
    $architectures = if ($Architecture -eq 'both') { @('x86','x64') } else { @($Architecture) }
    foreach ($arch in $architectures) {
        $platform = if ($arch -eq 'x86') { 'Win32' } else { 'x64' }
        $build = "build_$arch"
        Invoke-Native $cmake @('-S','.', '-B',$build,'-G','Visual Studio 17 2022','-A',$platform)
        Invoke-Native $cmake @('--build',$build,'--config','Release','--parallel','2')
        Invoke-Native $cmake @('--build',$build,'--config','Release','--target','legacy_smoke','dsp_checks','--parallel','2')
        Invoke-Native (Join-Path $PSScriptRoot "$build\Release\legacy_smoke.exe") @("$build\mettavoxad_artefacts\Release\VST\METTAVOXAD21.dll")
        Invoke-Native (Join-Path $PSScriptRoot "$build\Release\dsp_checks.exe") @("previews_$arch")
        if ($SigningThumbprint) {
            $binaries = @(Get-ChildItem "$build\mettavoxad_artefacts\Release" -Recurse -File | Where-Object { $_.Extension -in '.dll','.vst3' })
            foreach ($binary in $binaries) {
                Invoke-Native $signTool @('sign','/sha1',$SigningThumbprint,'/fd','SHA256','/tr','http://timestamp.digicert.com','/td','SHA256',$binary.FullName)
                Invoke-Native $signTool @('verify','/pa','/v',$binary.FullName)
            }
        }
        Invoke-Native $iscc @("/DPluginArch=$arch",'Installer/Windows/mettavoxad_setup.iss')
        $installer = Join-Path $PSScriptRoot "Installer\Windows\Output\mettavoxad_2.1.0_${arch}_Setup.exe"
        if ($SigningThumbprint) {
            Invoke-Native $signTool @('sign','/sha1',$SigningThumbprint,'/fd','SHA256','/tr','http://timestamp.digicert.com','/td','SHA256',$installer)
            Invoke-Native $signTool @('verify','/pa','/v',$installer)
        }
        Get-FileHash $installer -Algorithm SHA256 | Format-List
    }
    Write-Host 'Concluido: Installer\Windows\Output' -ForegroundColor Green
    if (!$SigningThumbprint) { Write-Host 'Build sem assinatura digital. Pode aparecer aviso de editor desconhecido.' }
} finally { Pop-Location }
