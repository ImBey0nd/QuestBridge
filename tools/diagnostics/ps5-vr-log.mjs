// SPDX-License-Identifier: GPL-3.0-or-later
// Offline interpretation of PS5VR log messages. No Sony calls or console access.
import { createHash } from 'node:crypto';
import { createReadStream } from 'node:fs';
import { pathToFileURL } from 'node:url';

export const MAX_LOG_BYTES = 1024 * 1024;
const MAX_LINES = 10000;
const MAX_ATTEMPTS = 128;
const STAGES = [
  'hmd_initialize', 'field_of_view', 'hmd_open', 'tracker_initialize',
  'tracker_register', 'reprojection_initialize', 'enable_vr_mode',
  'video_port', 'register_buffers', 'configure_output', 'render_config',
];
const HEX = '(0x[0-9a-fA-F]{8})(?![0-9a-fA-F])';
const patterns = [
  [new RegExp(`vr: sceHmd2Initialize -> ${HEX}$`), ['hmd_initialize']],
  [new RegExp(`vr: FOV rc ${HEX}(?: |$)`), ['field_of_view']],
  [new RegExp(`vr: sceHmd2Open -> ${HEX}; device status ([0-9]+) worn ([0-9]+) lens ([0-9]+) mm$`), ['hmd_open']],
  [new RegExp(`vr: tracker init ${HEX} register ${HEX}$`), ['tracker_initialize', 'tracker_register']],
  [new RegExp(`vr: sceHmd2ReprojectionInitialize -> ${HEX}$`), ['reprojection_initialize']],
  [new RegExp(`vr: EnableVrMode -> ${HEX}, VR port ${HEX}$`), ['enable_vr_mode', 'video_port']],
  [new RegExp(`vr: RegisterBuffers2 -> ${HEX}$`), ['register_buffers']],
  [new RegExp(`vr: 120 Hz output ${HEX}, half-rate render config ${HEX}$`), ['configure_output', 'render_config']],
  [new RegExp(`vr: frame ([0-9]+) setparam ${HEX} flip ${HEX}$`), ['frame_error']],
];

export function analyzeVrLog(text, { firmware = 'unknown', origin = 'unverified' } = {}) {
  if (typeof text !== 'string' || Buffer.byteLength(text) > MAX_LOG_BYTES) throw Error('Log exceeds the 1 MiB text limit');
  if (!/^(unknown|[0-9]{1,2}\.[0-9]{2})$/.test(firmware)) throw Error('Firmware must be unknown or NN.NN');
  if (!['unverified', 'synthetic', 'console_capture'].includes(origin)) throw Error('Invalid declared log origin');
  const lines = text.split(/\r?\n/);
  if (lines.length > MAX_LINES) throw Error('Log exceeds the 10000 line limit');
  if (lines.some(line => line.length > 4096)) throw Error('Log line exceeds 4096 characters');
  const attempts = [];
  const newAttempt = () => ({ events: [], submission_markers: 0, unparsed_relevant_lines: [] });
  let attempt = newAttempt();
  const flush = () => {
    if (!attempt.events.length && !attempt.submission_markers && !attempt.unparsed_relevant_lines.length) return;
    if (attempts.length >= MAX_ATTEMPTS) throw Error('Log exceeds the 128 attempt limit');
    const observed = new Set(attempt.events.filter(e => STAGES.includes(e.stage)).map(e => e.stage));
    const failed = attempt.events.filter(e => e.failed);
    let previous = -1;
    let ordered = true;
    for (const event of attempt.events) {
      const index = STAGES.indexOf(event.stage);
      if (index >= 0) {
        if (index < previous) ordered = false;
        previous = index;
      }
    }
    attempts.push({
      ...attempt,
      first_failure: failed[0] ?? null,
      missing_setup_stages: STAGES.filter(s => !observed.has(s)),
      setup_order_consistent: ordered,
      logged_setup_checks_passed: STAGES.every(s => observed.has(s)) && !failed.length && ordered
        && !attempt.unparsed_relevant_lines.length,
    });
    attempt = newAttempt();
  };
  for (const [index, raw] of lines.entries()) {
    const line = raw.trimEnd();
    if (!line.includes('vr: ')) continue;
    if (/vr: sceHmd2Initialize -> /.test(line) && attempt.events.some(e => e.stage === 'hmd_initialize')) flush();
    let matched = false;
    for (const [pattern, stages] of patterns) {
      const match = pattern.exec(line);
      if (!match) continue;
      if (stages[0] === 'hmd_open' && (!Number.isSafeInteger(Number(match[2])) || Number(match[2]) > 0xffffffff)) continue;
      matched = true;
      if (stages[0] === 'frame_error') {
        const codes = match.slice(2, 4).map(c => c.toLowerCase());
        attempt.events.push({ line: index + 1, stage: 'frame_submission', codes,
          failed: codes.some(c => Number(c) !== 0) });
      } else {
        stages.forEach((stage, i) => {
          const code = match[i + 1].toLowerCase();
          const handle = stage === 'hmd_open' || stage === 'video_port';
          const event = { line: index + 1, stage, code, failed: handle ? Number(code) >= 0x80000000 : Number(code) !== 0 };
          if (stage === 'hmd_open') event.device_status_raw = Number(match[2]);
          attempt.events.push(event);
        });
      }
      break;
    }
    if (matched) continue;
    if (/vr: frame [0-9]+ submitted \(buf [0-9]+\)$/.test(line)) {
      // Upstream emits this even when SetParam/SubmitFlip fail. Never count as acceptance.
      attempt.submission_markers++;
    } else if (/vr: .*no export table|vr: missing |vr: headset setup failed|vr: eye buffers failed/.test(line)) {
      attempt.events.push({ line: index + 1, stage: 'setup_failure', failed: true });
    } else if (/vr: (sceHmd2Initialize|FOV rc|sceHmd2Open|tracker init|sceHmd2ReprojectionInitialize|EnableVrMode|RegisterBuffers2|120 Hz output|frame )/.test(line)) {
      attempt.unparsed_relevant_lines.push(index + 1);
    }
  }
  flush();
  return {
    schema_version: 1,
    source_format: 'PS5VR 209eb7f0160000507a0ca017842941b8e8e88f46',
    declared_origin: origin,
    declared_firmware: firmware,
    input_sha256: createHash('sha256').update(text).digest('hex'),
    evidence_kind: 'offline_log_interpretation',
    hardware_verified: false,
    game_compatibility: 'not_demonstrated',
    tracking_injection: 'not_demonstrated',
    stereo_capture: 'not_demonstrated',
    attempts,
  };
}

async function main() {
  const [file, ...args] = process.argv.slice(2);
  if (!file || args.length % 2) throw Error('Usage: node tools/diagnostics/ps5-vr-log.mjs LOG [--origin synthetic|console_capture|unverified] [--firmware NN.NN]');
  const options = {};
  const seen = new Set();
  for (let i = 0; i < args.length; i += 2) {
    if (!['--firmware', '--origin'].includes(args[i]) || seen.has(args[i])) throw Error('Unknown or duplicate option');
    seen.add(args[i]);
    options[args[i].slice(2)] = args[i + 1];
  }
  const chunks = [];
  let size = 0;
  for await (const chunk of createReadStream(file)) {
    size += chunk.length;
    if (size > MAX_LOG_BYTES) throw Error('Log exceeds the 1 MiB text limit');
    chunks.push(chunk);
  }
  const text = new TextDecoder('utf-8', { fatal: true }).decode(Buffer.concat(chunks));
  process.stdout.write(JSON.stringify(analyzeVrLog(text, options), null, 2) + '\n');
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 2; });
}
