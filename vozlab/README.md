# Mettavox Clonner

App Windows x64 para clonagem local de voz em português brasileiro, geração de locuções e organização de perfis de voz e interpretação.

## Como funciona

- O perfil de voz guarda uma gravação de referência autorizada no computador.
- Estilos salvos ficam vinculados a um perfil. Uma amostra de estilo serve como áudio de referência para reproduzir a interpretação; use uma gravação da mesma pessoa do perfil. Isso é importante porque o motor usa o mesmo áudio para timbre e interpretação.
- O texto é sintetizado pelo pacote PT-BR do Chatterbox. Ritmo, expressividade e pausa entre frases controlam parâmetros reais da geração.
- O WAV gerado fica na pasta local do Mettavox Clonner e pode ser ouvido ou exportado pelo app.
- O modelo é baixado na primeira geração. O checkpoint específico de PT-BR tem aproximadamente 2,14 GB, além dos componentes compartilhados.
- Sem conexão à internet na primeira configuração, a geração não funciona. Depois de baixar o modelo, a inferência é local.
- O motor preserva a marca d'água perceptual incluída pelo Chatterbox.

## Requisito para as gravações

Use uma gravação sua ou de outra pessoa com autorização explícita. Para formar o perfil principal, prefira 15 a 30 segundos de fala limpa, sem música, reverberação ou outras vozes. O motor prepara localmente no máximo os primeiros 20 segundos. Gravações de estilo devem ser da mesma pessoa daquele perfil para evitar troca de identidade.

As quatro gravações enviadas para o projeto são referências de estilo: animada/meio impacto, padrão médio, médio-grave e clássica. Elas não são copiadas para o código nem para o instalador. No app, use **Novo estilo** para associar cada arquivo ao perfil correto. As gravações, modelos e locuções ficam em `userData/VozLab`.

## Perfis e prévia antes de salvar

Cada perfil representa uma identidade vocal, por exemplo **Clone Mariano** ou **Clone Edna**. Depois de selecionar a identidade, os estilos **Varejo, Impacto, Documentário, Institucional** e **Rádio/TV** definem como ela interpreta o texto; estilos personalizados ficam vinculados ao clone selecionado. Ao criar ou substituir um perfil, o app gera uma frase curta com a gravação escolhida. Você precisa ouvir a prévia antes de habilitar **Salvar perfil**. O perfil atual continua intacto até a nova gravação ser aprovada; depois pode ser renomeado, ter a gravação trocada ou ser excluído. O botão de parar interrompe a reprodução.

A prévia usa o mesmo motor PT-BR da locução final. Ela não é uma promessa de fidelidade: serve para você rejeitar uma gravação de referência que não tenha resultado aceitável. A primeira síntese pode levar vários minutos em CPU, porque baixa/carrega o modelo e executa o modelo local. Prévia repetida com os mesmos ajustes fica em cache durante a sessão.

## Fidelidade e limites

O pacote oficial declara clonagem por áudio de referência e um fine-tune dedicado ao português brasileiro. Isso não garante que a saída vá soar igual ao Mariano. Antes de chamar o produto de pronto, é obrigatório ouvir frases inéditas do usuário e avaliar timbre, sotaque, dicção e variação por estilo. O app não substitui automaticamente a referência por uma voz da biblioteca: sem motor ou referência válidos, informa o erro. O teste automático gera WAV com uma referência sintética e valida o motor; não mede similaridade com a voz do usuário. A validação de timbre, sotaque, dicção e naturalidade precisa ser feita ouvindo a voz real no Windows do usuário.

Este repositório ainda é uma compilação de desenvolvimento. A arquitetura do worker e o fluxo da interface foram implementados, mas o worker empacotado e a fidelidade ainda precisam ser validados em Windows x64. Não publique o instalador como versão final até concluir esses testes.

## Desenvolvimento

1. `cd vozlab` e `npm install`.
2. Instale Python 3.11 x64 e as dependências de `engine/requirements.txt`.
3. Instale uma versão compatível do PyTorch e PyTorch Audio para o sistema de teste.
4. Para executar em desenvolvimento: `npm start` (o app inicia `engine/worker.py` usando `python` ou `python3`; personalize com `VOZLAB_PYTHON`).
5. O workflow Windows cria o worker PyInstaller, empacota o app com NSIS e publica um artefato de compilação. A publicação de release deve ocorrer somente depois da validação do áudio.

## Motor

- Chatterbox, pacote de PT-BR: <https://huggingface.co/ResembleAI/Chatterbox-Multilingual-pt-br>
- Código e pesos compartilhados: <https://github.com/resemble-ai/chatterbox>
- Licença publicada pelo projeto do modelo: MIT.
- Identificador do idioma usado pelo worker: `pt`.
