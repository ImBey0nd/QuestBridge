#!/usr/bin/env bash
# Offline build only. Does not deploy, load, or execute the target ELF.
set -euo pipefail
if [[ $# != 1 || "$1" != /* ]]; then
 echo 'Usage: QB_CLANG=/path/clang QB_LLD=/path/ld.lld bash tools/build/ps5-observer.sh /absolute/output' >&2
 exit 2
fi
: "${QB_CLANG:=clang}"
: "${QB_LLD:=ld.lld}"
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
mkdir -p -- "$1"
out=$(cd -- "$1" && pwd)
case "$out/" in "$root/"*) echo 'Keep target artifacts outside the checkout.' >&2; exit 2;; esac
"$QB_CLANG" --version > "$out/compiler.txt"
"$QB_LLD" --version > "$out/linker.txt"
flags=(-target x86_64-unknown-freebsd -ffreestanding -fno-builtin -fPIC -mno-red-zone -fno-stack-protector -fvisibility=hidden -O2 -Wall -Wextra -Werror)
"$QB_CLANG" "${flags[@]}" -c "$root/ps5/host/observer/observer.c" -o "$out/observer.o"
"$QB_CLANG" "${flags[@]}" -c "$root/ps5/host/observer/entry.S" -o "$out/entry.o"
"$QB_LLD" -m elf_x86_64 -pie --no-dynamic-linker -z max-page-size=0x4000 -z separate-code -z noexecstack --entry=_start --build-id=none "$out/entry.o" "$out/observer.o" -o "$out/questbridge-observer.elf"
python3 - "$out/questbridge-observer.elf" "$out/audit.json" <<'PY_AUDIT'
import sys,struct,hashlib,json
from pathlib import Path
if not __debug__:raise RuntimeError('ELF audit requires Python assertions enabled')
artifact=Path(sys.argv[1]);data=artifact.read_bytes()
assert data[:6]==b'\x7fELF\x02\x01'
assert struct.unpack_from('<HH',data,16)==(3,62)
entry,phoff=struct.unpack_from('<QQ',data,24)
entsize,count=struct.unpack_from('<HH',data,54)
loads=[]
for i in range(count):
 kind,flags,offset,address,_,filesz,memsz,align=struct.unpack_from('<IIQQQQQQ',data,phoff+i*entsize)
 assert kind!=3,'No interpreter allowed'
 if kind==1:
  assert address%0x4000==0 and offset%0x4000==0 and align==0x4000
  assert flags&3!=3,'No writable/executable load segment allowed'
  loads.append((address,memsz,flags))
 if kind==2:
  for j in range(offset,offset+filesz,16):
   tag,value=struct.unpack_from('<QQ',data,j)
   if tag==0:break
   assert tag!=1,'No dynamic dependencies allowed'
   assert tag not in (2,7,8,17,18,23,35,36),'No runtime relocations allowed'
assert any(address<=entry<address+size and flags&1 for address,size,flags in loads)
Path(sys.argv[2]).write_text(json.dumps({
 'artifact_sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),
 'kind':'standalone_observer_candidate','sdk_crt_linked':False,
 'dynamic_dependencies':0,'runtime_relocations':0,'load_segments_16k_aligned':True,
 'hardware_tested':False,'execution_readiness':'unverified_candidate',
 'profile':'new standalone process only; never game/system-process injection'
},indent=2)+'\n')
PY_AUDIT
readelf -h -l -d -r -s "$out/questbridge-observer.elf" > "$out/elf-inspection.txt"
objdump -d "$out/questbridge-observer.elf" > "$out/disassembly.txt"
sha256sum "$root/ps5/host/observer/observer.c" "$root/ps5/host/observer/observer.h" "$root/ps5/host/observer/entry.S" "$out/questbridge-observer.elf" > "$out/SHA256SUMS"
echo 'Built standalone observer candidate; no SDK CRT linked. Not hardware verified.'
