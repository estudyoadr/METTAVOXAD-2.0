const styles = {
  "Varejo": {pace: 74, expression: 80, pauses: 28},
  "Institucional": {pace: 43, expression: 51, pauses: 66},
  "Documentário": {pace: 38, expression: 60, pauses: 72},
  "Rádio/TV": {pace: 67, expression: 73, pauses: 43},
  "Impacto": {pace: 56, expression: 91, pauses: 58}
};
let selectedStyle = "Varejo";
let selectedStyleId = null;
let selectedVoiceId = null;
let voiceProfiles = [];
let generationBusy = false;
let toastTimer;
let engineStatus = {engineInstalled: false, modelReady: false};
let currentAudioPath = null;
let activeAuditionButton = null;
let activeDraft = null;
let draftPreviewHeard = false;
let clonePreviewBusy = false;
const toast = document.querySelector("#toast");
function notify(message) {
  toast.textContent = message;
  toast.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove("show"), 2600);
}
function setRange(name, value, readoutId = `${name}Value`) {
  const input = document.querySelector(`#${name}`);
  const readout = document.querySelector(`#${readoutId}`);
  input.value = value;
  readout.textContent = `${value}%`;
  const pct = `${value}%`;
  input.style.background = `linear-gradient(90deg,#43c8ff 0%,#a178ff ${pct},#2a3248 ${pct})`;
}
function selectStyle(button, saved = null) {
  document.querySelectorAll(".style-option").forEach(item => item.classList.remove("selected"));
  button.classList.add("selected");
  selectedStyle = saved?.name || button.dataset.style;
  selectedStyleId = saved?.id || null;
  const preset = saved || styles[selectedStyle];
  if (preset) {
    setRange("pace", preset.pace);
    setRange("expression", preset.expression);
    setRange("pauses", preset.pauses);
  }
  updateGenerateButton();
}
document.querySelectorAll(".style-option[data-style]").forEach(button => button.addEventListener("click", () => selectStyle(button)));
[["pace", "paceValue"], ["expression", "expressionValue"], ["pauses", "pausesValue"]].forEach(([id, valueId]) => {
  const input = document.querySelector(`#${id}`);
  input.addEventListener("input", () => {
    document.querySelector(`#${valueId}`).textContent = `${input.value}%`;
    input.style.background = `linear-gradient(90deg,#43c8ff 0%,#a178ff ${input.value}%,#2a3248 ${input.value}%)`;
  });
  setRange(id, input.value, valueId);
});
const script = document.querySelector("#script");
script.addEventListener("input", () => {
  document.querySelector("#counter").textContent = `${script.value.length} caracteres`;
});
function addVoice() {
  if (!window.vozlab?.prepareVoice) return notify("O cadastro de voz não está disponível nesta compilação.");
  document.querySelector("#voiceName").value = "";
  document.querySelector("#voiceConsent").checked = false;
  const dialog = document.querySelector("#voiceDialog");
  if (!dialog.open) dialog.showModal();
  const nameField = document.querySelector("#voiceName");
  setTimeout(() => { nameField.focus(); nameField.select(); }, 0);
}
document.querySelectorAll("#addVoice,#addVoiceTile,#openProfiles").forEach(button => button.addEventListener("click", addVoice));
document.querySelector("#cancelVoice").addEventListener("click", () => document.querySelector("#voiceDialog").close());
async function beginDraftReview(prepared, name, profileId = null) {
  activeDraft = {draftId: prepared.draftId, name, profileId};
  draftPreviewHeard = false;
  document.querySelector("#draftTitle").textContent = profileId ? "Teste a gravação substituta" : "Teste o clone antes de salvar";
  document.querySelector("#draftDescription").textContent = "Perfil: " + name + ". Ouça a frase sintetizada. Só depois de ouvir você poderá salvar este perfil.";
  document.querySelector("#draftStatus").textContent = "Gerando uma frase de teste com esta gravação…";
  document.querySelector("#draftListen").disabled = true;
  document.querySelector("#draftStop").disabled = true;
  document.querySelector("#saveDraftVoice").disabled = true;
  document.querySelector("#draftAudio").removeAttribute("src");
  if (!document.querySelector("#draftDialog").open) document.querySelector("#draftDialog").showModal();
  await generateDraftPreview();
}
async function generateDraftPreview() {
  if (!activeDraft || generationBusy) return;
  const button = document.querySelector("#draftGenerate");
  generationBusy = true;
  clonePreviewBusy = true;
  draftPreviewHeard = false;
  document.querySelector("#saveDraftVoice").disabled = true;
  button.disabled = true;
  button.textContent = "Gerando prévia…";
  document.querySelector("#draftStatus").textContent = "Preparando motor e sintetizando a frase de teste. A primeira execução baixa os arquivos do modelo.";
  try {
    const result = await window.vozlab.previewDraft({
      draftId: activeDraft.draftId,
      pace: +document.querySelector("#pace").value,
      expression: +document.querySelector("#expression").value,
      pauses: +document.querySelector("#pauses").value
    });
    if (!result?.ok) throw new Error(result?.error || "Não foi possível gerar a prévia.");
    const player = document.querySelector("#draftAudio");
    player.src = result.url;
    player.load();
    document.querySelector("#draftListen").disabled = false;
    document.querySelector("#draftStatus").textContent = result.cached ? "Prévia pronta (em cache). Ouça para decidir se quer salvar." : "Prévia pronta. Ouça para decidir se quer salvar.";
    document.querySelector("#draftGenerate").textContent = "↻ Gerar outra prévia";
  } catch (error) {
    document.querySelector("#draftStatus").textContent = "Falha ao gerar a prévia: " + (error.message || "erro desconhecido");
    notify(error.message || "Falha ao gerar a prévia.");
  } finally {
    generationBusy = false;
    clonePreviewBusy = false;
    button.disabled = false;
    button.textContent = document.querySelector("#draftAudio").src ? "↻ Gerar outra prévia" : "✦ Gerar prévia";
    document.querySelector("#cancelDraft").disabled = false;
    document.querySelector("#changeDraftAudio").disabled = false;
    updateGenerateButton();
  }
}
async function discardActiveDraft() {
  if (activeDraft) await window.vozlab.discardVoiceDraft(activeDraft.draftId);
  activeDraft = null;
}
document.querySelector("#voiceForm").addEventListener("submit", async event => {
  event.preventDefault();
  const name = document.querySelector("#voiceName").value.trim();
  if (!name) return notify("Digite um nome para este perfil.");
  if (!document.querySelector("#voiceConsent").checked) return notify("Confirme que tem autorização para usar esta voz.");
  document.querySelector("#voiceDialog").close();
  try {
    const prepared = await window.vozlab.prepareVoice();
    if (prepared?.canceled) return;
    if (!prepared?.ok) throw new Error(prepared?.error || "Não foi possível abrir a gravação.");
    await beginDraftReview(prepared, name);
  } catch (error) {
    notify(error.message || "Não foi possível preparar o perfil.");
  }
});
document.querySelector("#draftGenerate").addEventListener("click", generateDraftPreview);
document.querySelector("#draftListen").addEventListener("click", async () => {
  const player = document.querySelector("#draftAudio");
  try { player.currentTime = 0; await player.play(); }
  catch (error) { notify("Não foi possível reproduzir a prévia."); }
});
document.querySelector("#draftAudio").addEventListener("play", () => {
  draftPreviewHeard = true;
  document.querySelector("#draftStop").disabled = false;
  document.querySelector("#saveDraftVoice").disabled = false;
});
document.querySelector("#draftAudio").addEventListener("ended", () => {
  document.querySelector("#draftStop").disabled = true;
  document.querySelector("#draftListen").textContent = "▶ Ouvir novamente";
});
document.querySelector("#draftStop").addEventListener("click", () => {
  const player = document.querySelector("#draftAudio");
  player.pause();
  try { player.currentTime = 0; } catch {}
  document.querySelector("#draftStop").disabled = true;
  document.querySelector("#draftListen").textContent = "▶ Ouvir prévia";
});
document.querySelector("#saveDraftVoice").addEventListener("click", async () => {
  if (!activeDraft || !draftPreviewHeard || generationBusy) return notify("Gere e ouça a prévia antes de salvar o perfil.");
  const request = {...activeDraft};
  const result = await window.vozlab.commitVoiceDraft({draftId: request.draftId, name: request.name, profileId: request.profileId, heardPreview: draftPreviewHeard});
  if (!result?.ok) return notify(result?.error || "Não foi possível salvar o perfil.");
  document.querySelector("#draftAudio").pause();
  document.querySelector("#draftDialog").close();
  activeDraft = null;
  selectedVoiceId = result.id;
  selectedStyleId = null;
  await renderVoices();
  await renderSavedCount();
  notify(result.replaced ? "Gravação do perfil “" + result.name + "” atualizada." : "Perfil “" + result.name + "” salvo.");
});
document.querySelector("#cancelDraft").addEventListener("click", async () => {
  if (generationBusy) return notify("Aguarde a geração da prévia terminar para cancelar.");
  await discardActiveDraft();
  document.querySelector("#draftAudio").pause();
  document.querySelector("#draftDialog").close();
});
document.querySelector("#changeDraftAudio").addEventListener("click", async () => {
  if (generationBusy || !activeDraft) return notify("Aguarde a geração da prévia terminar.");
  const previous = activeDraft;
  const prepared = await window.vozlab.prepareVoice();
  if (prepared?.canceled) return;
  if (!prepared?.ok) return notify(prepared?.error || "Não foi possível abrir outra gravação.");
  await window.vozlab.discardVoiceDraft(previous.draftId);
  await beginDraftReview(prepared, previous.name, previous.profileId);
});
document.querySelector("#draftDialog").addEventListener("cancel", async event => {
  event.preventDefault();
  if (generationBusy) return notify("Aguarde a geração da prévia terminar para cancelar.");
  await discardActiveDraft();
});
let editingVoiceId = null;
document.querySelector("#editVoice").addEventListener("click", () => {
  const profile = voiceProfiles.find(item => item.id === selectedVoiceId);
  if (!profile) return notify("Selecione um perfil para renomear.");
  editingVoiceId = profile.id;
  const input = document.querySelector("#editVoiceName");
  input.value = profile.name;
  document.querySelector("#editVoiceDialog").showModal();
  setTimeout(() => { input.focus(); input.select(); }, 0);
});
document.querySelector("#cancelEditVoice").addEventListener("click", () => document.querySelector("#editVoiceDialog").close());
document.querySelector("#editVoiceForm").addEventListener("submit", async event => {
  event.preventDefault();
  const name = document.querySelector("#editVoiceName").value.trim();
  if (!editingVoiceId || !name) return notify("Digite um nome para o perfil.");
  const result = await window.vozlab.renameVoice(editingVoiceId, name);
  if (!result?.ok) return notify(result?.error || "Não foi possível renomear o perfil.");
  document.querySelector("#editVoiceDialog").close();
  editingVoiceId = null;
  await renderVoices();
  notify("Perfil renomeado para “" + result.name + "”.");
});
document.querySelector("#replaceVoice").addEventListener("click", async () => {
  const profile = voiceProfiles.find(item => item.id === selectedVoiceId);
  if (!profile) return notify("Selecione um perfil para alterar a gravação.");
  try {
    const prepared = await window.vozlab.prepareVoice();
    if (prepared?.canceled) return;
    if (!prepared?.ok) throw new Error(prepared?.error || "Não foi possível escolher outra gravação.");
    await beginDraftReview(prepared, profile.name, profile.id);
  } catch (error) { notify(error.message || "Não foi possível alterar a gravação."); }
});
document.querySelector("#deleteVoice").addEventListener("click", async () => {
  const profile = voiceProfiles.find(item => item.id === selectedVoiceId);
  if (!profile) return notify("Selecione um perfil para excluir.");
  if (!window.confirm("Excluir o perfil “" + profile.name + "” e seus arquivos locais? Esta ação não pode ser desfeita.")) return;
  const result = await window.vozlab.deleteVoice(profile.id);
  if (!result?.ok) return notify(result?.error || "Não foi possível excluir o perfil.");
  stopReferenceAudio();
  selectedVoiceId = null;
  selectedStyleId = null;
  await renderVoices();
  await renderSavedCount();
  notify("Perfil “" + profile.name + "” excluído.");
});

