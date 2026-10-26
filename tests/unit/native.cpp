#include "protocol/tracking.hpp"
#include "video/frame.hpp"
#include "ps5/firmware/capabilities.hpp"
#include "ps5/runtime/bridge.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#define CHECK(x) do {if(!(x))throw std::runtime_error(#x);}while(false)
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}CHECK(rejected);}
int main(int argc,char** argv){
 try{
  qb::Tracking t;for(auto&d:t.devices)d.flags=15;
  auto b=qb::encode_tracking(t);
  CHECK(b.size()==156);CHECK(b[24]==0x3f&&b[25]==0x80);
  auto decoded=qb::decode_tracking(b.data(),b.size());CHECK(decoded.devices[2].flags==15);
  for(size_t i=0;i<156;i++)rejects([&]{qb::decode_tracking(b.data(),i);});
  t.devices[1].trigger=std::numeric_limits<float>::quiet_NaN();rejects([&]{qb::encode_tracking(t);});
  t.devices[1].trigger=0;t.devices[1].flags=16;rejects([&]{qb::encode_tracking(t);});
  const float s=std::sqrt(0.5f);
  qb::Calibration recenter({{2,1,3},{0,s,0,s}});
  const auto p=recenter.apply({{3,1,3},{0,s,0,s}});
  CHECK(std::abs(p.position.x)<0.0001f&&std::abs(p.position.z-1)<0.0001f&&std::abs(p.orientation.w-1)<0.0001f);
  qb::FrameAssembler a;a.begin(5,2,100);CHECK(a.add(5,1,{3,4},110));CHECK(!a.add(5,1,{3,4},120));
  CHECK(!a.add(6,0,{1},120));CHECK(!a.complete());CHECK(a.add(5,0,{1,2},130));
  CHECK(a.take()==std::vector<uint8_t>({1,2,3,4}));rejects([&]{a.take();});
  a.begin(6,2,0);CHECK(!a.add(6,0,{1},50001));rejects([&]{a.begin(7,4097,0);});
  const auto f=qb::synthetic_stereo(3);CHECK(f.left.size()==12&&f.right[0]==3);
  CHECK(!qb::firmware_capabilities("13.20").tracking_injection);CHECK(!qb::firmware_capabilities("unknown").stereo_capture);
  qb::UnavailablePs5Runtime runtime;CHECK(!runtime.submit_tracking({}));
  if(argc==2){std::ifstream fixture(argv[1],std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(fixture)),{});CHECK(bytes.size()==156);auto golden=qb::decode_tracking(bytes.data(),bytes.size());CHECK(golden.devices[0].pose.position.y>1.59f);CHECK(qb::encode_tracking(golden)==bytes);}
  std::cout<<"native validation, calibration, frame assembly, firmware gates passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
