# Validação METTAVOXAD 2.1.0

## Executado nesta preparação, em Linux x64

- Compilação Release completa do VST3, adaptador VST legado e executáveis de teste com JUCE 8.0.4.
- Adaptador legado: ABI, exportação VSTPluginMain, parâmetros, estado, áudio e ciclo de abertura/fechamento.
- Detector de pitch: 225 Hz em 44,1/48/96 kHz. Correção cromática: pico de saída 219,727 Hz para alvo 220 Hz. Escala menor testada na fronteira de oitava.
- Cada um dos oito controles de Autotune e oito de Robô alterou um sinal vocal sintético com harmônicos, formantes e altura variável.
- Três arquiteturas robóticas com saídas diferentes e alteração espectral; modo desligado/mistura zero transparentes e silêncio sem entrada após reset.
- Os 24 presets produziram áudio finito em SOLO e não modificaram os parâmetros de outras abas.
- As 16 combinações dos quatro módulos produziram áudio finito. SOLO isolou cada módulo. Estado salvo preservou interruptores e presets.
- Todas as abas OFF e bypass global transparentes. HQ processou áudio e informou sua latência de oversampling.
- 32 controles giratórios e quatro seletores de seis presets. Imagens das quatro abas renderizadas e inspecionadas. Limites verificados nos tamanhos 980×700 e 820×620.
- Movimento real do knob de transposição na interface alterou o parâmetro e o áudio final em +4 semitons; SOLO da interface isolou Autotune. Knob de mistura e SOLO do Robô alteraram o áudio final.
- YAML do workflow, matriz Win32/x64, caminhos dos arquivos e cópia visível do workflow conferidos.

Os registros completos ficam em DSP_RESULTADOS.txt e LEGACY_RESULTADOS.txt. Os testes usam sinais sintéticos: não substituem avaliação auditiva de uma locução real.

## Preparado para execução no GitHub Actions Windows

O workflow compila MSVC Win32 e x64, executa os testes de áudio/interface e da DLL, compila os instaladores Inno Setup e executa Tests/InstallerUpgrade.ps1. Esse teste só roda no GitHub Actions: instala versões anteriores de teste, atualiza para 2.1.0 e verifica remoção de DLLs/bundles antigos, atualização do registro e preservação de um plugin vizinho. Os logs são disponibilizados como artifacts.

Essa etapa Windows NÃO foi executada neste ambiente. O EXE e a remoção da instalação anterior precisam dessa validação em Windows; o Sound Forge 8 também precisa de teste no host real. Não há certificado de assinatura neste pacote, nem garantia de ausência de alertas do Windows. O passo a passo explica compilação e assinatura opcional.


## Correção após o print do GitHub

Link MSVC com exportações explícitas `main` e `VSTPluginMain` para x86/x64. O teste agora diferencia falha ao carregar a DLL e exportação ausente. O workflow usa `METTAVOXAD21.dll`, e seu nome é METTAVOXAD 2.1. Não repetir execuções antigas com código anterior. O teste nativo Linux foi recompilado e passou após essa correção; os novos flags MSVC ainda precisam ser executados no Windows.