async function renderVoices() {
  const profiles = await window.vozlab?.getProfiles?.();
  if (!profiles) return;
  voiceProfiles = profiles;
  const picker = document.querySelector("#voicePicker");
  picker.querySelectorAll(".saved-voice-row").forEach(item => item.remove());
  const placeholder = picker.querySelector(".placeholder-voice");
  placeholder.hidden = profiles.length > 0;
  if (profiles.length && !profiles.some(profile => profile.id === selectedVoiceId)) selectedVoiceId = profiles[0].id;
  document.querySelector("#editVoice").disabled = !profiles.some(profile => profile.id === selectedVoiceId);
  document.querySelector("#replaceVoice").disabled = !profiles.some(profile => profile.id === selectedVoiceId);
  document.querySelector("#deleteVoice").disabled = !profiles.some(profile => profile.id === selectedVoiceId);
  profiles.forEach(profile => {
    const row = document.createElement("div");
    row.className = "saved-voice-row";
    const button = document.createElement("button");
    button.className = `voice-option saved-voice-option${profile.id === selectedVoiceId ? " selected" : ""}`;
    button.type = "button";
    button.setAttribute("aria-label", `Selecionar perfil ${profile.name}`);
    const orb = document.createElement("span");
    orb.className = "voice-orb blue";
    const initials = document.createElement("span");
    initials.textContent = profile.name.slice(0, 2).toUpperCase();
    orb.append(initials);
    const copy = document.createElement("span");
    copy.className = "voice-copy";
    const title = document.createElement("strong"); title.textContent = profile.name;
    const detail = document.createElement("small"); detail.textContent = "Referência local";
    copy.append(title, detail);
    const check = document.createElement("span"); check.className = "check"; check.textContent = profile.id === selectedVoiceId ? "✓" : "";
    button.append(orb, copy, check);
    button.addEventListener("click", () => {
      stopReferenceAudio();
      const previewPlayer = document.querySelector("#previewAudio");
      previewPlayer.pause();
      try { previewPlayer.currentTime = 0; } catch {}
      selectedVoiceId = profile.id;
      selectedStyleId = null;
      picker.querySelectorAll(".voice-option").forEach(item => item.classList.remove("selected"));
      button.classList.add("selected");
      picker.querySelectorAll(".check").forEach(item => item.textContent = "");
      check.textContent = "✓";
      renderStyles();
      updateGenerateButton();
    });
    const audition = document.createElement("button");
    audition.type = "button";
    audition.className = "voice-audition";
    audition.textContent = "▶ Ouvir";
    audition.title = `Ouvir a gravação do perfil ${profile.name}`;
    audition.addEventListener("click", async () => {
      if (activeAuditionButton === audition) {
        stopReferenceAudio();
        return;
      }
      await playReferenceAudio(profile, audition);
    });
    row.append(button, audition);
    picker.insertBefore(row, picker.querySelector(".add-voice-option"));
  });
  await renderStyles();
  updateGenerateButton();
}
renderVoices();
document.querySelector("#manageVoice").addEventListener("click", async () => {
  const profiles = await window.vozlab?.getProfiles?.();
  notify(profiles?.length ? `${profiles.length} perfil(is) de voz salvo(s) neste computador.` : "Ainda não há gravações de referência cadastradas.");
});
document.querySelector("#addStyle").addEventListener("click", async () => {
  if (!window.vozlab?.addStyle) return notify("O cadastro de gravação de estilo será feito no app Windows.");
  if (!selectedVoiceId) return notify("Cadastre ou selecione uma voz antes de criar um estilo personalizado.");
  const dialog = document.querySelector("#styleDialog");
  document.querySelector("#styleName").value = "";
  document.querySelector("#styleConsent").checked = false;
  dialog.showModal();
  document.querySelector("#styleName").focus();
});
document.querySelector("#cancelStyle").addEventListener("click", () => document.querySelector("#styleDialog").close());
document.querySelector("#styleForm").addEventListener("submit", async event => {
  event.preventDefault();
  const name = document.querySelector("#styleName").value.trim();
  if (!name) return notify("Digite um nome para este estilo.");
  if (!document.querySelector("#styleConsent").checked) return notify("Confirme que a gravação pertence à mesma pessoa ou foi autorizada.");
  document.querySelector("#styleDialog").close();
  const result = await window.vozlab.addStyle({voiceId: selectedVoiceId, name, pace: +document.querySelector("#pace").value, expression: +document.querySelector("#expression").value, pauses: +document.querySelector("#pauses").value});
  if (result?.ok) { selectedStyleId = result.id; await renderStyles(); await renderSavedCount(); notify(`Estilo “${result.name}” salvo neste computador.`); }
  else if (result?.error) notify(result.error);
});
document.querySelector("#saveStyle").addEventListener("click", async () => {
  const profile = {id: selectedStyleId, voiceId: selectedVoiceId, name: selectedStyle, pace: +document.querySelector("#pace").value, expression: +document.querySelector("#expression").value, pauses: +document.querySelector("#pauses").value};
  const result = await window.vozlab?.saveStyle?.(profile);
  if (result?.ok) { selectedStyleId = result.id; await renderStyles(); await renderSavedCount(); }
  notify(result?.ok ? `Estilo “${result.name}” salvo.` : result?.error || "Não foi possível salvar este estilo.");
});
async function renderSavedCount() {
  const [voices, savedStyles] = await Promise.all([window.vozlab?.getProfiles?.(), window.vozlab?.getStyles?.()]);
  if (voices && savedStyles) document.querySelector("#savedCount").textContent = String(voices.length + savedStyles.length);
}
renderSavedCount();

