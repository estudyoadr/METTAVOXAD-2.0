const { app, BrowserWindow, dialog, ipcMain } = require('electron');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { spawn } = require('node:child_process');
const { pathToFileURL } = require('node:url');

const dataDir = () => path.join(app.getPath('userData'), 'VozLab');
const dataFile = name => path.join(dataDir(), name);
let mainWindow;
let worker;
let stdoutBuffer = '';
let workerReady = false;
let workerFailure = null;
const previewCache = new Map();
const pending = new Map();
function readJson(name, fallback) {
  try { return JSON.parse(fs.readFileSync(dataFile(name), 'utf8')); }
  catch { return fallback; }
}
function writeJson(name, data) {
  fs.mkdirSync(dataDir(), { recursive: true });
  fs.writeFileSync(dataFile(name), JSON.stringify(data, null, 2), 'utf8');
}
function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1420, height: 940, minWidth: 1050, minHeight: 740,
    backgroundColor: '#080b16', title: 'Mettavox Clonner — Estúdio de voz',
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    }
  });
  mainWindow.loadFile(path.join(__dirname, '..', 'ui', 'index.html'));
}

function startWorker() {
  if (worker) return;
  workerFailure = null;
  const environment = { ...process.env, VOZLAB_DATA_DIR: dataDir() };
  if (app.isPackaged) {
    const binary = path.join(process.resourcesPath, 'engine', 'VozLabEngine.exe');
    worker = spawn(binary, [], { stdio: ['pipe', 'pipe', 'pipe'], env: environment, windowsHide: true });
  } else {
    const python = process.env.VOZLAB_PYTHON || (process.platform === 'win32' ? 'python' : 'python3');
    worker = spawn(python, ['-u', path.join(__dirname, '..', 'engine', 'worker.py')], {
      stdio: ['pipe', 'pipe', 'pipe'], env: environment, windowsHide: true
    });
  }
  worker.stdout.setEncoding('utf8');
  worker.stdout.on('data', chunk => {
    stdoutBuffer += chunk;
    const lines = stdoutBuffer.split(/\r?\n/);
    stdoutBuffer = lines.pop();
    for (const line of lines) {
      if (!line.trim()) continue;
      let message;
      try { message = JSON.parse(line); } catch { continue; }
      if (message.event === 'progress') {
        mainWindow?.webContents.send('vozlab:engine-progress', message);
        continue;
      }
      const waiter = pending.get(message.id);
      if (waiter) {
        pending.delete(message.id);
        waiter(message);
      }
    }
  });
  worker.stderr.on('data', chunk => console.error('[VOZLAB engine]', String(chunk).trim()));
  worker.on('spawn', () => { workerReady = true; });
  worker.on('error', error => {
    workerFailure = error.message;
    workerReady = false;
    for (const [, resolve] of pending) resolve({ ok: false, error: `Não foi possível iniciar o motor local: ${error.message}` });
    pending.clear();
    worker = null;
  });
  worker.on('exit', code => {
    workerReady = false;
    worker = null;
    for (const [, resolve] of pending) resolve({ ok: false, error: `O motor local foi encerrado (código ${code}).` });
    pending.clear();
  });
}

function engineRequest(payload) {
  startWorker();
  return new Promise(resolve => {
    const id = crypto.randomUUID();
    const timer = setTimeout(() => {
      pending.delete(id);
      resolve({ ok: false, error: 'O motor local não respondeu a tempo.' });
    }, 30 * 60 * 1000);
    pending.set(id, result => { clearTimeout(timer); resolve(result); });
    const write = () => worker.stdin.write(`${JSON.stringify({ ...payload, id })}\n`);
    if (workerReady) write();
    else worker.once('spawn', write);
    worker.once('error', error => {
      clearTimeout(timer);
      pending.delete(id);
      resolve({ ok: false, error: `Motor local indisponível: ${error.message}` });
    });
  });
}

