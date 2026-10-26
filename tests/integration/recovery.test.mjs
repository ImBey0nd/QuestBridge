import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHost} from '../../desktop/diagnostics/host.mjs';
import {createSimulator} from '../../desktop/simulator/main.mjs';
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
async function until(fn,ms=5000){const end=Date.now()+ms;while(!fn()){if(Date.now()>end)throw Error('recovery timed out');await sleep(20);}}
test('running simulator obtains a fresh session after receiver restart',async()=>{
 const key=Buffer.alloc(32,10);
 let host=await createHost({key}),sim;
 const port=host.address.port;
 try{
  sim=await createSimulator({key,port});
  await until(()=>sim.stats.pongs>=1&&host.stats.tracking>=3);
  await host.close();host=null;
  host=await createHost({key,port});
  await until(()=>sim.stats.welcomes>=2&&sim.stats.reconnects>=1&&host.stats.tracking>=3);
  assert.ok(host.lastTracking);
 }finally{if(sim)await sim.close();if(host)await host.close();}
});
