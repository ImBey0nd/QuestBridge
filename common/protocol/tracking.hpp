#pragma once
#include "tracking/model.hpp"
#include <cstring>
#include <vector>
namespace qb {
inline void put32(std::vector<uint8_t>& b,uint32_t v){for(int s=24;s>=0;s-=8)b.push_back(static_cast<uint8_t>(v>>s));}
inline uint32_t get32(const uint8_t* b){return (uint32_t(b[0])<<24)|(uint32_t(b[1])<<16)|(uint32_t(b[2])<<8)|b[3];}
inline void putfloat(std::vector<uint8_t>& b,float f){uint32_t v;static_assert(sizeof(f)==4);std::memcpy(&v,&f,4);put32(b,v);}
inline float getfloat(const uint8_t* b){auto v=get32(b);float f;std::memcpy(&f,&v,4);return f;}
inline bool valid(const Device& d) {
 const auto p=d.pose.position;
 if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||std::abs(p.x)>1000||std::abs(p.y)>1000||std::abs(p.z)>1000||!valid(d.pose.orientation))return false;
 for(float v:d.stick)if(!std::isfinite(v)||std::abs(v)>1)return false;
 return std::isfinite(d.trigger)&&std::isfinite(d.grip)&&d.trigger>=0&&d.trigger<=1&&d.grip>=0&&d.grip<=1&&!(d.flags&~15u)&&!(d.buttons&~255u);
}
inline std::vector<uint8_t> encode_tracking(const Tracking& t){
 std::vector<uint8_t>b;b.reserve(156);
 for(const auto& d:t.devices){
  if(!valid(d))throw std::invalid_argument("tracking");
  auto p=d.pose.position;auto q=d.pose.orientation;
  for(float f:{p.x,p.y,p.z,q.x,q.y,q.z,q.w,d.stick[0],d.stick[1],d.trigger,d.grip})putfloat(b,f);
  put32(b,d.buttons);put32(b,d.flags);
 }return b;
}
inline Tracking decode_tracking(const uint8_t* b,size_t size){
 if(size!=156||!b)throw std::invalid_argument("tracking length");
 Tracking t;
 for(size_t i=0;i<3;i++){
  const auto p=b+i*52;auto& d=t.devices[i];
  d.pose={{getfloat(p),getfloat(p+4),getfloat(p+8)},{getfloat(p+12),getfloat(p+16),getfloat(p+20),getfloat(p+24)}};
  d.stick={getfloat(p+28),getfloat(p+32)};d.trigger=getfloat(p+36);d.grip=getfloat(p+40);
  d.buttons=get32(p+44);d.flags=get32(p+48);
  if(!valid(d))throw std::invalid_argument("tracking values");
 }return t;
}
}