async function renderStyles() {
  const grid = document.querySelector("#styleGrid");
  const addButton = document.querySelector("#addStyle");
  grid.querySelectorAll(".saved-style-option").forEach(item => item.remove());
  const allStyles = await window.vozlab?.getStyles?.();
  const custom = allStyles?.filter(item => item.voiceId === selectedVoiceId) || [];
  custom.forEach(style => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `style-option saved-style-option${style.id === selectedStyleId ? " selected" : ""}`;
    const icon = document.createElement("span"); icon.className = "style-icon violet"; icon.textContent = "♫";
    const copy = document.createElement("span");
    const title = document.createElement("strong"); title.textContent = style.name;
    const detail = document.createElement("small"); detail.textContent = style.reference ? "Amostra de interpretação" : "Ajustes salvos";
    copy.append(title, detail); button.append(icon, copy);
    button.addEventListener("click", () => selectStyle(button, style));
    grid.insertBefore(button, addButton);
  });
  if (selectedStyleId && !custom.some(style => style.id === selectedStyleId)) {
    selectedStyleId = null;
    const base = grid.querySelector(`.style-option[data-style="${CSS.escape(selectedStyle)}"]`) || grid.querySelector(".style-option[data-style]");
    if (base) selectStyle(base);
  }
  grid.querySelectorAll(".style-option[data-style]").forEach(button => button.classList.toggle("selected", !selectedStyleId && button.dataset.style === selectedStyle));
}
renderStyles();

