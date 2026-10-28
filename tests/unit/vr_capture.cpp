// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5/research/vr_snapshot.hpp"
#include "video/capture.hpp"
#include <cassert>
#include <iostream>
#include <functional>
using namespace qb::research;
static void rejects(const std::function<void()>& f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed);}
static void put(std::vector<uint8_t>& b,size_t off,uint64_t v,size_t n){for(size_t i=0;i<n;++i)b.at(off+i)=uint8_t(v>>(8*i));}
static void profiles(){
 for(const auto p:{LayerProfile::ps5vr_registration,LayerProfile::anyps5_plain,LayerProfile::anyps5_buffered}){
  const bool reg=p==LayerProfile::ps5vr_registration;
  const size_t stride=reg?304:p==LayerProfile::anyps5_plain?328:472;
  std::vector<uint8_t> header(reg?16:24),data(stride*2),option(128,0x5a);
  put(header,0,0x5000,8);put(header,8,2,4);
  if(!reg)put(header,16,0x9000,8);
  data[0]=0x10;data[stride]=0x12;data.back()=0xe7;
  SnapshotMemory mem({{0x1000,header},{0x5000,data},{0x9000,option}});
  const auto result=snapshot_layers(mem,0x1000,p);
  assert(result.opaque_layers.size()==2 && result.stride==stride);
  assert(result.opaque_layers[0][0]==0x10 && result.opaque_layers[1][0]==0x12);
  assert(result.opaque_layers[1].back()==0xe7);
  assert(result.opaque_option.size()==(reg?0:128));
  // Every truncated header and layer region must be rejected, not partially parsed.
  for(size_t n=0;n<header.size();++n)rejects([&]{snapshot_layers(SnapshotMemory({{0x1000,{header.begin(),header.begin()+n}},{0x5000,data},{0x9000,option}}),0x1000,p);});
  for(size_t n=1;n<data.size();++n)rejects([&]{snapshot_layers(SnapshotMemory({{0x1000,header},{0x5000,{data.begin(),data.begin()+n}},{0x9000,option}}),0x1000,p);});
  for(const uint32_t count:{17u,0x7fffffffu,0x80000000u,0xffffffffu}){
   auto bad=header;put(bad,8,count,4);rejects([&]{snapshot_layers(SnapshotMemory({{0x1000,bad}}),0x1000,p);});
  }
  auto bad=header;put(bad,0,0,8);rejects([&]{snapshot_layers(SnapshotMemory({{0x1000,bad}}),0x1000,p);});
  put(bad,8,0,4);if(!reg)put(bad,16,0,8);
  assert(snapshot_layers(SnapshotMemory({{0x1000,bad}}),0x1000,p).opaque_layers.empty());
  if(!reg){option.pop_back();rejects([&]{snapshot_layers(SnapshotMemory({{0x1000,header},{0x5000,data},{0x9000,option}}),0x1000,p);});}
 }
 rejects([]{SnapshotMemory({{UINT64_MAX,{1}}});});
 rejects([]{SnapshotMemory({{8,{1,2}},{9,{3}}});});
 rejects([]{SnapshotMemory({{0,std::vector<uint8_t>(1048577)}});});
 rejects([]{SnapshotMemory({{10,{1}}}).read(UINT64_MAX,2);});
 rejects([]{snapshot_layers(SnapshotMemory({{0,std::vector<uint8_t>(24)}}),0,static_cast<LayerProfile>(99));});
}
static void pixels(){
 using namespace qb::video;
 // Padded rows, asymmetric viewports and distinct eyes; real CPU byte copies.
 LinearRgba s{6,3,32,std::vector<uint8_t>(96,0xee)};
 for(size_t y=0;y<3;++y)for(size_t x=0;x<6;++x)for(size_t c=0;c<4;++c)s.bytes[y*32+x*4+c]=uint8_t(10*y+x);
 const auto pair=copy_stereo(s,{1,1,2,2},s,{4,0,2,2});
 assert(pair.left==std::vector<uint8_t>({11,11,11,11,12,12,12,12,21,21,21,21,22,22,22,22}));
 assert(pair.right==std::vector<uint8_t>({4,4,4,4,5,5,5,5,14,14,14,14,15,15,15,15}));
 s.bytes.assign(96,0);
 assert(pair.left.front()==11 && pair.right.front()==4); // Copies own their bytes after source reuse.
 rejects([&]{crop(s,{UINT32_MAX,0,1,1});});
 rejects([&]{crop(s,{0,0,7,1});});
 rejects([&]{copy_stereo(s,{0,0,1,1},s,{0,0,2,1});});
 s.pitch=0;rejects([&]{crop(s,{0,0,1,1});});
 s.pitch=32;s.bytes.resize(95);rejects([&]{crop(s,{0,0,1,1});});
}
static void ownership(){
 using qb::video::CaptureOrder;
 CaptureOrder a(10,1),b(11,1);
 rejects([&]{a.copy_started(10,1);});
 rejects([&]{a.producer_observed(11,1);});
 rejects([&]{a.producer_observed(10,2);});
 assert(!a.capture_source_read_finished() && !a.staging_reusable());
 a.producer_observed(10,1);a.copy_started(10,1);
 assert(!a.capture_source_read_finished()); // A timeout here must not free memory.
 rejects([&]{a.encoder_started(10,1);});
 a.copy_observed(10,1);
 assert(a.capture_source_read_finished() && !a.staging_reusable());
 a.encoder_started(10,1);assert(!a.staging_reusable());
 a.encoder_observed(10,1);assert(a.staging_reusable());
 rejects([&]{a.copy_observed(10,1);});
 assert(!b.capture_source_read_finished()); // Completion for another generation is irrelevant.
}
int main(){profiles();pixels();ownership();std::cout<<"Offline metadata, CPU stereo copy and observation ordering passed; no PS5/GPU integration\n";}
