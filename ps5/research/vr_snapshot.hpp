// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace qb::research {
// Offline source profiles, NOT firmware ABIs. Never dereference a recorded VA.
// References and limitations: docs/research/PSVR2_NATIVE_METHOD.md.
enum class LayerProfile { ps5vr_registration, anyps5_plain, anyps5_buffered };
struct Region { uint64_t address; std::vector<uint8_t> bytes; };
class SnapshotMemory {
 std::vector<Region> regions_;
public:
 explicit SnapshotMemory(std::vector<Region> regions):regions_(std::move(regions)) {
  if(regions_.size()>64)throw std::invalid_argument("too many regions");
  size_t total=0;
  for(size_t i=0;i<regions_.size();++i){
   const auto& r=regions_[i];
   if(r.bytes.empty() || r.address>UINT64_MAX-r.bytes.size() ||
      r.bytes.size()>1048576-total)throw std::invalid_argument("snapshot range/size");
   total+=r.bytes.size();
   for(size_t j=0;j<i;++j){const auto& s=regions_[j];
    if(r.address<s.address+s.bytes.size() && s.address<r.address+r.bytes.size())
     throw std::invalid_argument("overlapping snapshot regions");
   }
  }
 }
 std::vector<uint8_t> read(uint64_t address,size_t size)const {
  if(size>1048576 || address>UINT64_MAX-size)throw std::invalid_argument("read overflow");
  for(const auto& r:regions_){
   if(address>=r.address && address-r.address<=r.bytes.size() &&
      size<=r.bytes.size()-static_cast<size_t>(address-r.address)){
    auto begin=r.bytes.begin()+static_cast<size_t>(address-r.address);
    return {begin,begin+size};
   }
  }
  throw std::invalid_argument("missing snapshot range");
 }
};
inline uint64_t le(const std::vector<uint8_t>& bytes,size_t offset,size_t width){
 if(width>8 || offset>bytes.size() || width>bytes.size()-offset)
  throw std::invalid_argument("integer outside snapshot");
 uint64_t value=0;
 for(size_t i=0;i<width;++i)value|=uint64_t(bytes[offset+i])<<(i*8);
 return value;
}
struct LayerSnapshot {
 LayerProfile profile;
 size_t stride;
 std::vector<std::vector<uint8_t>> opaque_layers;
 std::vector<uint8_t> opaque_option;
 // This describes metadata only. No texture address, pixels, or GPU permission.
};
inline LayerSnapshot snapshot_layers(const SnapshotMemory& memory,uint64_t parameter,LayerProfile profile){
 const bool registration=profile==LayerProfile::ps5vr_registration;
 if(!registration && profile!=LayerProfile::anyps5_plain && profile!=LayerProfile::anyps5_buffered)
  throw std::invalid_argument("unknown source profile");
 const auto header=memory.read(parameter,registration?16:24);
 const uint64_t count=le(header,8,4), layers=le(header,0,8);
 // Research-tool bound, not a claimed Sony layer limit. Also rejects negative int32.
 if(count>16 || (count && !layers))throw std::invalid_argument("layer count/pointer");
 const size_t stride=registration?304:profile==LayerProfile::anyps5_plain?328:472;
 LayerSnapshot out{profile,stride,{},{}};
 if(count){
  const auto bytes=memory.read(layers,static_cast<size_t>(count)*stride);
  for(size_t i=0;i<count;++i)out.opaque_layers.emplace_back(bytes.begin()+i*stride,bytes.begin()+(i+1)*stride);
 }
 if(!registration){const auto option=le(header,16,8);if(option)out.opaque_option=memory.read(option,128);}
 return out;
}
}
