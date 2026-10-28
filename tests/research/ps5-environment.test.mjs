import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, writeFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { analyzeEnvironment, readEnvironment, ENVIRONMENT_FIELDS, MAX_ENVIRONMENT_BYTES } from '../../tools/diagnostics/ps5-environment.mjs';

const empty = () => ({ schema_version: 1, fields: Object.fromEntries(ENVIRONMENT_FIELDS.map(key => [key, null])) });
// Invented identifiers; these fixtures describe no actual hardware.
const record = origin => ({ value: 'invented fixture identity', origin, reference: 'fixture source only' });

test('empty template and public support claims cannot fill installed evidence', () => {
  const input = empty();
  assert.deepEqual(analyzeEnvironment(input).missing_fields, ENVIRONMENT_FIELDS);
  input.fields.loader_identity = record('public_reference');
  const result = analyzeEnvironment(input);
  assert.ok(result.missing_fields.includes('loader_identity'));
  assert.deepEqual(result.public_reference_only, ['loader_identity']);
  assert.deepEqual(result.reported_fields, []);
});

test('complete declared evidence still needs source review and never enables execution', () => {
  const input = empty();
  for (const field of ENVIRONMENT_FIELDS) input.fields[field] = record('existing_log');
  const result = analyzeEnvironment(input);
  assert.equal(result.status, 'awaiting_source_review');
  assert.deepEqual(result.missing_fields, []);
  for (const flag of ['hardware_verified', 'deployment_allowed', 'runtime_bindings_enabled']) assert.equal(result[flag], false);
  assert.equal(result.game_compatibility, 'not_demonstrated');
  assert.equal(JSON.stringify(result).includes('fixture'), false);
});

test('malformed, oversized and unsupported evidence records fail closed', () => {
  for (const bad of [null, [], {}, { ...empty(), deployment_allowed: true }]) assert.throws(() => analyzeEnvironment(bad));
  for (const bad of [false, {}, record('hardware_verified'), { ...record('owner_report'), value: '' },
    { ...record('owner_report'), reference: null }, { ...record('owner_report'), value: 'x'.repeat(1025) },
    { ...record('owner_report'), value: 'line\nbreak' }, { ...record('owner_report'), verified: true }]) {
    const input = empty(); input.fields.loader_identity = bad;
    assert.throws(() => analyzeEnvironment(input));
  }
});

test('file reader bounds bytes and rejects malformed UTF-8 and JSON', async () => {
  const dir = await mkdtemp(join(tmpdir(), 'qb-environment-'));
  const file = join(dir, 'input.json');
  try {
    for (const bytes of [Buffer.alloc(MAX_ENVIRONMENT_BYTES + 1, 32), Buffer.from([0xff]), Buffer.from('{"secret":"private",')]) {
      await writeFile(file, bytes);
      await assert.rejects(readEnvironment(file));
    }
    await writeFile(file, JSON.stringify(empty()));
    assert.equal((await readEnvironment(file)).status, 'missing_environment_evidence');
  } finally { await rm(dir, { recursive: true, force: true }); }
});
