#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace qb {
struct Vec3 { float x=0,y=0,z=0; };
struct Quat { float x=0,y=0,z=0,w=1; };
inline Quat multiply(Quat a,Quat b) {
 return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
 a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
 a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
 a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline Quat inverse(Quat q) {return {-q.x,-q.y,-q.z,q.w};}
inline Vec3 rotate(Quat q,Vec3 p) {
 auto r=multiply(multiply(q,{p.x,p.y,p.z,0}),inverse(q));return {r.x,r.y,r.z};
}
inline bool valid(Quat q) {
 // Match Node's validation of decoded float32 values using double arithmetic.
 const double n=double(q.x)*q.x+double(q.y)*q.y+double(q.z)*q.z+double(q.w)*q.w;
 return std::isfinite(n)&&std::abs(n-1)<=0.02;
}
inline bool finite(Vec3 p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline Quat normalized(Quat q) {
 if(!valid(q))throw std::invalid_argument("quaternion");
 const double n=std::sqrt(double(q.x)*q.x+double(q.y)*q.y+double(q.z)*q.z+double(q.w)*q.w);
 return {float(q.x/n),float(q.y/n),float(q.z/n),float(q.w/n)};
}
struct Pose { Vec3 position; Quat orientation; };
// Flags: position valid, orientation valid, position tracked, orientation tracked.
struct Device { Pose pose; std::array<float,2> stick{}; float trigger=0,grip=0; uint32_t buttons=0,flags=0; };
struct Tracking { std::array<Device,3> devices; }; // head, left grip, right grip
struct Capabilities { bool pose=false,controllers=false,controller_haptics=false,eye_tracking=false,head_haptics=false,adaptive_triggers=false; };
struct HapticRequest { uint32_t hand=0; float amplitude=0,frequency_hz=0; uint64_t duration_us=0; };
class Calibration {
 Pose origin_;
public:
 explicit Calibration(Pose origin):origin_(origin) {
  if(!finite(origin.position))throw std::invalid_argument("calibration position");
  origin_.orientation=normalized(origin.orientation);
 }
 Pose apply(Pose p) const {
  if(!finite(p.position))throw std::invalid_argument("pose position");
  p.orientation=normalized(p.orientation);
  const auto q=inverse(origin_.orientation);
  return {rotate(q,{p.position.x-origin_.position.x,p.position.y-origin_.position.y,p.position.z-origin_.position.z}),multiply(q,p.orientation)};
 }
};
class HeadsetBackend {
public:
 virtual ~HeadsetBackend()=default;
 virtual Capabilities capabilities() const=0;
 virtual bool sample(uint64_t predicted_time_us,Tracking& output)=0;
};
}
