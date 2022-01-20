/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_J2ENV_IMPL_H_
#define HYBRID_J2ENV_IMPL_H_

#include "JSEnv.h"

#include <map>
#include <string>
#include <vector>

#include "v8.h"

#include "inspector-proxy.h"
#include "j2v8-runtime.h"
#include "jsclass.h"

#include "jsenv-impl-v1000.h"

namespace hybrid {
class LogcatConsole;

struct JSEnvHandleScope;

class JSEnvImpl : public JSEnv {
 public:
  JSEnvImpl(J2V8Runtime* runtime);
  ~JSEnvImpl() override;

  int GetVersion() const override;

  void* DispatchJSEnvCommand(int cmd, void* data) override;

  bool HasException() const override;

  JSException GetException() const override;

  void ClearException() override;

  void SetException(JSException exception) override;

  bool RegisterCallbackOnObject(J2V8ObjectHandle object,
                                const char* domain,
                                UserFunctionCallback callback,
                                void* user_data,
                                uint32_t flags) override;

  JSObject NewFunction(JSFunctionCallback callback,
                       void* user_data,
                       uint32_t flags) override;

  // free the return buffer by 'free'
  bool ExecuteScript(const JSValue* code_value,
                     JSValue* presult,
                     const char* file_name = nullptr,
                     int start_lineno = 0,
                     uint32_t flags = 0) override;

  JSInspectorSession* CreateInspectorSession(
      JSInspectorClient* client,
      int context_group_id,
      const char* state,
      int is_jscontext_recreated) override;

  // version 1100
  JSObject J2V8ObjectHandleToJSObject(J2V8ObjectHandle j2v8_handle) override;
  // class support
  JSClass CreateClass(const JSClassDefinition* class_definition,
                      JSClass super) override;
  JSClass GetClass(const char* class_name) override;
  JSObject NewInstance(JSClass clazz) override;
  JSObject NewInstanceWithConstructor(JSClass clazz,
                                      const JSValue* args,
                                      int argc) override;
  JSObject GetClassConstructorFunction(JSClass clazz) override;

  // object type
  int GetObjectType(JSObject object) override;
  int GetTypedArrayType(JSObject object) override;

  // object reference
  JSObject NewObjectReference(JSObject object) override;
  JSObject NewObjectWeakReference(
      JSObject object,
      JSWeakReferenceCallback weak_callback = nullptr,
      void* user_data = nullptr) override;
  void DeleteObjectReference(JSObject object) override;
  void GetWeakReferenceCallbackInfo(const void* weak_data,
                                    void** p_user_data,
                                    void** p_internal_fields) override;

  // private data access
  void* GetObjectPrivateData(JSObject object) override;
  bool SetObjectPrivateData(JSObject object, void* user_data) override;
  void* GetObjectPrivateExtraData(JSObject object) override;
  bool SetObjectPrivateExtraData(JSObject object, void* user_data) override;

  // global object
  JSObject GetGlobalObject() override;
  bool SetGlobal(const JSValue* pkey, const JSValue* pvalue) override;
  bool GetGlobal(const JSValue* pkey,
                 JSValue* pvale,
                 uint32_t flags = 0) override;

  // object property
  bool GetObjectProperty(JSObject object,
                         const JSValue* pkey,
                         JSValue* pvalue,
                         uint32_t flags /* = 0*/) override;
  bool SetObjectProperty(JSObject object,
                         const JSValue* pkey,
                         const JSValue* pvalue) override;

  JSObject GetObjectPropertyNames(JSObject object) override;

  // array access
  size_t GetObjectLength(JSObject object) override;
  bool GetObjectAtIndex(JSObject object,
                        int index,
                        JSValue* pvalue,
                        uint32_t flags /* = 0*/) override;
  bool SetObjectAtIndex(JSObject object,
                        int index,
                        const JSValue* pvalue) override;

  // function call
  bool CallFunction(JSObject function,
                    JSObject self,
                    const JSValue* argv,
                    int argc,
                    JSValue* presult,
                    uint32_t flags /* =0 */) override;
  JSObject CallFunctionAsConstructor(JSObject function,
                                     const JSValue* argv,
                                     int argc) override;

  // object create
  JSObject NewObject() override;
  JSObject NewArray(size_t length) override;
  JSObject NewArrayWithValues(const JSValue* argv, int argc) override;

  // TypedArray access
  JSObject NewTypedArray(int array_type, size_t length) override;
  JSObject NewTypedArrayArrayBuffer(int array_type,
                                    JSObject array_buffer,
                                    size_t offset,
                                    size_t length) override;
  size_t GetTypedArrayCount(JSObject typed_array) override;
  void* GetTypedArrayPointer(JSObject typed_array,
                             size_t element_offset) override;
  JSObject GetTypedArrayArrayBuffer(JSObject typed_array) override;