function findDraft(draftId) {
  if (typeof draftId !== 'string' || !/^[0-9a-f-]{36}$/i.test(draftId)) return null;
  const folder = path.join(dataDir(), 'drafts');
  if (!fs.existsSync(folder)) return null;
  const file = fs.readdirSync(folder).find(name => name.startsWith(draftId + '.'));
  return file ? path.join(folder, file) : null;
}
ipcMain.handle('vozlab:prepare-voice', async () => {
  const picked = await dialog.showOpenDialog({
    title: 'Escolha uma gravação limpa e autorizada',
    properties: ['openFile'],
    filters: [{ name: 'Áudio', extensions: ['wav', 'mp3', 'flac', 'm4a', 'ogg'] }]
  });
  if (picked.canceled || !picked.filePaths[0]) return { ok: false, canceled: true };
  const source = picked.filePaths[0];
  const draftId = crypto.randomUUID();
  const folder = path.join(dataDir(), 'drafts');
  fs.mkdirSync(folder, { recursive: true });
  const destination = path.join(folder, draftId + path.extname(source).toLowerCase());
  fs.copyFileSync(source, destination);
  return { ok: true, draftId };
});
ipcMain.handle('vozlab:preview-draft', async (_event, request) => {
  const referencePath = findDraft(request?.draftId);
  if (!referencePath || !fs.existsSync(referencePath)) return { ok: false, error: 'A gravação temporária não foi encontrada. Escolha o áudio novamente.' };
  const stat = fs.statSync(referencePath);
  const cacheKey = ['draft', request.draftId, stat.size, stat.mtimeMs, request.pace, request.expression, request.pauses].join(':');
  const cached = previewCache.get(cacheKey);
  if (cached?.path && fs.existsSync(cached.path)) {
    return { ok: true, path: cached.path, url: pathToFileURL(cached.path).href, device: cached.device, cached: true };
  }
  const result = await engineRequest({
    command: 'generate', voicePath: referencePath,
    text: 'Olá! Esta é uma amostra da voz que você está criando. Ouça o resultado antes de salvar o perfil.',
    pace: request.pace, expression: request.expression, pauses: request.pauses
  });
  if (result.ok) {
    result.url = pathToFileURL(result.path).href;
    previewCache.set(cacheKey, { path: result.path, device: result.device });
  }
  return result;
});
ipcMain.handle('vozlab:commit-voice-draft', (_event, request) => {
  if (!request?.heardPreview || ![...previewCache.keys()].some(key => key.startsWith('draft:' + request.draftId + ':'))) {
    return { ok: false, error: 'Gere e ouça a prévia da clonagem antes de salvar o perfil.' };
  }
  const displayName = typeof request?.name === 'string' ? request.name.trim().slice(0, 50) : '';
  if (!displayName) return { ok: false, error: 'Digite um nome para o perfil de voz.' };
  const draftPath = findDraft(request?.draftId);
  if (!draftPath || !fs.existsSync(draftPath)) return { ok: false, error: 'A gravação temporária expirou. Escolha o áudio e faça a prévia novamente.' };
  const profiles = readJson('voices.json', []);
  const existing = request.profileId ? profiles.find(item => item.id === request.profileId) : null;
  if (request.profileId && !existing) return { ok: false, error: 'O perfil que você está substituindo não foi encontrado.' };
  const id = existing?.id || crypto.randomUUID();
  const profileDir = path.join(dataDir(), 'voices', id);
  fs.mkdirSync(profileDir, { recursive: true });
  const reference = path.join(profileDir, 'reference' + path.extname(draftPath).toLowerCase());
  const oldReference = existing?.reference;
  fs.copyFileSync(draftPath, reference);
  const profile = existing || { id, addedAt: new Date().toISOString(), consentConfirmed: true };
  profile.name = displayName;
  profile.reference = reference;
  profile.updatedAt = new Date().toISOString();
  if (!existing) profiles.push(profile);
  writeJson('voices.json', profiles);
  if (oldReference && path.resolve(oldReference) !== path.resolve(reference)) {
    try { fs.rmSync(oldReference, { force: true }); } catch {}
  }
  fs.rmSync(draftPath, { force: true });
  previewCache.clear();
  return { ok: true, id, name: displayName, replaced: Boolean(existing) };
});
ipcMain.handle('vozlab:discard-voice-draft', (_event, draftId) => {
  const draftPath = findDraft(draftId);
  if (draftPath) fs.rmSync(draftPath, { force: true });
  for (const [key, cached] of previewCache) {
    if (key.startsWith('draft:' + draftId + ':')) {
      if (cached?.path) fs.rmSync(cached.path, { force: true });
      previewCache.delete(key);
    }
  }
  return { ok: true };
});
ipcMain.handle('vozlab:get-profiles', () => readJson('voices.json', []));
ipcMain.handle('vozlab:rename-voice', (_event, { profileId, name }) => {
  const displayName = typeof name === 'string' ? name.trim().slice(0, 50) : '';
  if (!displayName) return { ok: false, error: 'Digite um nome para o perfil de voz.' };
  const profiles = readJson('voices.json', []);
  const profile = profiles.find(item => item.id === profileId);
  if (!profile) return { ok: false, error: 'Perfil de voz não encontrado.' };
  profile.name = displayName;
  profile.updatedAt = new Date().toISOString();
  writeJson('voices.json', profiles);
  return { ok: true, id: profile.id, name: profile.name };
});
ipcMain.handle('vozlab:delete-voice', (_event, profileId) => {
  const profiles = readJson('voices.json', []);
  if (!profiles.some(item => item.id === profileId)) return { ok: false, error: 'Perfil de voz não encontrado.' };
  writeJson('voices.json', profiles.filter(item => item.id !== profileId));
  writeJson('styles.json', readJson('styles.json', []).filter(item => item.voiceId !== profileId));
  fs.rmSync(path.join(dataDir(), 'voices', profileId), { recursive: true, force: true });
  previewCache.clear();
  return { ok: true };
});
ipcMain.handle('vozlab:preview-voice', (_event, profileId) => {
  const voice = readJson('voices.json', []).find(item => item.id === profileId);
  if (!voice || !fs.existsSync(voice.reference)) return { ok: false, error: 'A gravação de referência não foi encontrada.' };
  return { ok: true, url: pathToFileURL(voice.reference).href };
});
ipcMain.handle('vozlab:preview-clone', async (_event, request) => {
  const voice = readJson('voices.json', []).find(item => item.id === request.voiceId);
  if (!voice || !fs.existsSync(voice.reference)) return { ok: false, error: 'A gravação de referência não foi encontrada.' };
  const style = readJson('styles.json', []).find(item => item.id === request.styleId && item.voiceId === voice.id && item.reference && fs.existsSync(item.reference));
  const referencePath = style?.reference || voice.reference;
  const stat = fs.statSync(referencePath);
  const cacheKey = ['saved', voice.id, style?.id || '', stat.size, stat.mtimeMs, request.pace, request.expression, request.pauses].join(':');
  const cached = previewCache.get(cacheKey);
  if (cached?.path && fs.existsSync(cached.path)) {
    return { ok: true, path: cached.path, url: pathToFileURL(cached.path).href, device: cached.device, referenceStyle: style?.name || null, cached: true };
  }
  const result = await engineRequest({
    command: 'generate', voicePath: referencePath,
    text: 'Olá! Esta é uma amostra curta da voz selecionada. Ouça o resultado antes de escrever sua locução.',
    pace: request.pace, expression: request.expression, pauses: request.pauses
  });
  if (result.ok) {
    result.url = pathToFileURL(result.path).href;
    result.referenceStyle = style?.name || null;
    previewCache.set(cacheKey, { path: result.path, device: result.device });
  }
  return result;
});
ipcMain.handle('vozlab:get-styles', () => readJson('styles.json', []));
ipcMain.handle('vozlab:save-style', (_event, style) => {
  const voices = readJson('voices.json', []);
  if (!style.voiceId || !voices.some(voice => voice.id === style.voiceId)) {
    return { ok: false, error: 'Selecione uma voz antes de salvar um estilo.' };
  }
  const styles = readJson('styles.json', []);
  const existing = styles.find(item => item.id === style.id && item.voiceId === style.voiceId);
  if (existing) {
    existing.pace = style.pace;
    existing.expression = style.expression;
    existing.pauses = style.pauses;
    existing.updatedAt = new Date().toISOString();
    writeJson('styles.json', styles);
    return { ok: true, id: existing.id, name: existing.name };
  }
  const id = crypto.randomUUID();
  const { id: _requestedId, ...fields } = style;
  styles.push({ ...fields, id, savedAt: new Date().toISOString() });
  writeJson('styles.json', styles);
  return { ok: true, id, name: style.name };
});
ipcMain.handle('vozlab:add-style', async (_event, style) => {
  const voices = readJson('voices.json', []);
  if (!style.voiceId || !voices.some(voice => voice.id === style.voiceId)) {
    return { ok: false, error: 'Selecione uma voz antes de cadastrar o estilo.' };
  }
  const picked = await dialog.showOpenDialog({
    title: `Gravação de referência — ${style.name}`,
    properties: ['openFile'],
    filters: [{ name: 'Áudio', extensions: ['wav', 'mp3', 'flac', 'm4a', 'ogg'] }]
  });
  if (picked.canceled || !picked.filePaths[0]) return { ok: false };
  const source = picked.filePaths[0];
  const id = crypto.randomUUID();
  const styleDir = path.join(dataDir(), 'voices', style.voiceId, 'styles');
  fs.mkdirSync(styleDir, { recursive: true });
  const reference = path.join(styleDir, `${id}${path.extname(source).toLowerCase()}`);
  fs.copyFileSync(source, reference);
  const styles = readJson('styles.json', []);
  styles.push({ id, ...style, reference, savedAt: new Date().toISOString() });
  writeJson('styles.json', styles);
  return { ok: true, id, name: style.name };
});

