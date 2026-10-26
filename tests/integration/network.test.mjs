import {test} from 'node:test';
import assert from 'node:assert/strict';
import dgram from 'node:dgram';
import {createHost} from '../../desktop/diagnostics/host.mjs';
import {createSimulator} from '../../desktop/simulator/main.mjs';
import {Type,encode,decode,encodeTracking,synthetic} from '../../common/protocol/wire.mjs';
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,ms=4000){const end=Date.now()+ms;while(!fn()){if(Date.now()>end)throw Error('condition timed out');await sleep(20);}}
test('real UDP pairing, tracking, stereo, clock, peer expiry and reconnect',async()=>{
 const key=Buffer.alloc(32,8),host=await createHost({key,timeoutMs:600});let sim;
 try{
  sim=await createSimulator({key,port:host.address.port,video:true});
  await until(()=>host.stats.tracking>=10&&host.stats.video>=10&&sim.stats.pongs>=1);
  assert.ok(sim.stats.rttUs>=0);assert.equal(host.lastVideo.length,24);assert.equal(host.lastTracking.length,3);
  await sim.close();sim=null;
  await until(()=>host.stats.expired===1);
  sim=await createSimulator({key,port:host.address.port});
  await until(()=>sim.stats.welcomes===1&&sim.stats.pongs>=1);
  assert.equal(host.stats.rejected,0);
 }finally{if(sim)await sim.close();await host.close();}
});
test('replay, wrong key and malformed tracking never advance accepted state',async()=>{
 const key=Buffer.alloc(32,9),host=await createHost({key}),s=dgram.createSocket('udp4');
 try{
  const welcome=new Promise(resolve=>s.once('message',b=>resolve(decode(b,key))));
  s.send(encode({type:Type.HELLO,payload:Buffer.alloc(16,3)},key),host.address.port,'127.0.0.1');
  const p=await Promise.race([welcome,sleep(2000).then(()=>{throw Error('no welcome');})]);
  const packet=encode({type:Type.TRACKING,session:p.session,sequence:2n,payload:encodeTracking(synthetic())},key);
  s.send(packet,host.address.port,'127.0.0.1');await until(()=>host.stats.tracking===1);
  s.send(packet,host.address.port,'127.0.0.1');
  s.send(encode({type:Type.TRACKING,session:p.session,sequence:3n,payload:Buffer.alloc(156)},key),host.address.port,'127.0.0.1');
  s.send(encode({type:Type.PING,session:p.session,sequence:4n,payload:Buffer.alloc(8)},Buffer.alloc(32)),host.address.port,'127.0.0.1');
  await until(()=>host.stats.rejected===3);assert.equal(host.stats.accepted,1);
 }finally{s.close();await host.close();}
});
