import {readFileSync} from 'node:fs';
const load=path=>JSON.parse(readFileSync(path,'utf8').replace(/^\uFEFF/,''));
const db=load('docs/compatibility/games.json');
if(db.schema_version!==1||!Array.isArray(db.games))throw Error('database schema');
for(const game of db.games){
 for(const field of ['name','title_identifier','game_version','ps5_firmware','headset','required_features','launch_status','rendering_status','tracking_status','controller_status','known_issues','evidence','test_date'])if(!(field in game))throw Error('missing '+field);
 for(const field of ['launch_status','rendering_status','tracking_status','controller_status']){
  if(!['not_tested','blocked','partial','working','failed'].includes(game[field]))throw Error('status');
  if(game[field]==='working'&&(!game.evidence.length||!game.test_date))throw Error('working requires dated evidence');
 }
}
const firmware=load('ps5/firmware/13.20.json');
if(!firmware.tested&&Object.values(firmware.features).includes(true))throw Error('unverified feature enabled');
console.log('compatibility records valid');
