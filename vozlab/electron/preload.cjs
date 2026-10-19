const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('vozlab', {
  prepareVoice: () => ipcRenderer.invoke('vozlab:prepare-voice'),
  previewDraft: request => ipcRenderer.invoke('vozlab:preview-draft', request),
  commitVoiceDraft: request => ipcRenderer.invoke('vozlab:commit-voice-draft', request),
  discardVoiceDraft: draftId => ipcRenderer.invoke('vozlab:discard-voice-draft', draftId),
  renameVoice: (profileId, name) => ipcRenderer.invoke('vozlab:rename-voice', {profileId, name}),
  deleteVoice: profileId => ipcRenderer.invoke('vozlab:delete-voice', profileId),
  previewClone: request => ipcRenderer.invoke('vozlab:preview-clone', request),
  previewVoice: profileId => ipcRenderer.invoke('vozlab:preview-voice', profileId),
  getProfiles: () => ipcRenderer.invoke('vozlab:get-profiles'),
  getStyles: () => ipcRenderer.invoke('vozlab:get-styles'),
  saveStyle: style => ipcRenderer.invoke('vozlab:save-style', style),
  addStyle: style => ipcRenderer.invoke('vozlab:add-style', style),
  getEngineStatus: () => ipcRenderer.invoke('vozlab:engine-status'),
  generate: request => ipcRenderer.invoke('vozlab:generate', request),
  exportAudio: filePath => ipcRenderer.invoke('vozlab:export-audio', filePath),
  onEngineProgress: callback => ipcRenderer.on('vozlab:engine-progress', (_event, message) => callback(message))
});
