// SPDX-License-Identifier: GPL-3.0-or-later
// Offline investigation only: no process access, runtime ABI or hook installation.
import { createReadStream } from 'node:fs';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { classifyImports } from './ps5-imports.mjs';

const boundaries = {
  lifecycle: ['sceHmd2Initialize', 'sceHmd2Open', 'sceHmd2GetDeviceInformation', 'sceHmd2GetDeviceInformationByHandle'],
  tracking: ['sceVrTracker2Initialize', 'sceVrTracker2RegisterDevice', 'sceVrTracker2GetResult'],
  controller_state: ['scePadOpen', 'scePadVrControllerReadState'],
  buffer_registration: ['sceVideoOutOpen', 'sceVideoOutRegisterBuffers2'],
  reprojection_submission: ['sceHmd2ReprojectionSetParam', 'sceHmd2ReprojectionSetParamWithBuffer'],
  explicit_flip: ['sceVideoOutSubmitFlip'],
  display_timing: ['sceHmd2ReprojectionBeginFrame', 'sceHmd2ReprojectionGetPredictedDisplayTime'],
};

export function auditBoundaries(inventory, catalog) {
  if (!inventory || inventory.schema_version !== 1 ||
      !Array.isArray(inventory.imports) || inventory.imports.length > 65536 ||
      !Array.isArray(inventory.import_libraries) || !Array.isArray(inventory.needed_modules))
    throw Error('Expected a version-1 offline import inventory');
  for (const list of [inventory.import_libraries, inventory.needed_modules]) {
    const ids = new Set();
    for (const d of list) {
      if (!d || !Number.isInteger(d.id) || d.id < 0 || d.id > 65535 ||
          typeof d.name !== 'string' || d.name.length > 4096 || ids.has(d.id))
        throw Error('Invalid or duplicate provider declaration');
      ids.add(d.id);
    }
  }
  for (const item of inventory.imports) {
    if (!item || typeof item.raw !== 'string' || item.raw.length > 4096 ||
        !Array.isArray(item.relocations) || item.relocations.length > 65536)
      throw Error('Invalid import record');
    for (const r of item.relocations) {
      if (!r || !Number.isInteger(r.type) || typeof r.target_vaddr !== 'string' ||
          !/^0x[0-9a-fA-F]{1,16}$/.test(r.target_vaddr)) throw Error('Invalid relocation');
    }
  }
  // Reclassify from the local source catalog; never trust candidates/ready flags
  // supplied by an input JSON report.
  const imports = classifyImports(inventory, catalog);
  const groups = Object.entries(boundaries).map(([boundary, names]) => {
    const observations = imports.flatMap(item => item.symbol_candidates
      .filter(c => names.includes(c.name)).map(c => {
        const module = item.encoded_identity?.module;
        const providerConflict = c.library_metadata_matches === false ||
          (module != null && module.replace(/\.(sprx|prx)$/, '') !== c.module);
        return {
          name: c.name, raw: item.raw, expected_provider: c.module,
          provider_conflict: providerConflict,
          pointer_relocations: item.relocations.filter(r => r.type === 6 || r.type === 7).length,
          other_relocations: item.relocations.filter(r => r.type !== 6 && r.type !== 7).length,
          live_binding_verified: false,
        };
      }));
    return { boundary, observations, status: observations.length ? 'candidate_metadata_present' : 'not_observed_in_this_snapshot' };
  });
  return {
    schema_version: 1, evidence_kind: 'offline_boundary_audit', groups,
    limitations: [
      'Missing imports may be in other PRXs, runtime resolution or indirect submissions.',
      'Pointer relocations do not establish live slot contents, writability or a callable ABI.',
      'SetParam must be correlated with registration and GPU completion; explicit flips are not required in every consumer.',
      'Presence of every boundary still does not establish runtime coverage or stereo capture.',
    ],
    runtime_bindings_enabled: false, hardware_verified: false,
    interposition_verified: false, game_compatibility: 'not_demonstrated',
  };
}

async function main() {
  if (process.argv.length !== 3) throw Error('Usage: node tools/diagnostics/psvr2-boundaries.mjs IMPORT_REPORT.json');
  const chunks = []; let size = 0;
  for await (const c of createReadStream(process.argv[2])) {
    size += c.length;
    if (size > 32 * 1024 * 1024) throw Error('Import report exceeds 32 MiB');
    chunks.push(c);
  }
  const catalog = JSON.parse(await readFile(new URL('../../docs/research/PSVR2_INTERFACE_MAP.json', import.meta.url)));
  process.stdout.write(JSON.stringify(auditBoundaries(JSON.parse(Buffer.concat(chunks)), catalog), null, 2) + '\n');
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href)
  main().catch(e => { process.stderr.write(e.message + '\n'); process.exitCode = 2; });
