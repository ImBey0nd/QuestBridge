// SPDX-License-Identifier: GPL-3.0-or-later
#include "ps5/research/vr_snapshot.hpp"
#include <charconv>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static uint64_t number(const char* s){
 const std::string text(s);uint64_t value=0;
 const auto result=std::from_chars(text.data(),text.data()+text.size(),value,16);
 if(result.ec!=std::errc() || result.ptr!=text.data()+text.size() || text.empty())throw std::invalid_argument("expected hexadecimal number without 0x");
 return value;
}
int main(int argc,char** argv){
 try{
#ifdef _WIN32
  if(_setmode(_fileno(stdin),_O_BINARY)==-1)throw std::runtime_error("cannot select binary stdin");
#endif
  if(argc!=4)throw std::invalid_argument("Usage: qb_vr_snapshot registration|plain|buffered BASE_HEX PARAMETER_HEX < snapshot.bin");
  using namespace qb::research;
  const std::string name(argv[1]);
  const auto profile=name=="registration"?LayerProfile::ps5vr_registration:name=="plain"?LayerProfile::anyps5_plain:name=="buffered"?LayerProfile::anyps5_buffered:throw std::invalid_argument("unknown source profile");
  std::vector<uint8_t> bytes;char c;
  while(std::cin.get(c)){if(bytes.size()==1048576)throw std::invalid_argument("snapshot exceeds 1 MiB");bytes.push_back(static_cast<uint8_t>(c));}
  if(std::cin.bad())throw std::runtime_error("snapshot input read failed");
  const auto result=snapshot_layers(SnapshotMemory({{number(argv[2]),std::move(bytes)}}),number(argv[3]),profile);
  std::cout<<"{\"evidence_kind\":\"offline_source_profile_metadata\",\"layer_count\":"<<result.opaque_layers.size()
   <<",\"stride\":"<<result.stride<<",\"option_bytes\":"<<result.opaque_option.size()
   <<",\"pixels_captured\":false,\"firmware_13_20_verified\":false,\"runtime_bindings_enabled\":false}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
