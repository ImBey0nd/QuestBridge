#include <jni.h>
#define XR_USE_PLATFORM_ANDROID
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstring>
#include <string>
#include <vector>
extern "C" JNIEXPORT jstring JNICALL
Java_org_questbridge_probe_MainActivity_probeRuntime(JNIEnv* env,jobject activity) {
 auto text=[&](const std::string& s){return env->NewStringUTF(s.c_str());};
 auto failure=[&](const char* operation,XrResult r){return text(std::string(operation)+" returned "+std::to_string(r));};
 JavaVM* vm=nullptr;
 if(env->GetJavaVM(&vm)!=JNI_OK)return text("GetJavaVM failed");
 PFN_xrInitializeLoaderKHR initialize=nullptr;
 auto result=xrGetInstanceProcAddr(XR_NULL_HANDLE,"xrInitializeLoaderKHR",reinterpret_cast<PFN_xrVoidFunction*>(&initialize));
 if(XR_FAILED(result)||!initialize)return failure("xrInitializeLoaderKHR lookup",result);
 XrLoaderInitInfoAndroidKHR loader{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
 loader.applicationVM=vm;loader.applicationContext=activity;
 result=initialize(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loader));
 if(XR_FAILED(result))return failure("loader initialization",result);
 uint32_t count=0;
 result=xrEnumerateInstanceExtensionProperties(nullptr,0,&count,nullptr);
 if(XR_FAILED(result))return failure("enumerate extensions",result);
 std::vector<XrExtensionProperties> extensions(count,{XR_TYPE_EXTENSION_PROPERTIES});
 result=xrEnumerateInstanceExtensionProperties(nullptr,count,&count,extensions.data());
 if(XR_FAILED(result))return failure("extension properties",result);
 bool android=false;
 for(const auto& e:extensions)if(!std::strcmp(e.extensionName,XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME))android=true;
 if(!android)return text("Runtime lacks XR_KHR_android_create_instance");
 const char* enabled[]={XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
 XrInstanceCreateInfoAndroidKHR platform{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
 platform.applicationVM=vm;platform.applicationActivity=activity;
 XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
 create.next=&platform;create.enabledExtensionCount=1;create.enabledExtensionNames=enabled;
 std::strcpy(create.applicationInfo.applicationName,"QuestBridge probe");
 create.applicationInfo.apiVersion=XR_MAKE_VERSION(1,0,0);
 XrInstance instance=XR_NULL_HANDLE;
 result=xrCreateInstance(&create,&instance);
 if(XR_FAILED(result))return failure("xrCreateInstance",result);
 XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
 result=xrGetInstanceProperties(instance,&properties);
 if(XR_FAILED(result)){xrDestroyInstance(instance);return failure("xrGetInstanceProperties",result);}
 XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};systemInfo.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
 XrSystemId system=XR_NULL_SYSTEM_ID;
 result=xrGetSystem(instance,&systemInfo,&system);
 const std::string name=properties.runtimeName;
 xrDestroyInstance(instance);
 if(XR_FAILED(result))return failure("xrGetSystem",result);
 return text("Runtime: "+name+"; HMD system discovered. No XR session or tracking started.");
}
