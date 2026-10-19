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
let toastTimer;
let engineStatus = {engineInstalled: false, modelReady: false};
let currentAudioPath = null;
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
async function addVoice() {
  if (!window.vozlab?.addVoice) return notify("O cadastro de voz será habilitado no app Windows.");
  const result = await window.vozlab.addVoice();
  if (result?.ok) {
    selectedVoiceId = result.id;
    await renderVoices();
    notify(`Perfil “${result.name}” salvo neste computador.`);
  }
}
document.querySelectorAll("#addVoice,#addVoiceTile,#openProfiles").forEach(button => button.addEventListener("click", addVoice));
async function renderVoices() {
  const profiles = await window.vozlab?.getProfiles?.();
  if (!profiles) return;
  const picker = document.querySelector("#voicePicker");
  picker.querySelectorAll(".saved-voice-option").forEach(item => item.remove());
  const placeholder = picker.querySelector(".placeholder-voice");
  placeholder.hidden = profiles.length > 0;
  if (profiles.length && !profiles.some(profile => profile.id === selectedVoiceId)) selectedVoiceId = profiles[0].id;
  profiles.forEach(profile => {
    const button = document.createElement("button");
    button.className = `voice-option saved-voice-option${profile.id === selectedVoiceId ? " selected" : ""}`;
    button.type = "button";
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
      selectedVoiceId = profile.id;
      selectedStyleId = null;
      picker.querySelectorAll(".voice-option").forEach(item => item.classList.remove("selected"));
      button.classList.add("selected");
      picker.querySelectorAll(".check").forEach(item => item.textContent = "");
      check.textContent = "✓";
      renderStyles();
      updateGenerateButton();
    });
    picker.insertBefore(button, picker.querySelector(".add-voice-option"));
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
  if (!window.confirm("Esta gravação será usada como referência de voz e interpretação. Ela precisa ser da mesma pessoa deste perfil. Continuar?")) return;
  const name = window.prompt("Nome deste estilo de locução:", "Meu estilo");
  if (!name?.trim()) return;
  const result = await window.vozlab.addStyle({voiceId: selectedVoiceId, name: name.trim(), pace: +document.querySelector("#pace").value, expression: +document.querySelector("#expression").value, pauses: +document.querySelector("#pauses").value});
  if (result?.ok) { selectedStyleId = result.id; await renderStyles(); await renderSavedCount(); notify(`Estilo “${result.name}” salvo neste computador.`); }
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
  button.disabled = !engineStatus.engineInstalled || !selectedVoiceId || !hasText;
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
    setEngineMessage("Motor de voz indisponível", "Esta compilação não contém os componentes de clonagem.");
  } else if (result.modelReady) {
    setEngineMessage(`Motor pronto · ${result.device || "CPU"}`, "Use uma gravação sua ou autorizada como referência.", true);
  } else {
    setEngineMessage(`Pronto para configurar · ${result.device || "CPU"}`, "Primeiro uso baixa o checkpoint PT-BR de 2,14 GB e arquivos compartilhados.", true);
  }
  updateGenerateButton();
}

window.vozlab?.onEngineProgress?.(message => {
  setEngineMessage(message.message || "Preparando o motor…", "Aguarde; a primeira inicialização pode levar alguns minutos.");
});

document.querySelector("#generate").addEventListener("click", async () => {
  const text = document.querySelector("#script").value.trim();
  if (!selectedVoiceId) return notify("Cadastre ou selecione uma voz de referência.");
  if (!text) return notify("Digite o texto que deseja transformar em locução.");
  const button = document.querySelector("#generate");
  button.disabled = true;
  button.textContent = "Preparando…";
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
    button.innerHTML = "<span>✦</span> Gerar locução";
    updateGenerateButton();
  }
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
