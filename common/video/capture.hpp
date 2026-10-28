// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace qb::video {
// Project-owned CPU data types. No Sony texture descriptors or GPU operations.
struct LinearRgba { uint32_t width,height; size_t pitch; std::vector<uint8_t> bytes; };
struct Rect { uint32_t x,y,width,height; };
struct StereoCopy { uint32_t width,height; std::vector<uint8_t> left,right; };
inline std::vector<uint8_t> crop(const LinearRgba& surface,Rect r){
 if(!surface.width || !surface.height || surface.width>8192 || surface.height>8192 ||
    surface.pitch<size_t(surface.width)*4 || surface.pitch>65536 ||
    surface.bytes.size()>64*1024*1024 || surface.height>surface.bytes.size()/surface.pitch ||
    !r.width || !r.height || r.x>surface.width || r.y>surface.height ||
    r.width>surface.width-r.x || r.height>surface.height-r.y ||
    uint64_t(r.width)*r.height*4>32*1024*1024)
  throw std::invalid_argument("linear surface/viewport bounds");
 std::vector<uint8_t> out(size_t(r.width)*r.height*4);
 for(size_t y=0;y<r.height;++y)
  std::memcpy(out.data()+y*r.width*4,surface.bytes.data()+(r.y+y)*surface.pitch+size_t(r.x)*4,size_t(r.width)*4);
 return out;
}
inline StereoCopy copy_stereo(const LinearRgba& left,Rect l,const LinearRgba& right,Rect r){
 if(l.width!=r.width || l.height!=r.height)throw std::invalid_argument("eye dimensions differ");
 return {l.width,l.height,crop(left,l),crop(right,r)};
}

// Offline ordering validator for a future adapter. Calls record observations;
// they do NOT issue waits, establish cache coherence, copy GPU memory or encode.
// Identity includes the surface generation: a recycled slot cannot satisfy an old wait.
class CaptureOrder {
 uint64_t generation_,frame_;
 enum class Stage { producer,ready,copying,copied,encoding,done } stage_=Stage::producer;
 void require(uint64_t generation,uint64_t frame,Stage expected)const {
  if(generation!=generation_ || frame!=frame_ || stage_!=expected)
   throw std::logic_error("capture identity/order mismatch");
 }
public:
 CaptureOrder(uint64_t generation,uint64_t frame):generation_(generation),frame_(frame){}
 void producer_observed(uint64_t g,uint64_t f){require(g,f,Stage::producer);stage_=Stage::ready;}
 void copy_started(uint64_t g,uint64_t f){require(g,f,Stage::ready);stage_=Stage::copying;}
 void copy_observed(uint64_t g,uint64_t f){require(g,f,Stage::copying);stage_=Stage::copied;}
 void encoder_started(uint64_t g,uint64_t f){require(g,f,Stage::copied);stage_=Stage::encoding;}
 void encoder_observed(uint64_t g,uint64_t f){require(g,f,Stage::encoding);stage_=Stage::done;}
 // Capture's read claim only. Compositor/game claims are separate and may remain.
 bool capture_source_read_finished()const{return stage_==Stage::copied || stage_==Stage::encoding || stage_==Stage::done;}
 bool staging_reusable()const{return stage_==Stage::done;}
 // Timeout is not a completion event. In-flight memory must remain quarantined.
};
}
