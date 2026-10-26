import dgram from 'node:dgram';
import {randomBytes} from 'node:crypto';
import {pathToFileURL} from 'node:url';
import {Type,encode,decode,encodeTracking,keyFromHex,now,synthetic,clockEstimate} from '../../common/protocol/wire.mjs';
export async function createSimulator({key,address='127.0.0.1',port,rate=90,video=false}={}) {
 if(!Buffer.isBuffer(key)||key.length!==32||!Number.isFinite(rate)||rate<1||rate>240) throw Error('simulator config');
 const socket=dgram.createSocket('udp4');
 await new Promise((resolve,reject)=>{socket.once('error',reject);socket.connect(port,address,()=>{socket.removeListener('error',reject);resolve();});});
 let session=0n,sequence=0n,rx=0n,lastReply=now(),nonce=randomBytes(16),lastPing=0n;
 const pending=new Set(),stats={welcomes:0,sent:0,pongs:0,rejected:0,reconnects:0,rttUs:null,offsetUs:null};
 const send=(type,payload)=>socket.send(encode({type,session,sequence:++sequence,payload},key));
 socket.on('error',e=>{stats.socketError=e.message;});
 socket.on('message',b=>{
  try{
   const p=decode(b,key);
   if(p.type===Type.WELCOME&&!session&&p.session!==0n&&p.payload.equals(nonce)){
    session=p.session;rx=p.sequence;lastReply=now();stats.welcomes++;return;
   }
   if(p.session!==session||!session||p.sequence<=rx||p.type!==Type.PONG||p.payload.length!==24) throw Error('reply');
   const t0=p.payload.readBigUInt64BE(0);
   if(!pending.delete(t0.toString())) throw Error('unsolicited pong');
   const sample=clockEstimate(t0,p.payload.readBigUInt64BE(8),p.payload.readBigUInt64BE(16),now());
   rx=p.sequence;lastReply=now();stats.pongs++;stats.rttUs=Number(sample.rtt);stats.offsetUs=Number(sample.offset);
  }catch{stats.rejected++;}
 });
 const tick=()=>{
  const t=now();
  if(session&&t-lastReply>1500000n){session=0n;nonce=randomBytes(16);pending.clear();stats.reconnects++;}
  if(!session){send(Type.HELLO,nonce);return;}
  send(Type.TRACKING,encodeTracking(synthetic(Number(t%100000000n)/1e6)));stats.sent++;
  // Synthetic RAW stereo: 2 eyes x 2x2 RGB pixels, deliberately not a codec/game frame.
  if(video){const pixels=Buffer.alloc(24);pixels.fill(255,0,12);pixels.fill(Number(sequence%256n),12);send(Type.VIDEO,pixels);}
  if(t-lastPing>250000n){lastPing=t;const p=Buffer.alloc(8);p.writeBigUInt64BE(t);pending.add(t.toString());if(pending.size>16)pending.delete(pending.values().next().value);send(Type.PING,p);}
 };
 const timer=setInterval(tick,1000/rate);tick();
 return {stats,close:()=>new Promise(resolve=>{clearInterval(timer);socket.close(resolve);})};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 const sim=await createSimulator({key:keyFromHex(process.env.QB_KEY),address:process.env.QB_HOST??'127.0.0.1',port:Number(process.env.QB_PORT??42420),video:true});
 const timer=setInterval(()=>console.log(JSON.stringify({event:'simulator',...sim.stats})),1000);
 process.on('SIGINT',async()=>{clearInterval(timer);await sim.close();});
}
