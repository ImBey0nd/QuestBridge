// SPDX-License-Identifier: GPL-3.0-or-later
// Original offline metadata classifier. Does not install hooks or enable firmware.
import { createHash } from 'node:crypto';
import { createReadStream } from 'node:fs';
import { readFile } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-';
const salt = Buffer.from('518d64a635ded8c1e6b039b1c3e55230', 'hex');
export function symbolNid(name) {
  if (typeof name !== 'string' || !/^[A-Za-z_][A-Za-z0-9_]{0,255}$/.test(name)) throw Error('Invalid candidate symbol name');
  return createHash('sha1').update(name).update(salt).digest().subarray(0, 8).reverse()
    .toString('base64').replaceAll('/', '-').replaceAll('=', '');
}
function decodeId(text) {
  if (!text || text.length > 3) return null;
  let result = 0;
  for (const char of text) {
    const digit = alphabet.indexOf(char);
    if (digit < 0) return null;
    result = result * 64 + digit;
  }
  return result <= 65535 ? result : null;
}
export function classifyImports(inventory, catalog) {
  const candidates = catalog.modules.flatMap(m => m.entry_points.map(name => ({ name, module: m.module, nid: symbolNid(name) })));
  return inventory.imports.map(item => {
    const parts = item.raw.split('#');
    const encoded = parts.length === 3 && /^[A-Za-z0-9+-]{11}$/.test(parts[0]);
    const libraryId = encoded ? decodeId(parts[1]) : null;
    const moduleId = encoded ? decodeId(parts[2]) : null;
    const valid = encoded && libraryId !== null && moduleId !== null;
    const library = valid ? inventory.import_libraries.find(d => d.id === libraryId)?.name ?? null : null;
    const module = valid ? inventory.needed_modules.find(d => d.id === moduleId)?.name ?? null : null;
    const matches = candidates.filter(c => valid ? c.nid === parts[0] : c.name === item.raw);
    return {
      ...item,
      encoded_identity: valid ? { nid: parts[0], library_id: libraryId, module_id: moduleId, library, module } : null,
      symbol_candidates: matches.map(c => ({
        name: c.name, module: c.module, match: valid ? 'name_hash' : 'exact_text',
        library_metadata_matches: library === null ? null : library.replace(/\.(sprx|prx)$/, '') === c.module,
      })),
      callability_verified: false,
    };
  });
}

async function main() {
  let scanner, input, firmware = 'unknown';
  const args = process.argv.slice(2), seen = new Set();
  for (let i = 0; i < args.length; ++i) {
    if (args[i] === '--scanner' || args[i] === '--firmware') {
      if (seen.has(args[i]) || i + 1 === args.length) throw Error('Missing or duplicate option');
      seen.add(args[i]);
      const option = args[i++];
      if (option === '--scanner') scanner = args[i]; else firmware = args[i];
    } else if (args[i].startsWith('--') || input) throw Error('Unknown option or extra input');
    else input = args[i];
  }
  if (!scanner || !input) throw Error('Usage: node tools/diagnostics/ps5-imports.mjs --scanner SCANNER [--firmware NN.NN] FILE');
  if (!/^(unknown|[0-9]{1,2}\.[0-9]{2})$/.test(firmware)) throw Error('Invalid declared firmware');
  const chunks = []; let size = 0;
  for await (const chunk of createReadStream(input)) {
    size += chunk.length;
    if (size > 64 * 1024 * 1024) throw Error('ELF exceeds 64 MiB limit');
    chunks.push(chunk);
  }
  const bytes = Buffer.concat(chunks);
  // Hash and scanner receive the same snapshot, even if the source path changes.
  const native = spawnSync(scanner, [], { input: bytes, maxBuffer: 32 * 1024 * 1024, timeout: 10000, windowsHide: true });
  if (native.error) throw native.error;
  if (native.status !== 0) throw Error(native.stderr.toString('utf8').trim() || 'Native import scanner failed');
  const scan = JSON.parse(native.stdout.toString('utf8'));
  if (scan.file_size !== bytes.length) throw Error('Native scanner did not inspect the complete snapshot');
  const catalogBytes = await readFile(new URL('../../docs/research/PSVR2_INTERFACE_MAP.json', import.meta.url));
  const imports = classifyImports(scan, JSON.parse(catalogBytes));
  const report = {
    schema_version: 1, evidence_kind: 'offline_import_metadata', declared_firmware: firmware,
    input_sha256: createHash('sha256').update(bytes).digest('hex'),
    interface_catalog_sha256: createHash('sha256').update(catalogBytes).digest('hex'),
    ...scan, imports,
    vr_candidate_imports: imports.filter(i => i.symbol_candidates.length).length,
    runtime_bindings_enabled: false, interposition_verified: false, hardware_verified: false,
    game_compatibility: 'not_demonstrated',
  };
  process.stdout.write(JSON.stringify(report, null, 2) + '\n');
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 2; });
}
