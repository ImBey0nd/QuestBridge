// SPDX-License-Identifier: GPL-3.0-or-later
// Real host software encoding experiment. Synthetic input; no PS5 capture/integration.
import { spawn, spawnSync } from 'node:child_process';
import { once } from 'node:events';
import { access, mkdtemp, readFile, realpath, stat } from 'node:fs/promises';
import { constants } from 'node:fs';
import { tmpdir } from 'node:os';
import { delimiter, join } from 'node:path';
import { createHash } from 'node:crypto';
import { performance } from 'node:perf_hooks';
import { pathToFileURL } from 'node:url';

function integer(n, low, high) { if (!Number.isInteger(n) || n < low || n > high) throw Error('Experiment bounds'); return n; }
export function experimentOptions(input = {}) {
  const o = { eyeWidth: 320, height: 240, frames: 60, fps: 90, threads: 2, pattern: 'bars', ...input };
  integer(o.eyeWidth, 16, 2048); integer(o.height, 16, 2048); integer(o.frames, 8, 120);
  integer(o.fps, 30, 120); integer(o.threads, 1, 8);
  if (o.eyeWidth % 2 || o.height % 2 || !['bars', 'noise'].includes(o.pattern)) throw Error('Even dimensions and bars|noise required');
  return o;
}
export function makeFrame(o, id) {
  const width = o.eyeWidth * 2, plane = width * o.height;
  const frame = Buffer.alloc(plane * 3 / 2);
  let random = 0x6d2b79f5 ^ id;
  if (o.pattern === 'bars') {
    for (let x = 0; x < width; ++x) frame[x] = (x < o.eyeWidth ? 64 : 176) + ((x + id * 7) % 64 < 16 ? 24 : 0);
    for (let y = 1; y < o.height; ++y) frame.copy(frame, y * width, 0, width);
  } else {
    for (let y = 0; y < o.height; ++y) for (let x = 0; x < width; ++x) {
      random ^= random << 13; random ^= random >>> 17; random ^= random << 5;
      frame[y * width + x] = (x < o.eyeWidth ? 64 : 176) + (random >>> 0) % 32;
    }
  }
  frame.fill(110, plane, plane * 5 / 4); frame.fill(146, plane * 5 / 4);
  return frame;
}
export function parsePacket(line) {
  if (!line.trim() || line.startsWith('#')) return null;
  const fields = line.split(',').map(s => s.trim());
  if (fields.length < 6 || fields.length > 24 || !fields.slice(0, 5).every(s => /^-?\d+$/.test(s)) || !/^0x[0-9a-f]+$/i.test(fields[5])) throw Error('Unexpected framecrc packet record');
  let tail = fields.slice(6);
  if (/^F=0x[0-9a-f]+$/i.test(tail[0])) tail = tail.slice(1);
  if (tail.length) {
    const side = /^S=(\d+)$/.exec(tail[0]);
    if (!side || Number(side[1]) !== tail.length - 1 || !tail.slice(1).every(s => /^\d+$/.test(s) && Number(s) <= 1048576))
      throw Error('Invalid framecrc side data');
  }
  const [stream, dts, pts, duration, bytes] = fields.slice(0, 5).map(Number);
  if (![stream, dts, pts, duration, bytes].every(Number.isSafeInteger) || stream !== 0 || duration !== 1 || bytes <= 0 || bytes > 16 * 1024 * 1024) throw Error('Invalid encoded packet');
  return { dts, pts, bytes };
}
function command(binary, args, maxBuffer = 1024 * 1024) {
  const r = spawnSync(binary, args, { maxBuffer, timeout: 30000, windowsHide: true });
  if (r.error) throw r.error;
  if (r.status !== 0) throw Error(`${binary}: ${r.stderr.toString().slice(-2000)}`);
  return r.stdout;
}
const percentile = (values, q) => [...values].sort((a, b) => a - b)[Math.ceil(values.length * q) - 1];
async function executable(name) {
  for (const directory of (process.env.PATH ?? '').split(delimiter)) {
    if (!directory) continue;
    const candidate = join(directory, name);
    try { await access(candidate, constants.X_OK); if ((await stat(candidate)).isFile()) return await realpath(candidate); } catch { /* Try next PATH entry. */ }
  }
  throw Error(`Missing executable: ${name}`);
}

