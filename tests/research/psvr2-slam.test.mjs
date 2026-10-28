import test from 'node:test';
import assert from 'node:assert/strict';
import { decodeSlamRecord } from '../../tools/diagnostics/psvr2-slam.mjs';
function fixture() {
  // Invented bytes, not a console or headset capture.
  const bytes = Buffer.alloc(512);
  bytes.write('SLA'); bytes[3] = 1; bytes.writeUInt32LE(512, 4);
  bytes.writeUInt32LE(0xfffffff0, 8); bytes.writeUInt32LE(3, 12);
  [1, 2, -3, 1, 0, 0, 0].forEach((x, i) => bytes.writeFloatLE(x, 16 + 4 * i));
  return bytes;
}
test('native wire pose preserves unknown fields without inventing validity or clock alignment', () => {
  const bytes = fixture(); bytes.writeUInt32LE(99, 12);
  const result = decodeSlamRecord(bytes);
  assert.deepEqual(result.position_native, [1, 2, -3]);
  assert.deepEqual(result.orientation_native_wxyz, [1, 0, 0, 0]);
  assert.equal(result.timestamp_us_raw, 0xfffffff0);
  assert.equal(result.unknown1_raw, 99);
  assert.equal(result.tracking_validity, 'unknown');
  assert.equal(result.hardware_verified, false);
});
test('every truncation and appended bytes fail; header fields are mandatory', () => {
  const bytes = fixture();
  for (let i = 0; i < 512; ++i) assert.throws(() => decodeSlamRecord(bytes.subarray(0, i)));
  assert.throws(() => decodeSlamRecord(Buffer.concat([bytes, Buffer.alloc(1)])));
  for (const offset of [0, 1, 2, 3, 4, 5, 6, 7]) {
    const bad = Buffer.from(bytes); bad[offset] ^= 0xff;
    assert.throws(() => decodeSlamRecord(bad));
  }
});
test('nonfinite values and invalid rotations cannot become accepted poses', () => {
  for (const offset of [16, 20, 24, 28, 32, 36, 40]) {
    for (const value of [NaN, Infinity, -Infinity]) {
      const bad = fixture(); bad.writeFloatLE(value, offset);
      assert.throws(() => decodeSlamRecord(bad));
    }
  }
  const bad = fixture(); bad.fill(0, 28, 44);
  assert.throws(() => decodeSlamRecord(bad));
});