function updateGenerateButton() {
  const button = document.querySelector("#generate");
  if (!button) return;
  const hasText = Boolean(document.querySelector("#script").value.trim());
  button.disabled = generationBusy || !engineStatus.engineInstalled || !selectedVoiceId || !hasText;
  const hasSelectedProfile = voiceProfiles.some(profile => profile.id === selectedVoiceId);
  const previewButton = document.querySelector("#previewVoice");
  if (previewButton) previewButton.disabled = generationBusy || !hasSelectedProfile;
  document.querySelector("#editVoice").disabled = !hasSelectedProfile || generationBusy;
  document.querySelector("#replaceVoice").disabled = !hasSelectedProfile || generationBusy;
  document.querySelector("#deleteVoice").disabled = !hasSelectedProfile || generationBusy;
  button.title = !engineStatus.engineInstalled ? "Motor local indisponível nesta compilação" : !selectedVoiceId ? "Cadastre uma referência de voz" : !hasText ? "Digite o texto da locução" : "Gerar com o perfil selecionado";
}

function setEngineMessage(title, detail, ready = false) {
  document.querySelector("#engineTitle").textContent = title;
  document.querySelector("#engineDetail").textContent = detail;
  document.querySelector("#statusRing").classList.toggle("engine-ready", ready);
}

