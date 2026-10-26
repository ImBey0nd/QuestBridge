#pragma once
#include <string_view>
namespace qb {
// Fail closed: no runtime feature has hardware evidence yet, including requested 13.20.
struct FirmwareCapabilities {bool identified=false,tracking_injection=false,stereo_capture=false,module_abi_verified=false;};
inline FirmwareCapabilities firmware_capabilities(std::string_view) {return {};}
}
