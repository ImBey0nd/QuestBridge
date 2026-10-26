#pragma once
#include "tracking/model.hpp"
namespace qb {
enum class RuntimeStatus { unavailable, ready };
class VrRuntime {
public:
 virtual ~VrRuntime()=default;
 virtual RuntimeStatus status()const=0;
 virtual bool submit_tracking(const Tracking&)=0;
};
// Explicit offline test double; contains no Sony interfaces or game integration.
class MockVrRuntime final:public VrRuntime {
public:
 unsigned submissions=0;
 RuntimeStatus status()const override{return RuntimeStatus::ready;}
 bool submit_tracking(const Tracking&)override{++submissions;return true;}
};
class UnavailablePs5Runtime final:public VrRuntime {
public:
 RuntimeStatus status()const override{return RuntimeStatus::unavailable;}
 bool submit_tracking(const Tracking&)override{return false;}
};
}
