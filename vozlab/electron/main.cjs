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
    backgroundColor: '#080b16', title: 'VOZLAB — Estúdio de voz',
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

ipcMain.handle('vozlab:add-voice', async () => {
  const picked = await dialog.showOpenDialog({
    title: 'Escolha uma gravação de voz autorizada',
    properties: ['openFile'],
    filters: [{ name: 'Áudio', extensions: ['wav', 'mp3', 'flac', 'm4a', 'ogg'] }]
  });
  if (picked.canceled || !picked.filePaths[0]) return { ok: false };
  const source = picked.filePaths[0];
  const consent = await dialog.showMessageBox({
    type: 'warning', title: 'Autorização da voz',
    message: 'Use apenas uma gravação sua ou de uma pessoa que autorizou a clonagem. Para uma identidade vocal melhor, escolha fala limpa, sem música nem eco, com cerca de 15 a 30 segundos. O motor usará os primeiros 20 segundos como referência.',
    buttons: ['Tenho autorização e quero continuar', 'Cancelar'], defaultId: 0, cancelId: 1
  });
  if (consent.response === 1) return { ok: false };
  const displayName = path.basename(source, path.extname(source)).slice(0, 50);
  const ext = path.extname(source).toLowerCase();
  const profileId = crypto.randomUUID();
  const profileDir = path.join(dataDir(), 'voices', profileId);
  fs.mkdirSync(profileDir, { recursive: true });
  const reference = path.join(profileDir, `reference${ext}`);
  fs.copyFileSync(source, reference);
  const profiles = readJson('voices.json', []);
  profiles.push({ id: profileId, name: displayName, reference, addedAt: new Date().toISOString(), consentConfirmed: true });
  writeJson('voices.json', profiles);
  return { ok: true, id: profileId, name: displayName };
});

ipcMain.handle('vozlab:get-profiles', () => readJson('voices.json', []));
ipcMain.handle('vozlab:get-styles', () => readJson('styles.json', []));
ipcMain.handle('vozlab:save-style', (_event, style) => {
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
  styles.push({ id, ...style, savedAt: new Date().toISOString() });
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
  if (!sourcePath || !fs.existsSync(sourcePath)) return { ok: false };
  const picked = await dialog.showSaveDialog({
    title: 'Exportar locução em WAV', defaultPath: path.basename(sourcePath),
    filters: [{ name: 'WAV', extensions: ['wav'] }]
  });
  if (picked.canceled || !picked.filePath) return { ok: false };
  fs.copyFileSync(sourcePath, picked.filePath);
  return { ok: true, path: picked.filePath };
});

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => { if (BrowserWindow.getAllWindows().length === 0) createWindow(); });
});
app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