async function checkEngine() {
  if (!window.vozlab?.getEngineStatus) return;
  setEngineMessage("Verificando motor local…", "A gravação e a geração permanecem neste computador.");
  const result = await window.vozlab.getEngineStatus();
  engineStatus = result || {engineInstalled: false, modelReady: false};
  if (!result?.engineInstalled) {
    setEngineMessage("Motor de voz indisponível", result?.error ? `Falha ao iniciar: ${result.error}` : "O motor local não respondeu. Verifique o diagnóstico da compilação.");
  } else if (result.modelReady) {
    setEngineMessage(`Motor pronto · ${result.device || "CPU"}`, "Use uma gravação sua ou autorizada como referência.", true);
  } else {
    setEngineMessage(`Pronto para configurar · ${result.device || "CPU"}`, "Primeiro uso baixa o checkpoint PT-BR de 2,14 GB e arquivos compartilhados.", true);
  }
  updateGenerateButton();
}

window.vozlab?.onEngineProgress?.(message => {
  const button = document.querySelector("#generate");
  if (clonePreviewBusy) {
    const status = document.querySelector("#draftDialog").open ? document.querySelector("#draftStatus") : document.querySelector("#engineDetail");
    status.textContent = message.message || "Preparando a prévia da voz…";
    if (document.querySelector("#draftDialog").open) {
      document.querySelector("#draftGenerate").disabled = true;
      document.querySelector("#cancelDraft").disabled = true;
      document.querySelector("#changeDraftAudio").disabled = true;
    }
  }
  if (message.phase === "assets") {
    if (generationBusy && !clonePreviewBusy) button.textContent = "Baixando modelo…";
    setEngineMessage(message.message || "Baixando componentes do modelo…", "Primeiro uso: download local dos arquivos PT-BR. Esta etapa pode levar alguns minutos.", true);
  } else if (message.phase === "loading") {
    if (generationBusy && !clonePreviewBusy) button.textContent = "Carregando voz…";
    setEngineMessage(message.message || "Carregando modelo na memória…", "O modelo está sendo preparado para gerar a locução.");
  } else if (message.phase === "generating") {
    if (generationBusy && !clonePreviewBusy) button.textContent = message.current && message.total ? `Gerando ${message.current}/${message.total}…` : "Gerando áudio…";
    setEngineMessage(message.message || "Gerando a locução…", message.total ? `Frase ${message.current || 0} de ${message.total}.` : "A geração pode levar alguns instantes.");
  } else if (message.phase === "ready") {
    if (generationBusy && !clonePreviewBusy) button.textContent = "Finalizando áudio…";
    setEngineMessage(message.message || "Motor de voz pronto.", "Preparando o resultado.");
  } else {
    setEngineMessage(message.message || "Preparando o motor…", "Aguarde; a primeira inicialização pode levar alguns minutos.");
  }
});

