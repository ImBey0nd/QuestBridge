import {randomBytes} from 'node:crypto';
import {createHost} from './host.mjs';
import {createSimulator} from '../simulator/main.mjs';
const key=randomBytes(32),host=await createHost({key});
let sim;const started=performance.now();
try {
 sim=await createSimulator({key,port:host.address.port,video:true});
 await new Promise(r=>setTimeout(r,1500));
 const elapsedMs=performance.now()-started;
 console.log(JSON.stringify({elapsedMs,trackingHz:host.stats.tracking*1000/elapsedMs,receivedMbps:host.stats.wireBytes*8/elapsedMs/1000,event:'loopback_demo',host:host.stats,simulator:sim.stats,syntheticStereoOnly:true},null,2));
 if(host.stats.tracking<30||host.stats.video<30||!sim.stats.pongs) process.exitCode=1;
} finally {if(sim)await sim.close();await host.close();}