export async function encodeExperiment(input = {}) {
  const o = experimentOptions(input), width = o.eyeWidth * 2;
  const ffmpeg = await executable('ffmpeg'), ffprobe = await executable('ffprobe');
  const directory = await mkdtemp(join(tmpdir(), 'questbridge-encode-'));
  const output = join(directory, 'synthetic-stereo.h264');
  const version = command(ffmpeg, ['-version']).toString().split('\n')[0];
  const ffmpegSha256 = createHash('sha256').update(await readFile(ffmpeg)).digest('hex');
  const args = ['-hide_banner', '-loglevel', 'error', '-probesize', '32', '-analyzeduration', '0',
    '-f', 'rawvideo', '-pixel_format', 'yuv420p', '-video_size', `${width}x${o.height}`,
    '-framerate', String(o.fps), '-i', 'pipe:0', '-map', '0:v:0', '-an', '-c:v', 'libx264',
    '-threads', String(o.threads), '-preset', 'ultrafast', '-tune', 'zerolatency', '-crf', '23',
    '-g', String(o.fps), '-bf', '0', '-x264-params', 'rc-lookahead=0:sync-lookahead=0:scenecut=0:aud=1:repeat-headers=1',
    '-frames:v', String(o.frames), '-f', 'tee', `[f=framecrc:flush_packets=1]pipe:1|[f=h264]${output}`];
  const child = spawn(ffmpeg, args, { stdio: ['pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close');
  const started = performance.now(), sent = [], packets = [], latency = [], waiters = [];
  let text = '', stderr = '', failure, eof = false, beforeEof = 0, generationMs = 0, encodedTotal = 0;
  const fail = error => { failure ??= error; child.kill('SIGKILL'); while (waiters.length) waiters.shift()(); };
  child.on('error', fail); child.stdin.on('error', fail);
  child.on('close', code => { if (code !== 0 || packets.length !== o.frames) fail(Error(`Encoder exited early: ${stderr}`)); });
  child.stderr.on('data', chunk => { stderr = (stderr + chunk).slice(-8192); });
  child.stdout.on('data', chunk => {
    try {
      text += chunk;
      if (text.length > 32768) throw Error('Oversized packet log');
      let end;
      while ((end = text.indexOf('\n')) >= 0) {
        const line = text.slice(0, end); text = text.slice(end + 1);
        const packet = parsePacket(line); if (!packet) continue;
        encodedTotal += packet.bytes;
        if (encodedTotal > 128 * 1024 * 1024) throw Error('Encoded output exceeds 128 MiB experiment limit');
        const id = packets.length;
        if (id >= o.frames || sent[id] === undefined || packet.pts !== id || packet.dts !== id) throw Error('Packet identity/order mismatch');
        packets.push(packet); latency.push(performance.now() - sent[id]);
        if (!eof) ++beforeEof;
        while (waiters.length) waiters.shift()();
      }
    } catch (e) { fail(e); }
  });
  const timer = setTimeout(() => fail(Error('Encoder experiment timed out')), 60000);
  try {
    for (let id = 0; id < o.frames; ++id) {
      // Two frames at most await encoder output. Do not manufacture throughput
      // by building an unbounded raw-input queue. No target-cadence claim.
      while (!failure && sent.length - packets.length >= 2) await new Promise(resolve => waiters.push(resolve));
      if (failure) throw failure;
      const generateStart = performance.now();
      const frame = makeFrame(o, id); // Generation time excluded from per-packet latency, included in wall throughput.
      generationMs += performance.now() - generateStart;
      sent.push(performance.now());
      await new Promise((resolve, reject) => child.stdin.write(frame, error => error ? reject(error) : resolve()));
    }
    eof = true; child.stdin.end();
    const [code] = await closed;
    if (failure) throw failure;
    if (code !== 0) throw Error(`ffmpeg failed: ${stderr}`);
    if (packets.length !== o.frames || beforeEof < o.frames - 2 || text.trim()) throw Error('Encoder buffered/missed frames');
    const wallMs = performance.now() - started;
    const probe = JSON.parse(command(ffprobe, ['-v', 'error', '-show_frames', '-show_streams', '-show_entries',
      'frame=pict_type:stream=width,height,codec_name,has_b_frames', '-of', 'json', output], 2 * 1024 * 1024));
    const stream = probe.streams[0];
    if (probe.frames.length !== o.frames || stream.width !== width || stream.height !== o.height || stream.has_b_frames !== 0 ||
        probe.frames.some(f => f.pict_type === 'B')) throw Error('Decode count/format/reordering check failed');
    const decoded = command(ffmpeg, ['-v', 'error', '-i', output, '-frames:v', '1', '-pix_fmt', 'yuv420p', '-f', 'rawvideo', 'pipe:1'], 16 * 1024 * 1024);
    if (decoded.length !== width * o.height * 3 / 2) throw Error('Decoded frame size');
    const expected = makeFrame(o, 0); let leftError = 0, rightError = 0, leftMean = 0, rightMean = 0;
    for (let y = 0; y < o.height; ++y) for (let x = 0; x < width; ++x) {
      const at = y * width + x, error = Math.abs(decoded[at] - expected[at]);
      if (x < o.eyeWidth) { leftError += error; leftMean += decoded[at]; }
      else { rightError += error; rightMean += decoded[at]; }
    }
    const pixels = o.eyeWidth * o.height;
    if (rightMean / pixels - leftMean / pixels < 70 || Math.max(leftError, rightError) / pixels > 12) throw Error('Stereo eye content check failed');
    const steady = latency.slice(4);
    return {
      schema_version: 1, evidence_kind: 'host_software_encode_synthetic_input', options: o,
      ffmpeg_version: version, ffmpeg_executable_sha256: ffmpegSha256, ffmpeg_path: ffmpeg, ffprobe_path: ffprobe, command: [ffmpeg, ...args],
      input_format: 'side_by_side_yuv420p', max_pending_frames: 2, encoded_frames: packets.length,
      packets_observed_before_input_eof: beforeEof, decoded_frames: probe.frames.length, b_frames: 0,
      wall_ms: wallMs, wall_fps_including_generation_startup_and_drain: o.frames * 1000 / wallMs,
      generation_ms: generationMs,
      packet_latency_ms: { first: latency[0], median_after_first_4: percentile(steady, 0.5), p95_after_first_4: percentile(steady, 0.95), max: Math.max(...latency) },
      latency_definition: 'Start of raw frame write to receipt of completed encoded-packet checksum through tee/framecrc pipe; includes pipe/queue/mux observation. Not motion-to-photon.',
      decoded_first_frame_luma_mae: { left: leftError / pixels, right: rightError / pixels },
      encoded_bytes: (await stat(output)).size, output_sha256: createHash('sha256').update(await readFile(output)).digest('hex'), output,
      ps5_capture_verified: false, ps5_encoder_verified: false, quest_decode_verified: false, runtime_bindings_enabled: false,
    };
  } finally { clearTimeout(timer); if (child.exitCode === null) child.kill('SIGKILL'); }
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const options = {};
  for (const arg of process.argv.slice(2)) {
    const match = /^--(eyeWidth|height|frames|fps|threads|pattern)=(.+)$/.exec(arg);
    if (!match || Object.hasOwn(options, match[1])) throw Error('Unknown/duplicate experiment option');
    options[match[1]] = match[1] === 'pattern' ? match[2] : Number(match[2]);
  }
  encodeExperiment(options).then(r => process.stdout.write(JSON.stringify(r, null, 2) + '\n')).catch(e => { process.stderr.write(e.message + '\n'); process.exitCode = 2; });
}
