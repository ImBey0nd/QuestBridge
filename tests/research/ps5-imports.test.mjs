import test from 'node:test';
import assert from 'node:assert/strict';
import { classifyImports, symbolNid } from '../../tools/diagnostics/ps5-imports.mjs';
const catalog = { modules: [{ module: 'libSceHmd2', entry_points: ['sceHmd2Initialize'] }] };
const inventory = raw => ({ imports: [{ raw, relocations: [] }], import_libraries: [{ id: 2, name: 'libSceHmd2' }], needed_modules: [{ id: 1, name: 'libSceHmd2' }] });

test('candidate hashing agrees with four vectors executed from the pinned SDK encoder', () => {
  for (const [name, nid] of [
    ['sceHmd2Initialize', 'c812oYs7Vsc'], ['sceVrTracker2QueryMemory', 'TwqZnaIjWv4'],
    ['scePadVrControllerReadState', 'iA-DdobUen8'], ['sceHmd2ReprojectionSetParam', 'xMo9ENEu2E0'],
  ]) assert.equal(symbolNid(name), nid);
  assert.throws(() => symbolNid('name#A#A'));
});

test('NID identity uses module and library declarations, without inferring a callable ABI', () => {
  const [r] = classifyImports(inventory('c812oYs7Vsc#C#B'), catalog);
  assert.equal(r.encoded_identity.library, 'libSceHmd2');
  assert.equal(r.encoded_identity.module_id, 1);
  assert.equal(r.symbol_candidates[0].name, 'sceHmd2Initialize');
  assert.equal(r.symbol_candidates[0].library_metadata_matches, true);
  assert.equal(r.callability_verified, false);
});

test('mismatched or absent provider metadata is preserved and never upgraded', () => {
  const a = inventory('c812oYs7Vsc#C#B'); a.import_libraries[0].name = 'other';
  assert.equal(classifyImports(a, catalog)[0].symbol_candidates[0].library_metadata_matches, false);
  a.import_libraries = [];
  assert.equal(classifyImports(a, catalog)[0].symbol_candidates[0].library_metadata_matches, null);
  for (const raw of ['c812oYs7Vsc#?#B', 'c812oYs7Vsc#---#B', 'c812oYs7Vsc#C#B#C', 'unresolved#C#B']) {
    const [r] = classifyImports(inventory(raw), catalog);
    assert.equal(r.encoded_identity, null); assert.equal(r.symbol_candidates.length, 0);
  }
});

test('plain imports and multiple candidates remain explicit source matches', () => {
  const r = classifyImports(inventory('sceHmd2Initialize'), catalog)[0];
  assert.equal(r.symbol_candidates[0].match, 'exact_text');
  assert.equal(r.encoded_identity, null);
  const duplicate = { modules: [...catalog.modules, { module: 'other', entry_points: ['sceHmd2Initialize'] }] };
  assert.equal(classifyImports(inventory('c812oYs7Vsc#C#B'), duplicate)[0].symbol_candidates.length, 2);
  assert.equal(classifyImports(inventory('puts'), catalog)[0].symbol_candidates.length, 0);
});