ipcMain.handle('vozlab:engine-status', async () => {
  const result = await engineRequest({ command: 'status' });
  if (!result.ok && workerFailure) result.error = workerFailure;
  return result;
});
ipcMain.handle('vozlab:generate', async (_event, request) => {
  const voices = readJson('voices.json', []);
  const voice = voices.find(item => item.id === request.voiceId);
  if (!voice || !fs.existsSync(voice.reference)) return { ok: false, error: 'Selecione uma voz com uma gravação de referência válida.' };
  const style = readJson('styles.json', []).find(item => item.id === request.styleId && item.voiceId === voice.id && item.reference && fs.existsSync(item.reference));
  const result = await engineRequest({
    command: 'generate', voicePath: style?.reference || voice.reference, text: request.text,
    pace: request.pace, expression: request.expression, pauses: request.pauses
  });
  if (result.ok) {
    result.url = pathToFileURL(result.path).href;
    result.referenceStyle = style?.name || null;
  }
  return result;
});

ipcMain.handle('vozlab:export-audio', async (_event, sourcePath) => {
  const generatedRoot = `${path.resolve(dataDir(), 'generated')}${path.sep}`;
  const resolvedSource = sourcePath ? path.resolve(sourcePath) : '';
  if (!resolvedSource.startsWith(generatedRoot) || !fs.existsSync(resolvedSource) || path.extname(resolvedSource).toLowerCase() !== '.wav') return { ok: false };
  const picked = await dialog.showSaveDialog({
    title: 'Exportar locução em WAV', defaultPath: path.basename(resolvedSource),
    filters: [{ name: 'WAV', extensions: ['wav'] }]
  });
  if (picked.canceled || !picked.filePath) return { ok: false };
  fs.copyFileSync(resolvedSource, picked.filePath);
  return { ok: true, path: picked.filePath };
});

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => { if (BrowserWindow.getAllWindows().length === 0) createWindow(); });
});
app.on('before-quit', () => {
  if (worker && !worker.killed) {
    try { worker.stdin.write(`${JSON.stringify({command: 'shutdown', id: crypto.randomUUID()})}\n`); } catch {}
    worker.kill();
  }
});
app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
