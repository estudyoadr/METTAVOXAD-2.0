# Envio simplificado para o GitHub

Envie o próprio METTAVOXAD_2.1.0_ENVIO_SIMPLES.zip para a raiz do repositório e finalize em Commit changes. Depois substitua o workflow pelo conteúdo de COPIAR_WORKFLOW_GITHUB.txt deste pacote e salve. O workflow extrai o ZIP automaticamente. Essa opção dispensa enviar as pastas separadamente. Veja COMECE_AQUI_ENVIO_SIMPLES.txt.

As instruções de envio de pastas abaixo são uma alternativa para quem deseja manter os arquivos extraídos no repositório.

# Compilar METTAVOXAD 2.1.0 no Windows

O ZIP contém código-fonte. A compilação produz DLL VST legada, VST3 e dois instaladores EXE. Sound Forge 8 usa a DLL x86 mesmo em Windows de 64 bits. Hosts de 64 bits usam x64. Não há versão XP/Vista; a compatibilidade real com cada host e versão de Windows deve ser verificada no computador de destino.

## GitHub — sem arrastar a pasta .github

1. Extraia o ZIP. Abra seu repositório no GitHub.
2. Envie CMakeLists.txt, COMPILAR_WINDOWS.ps1, Source/, Tests/, Installer/ e Docs/ para a raiz. Não deixe esses arquivos dentro de uma pasta adicional.
3. Para atualizar um repositório antigo, substitua os arquivos de mesmo nome; não misture código antigo e novo.
4. Clique em Add file → Create new file. No campo do nome, digite exatamente `.github/workflows/build_vst3_windows_linux.yml`.
5. Abra o arquivo COPIAR_WORKFLOW_GITHUB.txt deste pacote, copie TODO o conteúdo e cole no editor do GitHub. Clique em Commit changes. Esse procedimento cria a pasta oculta sem precisar arrastá-la.
6. Se já existir esse workflow, edite e substitua seu conteúdo. Remova workflows de compilação duplicados que apontem para arquivos antigos.
7. Abra Actions → workflow METTAVOXAD → Run workflow. A execução também ocorre ao enviar alterações para main ou master.
8. Espere os jobs Windows x86 e x64 ficarem verdes. Abra a execução e baixe os artifacts METTAVOXAD21-Windows-x86 e METTAVOXAD21-Windows-x64.
9. Extraia o artifact e execute `mettavoxad_2.1.0_x86_Setup.exe` ou `mettavoxad_2.1.0_x64_Setup.exe`. O caminho no artifact pode conter Installer/Windows/Output.
10. Se falhar, abra o PRIMEIRO passo vermelho e consulte o erro completo. O workflow verifica áudio, estado, presets, interface e a exportação da DLL antes de criar o EXE.

Não envie build/, build_x86/, build_x64/, JUCE/ ou binários antigos ao repositório. O CMake baixa JUCE 8.0.4. Se sua distribuição do plugin for pública/comercial, cumpra a licença aplicável do JUCE e das dependências.

## PowerShell — compilação local

No Windows 10/11 de 64 bits, instale Git, CMake 3.22 ou superior, Visual Studio 2022 com Desenvolvimento para desktop com C++ (MSVC x86/x64 e Windows SDK) e Inno Setup 6.3 ou superior. O processo precisa de acesso ao GitHub para baixar JUCE.

Abra PowerShell na pasta extraída que contém CMakeLists.txt e execute:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\COMPILAR_WINDOWS.ps1 -Architecture both
```

A política Bypass vale apenas para esse processo; não altera permanentemente o computador. Para compilar só uma arquitetura, use `-Architecture x86` ou `-Architecture x64`.

Os EXEs ficam em `Installer\Windows\Output`. Os plugins ficam em `build_x86\mettavoxad_artefacts\Release` e `build_x64\mettavoxad_artefacts\Release`. A DLL chama-se `METTAVOXAD21.dll`; o bundle chama-se `METTAVOXAD21.vst3`.

## Instalar e atualizar

Feche Sound Forge, REAPER e os demais hosts antes de instalar. O instalador mantém o identificador de instalação da 2.0.1 para atualizar a mesma arquitetura e manter uma entrada de desinstalação. Remove as DLLs antigas conhecidas do destino e os bundles antigos da pasta VST3 correspondente. O x64 também remove a 2.0.0 com o desinstalador registrado; se ele estiver ausente ou falhar, a instalação para e explica o motivo.

A instalação x86 não remove a x64 e vice-versa. Isso permite usar Sound Forge 8 e um host moderno de 64 bits no mesmo Windows. Cópias manuais em outras pastas não são procuradas nem apagadas automaticamente. Remova somente as cópias antigas do METTAVOXAD e mantenha os outros plugins.

No Sound Forge 8, inclua a pasta VST2 exibida pelo instalador nas preferências de VST do host. O destino usual é `C:\Program Files (x86)\Common Files\VST2\mettavoxad` no Windows x64; no Windows x86, `C:\Program Files\Common Files\VST2\mettavoxad`. Refaça a busca e escolha METTAVOXAD 2.1. A nova identidade do plugin pode exigir reinserir a instância em projetos antigos; guarde seus valores antes de atualizar.

Para conferir os módulos, escolha um preset de Autotune ou Robô e use SOLO. Desative BYPASS GLOBAL. Para Robô, compare mistura 0% e 100%. Para uma mudança evidente na altura da locução, use Voz Grave -4 ou Voz Aerea +4.

## Assinatura digital

O script aceita `-SigningThumbprint` com um certificado de assinatura de código já instalado e acessível. Exemplo:

```powershell
.\COMPILAR_WINDOWS.ps1 -Architecture both -SigningThumbprint 'SEU_THUMBPRINT'
```

O script assina e verifica as DLLs, os módulos VST3 e o EXE usando SignTool e timestamp SHA-256. Sem certificado, o build é sem assinatura. Assinatura não garante ausência de alertas do SmartScreen ou do antivírus; não desative proteções do Windows. O pacote não contém certificado ou chave privada.


Limpeza adicional: o instalador remove as DLLs antigas conhecidas do BAT 1.1 nas pastas padrão Steinberg/VSTPlugins, Sony, Sonic Foundry, Common Files/VST2, no caminho VST registrado por arquitetura e na pasta mettavoxad_VST_Plugins da Área de Trabalho. Só os nomes conhecidos do METTAVOXAD são removidos. Cópias em outros locais continuam exigindo remoção manual.
