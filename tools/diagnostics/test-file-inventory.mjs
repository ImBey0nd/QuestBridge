// SPDX-License-Identifier: GPL-3.0-or-later
// Optional real Chromium check of the local artifact using synthetic bytes only.
// Uses a local DevTools pipe, not a listening debug server. No console connection.
import { spawn } from 'node:child_process';
import { createServer } from 'node:http';
import { mkdtemp, writeFile, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash, randomBytes } from 'node:crypto';
import assert from 'node:assert/strict';

const directory = await mkdtemp(join(tmpdir(), 'qb-local-inventory-'));
const artifactPath = fileURLToPath(new URL('./ps5-file-inventory.html',import.meta.url));
const artifact = await readFile(artifactPath);
// This managed Chromium forbids file: URLs. Test the exact HTML through an
// authenticated one-time loopback server; this does not test file: navigation.
const token = '/'+randomBytes(32).toString('hex');
const server = createServer((request,response) => {
  if (request.method !== 'GET' || request.url !== token) { response.writeHead(404);response.end();return; }
  response.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store'});response.end(artifact);
});
await new Promise((resolve,reject) => { server.once('error',reject);server.listen(0,'127.0.0.1',resolve); });
const browser = spawn('chromium', ['--headless', '--no-sandbox', '--disable-gpu',
  '--disable-background-networking', '--disable-component-update', '--no-first-run',
  '--disable-sync', '--remote-debugging-pipe', `--user-data-dir=${join(directory, 'profile')}`],
  { stdio: ['ignore', 'ignore', 'pipe', 'pipe', 'pipe'] });
let sequence = 0, incoming = '', errorText = '', closed = false;
const pending = new Map();
browser.stderr.on('data', bytes => { errorText = (errorText + bytes).slice(-3000); });
function stopPending(message) { for (const p of pending.values()) { clearTimeout(p.timer); p.reject(Error(message)); } pending.clear(); }
browser.on('error', error => stopPending(error.message));
browser.on('close', () => { closed = true; stopPending('Chromium closed: ' + errorText); });
browser.stdio[3].on('error', error => stopPending(error.message));
browser.stdio[4].on('data', bytes => {
  incoming += bytes; let end;
  while ((end = incoming.indexOf('\0')) >= 0) {
    const message = JSON.parse(incoming.slice(0,end)); incoming = incoming.slice(end+1);
    if (!message.id) continue;
    const p = pending.get(message.id); if (!p) continue;
    pending.delete(message.id); clearTimeout(p.timer);
    if (message.error) p.reject(Error(JSON.stringify(message.error))); else p.resolve(message.result);
  }
});
function call(method, params = {}, sessionId) {
  if (closed) return Promise.reject(Error('Chromium unavailable'));
  const id = ++sequence;
  return new Promise((resolve,reject) => {
    const timer = setTimeout(() => { pending.delete(id); reject(Error('DevTools timeout: ' + method)); },10000);
    pending.set(id,{resolve,reject,timer}); browser.stdio[3].write(JSON.stringify({id,method,params,sessionId})+'\0');
  });
}
try {
  const version = await call('Browser.getVersion');
  const {targetId} = await call('Target.createTarget',{url:'about:blank'});
  const {sessionId} = await call('Target.attachToTarget',{targetId,flatten:true});
  const evaluate = async expression => {
    const r = await call('Runtime.evaluate',{expression,awaitPromise:true,returnByValue:true},sessionId);
    if (r.exceptionDetails) throw Error(JSON.stringify(r.exceptionDetails));return r.result.value;
  };
  const url = `http://127.0.0.1:${server.address().port}${token}`;
  const navigation = await call('Page.navigate',{url},sessionId);
  if (navigation.errorText) throw Error('Navigation failed: '+navigation.errorText);
  try {
    assert.equal(await evaluate(`new Promise((resolve,reject)=>{const end=Date.now()+5000;const check=()=>{if(globalThis.QbFileInventory)resolve(true);else if(Date.now()>end)reject(Error('Page not ready'));else setTimeout(check,20)};check()})`),true);
  } catch (error) {
    const state = await evaluate(`({url:location.href,state:document.readyState,title:document.title,body:document.body?.textContent.slice(0,300)})`);
    throw Error(error.message+' '+JSON.stringify(state));
  }
  const bytes = Buffer.alloc(64);bytes.set([127,69,76,70,2,1,1]);
  bytes.writeUInt16LE(3,16);bytes.writeUInt16LE(62,18);bytes.writeUInt32LE(1,20);bytes.writeUInt16LE(64,52);
  const input = join(directory,'libSceHmd2.sprx');await writeFile(input,bytes);
  const {root} = await call('DOM.getDocument',{},sessionId);
  const {nodeId} = await call('DOM.querySelector',{nodeId:root.nodeId,selector:'#files'},sessionId);
  await call('DOM.setFileInputFiles',{nodeId,files:[input]},sessionId);
  await evaluate(`document.getElementById('analyze').click()`);
  const report = await evaluate(`new Promise((resolve,reject)=>{const end=Date.now()+5000;const check=()=>{if(!document.getElementById('analyze').disabled){try{resolve(JSON.parse(document.getElementById('status').textContent.split('\\n\\n').slice(1).join('\\n\\n')))}catch(e){reject(Error(document.getElementById('status').textContent))}}else if(Date.now()>end)reject(Error('Report timed out'));else setTimeout(check,20)};check()})`);
  assert.equal(report.files[0].sha256,createHash('sha256').update(bytes).digest('hex'));
  assert.equal(report.files[0].header.kind,'elf64_little_endian_amd64_header');
  assert.equal(report.console_contacted,false);assert.equal(report.original_files_included,false);
  assert.equal(await evaluate(`document.getElementById('download').href.startsWith('blob:') && !document.getElementById('download').hidden`),true);
  await call('Browser.setDownloadBehavior',{behavior:'allow',downloadPath:directory});
  await evaluate(`document.getElementById('download').click()`);
  let downloaded;
  for (let i=0;i<100;++i) {
    try { downloaded=JSON.parse(await readFile(join(directory,'questbridge-inventario-local.json'),'utf8'));break; } catch { await new Promise(resolve=>setTimeout(resolve,20)); }
  }
  assert.deepEqual(downloaded,report);
  process.stdout.write(JSON.stringify({evidence_kind:'real_browser_synthetic_local_file_test',browser:version.product,
    artifact_sha256:createHash('sha256').update(await readFile(artifactPath)).digest('hex'),
    test_origin:'authenticated_loopback_http',file_url_navigation:'blocked_by_managed_browser_policy_not_tested',
    file_input_report_and_download_passed:true,ps5_contacted:false,ps5_compatibility_verified:false},null,2)+'\n');
} finally {
  if (!closed) { try { await call('Browser.close'); } catch { browser.kill('SIGKILL'); } }
  server.closeAllConnections();await new Promise(resolve=>server.close(resolve));
  stopPending('Test finished');await rm(directory,{recursive:true,force:true,maxRetries:5,retryDelay:100});
}
