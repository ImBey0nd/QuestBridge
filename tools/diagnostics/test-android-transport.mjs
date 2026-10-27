// SPDX-License-Identifier: GPL-3.0-or-later
// Tests the exact Android Java transport on a host JVM using synthetic poses.
import { spawn, spawnSync } from 'node:child_process';
import { mkdtemp, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { randomBytes } from 'node:crypto';
import { createSocket } from 'node:dgram';
import { Type, encode, decode } from '../../common/protocol/wire.mjs';
import assert from 'node:assert/strict';
import { createHost } from '../../desktop/diagnostics/host.mjs';
const directory=await mkdtemp(join(tmpdir(),'qb-java-transport-'));
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
const until=async predicate=>{const end=Date.now()+10000;while(!predicate()){if(Date.now()>end)throw Error('Java transport condition timed out');await sleep(20);}};
const jvmTool=name=>process.env.JAVA_HOME?join(process.env.JAVA_HOME,'bin',name):name;
let host,child,exited,exitResult,adversary,attackTimer,welcomeTimer,native,nativeExited,nativeExitResult,output='';
try {
 const compile=spawnSync(jvmTool('java'),['--module','jdk.compiler/com.sun.tools.javac.Main','-source','17','-target','17','-d',directory,
  resolve('headset/android-openxr/app/src/main/java/org/questbridge/probe/QuestTransport.java'),
  resolve('tests/android/TransportHarness.java')],{encoding:'utf8'});
 assert.equal(compile.status,0,compile.stderr||compile.error?.message);
 const key=randomBytes(32);host=await createHost({key});const port=host.address.port;
 child=spawn(jvmTool('java'),['-cp',directory,'TransportHarness',String(port),key.toString('hex')]);
 exited=new Promise((resolve,reject)=>{child.on('error',reject);child.on('close',(code,signal)=>{exitResult={code,signal};resolve(exitResult);});});
 child.stdout.on('data',data=>{output+=data.toString();});child.stderr.on('data',data=>{output+=data.toString();});
 await until(()=>host.stats.tracking>=5&&host.lastTracking[0].flags===15);
 assert.equal(host.stats.rejected,0);assert.ok(host.lastTracking[0].position[1]>1.59);
 await until(()=>output.includes('FIXTURE_STALE'));
 await until(()=>host.lastTracking[0].flags===0);
 assert.ok(host.lastTracking.every(d=>d.flags===0&&d.buttons===0&&d.trigger===0));
 await until(()=>output.includes('FIXTURE_RESTART'));
 await host.close();host=null;
 await sleep(700);
 host=await createHost({key,port});
 await until(()=>host.stats.tracking>=5&&host.lastTracking[0].flags===15);
 // Old-session packets are expected to be rejected during the timeout window.
 const rejectedAfterPair=host.stats.rejected, acceptedAfterPair=host.stats.tracking;
 await sleep(150);
 assert.equal(host.stats.rejected,rejectedAfterPair);
 assert.ok(host.stats.tracking>acceptedAfterPair);
 const result=await Promise.race([exited,sleep(10000).then(()=>{throw Error('Java shutdown timed out');})]);
 assert.equal(result.code,0,output);
 assert.ok(output.includes('reconnecting'));
 assert.equal((output.match(/Receiver paired/g)||[]).length,2);
 await host.close();host=null;
 // A peer knowing the key still cannot substitute an unrelated nonce or ping.
 adversary=createSocket('udp4');
 await new Promise(resolve=>adversary.bind(0,'127.0.0.1',resolve));
 const adversarialPort=adversary.address().port, nonces=new Set();
 let trackingBeforeWelcome=0,trackingAfterWelcome=0,welcomed=false,first=true,sequence=10n;
 adversary.on('message',(bytes,remote)=>{
  const incoming=decode(bytes,key);
  const send=packet=>adversary.send(packet,remote.port,remote.address);
  if(incoming.type===Type.TRACKING){if(welcomed)trackingAfterWelcome++;else trackingBeforeWelcome++;return;}
  if(incoming.type!==Type.HELLO)return;
  nonces.add(incoming.payload.toString('hex'));
  if(!first)return;first=false;
  const wrongMac=encode({type:Type.WELCOME,session:11n,sequence:1n,payload:incoming.payload},key);wrongMac[wrongMac.length-1]^=1;send(wrongMac);
  send(encode({type:Type.WELCOME,session:11n,sequence:1n,payload:randomBytes(16)},key));
  send(encode({type:Type.WELCOME,session:0n,sequence:1n,payload:incoming.payload},key));
  send(encode({type:Type.WELCOME,session:11n,sequence:0n,payload:incoming.payload},key));
  welcomeTimer=setTimeout(()=>{
   welcomed=true;send(encode({type:Type.WELCOME,session:11n,sequence:1n,payload:incoming.payload},key));
   attackTimer=setInterval(()=>send(encode({type:Type.PONG,session:11n,sequence:++sequence,payload:Buffer.alloc(24)},key)),50);
  },600);
 });
 output='';exitResult=null;
 child=spawn(jvmTool('java'),['-cp',directory,'TransportHarness',String(adversarialPort),key.toString('hex')]);
 exited=new Promise((resolve,reject)=>{child.on('error',reject);child.on('close',(code,signal)=>{exitResult={code,signal};resolve(exitResult);});});
 child.stdout.on('data',data=>{output+=data.toString();});child.stderr.on('data',data=>{output+=data.toString();});
 const adversarialResult=await Promise.race([exited,sleep(12000).then(()=>{throw Error('Adversarial transport shutdown timed out');})]);
 assert.equal(adversarialResult.code,0,output);
 assert.equal(trackingBeforeWelcome,0);
 assert.ok(trackingAfterWelcome>0);
 assert.equal((output.match(/Receiver paired/g)||[]).length,1);
 assert.ok(nonces.size>=2,'Invalid ping echoes kept a dead session alive');
 if(process.argv[2]){
  let nativeOutput='',listening,summary;
  const env={...process.env,QB_KEY:key.toString('hex'),QB_BIND:'127.0.0.1',QB_PORT:'0',QB_RUN_MS:'15000'};delete env.QB_CONFIG;
  native=spawn(resolve(process.argv[2]),[],{env});native.stderr.resume();
  native.stdout.on('data',bytes=>{nativeOutput+=bytes;for(const line of nativeOutput.split('\n').slice(0,-1)){
   const record=JSON.parse(line);if(record.event==='listening')listening=record;if(record.event==='summary')summary=record;
  }});
  nativeExited=new Promise((resolve,reject)=>{native.on('error',reject);native.on('close',(code,signal)=>{nativeExitResult={code,signal};resolve(nativeExitResult);});});
  await until(()=>listening);
  output='';exitResult=null;
  child=spawn(jvmTool('java'),['-cp',directory,'TransportHarness',String(listening.port),key.toString('hex')]);
  exited=new Promise((resolve,reject)=>{child.on('error',reject);child.on('close',(code,signal)=>{exitResult={code,signal};resolve(exitResult);});});
  child.stdout.on('data',data=>{output+=data.toString();});child.stderr.on('data',data=>{output+=data.toString();});
  const javaResult=await exited;assert.equal(javaResult.code,0,output);
  native.kill('SIGINT');const nativeResult=await nativeExited;
  assert.equal(nativeResult.code,0);assert.ok(summary.tracking>=20);assert.equal(summary.rejected,0);
  assert.equal(summary.vr_runtime,'unavailable');
  console.log('Android Java sender and native C++ receiver interoperated over authenticated UDP');
 }
 console.log('Android adversarial receive: wrong MAC, nonce, session, sequence and ping echo rejected');
 console.log('Android Java transport: authenticated pairing, tracking, stale neutralization, receiver restart and shutdown passed');
} finally {
 if(child&&!exitResult){child.kill('SIGTERM');await exited;}
 if(host)await host.close();
 if(native&&!nativeExitResult){native.kill('SIGINT');await nativeExited;}
 clearInterval(attackTimer);clearTimeout(welcomeTimer);
 if(adversary)await new Promise(resolve=>adversary.close(resolve));
 await rm(directory,{recursive:true,force:true});
}
