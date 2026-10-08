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

## Executado no GitHub Actions Windows em 08/10/2026

Execução: https://github.com/estudyoadr/METTAVOXAD-2.0/actions/runs/37713912663
Código validado: ced65acd1bfbf31785fa96aa60d302daae8fdf00.

As matrizes MSVC Win32 e x64 concluíram com sucesso: compilação da DLL VST legado e VST3, carregamento da DLL e ciclo de áudio/estado, testes de DSP e interface, criação dos instaladores EXE e teste de atualização de instalação.

Tests/InstallerUpgrade.ps1 instalou versões anteriores de teste, atualizou para 2.1.0 e verificou remoção de DLLs/bundles antigos, atualização do registro e preservação de um plugin vizinho. Os logs estão nos artifacts de cada arquitetura. O teste cobre as instalações e caminhos conhecidos; não faz uma busca indiscriminada por arquivos no computador.

O Sound Forge 8 ainda precisa de teste no host real, usando a DLL x86 mesmo em Windows de 64 bits. Não há certificado de assinatura neste pacote, nem garantia de ausência de alertas do Windows. O passo a passo explica compilação e assinatura opcional.

## Correção das exportações Windows

O linker MSVC usa Source/LegacyVst.def explicitamente para resolver a decoração dos símbolos e exportar main e VSTPluginMain em x86/x64. Os dois testes de carregamento da DLL passaram no Windows. O produto e a DLL usam o nome METTAVOXAD21 para diferenciar esta versão.
