// Cross-language real UDP tests. Synthetic poses only; no device or game evidence.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { randomBytes } from 'node:crypto';
import dgram from 'node:dgram';
import { once } from 'node:events';
import { mkdtemp, writeFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { createSimulator } from '../../desktop/simulator/main.mjs';
import { encode, decode, encodeTracking, synthetic, Type } from '../../common/protocol/wire.mjs';
const executable = process.argv[2];
if (!executable) throw Error('Native receiver executable required');
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(predicate, timeout = 5000) {
  const end = Date.now() + timeout;
  while (!predicate()) { if (Date.now() > end) throw Error('Native UDP condition timed out'); await sleep(20); }
}
async function host(key, { duration = 3600, port = 0 } = {}) {
  const env = { ...process.env, QB_KEY: key.toString('hex'), QB_BIND: '127.0.0.1', QB_PORT: String(port), QB_RUN_MS: String(duration) };
  delete env.QB_CONFIG;
  const child = spawn(executable, [], { env, stdio: ['ignore', 'pipe', 'pipe'] });
  let pending = '', error = '', listening, summary;
  child.stdout.setEncoding('utf8'); child.stderr.setEncoding('utf8');
  child.stdout.on('data', data => {
    pending += data;
    for (;;) {
      const end = pending.indexOf('\n'); if (end < 0) break;
      const line = JSON.parse(pending.slice(0, end)); pending = pending.slice(end + 1);
      if (line.event === 'listening') listening = line;
      if (line.event === 'summary') summary = line;
    }
  });
  child.stderr.on('data', data => { error += data; });
  const exited = new Promise((resolve, reject) => {
    child.once('error', reject);
    child.once('exit', (code, signal) => code === 0 ? resolve() : reject(Error(`Native receiver exited ${code ?? signal}: ${error}`)));
  });
  // Attach immediately so early process errors cannot become unhandled rejections.
  exited.catch(() => {});
  try { await Promise.race([until(() => listening, 3000), exited.then(() => { throw Error('Receiver exited before listening'); })]); }
  catch (error) { child.kill(); await exited.catch(() => {}); throw error; }
  return {
    port: listening.port,
    async finish() { await exited; assert.ok(summary); return summary; },
    async stop() { if (child.exitCode === null) child.kill('SIGINT'); await exited; return summary; },
  };
}
for (const settings of [
  { QB_KEY: 'invalid' }, { QB_PORT: '65536' }, { QB_RUN_MS: '0' }, { QB_BIND: 'not-an-address' },
]) {
  const child = spawn(executable, [], { env: { ...process.env, QB_KEY: randomBytes(32).toString('hex'), QB_RUN_MS: '1', ...settings }, stdio: ['ignore', 'pipe', 'pipe'] });
  let stdout = ''; child.stdout.on('data', d => { stdout += d; });
  child.stderr.resume();
  const [code] = await once(child, 'exit');
  assert.equal(code, 2); assert.equal(stdout, '');
}
{
  const folder = await mkdtemp(join(tmpdir(), 'qb-private-config-'));
  try {
    const file = join(folder, 'receiver.conf');
    await writeFile(file, `key=${randomBytes(32).toString('hex')}\nport=0\nrun_ms=50\n`, { mode: 0o600 });
    const env = { ...process.env, QB_CONFIG: file };
    for (const name of ['QB_KEY', 'QB_BIND', 'QB_PORT', 'QB_RUN_MS']) delete env[name];
    const child = spawn(executable, [], { env, stdio: ['ignore', 'pipe', 'pipe'] });
    let stdout = ''; child.stdout.on('data', d => { stdout += d; }); child.stderr.resume();
    const [code] = await once(child, 'exit'); assert.equal(code, 0);
    const lines = stdout.trim().split('\n').map(line => JSON.parse(line));
    assert.equal(lines[0].event, 'listening'); assert.ok(lines[0].port > 0);
    assert.equal(lines[1].accepted, 0); assert.equal(stdout.includes('key='), false);
  } finally { await rm(folder, { recursive: true, force: true }); }
}
const key = randomBytes(32);
{
  const native = await host(key); let simulator;
  try {
    simulator = await createSimulator({ key, port: native.port, video: true });
    await until(() => simulator.stats.sent >= 30 && simulator.stats.pongs >= 2);
    await simulator.close(); simulator = null;
    const result = await native.finish();
    assert.ok(result.tracking >= 30 && result.video >= 30);
    assert.equal(result.rejected, 0); assert.equal(result.expired, 1);
    assert.equal(result.snapshot_fresh, false); assert.equal(result.paired, false);
    assert.equal(result.vr_runtime, 'unavailable');
  } finally { if (simulator) await simulator.close(); await native.stop(); }
}
{
  const native = await host(key, { duration: 10000 }); const socket = dgram.createSocket('udp4');
  const other = dgram.createSocket('udp4');
  const send = (s, bytes) => new Promise((resolve, reject) => s.send(bytes, native.port, '127.0.0.1', e => e ? reject(e) : resolve()));
  try {
    const reply = once(socket, 'message');
    await send(socket, encode({ type: Type.HELLO, payload: randomBytes(16) }, key));
    const [welcome] = await Promise.race([reply, sleep(2000).then(() => { throw Error('No native welcome'); })]);
    const { session } = decode(welcome, key);
    const packet = sequence => encode({ type: Type.TRACKING, session, sequence, payload: encodeTracking(synthetic()) }, key);
    await send(socket, packet(2n));
    await send(socket, encode({ type: Type.TRACKING, session, sequence: 3n, payload: Buffer.alloc(156) }, key));
    await send(socket, encode({ type: Type.PING, session, sequence: 3n, payload: Buffer.alloc(8) }, randomBytes(32)));
    await send(socket, packet(3n)); await send(socket, packet(3n));
    await send(other, packet(4n)); await send(socket, Buffer.alloc(4096));
    await sleep(150);
    const result = await native.stop();
    assert.equal(result.tracking, 2); assert.equal(result.accepted, 2); assert.equal(result.rejected, 5);
  } finally { socket.close(); other.close(); await native.stop(); }
}
{
  let native = await host(key, { duration: 10000 }); let simulator;
  try {
    const port = native.port;
    simulator = await createSimulator({ key, port });
    await until(() => simulator.stats.pongs >= 1);
    await native.stop(); native = null;
    native = await host(key, { duration: 10000, port });
    await until(() => simulator.stats.welcomes >= 2 && simulator.stats.pongs >= 2, 5000);
    assert.ok(simulator.stats.reconnects >= 1);
  } finally { if (simulator) await simulator.close(); if (native) await native.stop(); }
}
console.log('Native/Node UDP pairing, HMAC, tracking/video, expiry, malformed/replay rejection and receiver restart passed');
