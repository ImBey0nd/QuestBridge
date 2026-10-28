import test from 'node:test';
import assert from 'node:assert/strict';
import { experimentOptions, makeFrame, parsePacket } from '../../tools/diagnostics/encode-stereo.mjs';
test('stereo encoder input bounds and explicit eye content', () => {
  const o = experimentOptions({eyeWidth: 32, height: 16});
  const f = makeFrame(o, 1);
  assert.equal(f.length, 64 * 16 * 3 / 2);
  assert.ok(f[0] < 100 && f[32] > 170);
  assert.notDeepEqual(f, makeFrame(o, 2));
  for (const input of [{eyeWidth: 2049}, {height: 17}, {frames: 1000}, {threads: 0}, {fps: Infinity}, {pattern: 'game'}])
    assert.throws(() => experimentOptions(input));
});
test('completed packet records preserve PTS and reject malformed/unbounded sizes', () => {
  assert.deepEqual(parsePacket('0, 7, 7, 1, 256, 0x123abc'), {dts: 7, pts: 7, bytes: 256});
  assert.deepEqual(parsePacket('0, 7, 7, 1, 256, 0x123abc, F=0x0, S=1, 8'), {dts: 7, pts: 7, bytes: 256});
  assert.equal(parsePacket('#tb 0: 1/90'), null);
  for (const line of ['0,0,0,1,0,0x00','0,0,0,1,999999999,0x00','0,0,0,1,20,no','0,0,0,1','1,0,0,1,20,0x00','0,0,0,1,20,0x00,S=2,8'])
    assert.throws(() => parsePacket(line));
});
