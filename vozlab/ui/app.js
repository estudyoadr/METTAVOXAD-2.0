const styles = {
  "Mercado": {pace: 74, expression: 80, pauses: 28},
  "Institucional": {pace: 43, expression: 51, pauses: 66},
  "Documentário": {pace: 38, expression: 60, pauses: 72},
  "Rádio/TV": {pace: 67, expression: 73, pauses: 43},
  "Impacto": {pace: 56, expression: 91, pauses: 58}
};
let selectedStyle = "Mercado";
let selectedStyleId = null;
let selectedVoiceId = null;
let voiceProfiles = [];
let generationBusy = false;
let toastTimer;
let engineStatus = {engineInstalled: false, modelReady: false};
let currentAudioPath = null;
let activeAuditionButton = null;
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
  if (!window.vozlab?.addVoice) return notify("O cadastro de voz será habilitado no app Windows.");
  document.querySelector("#voiceName").value = "";
  document.querySelector("#voiceConsent").checked = false;
  const dialog = document.querySelector("#voiceDialog");
  if (!dialog.open) dialog.showModal();
  const nameField = document.querySelector("#voiceName");
  setTimeout(() => { nameField.focus(); nameField.select(); }, 0);
}
document.querySelectorAll("#addVoice,#addVoiceTile,#openProfiles").forEach(button => button.addEventListener("click", addVoice));
document.querySelector("#cancelVoice").addEventListener("click", () => document.querySelector("#voiceDialog").close());
document.querySelector("#voiceForm").addEventListener("submit", async event => {
  event.preventDefault();
  const name = document.querySelector("#voiceName").value.trim();
  if (!name) return notify("Digite um nome para este perfil.");
  if (!document.querySelector("#voiceConsent").checked) return notify("Confirme que tem autorização para usar esta voz.");
  document.querySelector("#voiceDialog").close();
  try {
    const result = await window.vozlab.addVoice(name);
    if (result?.ok) {
      selectedVoiceId = result.id;
      await renderVoices();
      notify(`Perfil “${result.name}” salvo neste computador.`);
    } else if (result?.error) notify(result.error);
  } catch (error) {
    notify(error.message || "Não foi possível cadastrar a voz.");
  }
});
async function renderVoices() {
  const profiles = await window.vozlab?.getProfiles?.();
  if (!profiles) return;
  voiceProfiles = profiles;
  const picker = document.querySelector("#voicePicker");
  picker.querySelectorAll(".saved-voice-option").forEach(item => item.remove());
  const placeholder = picker.querySelector(".placeholder-voice");
  placeholder.hidden = profiles.length > 0;
  if (profiles.length && !profiles.some(profile => profile.id === selectedVoiceId)) selectedVoiceId = profiles[0].id;
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
  const previewButton = document.querySelector("#previewVoice");
  if (previewButton) previewButton.disabled = !voiceProfiles.some(profile => profile.id === selectedVoiceId);
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
  if (message.phase === "assets") {
    if (generationBusy) button.textContent = "Baixando modelo…";
    setEngineMessage(message.message || "Baixando componentes do modelo…", "Primeiro uso: download local dos arquivos PT-BR. Esta etapa pode levar alguns minutos.", true);
  } else if (message.phase === "loading") {
    if (generationBusy) button.textContent = "Carregando voz…";
    setEngineMessage(message.message || "Carregando modelo na memória…", "O modelo está sendo preparado para gerar a locução.");
  } else if (message.phase === "generating") {
    if (generationBusy) button.textContent = message.current && message.total ? `Gerando ${message.current}/${message.total}…` : "Gerando áudio…";
    setEngineMessage(message.message || "Gerando a locução…", message.total ? `Frase ${message.current || 0} de ${message.total}.` : "A geração pode levar alguns instantes.");
  } else if (message.phase === "ready") {
    if (generationBusy) button.textContent = "Finalizando áudio…";
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
document.querySelector("#previewVoice").addEventListener("click", async event => {
  const button = event.currentTarget;
  if (activeAuditionButton === button) {
    stopReferenceAudio();
    return;
  }
  const profile = voiceProfiles.find(item => item.id === selectedVoiceId);
  if (!profile) return notify("Selecione uma voz salva para ouvir a gravação de referência.");
  await playReferenceAudio(profile, button);
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
