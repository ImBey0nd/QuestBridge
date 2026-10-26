#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
namespace qb {
enum class Codec { raw_rgb8, h264, hevc };
struct VideoFormat {uint32_t eye_width=0,eye_height=0,refresh_millihz=0;Codec codec=Codec::raw_rgb8;};
struct StereoFrame {uint64_t id=0,capture_us=0;VideoFormat format;std::vector<uint8_t> left,right;};
inline StereoFrame synthetic_stereo(uint64_t id) {
 StereoFrame f{id,0,{2,2,90000,Codec::raw_rgb8},{},{}};f.left.assign(12,255);f.right.assign(12,static_cast<uint8_t>(id));return f;
}
// One in-flight frame: deliberately bounded, expires incomplete frames, no codec implementation.
class FrameAssembler {
 uint64_t id_=0,started_=0;size_t expected_=0,received_=0;
 std::vector<std::vector<uint8_t>> chunks_;
public:
 void begin(uint64_t id,size_t chunks,uint64_t now_us) {
  if(!chunks||chunks>4096)throw std::invalid_argument("chunk count");
  id_=id;started_=now_us;expected_=chunks;received_=0;chunks_.assign(chunks,{});
 }
 bool add(uint64_t id,size_t index,const std::vector<uint8_t>& data,uint64_t now_us) {
  if(!expected_||id!=id_||index>=expected_||data.empty()||data.size()>1024)return false;
  if(now_us<started_||now_us-started_>50000){expected_=0;chunks_.clear();return false;}
  if(!chunks_[index].empty())return false;
  chunks_[index]=data;++received_;return true;
 }
 bool complete()const{return expected_&&received_==expected_;}
 std::vector<uint8_t> take(){
  if(!complete())throw std::logic_error("incomplete");
  std::vector<uint8_t> out;for(const auto& c:chunks_)out.insert(out.end(),c.begin(),c.end());
  expected_=0;chunks_.clear();return out;
 }
};
}