document.querySelector("#generate").addEventListener("click", async () => {
  const text = document.querySelector("#script").value.trim();
  if (!selectedVoiceId) return notify("Cadastre ou selecione uma voz de referência.");
  if (!text) return notify("Digite o texto que deseja transformar em locução.");
  const button = document.querySelector("#generate");
  generationBusy = true;
  button.disabled = true;
  button.textContent = "Preparando motor…";
  document.querySelector("#previewState").innerHTML = "<i></i> GERANDO";
  setEngineMessage("Gerando locução…", "Se for o primeiro uso, o app baixa o modelo PT-BR.");
  try {
    const result = await window.vozlab.generate({
      voiceId: selectedVoiceId, styleId: selectedStyleId, text,
      pace: +document.querySelector("#pace").value,
      expression: +document.querySelector("#expression").value,
      pauses: +document.querySelector("#pauses").value
    });
    if (!result?.ok) throw new Error(result?.error || "Não foi possível gerar o áudio.");
    currentAudioPath = result.path;
    const player = document.querySelector("#previewAudio");
    player.src = result.url;
    player.load();
    document.querySelector("#previewTitle").textContent = "Locução pronta";
    document.querySelector("#previewDetail").textContent = `WAV · ${result.device || "CPU"} · ${result.referenceStyle ? `amostra “${result.referenceStyle}”` : "perfil de voz"}`;
    document.querySelector("#previewState").innerHTML = "<i></i> PRONTA";
    document.querySelector("#playPreview").disabled = false;
    document.querySelector("#stopPreview").disabled = false;
    document.querySelector("#exportAudio").disabled = false;
    setEngineMessage(`Motor pronto · ${result.device || "CPU"}`, "O áudio foi salvo localmente.", true);
    engineStatus = {...engineStatus, engineInstalled: true, modelReady: true};
    notify("Locução gerada e salva neste computador.");
  } catch (error) {
    document.querySelector("#previewState").innerHTML = "<i></i> ERRO";
    setEngineMessage("Falha ao gerar", error.message || "Confira o motor e a gravação de referência.");
    notify(error.message || "Falha ao gerar a locução.");
  } finally {
    generationBusy = false;
    button.innerHTML = "<span>✦</span> Gerar locução";
    updateGenerateButton();
  }
});

