import {createHmac,timingSafeEqual} from 'node:crypto';
export const VERSION=1, MAX_PACKET=1200, HEADER=40, TAG=32;
export const Type=Object.freeze({HELLO:1,WELCOME:2,TRACKING:3,PING:4,PONG:5,VIDEO:6});
export const now=()=>process.hrtime.bigint()/1000n;
export function keyFromHex(hex) {
 if(!/^[0-9a-f]{64}$/i.test(hex??'')) throw Error('QB_KEY must be 64 hex digits (32 random bytes)');
 return Buffer.from(hex,'hex');
}
export function encode({type,session=0n,sequence=0n,time=now(),payload=Buffer.alloc(0)},key) {
 if(!Object.values(Type).includes(type)||payload.length>MAX_PACKET-HEADER-TAG) throw Error('packet bounds/type');
 const b=Buffer.alloc(HEADER+payload.length+TAG);
 b.write('QBRG');b.writeUInt16BE(VERSION,4);b.writeUInt16BE(type,6);
 b.writeUInt16BE(payload.length,8);b.writeBigUInt64BE(session,12);
 b.writeBigUInt64BE(sequence,20);b.writeBigUInt64BE(time,28);
 payload.copy(b,HEADER);
 createHmac('sha256',key).update(b.subarray(0,-TAG)).digest().copy(b,b.length-TAG);
 return b;
}
export function decode(b,key) {
 if(!Buffer.isBuffer(b)||b.length<HEADER+TAG||b.length>MAX_PACKET) throw Error('length');
 if(b.toString('ascii',0,4)!=='QBRG'||b.readUInt16BE(4)!==VERSION) throw Error('magic/version');
 const type=b.readUInt16BE(6);
 if(!Object.values(Type).includes(type)||b.readUInt16BE(10)!==0||b.readUInt32BE(36)!==0||b.readUInt16BE(8)!==b.length-HEADER-TAG) throw Error('header');
 const mac=createHmac('sha256',key).update(b.subarray(0,-TAG)).digest();
 if(!timingSafeEqual(mac,b.subarray(-TAG))) throw Error('authentication');
 return {type,session:b.readBigUInt64BE(12),sequence:b.readBigUInt64BE(20),time:b.readBigUInt64BE(28),payload:b.subarray(HEADER,-TAG)};
}
// Three fixed records: head, left grip, right grip. Invalid poses still carry finite identity transforms.
export function encodeTracking(devices) {
 if(devices.length!==3) throw Error('three devices required');
 const b=Buffer.alloc(156);
 devices.forEach((d,i)=>{
  const o=i*52;
  [...d.position,...d.orientation,d.stick[0],d.stick[1],d.trigger,d.grip].forEach((v,j)=>b.writeFloatBE(v,o+j*4));
  b.writeUInt32BE(d.buttons,o+44);b.writeUInt32BE(d.flags,o+48);
 });
 decodeTracking(b);return b;
}
export function decodeTracking(b) {
 if(b.length!==156) throw Error('tracking length');
 return [0,1,2].map(i=>{
  const o=i*52,a=Array.from({length:11},(_,j)=>b.readFloatBE(o+j*4));
  if(a.some(v=>!Number.isFinite(v))||a.slice(0,3).some(v=>Math.abs(v)>1000)) throw Error('nonfinite/position');
  const norm=a.slice(3,7).reduce((s,v)=>s+v*v,0);
  if(Math.abs(norm-1)>0.02||a.slice(7,9).some(v=>Math.abs(v)>1)||a.slice(9).some(v=>v<0||v>1)) throw Error('tracking range');
  const flags=b.readUInt32BE(o+48),buttons=b.readUInt32BE(o+44);
  if(flags&~15||buttons&~255) throw Error('unknown input flags');
  return {position:a.slice(0,3),orientation:a.slice(3,7),stick:a.slice(7,9),trigger:a[9],grip:a[10],buttons,flags};
 });
}
export function clockEstimate(t0,t1,t2,t3) {
 const rtt=(t3-t0)-(t2-t1);
 if(rtt<0n||t3<t0||t2<t1) throw Error('invalid clock sample');
 return {rtt,offset:((t1-t0)+(t2-t3))/2n};
}
export function synthetic(t=0) {
 return [0,1,2].map(i=>({position:[i===0?Math.sin(t)*0.1:(i===1?-0.25:0.25),i===0?1.6:1.2,-0.3],
 orientation:[0,0,0,1],stick:[0,0],trigger:0,grip:0,buttons:0,flags:15}));
}
