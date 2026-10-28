// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
extern "C" {
#include "ps5/host/observer/observer.h"
}
namespace {
std::array<unsigned char,12> prefix{0x48,0xc7,0xc0,0x14,0,0,0,0x49,0x89,0xca,0x0f,0x05};
unsigned calls=0,writes=0;long pid=123,writeLimit=1000;std::string output;
struct Exited {int status;};
void require(bool value){if(!value)throw std::runtime_error("observer host contract failed");}
}
extern "C" long qb_observer_getpid(const unsigned char* stub){require(stub==prefix.data());++calls;return pid;}
extern "C" long qb_observer_write(const unsigned char* stub,int fd,const char* bytes,unsigned long length){
 require(stub==prefix.data()+10 && fd==1);++writes;
 if(writeLimit<=0)return writeLimit;
 auto count=length<static_cast<unsigned long>(writeLimit)?length:static_cast<unsigned long>(writeLimit);
 output.append(bytes,count);return static_cast<long>(count);
}
extern "C" QB_OBSERVER_NORETURN void qb_observer_exit(const unsigned char* stub,int status){require(stub==prefix.data()+10);throw Exited{status};}
int main(){try{
 qb_observer_args args{prefix.data()};
 require(qb_observe(nullptr)==-1);qb_observer_args null{};require(qb_observe(&null)==-1);
 for(size_t i=0;i<prefix.size();++i){prefix[i]^=0xff;require(qb_observe(&args)==-2);prefix[i]^=0xff;}
 require(calls==0 && writes==0);
 pid=0;require(qb_observe(&args)==-3);require(writes==0);pid=123;
 writeLimit=-1;require(qb_observe(&args)==-4);
 writeLimit=0;require(qb_observe(&args)==-4);
 writeLimit=3;require(qb_observe(&args)==0);
 require(output=="QuestBridge observer v1: startup=custom; self_pid=positive; vr=unavailable\n");
 require(args.getpid_stub==prefix.data());
 for(int status:{0,-1,-4}){
  try{qb_observer_finish(&args,status);require(false);}catch(const Exited& exit){require(exit.status==(status==0?0:1));}
 }
 std::cout<<"observer rejects unsupported instruction profiles and handles short/error writes (inert host callbacks only)\n";
 return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