function stopReferenceAudio() {
  const player = document.querySelector("#referenceAudio");
  player.pause();
  try { player.currentTime = 0; } catch {}
  if (activeAuditionButton) {
    activeAuditionButton.textContent = activeAuditionButton.dataset.idleLabel || "▶ Ouvir";
    activeAuditionButton.title = activeAuditionButton.dataset.idleTitle || "";
  }
  activeAuditionButton = null;
}
async function playReferenceAudio(profile, button) {
  stopReferenceAudio();
  try {
    const result = await window.vozlab?.previewVoice?.(profile.id);
    if (!result?.ok) throw new Error(result?.error || "A gravação de referência não foi encontrada.");
    const player = document.querySelector("#referenceAudio");
    player.src = result.url;
    player.load();
    await player.play();
    if (button) {
      button.dataset.idleLabel ||= button.textContent;
      button.dataset.idleTitle ||= button.title;
      button.textContent = "■ Parar";
      button.title = "Parar prévia de " + profile.name;
      activeAuditionButton = button;
    }
  } catch (error) {
    notify(error.message || "Não foi possível reproduzir a gravação de referência.");
  }
}
document.querySelector("#referenceAudio").addEventListener("ended", stopReferenceAudio);
document.querySelector("#previewVoice").addEventListener("click", async () => {
  const profile = voiceProfiles.find(item => item.id === selectedVoiceId);
  if (!profile) return notify("Selecione um perfil para testar a voz clonada.");
  if (generationBusy) return notify("Aguarde o processamento atual terminar.");
  stopReferenceAudio();
  generationBusy = true;
  clonePreviewBusy = true;
  updateGenerateButton();
  document.querySelector("#previewState").innerHTML = "<i></i> TESTE";
  document.querySelector("#previewTitle").textContent = "Teste da voz clonada";
  document.querySelector("#previewDetail").textContent = "Gerando uma frase curta com o perfil selecionado.";
  setEngineMessage("Preparando teste da voz…", "Na primeira execução, o motor baixa e carrega o modelo PT-BR.");
  try {
    const result = await window.vozlab.previewClone({
      voiceId: profile.id, styleId: selectedStyleId,
      pace: +document.querySelector("#pace").value,
      expression: +document.querySelector("#expression").value,
      pauses: +document.querySelector("#pauses").value
    });
    if (!result?.ok) throw new Error(result?.error || "Não foi possível gerar a prévia.");
    currentAudioPath = result.path;
    const player = document.querySelector("#previewAudio");
    player.src = result.url;
    player.load();
    document.querySelector("#previewDetail").textContent = "Frase sintetizada · " + (result.device || "CPU") + (result.referenceStyle ? " · amostra “" + result.referenceStyle + "”" : " · perfil de voz");
    document.querySelector("#previewState").innerHTML = "<i></i> PRÉVIA";
    document.querySelector("#playPreview").disabled = false;
    document.querySelector("#stopPreview").disabled = false;
    document.querySelector("#exportAudio").disabled = true;
    await player.play();
    setEngineMessage("Prévia clonada pronta", "Ouça o resultado antes de escrever ou gerar a locução.", true);
  } catch (error) {
    document.querySelector("#previewState").innerHTML = "<i></i> ERRO";
    setEngineMessage("Falha na prévia", error.message || "Confira o motor e a gravação de referência.");
    notify(error.message || "Falha ao sintetizar a prévia.");
  } finally {
    generationBusy = false;
    clonePreviewBusy = false;
    updateGenerateButton();
  }
});

