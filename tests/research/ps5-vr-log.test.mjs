import test from 'node:test';
import assert from 'node:assert/strict';
import { analyzeVrLog, MAX_LOG_BYTES } from '../../tools/diagnostics/ps5-vr-log.mjs';

// Invented log data exercises interpretation only. No console run or Sony ABI.
const setup = [
  'vr: sceHmd2Initialize -> 0x00000000',
  'vr: FOV rc 0x00000000 out 1.0 in 1.0 top 1.0 bottom 1.0',
  'vr: sceHmd2Open -> 0x00000001; device status 2 worn 0 lens 0 mm',
  'vr: tracker init 0x00000000 register 0x00000000',
  'vr: sceHmd2ReprojectionInitialize -> 0x00000000',
  'vr: EnableVrMode -> 0x00000000, VR port 0x00000001',
  'vr: RegisterBuffers2 -> 0x00000000',
  'vr: 120 Hz output 0x00000000, half-rate render config 0x00000000',
];

test('complete synthetic setup remains log evidence, never hardware or compatibility', () => {
  const r = analyzeVrLog(setup.join('\n'), { origin: 'synthetic', firmware: '13.20' });
  assert.equal(r.attempts[0].logged_setup_checks_passed, true);
  assert.equal(r.attempts[0].events.find(e => e.stage === 'hmd_open').device_status_raw, 2);
  assert.equal(r.hardware_verified, false);
  assert.equal(r.game_compatibility, 'not_demonstrated');
  assert.equal(analyzeVrLog(setup.join('\n'), { origin: 'console_capture' }).hardware_verified, false);
});

test('unconditional submitted marker cannot erase SetParam or flip errors', () => {
  const r = analyzeVrLog([...setup,
    'vr: frame 0 setparam 0x8a720002 flip 0xffffffff',
    'vr: frame 0 submitted (buf 0)',
  ].join('\n'));
  const a = r.attempts[0];
  assert.equal(a.first_failure.stage, 'frame_submission');
  assert.equal(a.logged_setup_checks_passed, false);
  assert.equal(a.submission_markers, 1);
});

test('separate initialization attempts cannot combine into a successful setup', () => {
  const r = analyzeVrLog([...setup.slice(0, 4), ...setup.slice(0, 1), ...setup.slice(4)].join('\n'));
  assert.equal(r.attempts.length, 2);
  assert.ok(r.attempts.every(a => !a.logged_setup_checks_passed));
  assert.ok(r.attempts[1].missing_setup_stages.includes('hmd_open'));
});

test('negative handles and failed enable call remain failures even with later success lines', () => {
  for (const replacement of [
    ['vr: sceHmd2Open -> 0xffffffff; device status 2 worn 0 lens 0 mm', 'hmd_open', 2],
    ['vr: EnableVrMode -> 0x8a720002, VR port 0x00000001', 'enable_vr_mode', 5],
  ]) {
    const lines = [...setup]; lines[replacement[2]] = replacement[0];
    const a = analyzeVrLog(lines.join('\n')).attempts[0];
    assert.equal(a.first_failure.stage, replacement[1]);
    assert.equal(a.logged_setup_checks_passed, false);
  }
});

test('unknown, truncated and out-of-order setup messages never establish completeness', () => {
  for (const lines of [
    [...setup.slice(0, 7), 'vr: 120 Hz output 0x0000'],
    [setup[1], setup[0], ...setup.slice(2)],
    [...setup, 'vr: RegisterBuffers2 -> 0x000000000'],
    [...setup, 'vr: sceHmd2Open -> 0x00000001; device status 99999999999999999999 worn 0 lens 0 mm'],
  ]) assert.equal(analyzeVrLog(lines.join('\n')).attempts[0].logged_setup_checks_passed, false);
  assert.deepEqual(analyzeVrLog('unrelated log').attempts, []);
});

test('module resolution failure and log prefixes survive parsing without copying addresses', () => {
  const a = analyzeVrLog('time vr: libSceHmd2.sprx: no export table for this firmware (base hidden)\n' + setup[0]).attempts[0];
  assert.equal(a.first_failure.stage, 'setup_failure');
  assert.equal(JSON.stringify(a).includes('base hidden'), false);
});

test('bounded input and explicit provenance reject invalid evidence labels', () => {
  assert.throws(() => analyzeVrLog('x'.repeat(MAX_LOG_BYTES + 1)));
  assert.throws(() => analyzeVrLog('\n'.repeat(10001)));
  assert.throws(() => analyzeVrLog('x'.repeat(4097)));
  assert.throws(() => analyzeVrLog(Array(129).fill(setup[0]).join('\n')));
  assert.throws(() => analyzeVrLog('', { firmware: '13.20 supported' }));
  assert.throws(() => analyzeVrLog('', { origin: 'hardware_verified' }));
});
