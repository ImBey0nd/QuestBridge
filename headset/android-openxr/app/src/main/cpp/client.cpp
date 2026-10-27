// SPDX-License-Identifier: GPL-3.0-or-later
// Original QuestBridge OpenXR diagnostic client. No Sony runtime integration.
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <android/log.h>
#include <android/window.h>
#include <android_native_app_glue.h>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "tracking/model.hpp"
#include "math/render.hpp"
#include "protocol/tracking.hpp"

namespace {
void check(XrResult r, const char* operation) {
 if(XR_FAILED(r))throw std::runtime_error(std::string(operation)+": "+std::to_string(r));
}
#define XR_CALL(call) check((call), #call)
void log(const std::string& text) {__android_log_print(ANDROID_LOG_INFO,"QuestBridge","%s",text.c_str());}
using qb::render::Matrix;
using qb::render::multiply;
using qb::render::transform;
qb::Pose pose(XrPosef p) {return {{p.position.x,p.position.y,p.position.z},{p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w}};}
struct Eye {
 XrSwapchain swapchain=XR_NULL_HANDLE;
 uint32_t width=0,height=0;
 std::vector<XrSwapchainImageOpenGLESKHR> images;
 GLuint framebuffer=0,depth=0;
};
struct Client {
 android_app* app;
 JavaVM* vm;
 JNIEnv* jni=nullptr;
 jmethodID trackingCallback=nullptr;
 bool attached=false,resumed=false,window=false,running=false,done=false;
 XrInstance instance=XR_NULL_HANDLE;
 XrSession session=XR_NULL_HANDLE;
 XrSystemId system=XR_NULL_SYSTEM_ID;
 XrSpace local=XR_NULL_HANDLE,head=XR_NULL_HANDLE;
 std::array<XrSpace,2> hands{};
 XrActionSet actionSet=XR_NULL_HANDLE;
 XrAction gripPose=XR_NULL_HANDLE,trigger=XR_NULL_HANDLE,stick=XR_NULL_HANDLE,squeeze=XR_NULL_HANDLE;
 std::array<XrAction,6> buttons{};
 std::array<XrPath,2> handPaths{};
 XrSessionState state=XR_SESSION_STATE_UNKNOWN;
 EGLDisplay display=EGL_NO_DISPLAY;
 EGLContext context=EGL_NO_CONTEXT;
 EGLSurface surface=EGL_NO_SURFACE;
 EGLConfig config=nullptr;
 std::array<Eye,2> eyes;
 GLuint program=0,vao=0,buffer=0;
 GLint mvpLocation=-1,colorLocation=-1;
 qb::Tracking tracking{};
 std::chrono::steady_clock::time_point lastLog{};
 explicit Client(android_app* a):app(a),vm(a->activity->vm) {
  JNIEnv* env=nullptr;
  if(vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK){
   if(vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)throw std::runtime_error("AttachCurrentThread");attached=true;
  }
  jni=env;
 }
 ~Client(){shutdown();if(attached)vm->DetachCurrentThread();}
 void shutdown() {
  // Session destruction ends any running session; GL resources stay current until deleted.
  for(auto space:hands)if(space)xrDestroySpace(space);
  hands={};if(head)xrDestroySpace(head);head=XR_NULL_HANDLE;if(local)xrDestroySpace(local);local=XR_NULL_HANDLE;
  for(auto& eye:eyes){
   if(eye.framebuffer)glDeleteFramebuffers(1,&eye.framebuffer);
   if(eye.depth)glDeleteRenderbuffers(1,&eye.depth);
   if(eye.swapchain)xrDestroySwapchain(eye.swapchain);
   eye={};
  }
  if(program)glDeleteProgram(program);program=0;
  if(buffer)glDeleteBuffers(1,&buffer);buffer=0;
  if(vao)glDeleteVertexArrays(1,&vao);vao=0;
  if(session)xrDestroySession(session);session=XR_NULL_HANDLE;
  if(actionSet)xrDestroyActionSet(actionSet);actionSet=XR_NULL_HANDLE;
  if(instance)xrDestroyInstance(instance);instance=XR_NULL_HANDLE;
  if(display!=EGL_NO_DISPLAY){
   eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
   if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);
   if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);
   eglTerminate(display);
  }
  display=EGL_NO_DISPLAY;context=EGL_NO_CONTEXT;surface=EGL_NO_SURFACE;running=false;state=XR_SESSION_STATE_UNKNOWN;
 }
 XrPath path(const char* name){XrPath value;XR_CALL(xrStringToPath(instance,name,&value));return value;}
 void initialize() {
  jclass activityClass=jni->GetObjectClass(app->activity->clazz);
  trackingCallback=jni->GetMethodID(activityClass,"onTracking","([B)V");jni->DeleteLocalRef(activityClass);
  if(!trackingCallback){jni->ExceptionClear();throw std::runtime_error("Missing tracking callback");}
  PFN_xrInitializeLoaderKHR initializeLoader=nullptr;
  XR_CALL(xrGetInstanceProcAddr(XR_NULL_HANDLE,"xrInitializeLoaderKHR",reinterpret_cast<PFN_xrVoidFunction*>(&initializeLoader)));
  if(!initializeLoader)throw std::runtime_error("No loader initialization function");
  XrLoaderInitInfoAndroidKHR loader{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};loader.applicationVM=vm;loader.applicationContext=app->activity->clazz;
  XR_CALL(initializeLoader(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR*>(&loader)));
  const char* extensions[]={XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};
  XrInstanceCreateInfoAndroidKHR android{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};android.applicationVM=vm;android.applicationActivity=app->activity->clazz;
  XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};create.next=&android;create.enabledExtensionCount=2;create.enabledExtensionNames=extensions;
  std::strcpy(create.applicationInfo.applicationName,"QuestBridge diagnostic");create.applicationInfo.applicationVersion=2;create.applicationInfo.apiVersion=XR_MAKE_VERSION(1,0,0);
  XR_CALL(xrCreateInstance(&create,&instance));
  XrSystemGetInfo sys{XR_TYPE_SYSTEM_GET_INFO};sys.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;XR_CALL(xrGetSystem(instance,&sys,&system));
  PFN_xrGetOpenGLESGraphicsRequirementsKHR requirements=nullptr;
  XR_CALL(xrGetInstanceProcAddr(instance,"xrGetOpenGLESGraphicsRequirementsKHR",reinterpret_cast<PFN_xrVoidFunction*>(&requirements)));
  if(!requirements)throw std::runtime_error("No GLES requirements function");
  XrGraphicsRequirementsOpenGLESKHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};XR_CALL(requirements(instance,system,&req));
  // An offscreen EGL surface holds the context; OpenXR owns all visible eye images.
  display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major=0,minor=0;
  if(display==EGL_NO_DISPLAY||!eglInitialize(display,&major,&minor)||!eglBindAPI(EGL_OPENGL_ES_API))throw std::runtime_error("EGL initialization");
  const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLint count=0;
  if(!eglChooseConfig(display,attrs,&config,1,&count)||count!=1)throw std::runtime_error("No GLES3 EGL config");
  const EGLint contextAttrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttrs);
  const EGLint surfaceAttrs[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};surface=eglCreatePbufferSurface(display,config,surfaceAttrs);
  if(context==EGL_NO_CONTEXT||surface==EGL_NO_SURFACE||!eglMakeCurrent(display,surface,surface,context))throw std::runtime_error("EGL context");
  GLint glMajor=0,glMinor=0;glGetIntegerv(GL_MAJOR_VERSION,&glMajor);glGetIntegerv(GL_MINOR_VERSION,&glMinor);
  const auto version=XR_MAKE_VERSION(glMajor,glMinor,0);
  if(version<req.minApiVersionSupported||version>req.maxApiVersionSupported)throw std::runtime_error("Unsupported runtime GLES version");
  XrGraphicsBindingOpenGLESAndroidKHR binding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};binding.display=display;binding.config=config;binding.context=context;
  XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};sessionInfo.next=&binding;sessionInfo.systemId=system;XR_CALL(xrCreateSession(instance,&sessionInfo,&session));
  XrReferenceSpaceCreateInfo ref{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};ref.poseInReferenceSpace.orientation.w=1;ref.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;
  XR_CALL(xrCreateReferenceSpace(session,&ref,&local));ref.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;XR_CALL(xrCreateReferenceSpace(session,&ref,&head));
  setupActions();setupEyes();setupGraphics();log("XR session created; LOCAL origin, real pose sampling enabled; no PS5 integration");
 }
 void setupActions() {
  XrActionSetCreateInfo info{XR_TYPE_ACTION_SET_CREATE_INFO};std::strcpy(info.actionSetName,"tracking");std::strcpy(info.localizedActionSetName,"QuestBridge tracking");XR_CALL(xrCreateActionSet(instance,&info,&actionSet));
  handPaths={path("/user/hand/left"),path("/user/hand/right")};
  auto action=[&](const char* name,XrActionType type,XrAction& out){
   XrActionCreateInfo ci{XR_TYPE_ACTION_CREATE_INFO};std::strcpy(ci.actionName,name);std::strcpy(ci.localizedActionName,name);ci.actionType=type;ci.countSubactionPaths=2;ci.subactionPaths=handPaths.data();XR_CALL(xrCreateAction(actionSet,&ci,&out));
  };
  action("grip_pose",XR_ACTION_TYPE_POSE_INPUT,gripPose);action("trigger",XR_ACTION_TYPE_FLOAT_INPUT,trigger);action("thumbstick",XR_ACTION_TYPE_VECTOR2F_INPUT,stick);
  action("squeeze",XR_ACTION_TYPE_FLOAT_INPUT,squeeze);
  const char* buttonNames[]={"primary","secondary","menu","stick_click","primary_touch","secondary_touch"};
  for(size_t i=0;i<buttons.size();++i)action(buttonNames[i],XR_ACTION_TYPE_BOOLEAN_INPUT,buttons[i]);
  std::vector<XrActionSuggestedBinding> binds={
   {gripPose,path("/user/hand/left/input/grip/pose")},{gripPose,path("/user/hand/right/input/grip/pose")},
   {trigger,path("/user/hand/left/input/trigger/value")},{trigger,path("/user/hand/right/input/trigger/value")},
   {stick,path("/user/hand/left/input/thumbstick")},{stick,path("/user/hand/right/input/thumbstick")},
   {squeeze,path("/user/hand/left/input/squeeze/value")},{squeeze,path("/user/hand/right/input/squeeze/value")}};
  const char* bindingPaths[6][2]={
   {"/user/hand/left/input/x/click","/user/hand/right/input/a/click"},
   {"/user/hand/left/input/y/click","/user/hand/right/input/b/click"},
   {"/user/hand/left/input/menu/click",nullptr},
   {"/user/hand/left/input/thumbstick/click","/user/hand/right/input/thumbstick/click"},
   {"/user/hand/left/input/x/touch","/user/hand/right/input/a/touch"},
   {"/user/hand/left/input/y/touch","/user/hand/right/input/b/touch"}};
  for(size_t i=0;i<6;++i)for(size_t h=0;h<2;++h)if(bindingPaths[i][h])binds.push_back({buttons[i],path(bindingPaths[i][h])});
  XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};suggested.interactionProfile=path("/interaction_profiles/oculus/touch_controller");suggested.countSuggestedBindings=static_cast<uint32_t>(binds.size());suggested.suggestedBindings=binds.data();
  XR_CALL(xrSuggestInteractionProfileBindings(instance,&suggested));
  for(size_t i=0;i<2;++i){XrActionSpaceCreateInfo space{XR_TYPE_ACTION_SPACE_CREATE_INFO};space.action=gripPose;space.subactionPath=handPaths[i];space.poseInActionSpace.orientation.w=1;XR_CALL(xrCreateActionSpace(session,&space,&hands[i]));}
  XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&actionSet;XR_CALL(xrAttachSessionActionSets(session,&attach));
 }
 void setupEyes() {
  uint32_t count=0;XR_CALL(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr));
  if(count!=2)throw std::runtime_error("Expected stereo views");
  std::array<XrViewConfigurationView,2> configs{{{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}}};
  XR_CALL(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,configs.data()));
  XR_CALL(xrEnumerateSwapchainFormats(session,0,&count,nullptr));std::vector<int64_t> formats(count);XR_CALL(xrEnumerateSwapchainFormats(session,count,&count,formats.data()));
  int64_t format=0;for(auto f:formats)if(f==GL_SRGB8_ALPHA8){format=f;break;}
  if(!format)throw std::runtime_error("Runtime lacks sRGB eye swapchain");
  for(size_t i=0;i<2;++i){auto& e=eyes[i];e.width=configs[i].recommendedImageRectWidth;e.height=configs[i].recommendedImageRectHeight;
   XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;ci.format=format;ci.sampleCount=1;ci.width=e.width;ci.height=e.height;ci.faceCount=1;ci.arraySize=1;ci.mipCount=1;
   XR_CALL(xrCreateSwapchain(session,&ci,&e.swapchain));XR_CALL(xrEnumerateSwapchainImages(e.swapchain,0,&count,nullptr));e.images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
   XR_CALL(xrEnumerateSwapchainImages(e.swapchain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(e.images.data())));
   glGenFramebuffers(1,&e.framebuffer);glGenRenderbuffers(1,&e.depth);glBindRenderbuffer(GL_RENDERBUFFER,e.depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,e.width,e.height);
  }
 }
 GLuint shader(GLenum type,const char* source) {
  GLuint result=glCreateShader(type);glShaderSource(result,1,&source,nullptr);glCompileShader(result);GLint ok=0;glGetShaderiv(result,GL_COMPILE_STATUS,&ok);
  if(!ok){glDeleteShader(result);throw std::runtime_error("Diagnostic shader compile failed");}return result;
 }
 void setupGraphics(){
  const char* vs="#version 300 es\nlayout(location=0) in vec3 vertex;uniform mat4 mvp;void main(){gl_Position=mvp*vec4(vertex,1.0);}";
  const char* fs="#version 300 es\nprecision mediump float;uniform vec3 color;out vec4 pixel;void main(){pixel=vec4(color,1.0);}";
  GLuint vertex=shader(GL_VERTEX_SHADER,vs),fragment=shader(GL_FRAGMENT_SHADER,fs);program=glCreateProgram();glAttachShader(program,vertex);glAttachShader(program,fragment);glLinkProgram(program);glDeleteShader(vertex);glDeleteShader(fragment);
  GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("Diagnostic shader link failed");mvpLocation=glGetUniformLocation(program,"mvp");colorLocation=glGetUniformLocation(program,"color");
  // Unit cube, twelve triangles; scale/pose supplied per draw.
  constexpr float cube[]={-1,-1,-1, 1,-1,-1, 1,1,-1, -1,-1,-1, 1,1,-1, -1,1,-1,
   -1,-1,1, 1,1,1, 1,-1,1, -1,-1,1, -1,1,1, 1,1,1,
   -1,-1,-1, -1,1,-1, -1,1,1, -1,-1,-1, -1,1,1, -1,-1,1,
   1,-1,-1, 1,-1,1, 1,1,1, 1,-1,-1, 1,1,1, 1,1,-1,
   -1,1,-1, 1,1,-1, 1,1,1, -1,1,-1, 1,1,1, -1,1,1,
   -1,-1,-1, 1,-1,1, 1,-1,-1, -1,-1,-1, -1,-1,1, 1,-1,1};
  glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&buffer);glBindBuffer(GL_ARRAY_BUFFER,buffer);glBufferData(GL_ARRAY_BUFFER,sizeof(cube),cube,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
 }
 qb::Device locate(XrSpace space,XrTime time) {
  XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};XR_CALL(xrLocateSpace(space,local,time,&location));qb::Device d;
  constexpr auto pv=XR_SPACE_LOCATION_POSITION_VALID_BIT,ov=XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
  if(location.locationFlags&pv){d.pose.position=pose(location.pose).position;d.flags|=1;}
  if(location.locationFlags&ov){d.pose.orientation=pose(location.pose).orientation;d.flags|=2;}
  if((location.locationFlags&(pv|XR_SPACE_LOCATION_POSITION_TRACKED_BIT))==(pv|XR_SPACE_LOCATION_POSITION_TRACKED_BIT))d.flags|=4;
  if((location.locationFlags&(ov|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))==(ov|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))d.flags|=8;
  return d;
 }
 void sample(XrTime time){
  tracking={};tracking.devices[0]=locate(head,time);
  XrActiveActionSet active{actionSet,XR_NULL_PATH};XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;
  const auto syncResult=xrSyncActions(session,&sync);
  if(syncResult!=XR_SESSION_NOT_FOCUSED){check(syncResult,"xrSyncActions");
   for(size_t i=0;i<2;++i){auto& d=tracking.devices[i+1];
    XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};get.subactionPath=handPaths[i];get.action=gripPose;XrActionStatePose statePose{XR_TYPE_ACTION_STATE_POSE};XR_CALL(xrGetActionStatePose(session,&get,&statePose));
    if(statePose.isActive)d=locate(hands[i],time);
    get.action=trigger;XrActionStateFloat f{XR_TYPE_ACTION_STATE_FLOAT};XR_CALL(xrGetActionStateFloat(session,&get,&f));if(f.isActive)d.trigger=f.currentState;
    get.action=squeeze;f={XR_TYPE_ACTION_STATE_FLOAT};XR_CALL(xrGetActionStateFloat(session,&get,&f));if(f.isActive)d.grip=f.currentState;
    constexpr uint32_t buttonBits[]={1,2,4,8,64,128};
    for(size_t b=0;b<buttons.size();++b){get.action=buttons[b];XrActionStateBoolean pressed{XR_TYPE_ACTION_STATE_BOOLEAN};XR_CALL(xrGetActionStateBoolean(session,&get,&pressed));if(pressed.isActive&&pressed.currentState)d.buttons|=buttonBits[b];}
    get.action=stick;XrActionStateVector2f v{XR_TYPE_ACTION_STATE_VECTOR2F};XR_CALL(xrGetActionStateVector2f(session,&get,&v));if(v.isActive)d.stick={v.currentState.x,v.currentState.y};
   }
  }
  const auto bytes=qb::encode_tracking(tracking);
  jbyteArray payload=jni->NewByteArray(static_cast<jsize>(bytes.size()));
  if(!payload){jni->ExceptionClear();throw std::runtime_error("Tracking callback allocation failed");}
  jni->SetByteArrayRegion(payload,0,static_cast<jsize>(bytes.size()),reinterpret_cast<const jbyte*>(bytes.data()));
  jni->CallVoidMethod(app->activity->clazz,trackingCallback,payload);jni->DeleteLocalRef(payload);
  if(jni->ExceptionCheck()){jni->ExceptionClear();throw std::runtime_error("Tracking callback rejected sample");}
  const auto now=std::chrono::steady_clock::now();if(now-lastLog>=std::chrono::seconds(1)){
   const auto& h=tracking.devices[0];char message[320];
   std::snprintf(message,sizeof(message),"tracking predicted_xr_time=%lld head_flags=%u xyz=%.3f,%.3f,%.3f left_flags=%u right_flags=%u triggers=%.3f,%.3f",static_cast<long long>(time),h.flags,h.pose.position.x,h.pose.position.y,h.pose.position.z,tracking.devices[1].flags,tracking.devices[2].flags,tracking.devices[1].trigger,tracking.devices[2].trigger);log(message);lastLog=now;
  }
 }
 void draw(const Matrix& vp,qb::Pose p,qb::Vec3 scale,qb::Vec3 color){
  Matrix model=transform(p);for(int row=0;row<3;++row){model.v[row]*=scale.x;model.v[4+row]*=scale.y;model.v[8+row]*=scale.z;}
  const auto mvp=multiply(vp,model);glUniformMatrix4fv(mvpLocation,1,GL_FALSE,mvp.v);glUniform3f(colorLocation,color.x,color.y,color.z);glDrawArrays(GL_TRIANGLES,0,36);
 }
 void render(Eye& eye,const XrView& v,uint32_t index){
  glBindFramebuffer(GL_FRAMEBUFFER,eye.framebuffer);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,eye.images[index].image,0);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,eye.depth);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Eye framebuffer incomplete");
  glViewport(0,0,eye.width,eye.height);glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glClearColor(0.015f,0.025f,0.04f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(program);glBindVertexArray(vao);
  const auto vp=multiply(qb::render::projection(v.fov.angleLeft,v.fov.angleRight,v.fov.angleDown,v.fov.angleUp),qb::render::view(pose(v.pose)));
  // LOCAL origin is runtime-defined, not assumed to be the physical floor.
  draw(vp,{{0,-0.5f,-1.5f},{}},{0.6f,0.025f,0.6f},{0.12f,0.18f,0.24f});
  draw(vp,{{0,0,-1.5f},{}},{0.1f,0.1f,0.1f},{0.1f,0.8f,0.3f});
  for(size_t i=1;i<3;++i)if((tracking.devices[i].flags&3)==3)draw(vp,tracking.devices[i].pose,{0.04f,0.04f,0.08f},i==1?qb::Vec3{0.2f,0.4f,1}:qb::Vec3{1,0.4f,0.15f});
  glBindFramebuffer(GL_FRAMEBUFFER,0);glFlush();
 }
 void frame(){
  XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};XrFrameState fs{XR_TYPE_FRAME_STATE};XR_CALL(xrWaitFrame(session,&wait,&fs));XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};XR_CALL(xrBeginFrame(session,&begin));
  sample(fs.predictedDisplayTime);
  std::array<XrCompositionLayerProjectionView,2> projectionViews{{{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}}};
  XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=local;layer.viewCount=2;layer.views=projectionViews.data();
  const XrCompositionLayerBaseHeader* layers[]={reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer)};
  bool rendered=false;
  if(fs.shouldRender){
   XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};locateInfo.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;locateInfo.displayTime=fs.predictedDisplayTime;locateInfo.space=local;
   XrViewState viewState{XR_TYPE_VIEW_STATE};uint32_t count=0;std::array<XrView,2> views{{{XR_TYPE_VIEW},{XR_TYPE_VIEW}}};XR_CALL(xrLocateViews(session,&locateInfo,&viewState,2,&count,views.data()));
   if(count==2&&(viewState.viewStateFlags&(XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT))==(XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT)){
    for(size_t i=0;i<2;++i){auto& e=eyes[i];uint32_t index=0;XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};XR_CALL(xrAcquireSwapchainImage(e.swapchain,&acquire,&index));
     XrSwapchainImageWaitInfo imageWait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};imageWait.timeout=XR_INFINITE_DURATION;XR_CALL(xrWaitSwapchainImage(e.swapchain,&imageWait));render(e,views[i],index);
     XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};XR_CALL(xrReleaseSwapchainImage(e.swapchain,&release));
     auto& pv=projectionViews[i];pv.pose=views[i].pose;pv.fov=views[i].fov;pv.subImage.swapchain=e.swapchain;pv.subImage.imageRect.extent={static_cast<int32_t>(e.width),static_cast<int32_t>(e.height)};
    }rendered=true;
   }
  }
  XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=fs.predictedDisplayTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;end.layerCount=rendered?1:0;end.layers=rendered?layers:nullptr;XR_CALL(xrEndFrame(session,&end));
 }
 void events(){
  XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};XrResult result;
  while((result=xrPollEvent(instance,&event))==XR_SUCCESS){
   if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED){const auto* change=reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);state=change->state;log("XR session state="+std::to_string(state));
    if(state==XR_SESSION_STATE_READY&&!running){XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};begin.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;XR_CALL(xrBeginSession(session,&begin));running=true;}
    if(state==XR_SESSION_STATE_STOPPING&&running){XR_CALL(xrEndSession(session));running=false;tracking={};}
    if(state==XR_SESSION_STATE_EXITING||state==XR_SESSION_STATE_LOSS_PENDING)done=true;
   }else if(event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)done=true;
   event={XR_TYPE_EVENT_DATA_BUFFER};
  }if(result!=XR_EVENT_UNAVAILABLE)check(result,"xrPollEvent");
 }
};
void command(android_app* app,int32_t cmd){auto* c=static_cast<Client*>(app->userData);if(!c)return;
 if(cmd==APP_CMD_RESUME)c->resumed=true;if(cmd==APP_CMD_PAUSE)c->resumed=false;
 if(cmd==APP_CMD_INIT_WINDOW)c->window=app->window!=nullptr;
 if(cmd==APP_CMD_TERM_WINDOW){c->window=false;c->shutdown();}
}
}
extern "C" void android_main(android_app* app){
 ANativeActivity_setWindowFlags(app->activity,AWINDOW_FLAG_KEEP_SCREEN_ON|AWINDOW_FLAG_FULLSCREEN,0);
 try{
  Client client(app);app->userData=&client;app->onAppCmd=command;
  while(!app->destroyRequested&&!client.done){
   int events=0;android_poll_source* source=nullptr;
   while(ALooper_pollOnce(client.running?0:20,nullptr,&events,reinterpret_cast<void**>(&source))>=0){if(source)source->process(app,source);if(app->destroyRequested)break;}
   if(app->destroyRequested)break;
   if(!client.instance&&client.window&&client.resumed)client.initialize();
   if(client.instance){client.events();if(client.running&&!client.done)client.frame();}
  }
  app->onAppCmd=nullptr;app->userData=nullptr;
 }catch(const std::exception& error){log(std::string("XR diagnostic failed: ")+error.what());app->onAppCmd=nullptr;app->userData=nullptr;ANativeActivity_finish(app->activity);}
}