  // ArrayBuffer access
  JSObject NewArrayBuffer(size_t length) override;
  JSObject NewArrayBufferExternal(
      void* byte,
      size_t length,
      JSArrayBufferReleaseExteranlCallback release_callback,
      void* user_data) override;
  size_t GetArrayBufferLength(JSObject object) override;
  void* GetArrayBufferPointer(JSObject object,
                              size_t* plength = nullptr) override;

  // promise
  JSObject CreateResolver() override;
  JSObject GetPromiseFromResolver(JSObject resolver) override;
  bool Resolve(JSObject object, const JSValue* pvalue) override;
  bool Reject(JSObject object, const JSValue* pvalue) override;

  bool SetPromiseThen(JSObject promise, JSObject function) override;
  bool SetPromiseCatch(JSObject promise, JSObject function) override;
  bool PromiseHasHandler(JSObject promise) override;
  int GetPromiseState(JSObject promise) override;
  bool GetPromiseResult(JSObject promise,
                        JSValue* pvalue,
                        uint32_t flags = 0) override;

  // scope
  void PushScope() override;
  void PopScope() override;

  static JSEnvImpl* From(J2V8Runtime* runtime);
  static JSEnvImpl* From(v8::Isolate* isolate);

  void Detach();

  void ThrowException(v8::TryCatch* ptry_catch);

  bool ThrowExceptionToV8();

  hybrid_v1000::JSEnv* GetJSEnvV1000() { return &jsenv_v1000_; }

  void AddReference() override;
  void Release() override;

  inline v8::Isolate* isolate() const { return isolate_; };
  inline J2V8Runtime* runtime() const { return runtime_; };
  inline v8::Local<v8::Context> context() const {
    return J2V8RuntimeGetContext(runtime_);
  }

  void ResetLogcat();

  inline void SetQuickAppJSRuntimeHandle(void* handle) {
    quickapp_jsruntime_handle_ = handle;
  }

  inline void* GetQuickAppJSRuntimeHandle() {
    return quickapp_jsruntime_handle_;
  }

 private:
  struct UserCallbackInfo {
    UserCallbackInfo(J2V8ObjectHandle handle,
                     const char* domain,
                     UserFunctionCallback callback,
                     void* user_data,
                     uint32_t flags)
        : owner_handle(handle),
          domain(domain),
          callback(callback),
          user_data(user_data),
          flags(flags) {}

    J2V8ObjectHandle owner_handle;
    std::string domain;
    UserFunctionCallback callback;
    void* user_data;
    uint32_t flags;
  };

  struct JSFunctionCallbackInfo {
    JSFunctionCallbackInfo(JSFunctionCallback callback,
                           void* user_data,
                           uint32_t flags)
        : callback(callback), user_data(user_data), flags(flags) {}

    JSFunctionCallback callback;
    void* user_data;
    uint32_t flags;
  };

  struct JSExceptionImpl {
    JSExceptionImpl() : type(JSException::kNoneException) {}

    JSExceptionImpl(const JSException& e) : type(e.type), message(e.message) {}

    JSExceptionImpl& operator=(const JSException& e) {
      type = e.type;
      message = e.message;
      return *this;
    }

    void Set(int tp, const std::string& msg) {
      type = tp;
      message = msg;
    }

    void Clear() {
      type = JSException::kNoneException;
      message.clear();
    }

    int type;
    std::string message;
  };

  static void CallUserCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CallJSFunctionCallback(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ParseDomain(const char* domain,
                          std::string* powner_name,
                          std::string* pfunc_name);
  v8::Local<v8::Object> CreateDomainObject(v8::Local<v8::Object> object,
                                           const std::string owner_name);

  v8::Local<v8::Object> CallConstructor(v8::Local<v8::Function> function,
                                        const JSValue* args,
                                        int argc);
  void* GetJSObjectPrivateData(JSObject object, int index);
  bool SetJSObjectPrivateData(JSObject object, int index, void* pdata);

  J2V8Runtime* runtime_;
  v8::Isolate* isolate_;
  std::unique_ptr<LogcatConsole> logcat_console_;
  int ref_count_;
  std::vector<UserCallbackInfo> user_callbacks_;
  std::vector<JSFunctionCallbackInfo> jsfunction_callbacks_;
  std::map<std::string, std::unique_ptr<JSClassTemplate>> js_classes_;
  JSExceptionImpl exception_;
  std::vector<JSEnvHandleScope*> handle_scopes_;

  // QuickAppJSRuntime handle
  void* quickapp_jsruntime_handle_;
  // support hybrid_v1000 interface
  hybrid_v1000::JSEnvImpl jsenv_v1000_;
};

}  // namespace hybrid

#endif  // HYBRID_J2ENV_IMPL_H_
