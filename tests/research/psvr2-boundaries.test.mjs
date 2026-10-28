import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { auditBoundaries } from '../../tools/diagnostics/psvr2-boundaries.mjs';
import { symbolNid } from '../../tools/diagnostics/ps5-imports.mjs';
const catalog = JSON.parse(readFileSync(new URL('../../docs/research/PSVR2_INTERFACE_MAP.json', import.meta.url)));
const snapshot = names => ({ schema_version: 1, imports: names.map(raw => ({raw, relocations: [{type: 7, target_vaddr: '0x1000'}]})), import_libraries: [], needed_modules: [] });
const group = (r, name) => r.groups.find(g => g.boundary === name);

test('buffered reprojection is discoverable even without an explicit flip import', () => {
  const r = auditBoundaries(snapshot(['sceHmd2ReprojectionSetParamWithBuffer', 'sceVideoOutRegisterBuffers2']), catalog);
  assert.equal(group(r, 'reprojection_submission').observations[0].pointer_relocations, 1);
  assert.equal(group(r, 'explicit_flip').status, 'not_observed_in_this_snapshot');
  assert.equal(r.runtime_bindings_enabled, false);
  assert.equal(r.game_compatibility, 'not_demonstrated');
});

test('hash matches with conflicting module providers remain conflicts', () => {
  const s = snapshot([symbolNid('sceHmd2ReprojectionSetParam') + '#A#A']);
  s.import_libraries = [{id: 0, name: 'libSceHmd2'}];
  s.needed_modules = [{id: 0, name: 'unrelated'}];
  const o = group(auditBoundaries(s, catalog), 'reprojection_submission').observations[0];
  assert.equal(o.provider_conflict, true);
  assert.equal(o.live_binding_verified, false);
});

test('all named imports and forged verification fields cannot establish compatibility', () => {
  const s = snapshot(catalog.modules.flatMap(m => m.entry_points));
  s.hardware_verified = true; s.runtime_bindings_enabled = true;
  const r = auditBoundaries(s, catalog);
  assert.ok(r.groups.every(g => g.observations.length));
  assert.equal(r.hardware_verified, false);
  assert.equal(r.interposition_verified, false);
  const forged = snapshot(['puts']);
  forged.imports[0].symbol_candidates = [{name: 'sceVrTracker2GetResult'}];
  assert.equal(group(auditBoundaries(forged, catalog), 'tracking').observations.length, 0);
});

test('ambiguous declarations and malformed relocation records are rejected', () => {
  const s = snapshot(['sceHmd2Initialize']);
  s.needed_modules = [{id: 1, name: 'a'}, {id: 1, name: 'b'}];
  assert.throws(() => auditBoundaries(s, catalog), /duplicate/);
  s.needed_modules = []; s.imports[0].relocations[0].target_vaddr = '0x10000000000000000';
  assert.throws(() => auditBoundaries(s, catalog), /relocation/);
});
