import {test} from 'node:test';
import assert from 'node:assert/strict';
import {Type,encode,decode,encodeTracking,decodeTracking,synthetic,clockEstimate,keyFromHex} from '../../common/protocol/wire.mjs';
const key=Buffer.alloc(32,7);
test('stable big endian header and tracking roundtrip',()=>{
 const p=encode({type:Type.TRACKING,session:42n,sequence:9n,time:123n,payload:encodeTracking(synthetic())},key);
 assert.equal(p.subarray(0,12).toString('hex'),'5142524700010003009c0000');
 const d=decode(p,key);assert.equal(d.session,42n);assert.equal(d.sequence,9n);assert.equal(d.time,123n);
 assert.equal(decodeTracking(d.payload)[0].orientation[3],1);
});
test('reject every truncation, corruption, trailing data, bad key',()=>{
 const p=encode({type:Type.TRACKING,payload:encodeTracking(synthetic())},key);
 for(let i=0;i<p.length;i++)assert.throws(()=>decode(p.subarray(0,i),key));
 for(let i=0;i<p.length;i++){const b=Buffer.from(p);b[i]^=1;assert.throws(()=>decode(b,key));}
 assert.throws(()=>decode(Buffer.concat([p,Buffer.alloc(1)]),key));
 assert.throws(()=>decode(p,Buffer.alloc(32)));
});
test('semantic validation rejects NaN, scale, range, flag and button errors',()=>{
 for(const mutate of [d=>d[0].position[0]=NaN,d=>d[0].position[0]=1001,d=>d[0].orientation[3]=2,d=>d[1].trigger=1.1,d=>d[1].stick[0]=-1.1,d=>d[2].flags=16,d=>d[1].buttons=256]){
  const d=synthetic();mutate(d);assert.throws(()=>encodeTracking(d));
 }
 assert.throws(()=>decodeTracking(Buffer.alloc(155)));
});
test('clock sample uses all four timestamps',()=>{
 assert.deepEqual(clockEstimate(100n,130n,140n,130n),{rtt:20n,offset:20n});
 assert.throws(()=>clockEstimate(100n,130n,120n,110n));
});
test('pairing key must be strong length and valid encoding',()=>{
 assert.throws(()=>keyFromHex('password'));assert.equal(keyFromHex('ff'.repeat(32)).length,32);
});
test('bounded deterministic malformed datagram corpus',()=>{
 let seed=7;
 for(let n=0;n<2000;n++){const b=Buffer.alloc(n%1250);for(let i=0;i<b.length;i++){seed=(Math.imul(seed,1664525)+1013904223)>>>0;b[i]=seed&255;}assert.throws(()=>decode(b,key));}
});
