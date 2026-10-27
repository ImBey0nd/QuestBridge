// SPDX-License-Identifier: GPL-3.0-or-later
// Original offline decoder of the public Monado SLAM wire layout.
// Layout reference: docs/research/PSVR2_CONNECTION.md. No USB/device access.
import { pathToFileURL } from 'node:url';
export const RECORD_BYTES = 512;
export function decodeSlamRecord(bytes) {
  if (!Buffer.isBuffer(bytes) || bytes.length !== RECORD_BYTES) throw Error('Expected one 512-byte SLAM record');
  if (bytes.toString('ascii', 0, 3) !== 'SLA' || bytes[3] !== 1 || bytes.readUInt32LE(4) !== RECORD_BYTES) throw Error('Unsupported SLAM header');
  const position = Array.from({ length: 3 }, (_, i) => bytes.readFloatLE(16 + i * 4));
  const orientationWxyz = Array.from({ length: 4 }, (_, i) => bytes.readFloatLE(28 + i * 4));
  if (![...position, ...orientationWxyz].every(Number.isFinite)) throw Error('Nonfinite SLAM pose');
  const norm = orientationWxyz.reduce((sum, value) => sum + value * value, 0);
  if (Math.abs(norm - 1) > 0.02) throw Error('Nonunit SLAM orientation');
  return {
    timestamp_us_raw: bytes.readUInt32LE(8),
    unknown1_raw: bytes.readUInt32LE(12),
    position_native: position,
    orientation_native_wxyz: orientationWxyz,
    // No guessed confidence bits, clock alignment, sensor-to-eye calibration or Sony ABI.
    tracking_validity: 'unknown',
    evidence_kind: 'offline_usb_payload_decode',
    hardware_verified: false,
  };
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  if (process.argv.length !== 3) {
    process.stderr.write('Usage: node tools/diagnostics/psvr2-slam.mjs RECORD.bin\n'); process.exitCode = 2;
  } else {
    // Read a fixed-size buffer rather than allocating an arbitrary input file.
    const { open } = await import('node:fs/promises');
    let file;
    try {
      file = await open(process.argv[2], 'r');
      const bytes = Buffer.alloc(RECORD_BYTES + 1);
      let size = 0;
      while (size < bytes.length) {
        const result = await file.read(bytes, size, bytes.length - size, null);
        if (!result.bytesRead) break;
        size += result.bytesRead;
      }
      process.stdout.write(JSON.stringify(decodeSlamRecord(bytes.subarray(0, size)), null, 2) + '\n');
    } catch (error) {
      const known = ['Expected one 512-byte SLAM record', 'Unsupported SLAM header', 'Nonfinite SLAM pose', 'Nonunit SLAM orientation'];
      process.stderr.write((known.includes(error.message) ? error.message : 'Could not read SLAM record') + '\n');
      process.exitCode = 2;
    } finally { await file?.close(); }
  }
}
