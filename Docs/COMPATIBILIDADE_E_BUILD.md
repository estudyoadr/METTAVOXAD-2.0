# Compatibilidade e build 2.1.0

| Uso | Arquitetura | Formato |
|---|---|---|
| Sound Forge 8, processo de 32 bits | x86 | DLL VST legada METTAVOXAD21.dll |
| Outros hosts de 32 bits | x86 | DLL legada ou VST3 se suportado pelo host |
| Hosts de 64 bits | x64 | DLL legada ou VST3 se suportado pelo host |
| Linux x64 | x64 | VST3 |

A arquitetura acompanha o host, não apenas o Windows. O adaptador VST legado é código original neste pacote e não depende de um SDK VST2 externo. O processamento/editor é compartilhado com VST3. Não há bridge x86/x64.

JUCE fixado em 8.0.4, C++17, MSVC com runtime estático. Os instaladores configuram Windows 7 SP1 como mínimo; a compilação local recomendada é feita em Windows 10/11 x64. Esse mínimo é uma configuração, não uma certificação de compatibilidade. Sound Forge 8 e o instalador Windows precisam de teste real no destino.

Veja PASSO_A_PASSO_WINDOWS.md para GitHub, PowerShell, atualização e assinatura. A pasta .github está incluída no ZIP e o arquivo COPIAR_WORKFLOW_GITHUB.txt permite criar o workflow pela interface do GitHub.
