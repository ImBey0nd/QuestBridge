import dgram from 'node:dgram';
import {randomBytes} from 'node:crypto';
import {pathToFileURL} from 'node:url';
import {Type,encode,decode,decodeTracking,keyFromHex,now} from '../../common/protocol/wire.mjs';
export async function createHost({key,address='127.0.0.1',port=0,timeoutMs=2000}={}) {
 if(!Buffer.isBuffer(key)||key.length!==32) throw Error('32-byte key required');
 const socket=dgram.createSocket('udp4'),stats={accepted:0,rejected:0,tracking:0,expired:0,gaps:0,video:0,wireBytes:0};
 let peer=null,tx=0n,lastTracking=null,lastVideo=null;
 const send=(type,payload=Buffer.alloc(0))=>socket.send(encode({type,session:peer.session,sequence:++tx,payload},key),peer.port,peer.address);
 socket.on('message',(bytes,rinfo)=>{
  try {
   stats.wireBytes+=bytes.length;
   const p=decode(bytes,key);
   if(p.type===Type.HELLO) {
    if(p.session!==0n||p.payload.length!==16) throw Error('hello');
    if(peer) {
     if(peer.address!==rinfo.address||peer.port!==rinfo.port||!p.payload.equals(peer.nonce)) throw Error('busy');
    } else {
     peer={address:rinfo.address,port:rinfo.port,session:randomBytes(8).readBigUInt64BE()||1n,nonce:Buffer.from(p.payload),rx:0n,last:now()};
    }
    send(Type.WELCOME,peer.nonce);return;
   }
   if(!peer||peer.address!==rinfo.address||peer.port!==rinfo.port||peer.session!==p.session||p.sequence<=peer.rx) throw Error('session/replay');
   let tracking=null;
   if(p.type===Type.TRACKING) tracking=decodeTracking(p.payload);
   else if(p.type===Type.PING) {if(p.payload.length!==8) throw Error('ping');}
   else if(p.type===Type.VIDEO) {if(p.payload.length!==24) throw Error('synthetic stereo frame');}
   else throw Error('direction');
   if(peer.rx&&p.sequence>peer.rx+1n) stats.gaps+=Number(p.sequence-peer.rx-1n);
   peer.rx=p.sequence;peer.last=now();stats.accepted++;
   if(tracking){lastTracking=tracking;stats.tracking++;}
   if(p.type===Type.VIDEO) {lastVideo=Buffer.from(p.payload);stats.video++;}
   if(p.type===Type.PING) {
    const payload=Buffer.alloc(24);p.payload.copy(payload);payload.writeBigUInt64BE(now(),8);payload.writeBigUInt64BE(now(),16);send(Type.PONG,payload);
   }
  } catch {stats.rejected++;}
 });
 await new Promise((resolve,reject)=>{socket.once('error',reject);socket.bind(port,address,()=>{socket.removeListener('error',reject);resolve();});});
 socket.on('error',e=>{stats.socketError=e.message;});
 const timer=setInterval(()=>{if(peer&&now()-peer.last>BigInt(timeoutMs)*1000n){peer=null;stats.expired++;}},50);
 return {address:socket.address(),stats,get lastTracking(){return lastTracking;},get lastVideo(){return lastVideo;},
 close:()=>new Promise(resolve=>{clearInterval(timer);socket.close(resolve);})};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 const host=await createHost({key:keyFromHex(process.env.QB_KEY),address:process.env.QB_BIND??'127.0.0.1',port:Number(process.env.QB_PORT??42420)});
 console.log(JSON.stringify({event:'listening',...host.address}));
 const timer=setInterval(()=>console.log(JSON.stringify({event:'telemetry',...host.stats})),1000);
 process.on('SIGINT',async()=>{clearInterval(timer);await host.close();});
}
