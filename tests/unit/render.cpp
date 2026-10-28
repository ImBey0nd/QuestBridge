// SPDX-License-Identifier: GPL-3.0-or-later
#include "math/render.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void near(float a,float b) {if(std::abs(a-b)>0.0001f)throw std::runtime_error("render math mismatch");}
std::array<float,4> point(const qb::render::Matrix& m,std::array<float,4> p) {
 std::array<float,4> out{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)out[r]+=m.v[c*4+r]*p[c];return out;
}
}
int main(){try{
 const float s=std::sqrt(0.5f);
 const qb::Pose camera{{2,1,3},{0,s,0,s}};
 const auto identity=qb::render::multiply(qb::render::view(camera),qb::render::transform(camera));
 for(int c=0;c<4;++c)for(int r=0;r<4;++r)near(identity.v[c*4+r],c==r?1.0f:0.0f);
 // Asymmetric frustum edges must project to the matching NDC corners.
 const float l=-0.7f,r=0.9f,d=-0.6f,u=0.8f;
 const auto projection=qb::render::projection(l,r,d,u);
 for(float z:{-0.05f,-1.0f,-100.0f}){
  const auto left=point(projection,{-z*std::tan(l),-z*std::tan(d),z,1});
  const auto right=point(projection,{-z*std::tan(r),-z*std::tan(u),z,1});
  near(left[0]/left[3],-1);near(left[1]/left[3],-1);
  near(right[0]/right[3],1);near(right[1]/right[3],1);
 }
 const auto n=point(projection,{0,0,-0.05f,1}),f=point(projection,{0,0,-100,1});
 near(n[2]/n[3],-1);near(f[2]/f[3],1);
 bool rejected=false;try{qb::render::projection(1,-1,-1,1);}catch(const std::invalid_argument&){rejected=true;}
 if(!rejected)throw std::runtime_error("inverted frustum accepted");
 std::cout<<"render view inverse and asymmetric stereo projection passed\n";
 return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
