// SPDX-License-Identifier: GPL-3.0-or-later
// Offline completeness check only. No console access or compatibility decisions.
import { createReadStream } from 'node:fs';
import { pathToFileURL } from 'node:url';

export const MAX_ENVIRONMENT_BYTES = 64 * 1024;
export const ENVIRONMENT_FIELDS = [
  'model_revision', 'firmware_full_build', 'relapse_identity', 'kstuff_identity',
  'loader_identity', 'loader_source', 'loader_stdout', 'game_identity', 'module_metadata',
];
const ORIGINS = ['owner_report', 'system_screen', 'existing_log', 'local_metadata', 'public_reference'];
const object = value => value !== null && typeof value === 'object' && !Array.isArray(value);
const keys = (value, expected) => object(value)
  && Object.keys(value).length === expected.length
  && expected.every(key => Object.hasOwn(value, key));
const text = value => typeof value === 'string' && value.trim().length > 0
  && value.length <= 1024 && !/[\u0000-\u001f\u007f]/u.test(value);

export function analyzeEnvironment(input) {
  if (!keys(input, ['schema_version', 'fields']) || input.schema_version !== 1
      || !keys(input.fields, ENVIRONMENT_FIELDS)) throw Error('Invalid environment schema');
  const missing = [], publicOnly = [], reported = [];
  for (const field of ENVIRONMENT_FIELDS) {
    const record = input.fields[field];
    if (record === null) { missing.push(field); continue; }
    if (!keys(record, ['value', 'origin', 'reference']) || !text(record.value)
        || !ORIGINS.includes(record.origin) || !text(record.reference)) {
      throw Error(`Invalid record: ${field}`);
    }
    if (record.origin === 'public_reference') { missing.push(field); publicOnly.push(field); }
    else reported.push({ field, declared_origin: record.origin });
  }
  // Even a complete self-declared record needs human source/ABI review.
  // Do not echo submitted values, addresses, paths or log text in the summary.
  return {
    schema_version: 1,
    evidence_kind: 'offline_environment_completeness',
    status: missing.length ? 'missing_environment_evidence' : 'awaiting_source_review',
    missing_fields: missing,
    public_reference_only: publicOnly,
    reported_fields: reported,
    hardware_verified: false,
    deployment_allowed: false,
    runtime_bindings_enabled: false,
    game_compatibility: 'not_demonstrated',
  };
}

export async function readEnvironment(file) {
  const chunks = [];
  let size = 0;
  for await (const chunk of createReadStream(file)) {
    size += chunk.length;
    if (size > MAX_ENVIRONMENT_BYTES) throw Error('Environment input exceeds 64 KiB');
    chunks.push(chunk);
  }
  const decoded = new TextDecoder('utf-8', { fatal: true }).decode(Buffer.concat(chunks));
  let input;
  try { input = JSON.parse(decoded); }
  catch { throw Error('Environment input must be valid JSON'); }
  return analyzeEnvironment(input);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const args = process.argv.slice(2);
  if (args.length !== 1) {
    process.stderr.write('Usage: node tools/diagnostics/ps5-environment.mjs FILE.json\n');
    process.exitCode = 2;
  } else {
    readEnvironment(args[0]).then(result => {
      process.stdout.write(JSON.stringify(result, null, 2) + '\n');
    }).catch(error => {
      // Filesystem and decoder diagnostics can include private input details.
      const safe = error.message.startsWith('Invalid record:') || [
        'Invalid environment schema', 'Environment input exceeds 64 KiB',
        'Environment input must be valid JSON',
      ].includes(error.message);
      process.stderr.write((safe ? error.message : 'Could not read UTF-8 environment file') + '\n');
      process.exitCode = 2;
    });
  }
}
