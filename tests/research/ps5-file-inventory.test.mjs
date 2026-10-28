import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createHash, webcrypto } from 'node:crypto';
import { runInNewContext } from 'node:vm';

const html = readFileSync(new URL('../../tools/diagnostics/ps5-file-inventory.html', import.meta.url), 'utf8');
const script = /<script id="inventory-script">([\s\S]*?)<\/script>/.exec(html)[1];
const elements = new Map();
const context = { crypto: webcrypto, document: { getElementById(id) {
  if (!elements.has(id)) elements.set(id, { addEventListener() {} }); return elements.get(id);
} } };
runInNewContext(script, context);
const api = context.QbFileInventory;
function file(bytes, name = 'libSceHmd2.sprx') {
  return { name, size: bytes.length, localPath: 'PRIVATE_PATH_MUST_NOT_APPEAR', async arrayBuffer() { return Uint8Array.from(bytes).buffer; } };
}
function elfHeader() {
  const b = Buffer.alloc(64); b.set([127,69,76,70,2,1,1]);
  b.writeUInt16LE(3,16);b.writeUInt16LE(62,18);b.writeUInt32LE(1,20);b.writeUInt16LE(64,52);return b;
}
test('local inventory hashes exact chosen bytes without exporting content, paths or verification claims', async () => {
  const b = elfHeader(), r = await api.inventoryFiles([file(b)]);
  assert.equal(r.files[0].sha256, createHash('sha256').update(b).digest('hex'));
  assert.equal(r.files[0].header.kind, 'elf64_little_endian_amd64_header');
  assert.equal(r.target_firmware_verified, false); assert.equal(r.decryption_verified, false);
  assert.equal(r.module_abi_verified, false); assert.equal(r.runtime_bindings_enabled, false);
  assert.equal(r.original_files_included, false); assert.equal(r.console_contacted, false);
  assert.equal(r.uploads_performed, false);
  assert.ok(!JSON.stringify(r).includes('PRIVATE_PATH_MUST_NOT_APPEAR'));
  assert.ok(!Object.hasOwn(r.files[0], 'data'));
});
test('all names and allocation bounds are checked before any selected file is read', async () => {
  let read = false;
  const good = { name:'libScePad.sprx', size:1, arrayBuffer(){read=true;throw Error('must not read');} };
  await assert.rejects(api.inventoryFiles([good,{...good,name:'eboot.bin'}]), /Nombre/);assert.equal(read,false);
  for (const selection of [[],[...Array(7)].map(()=>good),[{...good,size:67108865}],[{...good,size:0}],
    [good,{...good,name:'LIBSCEPAD.SPRX'}],
    ['libSceHmd2','libSceVrTracker2','libScePad','libSceVideoOut','libSceAgcDriver'].map(n=>({...good,name:n+'.sprx',size:67108864}))])
    await assert.rejects(api.inventoryFiles(selection));
  assert.equal(read,false);
  const mismatched = { ...file(Buffer.alloc(4)), size:8 };
  await assert.rejects(api.inventoryFiles([mismatched]), /tamaño/);
});
test('every short ELF prefix and unsupported header stays unverified; other data is not called encrypted', () => {
  const b = elfHeader();
  for (let n=0;n<64;++n) assert.notEqual(api.classifyHeader(b.subarray(0,n)).kind,'elf64_little_endian_amd64_header');
  for (const at of [4,5,6,18,20,52]) { const bad=Buffer.from(b);bad[at]^=255;assert.notEqual(api.classifyHeader(bad).kind,'elf64_little_endian_amd64_header'); }
  assert.equal(api.classifyHeader(Buffer.from('normal text')).kind,'not_plain_elf_header');
});
test('browser artifact is self-contained and blocks network connections in its content policy', () => {
  assert.match(html,/connect-src 'none'/);
  assert.doesNotMatch(html,/<script\b[^>]*\bsrc\s*=/i);
  assert.doesNotMatch(script,/\b(fetch|XMLHttpRequest|WebSocket|eval)\s*\(/);
  assert.doesNotMatch(script,/\.innerHTML\s*=/);
});
