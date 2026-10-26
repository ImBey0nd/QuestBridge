import {mkdirSync,writeFileSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {encodeTracking,synthetic} from '../../common/protocol/wire.mjs';
mkdirSync('build',{recursive:true});
writeFileSync('build/tracking-golden.bin',encodeTracking(synthetic()));
const executable=process.argv[2]??(process.platform==='win32'?'build/qb_native_tests.exe':'build/qb_native_tests');
const r=spawnSync(executable,['build/tracking-golden.bin'],{stdio:'inherit'});
if(r.error)throw r.error;
process.exitCode=r.status??1;
