// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "tracking/model.hpp"
namespace qb::render {
struct Matrix {float v[16]{};};
inline Matrix multiply(const Matrix& a,const Matrix& b) {
 Matrix c;
 for(int col=0;col<4;++col)for(int row=0;row<4;++row)for(int k=0;k<4;++k)c.v[col*4+row]+=a.v[k*4+row]*b.v[col*4+k];
 return c;
}
inline Matrix transform(qb::Pose p) {
 const auto q=p.orientation;
 Matrix m;
 m.v[0]=1-2*(q.y*q.y+q.z*q.z);m.v[1]=2*(q.x*q.y+q.w*q.z);m.v[2]=2*(q.x*q.z-q.w*q.y);
 m.v[4]=2*(q.x*q.y-q.w*q.z);m.v[5]=1-2*(q.x*q.x+q.z*q.z);m.v[6]=2*(q.y*q.z+q.w*q.x);
 m.v[8]=2*(q.x*q.z+q.w*q.y);m.v[9]=2*(q.y*q.z-q.w*q.x);m.v[10]=1-2*(q.x*q.x+q.y*q.y);
 m.v[12]=p.position.x;m.v[13]=p.position.y;m.v[14]=p.position.z;m.v[15]=1;
 return m;
}
inline Matrix view(qb::Pose p) {
 const auto q=qb::inverse(p.orientation);
 return transform({qb::rotate(q,{-p.position.x,-p.position.y,-p.position.z}),q});
}
inline Matrix projection(float left,float right,float down,float up) {
 const float l=std::tan(left),r=std::tan(right),d=std::tan(down),u=std::tan(up);
 if(!std::isfinite(l)||!std::isfinite(r)||!std::isfinite(d)||!std::isfinite(u)||r<=l||u<=d)throw std::invalid_argument("view FOV");
 Matrix m;m.v[0]=2/(r-l);m.v[5]=2/(u-d);m.v[8]=(r+l)/(r-l);m.v[9]=(u+d)/(u-d);
 m.v[10]=-(100.0f+0.05f)/(100.0f-0.05f);m.v[11]=-1;m.v[14]=-2*100.0f*0.05f/(100.0f-0.05f);return m;
}
}