document.querySelector("#stopPreview").addEventListener("click", () => {
  const player = document.querySelector("#previewAudio");
  player.pause();
  try { player.currentTime = 0; } catch {}
});
document.querySelector("#playPreview").addEventListener("click", async () => {
  const player = document.querySelector("#previewAudio");
  if (player.paused) {
    try { await player.play(); } catch { notify("Não foi possível reproduzir a prévia."); }
  } else player.pause();
});
document.querySelector("#previewAudio").addEventListener("play", () => { document.querySelector("#playPreview").textContent = "Ⅱ"; });
document.querySelector("#previewAudio").addEventListener("pause", () => { document.querySelector("#playPreview").textContent = "▶"; });
document.querySelector("#previewAudio").addEventListener("loadedmetadata", event => {
  const duration = Number.isFinite(event.target.duration) ? event.target.duration : 0;
  const minutes = Math.floor(duration / 60).toString().padStart(2, "0");
  const seconds = Math.floor(duration % 60).toString().padStart(2, "0");
  document.querySelector("#timecode").innerHTML = `00:00 <i></i> ${minutes}:${seconds}`;
});
document.querySelector("#exportAudio").addEventListener("click", async () => {
  if (!currentAudioPath) return;
  const result = await window.vozlab?.exportAudio?.(currentAudioPath);
  if (result?.ok) notify("WAV exportado.");
});
script.addEventListener("input", updateGenerateButton);
checkEngine();
