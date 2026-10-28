// SPDX-License-Identifier: GPL-3.0-or-later
#include "observer.h"
#if defined(_MSC_VER)
#include <stdlib.h>
#endif

// Instruction profile corroborated by the pinned SDK syscall initializer:
// mov rax,20; mov r10,rcx; syscall. This is checked before any indirect call.
// This does not validate arbitrary hostile pointers: the loader is trusted to
// supply valid mapped argument/code memory. No firmware address is embedded.
static const unsigned char getpid_prefix[12] = {
 0x48,0xc7,0xc0,0x14,0,0,0,0x49,0x89,0xca,0x0f,0x05
};
int qb_observe(const struct qb_observer_args* args) {
 if(!args || !args->getpid_stub)return -1;
 const unsigned char* stub=args->getpid_stub;
 for(unsigned long i=0;i<sizeof(getpid_prefix);++i)if(stub[i]!=getpid_prefix[i])return -2;
 if(qb_observer_getpid(stub)<=0)return -3;
 static const char report[]="QuestBridge observer v1: startup=custom; self_pid=positive; vr=unavailable\n";
 unsigned long written=0;
 while(written<sizeof(report)-1){
  // The validated instruction sequence puts syscall at byte 10. The call
  // executes the existing libkernel stub, not a syscall instruction in payload text.
  const long result=qb_observer_write(stub+10,1,report+written,sizeof(report)-1-written);
  if(result<=0 || (unsigned long)result>sizeof(report)-1-written)return -4;
  written+=(unsigned long)result;
 }
 return 0;
}

QB_OBSERVER_NORETURN void qb_observer_finish(const struct qb_observer_args* args,int status) {
 // Returning into the placeholder application's original entry is not a
 // valid standalone termination policy. Exit only through a validated stub.
 if(args && args->getpid_stub){
  const unsigned char* stub=args->getpid_stub;
  unsigned long i=0;
  for(;i<sizeof(getpid_prefix);++i)if(stub[i]!=getpid_prefix[i])break;
  if(i==sizeof(getpid_prefix))qb_observer_exit(stub+10,status==0?0:1);
 }
 // No safe syscall origin was established. End this standalone observation
 // process rather than calling an unverified pointer or resuming unknown code.
 #if defined(_MSC_VER)
 abort();
 #else
 __builtin_trap();
 #endif
}
