# METTAVOXAD 2.1 — uso

Cada aba possui oito controles giratórios e seis presets próprios. A cor e o brilho dos controles acompanham a intensidade; controles bipolares usam a distância do valor neutro. Um módulo desligado fica visualmente atenuado.

Os quatro interruptores ON/OFF ficam no topo em todas as abas. É possível ligar qualquer combinação. SOLO liga apenas o módulo correspondente e desliga os outros três. BYPASS GLOBAL devolve o áudio original, independentemente dos interruptores. HQ aplica oversampling somente ao processamento Vocal.

Selecionar um preset modifica somente os parâmetros da aba correspondente e liga aquela aba. As outras abas mantêm seus parâmetros e interruptores. Ajustes manuais personalizam os valores; o nome selecionado indica o preset de partida. Os valores são preservados pelo estado do plugin.

## Vocal

Peso, calor, clareza, presença, compressão, profundidade, harmonia e saída. Presets: Radio Clara, TV Presente, Podcast Natural, Cinema Intimo, Trailer Profundo e Voz Quente.

## Autotune

Intensidade define a força da correção. Velocidade é o tempo de resposta em milissegundos: valores menores corrigem mais rapidamente. Transposição muda a altura em até uma oitava para cada lado. Humanização reduz a correção de pequenas variações. Mistura combina voz original e processada. Saída ajusta o nível. Tonalidade e escala são seletores giratórios discretos.

Presets: Locucao Natural, Radio Polido, Pop Afinado, Hard Tune Digital, Voz Grave -4 e Voz Aerea +4. Para canto, ajuste a tonalidade e a escala à música. A escala Cromática considera todos os semitons; nela, alterar a tonalidade não altera as notas permitidas.

Para uma mudança imediatamente perceptível em locução, escolha Voz Grave -4 ou Voz Aerea +4 e pressione SOLO. Para avaliar afinação, use uma voz sustentada levemente desafinada: uma voz já afinada pode mudar pouco. Intensidade zero, sem transposição, e mistura zero são ajustes deliberadamente sem correção. Este é um afinador próprio, sem relação com o produto comercial Auto-Tune.

## Voz Robô

Três arquiteturas: Vocoder Neon, Hematron Cinema e Prisma Digital. O motor usa 24 bandas, portadora interna com osciladores limitados em banda, remapeamento de formantes, preservação de consoantes e espaço estéreo. Hematron adiciona uma camada vocal grave; Prisma usa intervalos de quinta e oitava na portadora.

Controles: mistura, timbre, textura, formantes, nota da portadora, largura, espaço e saída. Mistura 100% dá a transformação completa; timbre abre o brilho; textura aumenta a riqueza harmônica; formantes alteram o caráter da voz; portadora muda a base musical; largura abre o estéreo; espaço acrescenta reverberação.

Presets: Neon Claro, Hematron Cinema, Android Elegante, Prisma Pop, Trailer Futurista e Digital Suave. Comece com Neon Claro ou Hematron Cinema em SOLO e fale com boa articulação. Compare mistura 0% e 100%. O efeito acompanha o envelope da voz; não deve gerar som continuamente sem sinal de entrada. Largura exige reprodução estéreo para ser avaliada completamente.

## Master

Grave, médio, agudo, multibanda, exciter, largura, loudness e teto. Presets: Radio Equilibrado, Podcast Limpo, Cinema Amplo, Impacto Controlado, Voz Suave e Musica Brilhante. Largura não cria estéreo a partir de uma fonte perfeitamente mono.

## Cadeia

Autotune → Vocal → Voz Robô → Master → proteção final. Os módulos desligados são ignorados. Todas as abas OFF e BYPASS GLOBAL são transparentes. A proteção final impede amostras fora da faixa quando existe processamento ativo; ela não é uma normalização automática.

## Atualização

Feche o host antes de executar o EXE. O instalador substitui 2.0.1 da mesma arquitetura e limpa os nomes antigos conhecidos. O instalador x64 também executa o desinstalador registrado da antiga 2.0.0. Cópias manuais em pastas diferentes precisam ser removidas dessa pasta. A versão nova usa nome e identificador próprios; projetos antigos podem precisar que a instância seja reinserida. Refaça a busca de plugins no host após atualizar.


Limpeza adicional: o instalador remove as DLLs antigas conhecidas do BAT 1.1 nas pastas padrão Steinberg/VSTPlugins, Sony, Sonic Foundry, Common Files/VST2, no caminho VST registrado por arquitetura e na pasta mettavoxad_VST_Plugins da Área de Trabalho. Só os nomes conhecidos do METTAVOXAD são removidos. Cópias em outros locais continuam exigindo remoção manual.
