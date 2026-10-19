const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('vozlab', {
  addVoice: () => ipcRenderer.invoke('vozlab:add-voice'),
  getProfiles: () => ipcRenderer.invoke('vozlab:get-profiles'),
  getStyles: () => ipcRenderer.invoke('vozlab:get-styles'),
  saveStyle: style => ipcRenderer.invoke('vozlab:save-style', style),
  addStyle: style => ipcRenderer.invoke('vozlab:add-style', style),
  getEngineStatus: () => ipcRenderer.invoke('vozlab:engine-status'),
  generate: request => ipcRenderer.invoke('vozlab:generate', request),
  exportAudio: filePath => ipcRenderer.invoke('vozlab:export-audio', filePath),
  onEngineProgress: callback => ipcRenderer.on('vozlab:engine-progress', (_event, message) => callback(message))
});
