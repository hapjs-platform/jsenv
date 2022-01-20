/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef J2V8_JSENV_V1000_H_
#define J2V8_JSENV_V1000_H_

#include <dlfcn.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "JSEnv.h"

#define JSENV_VERSION_V1000 1000

namespace hybrid_v1000 {

using J2V8ObjectHandle = hybrid::J2V8ObjectHandle;
using J2V8Handle = hybrid::J2V8Handle;
using JSValue = hybrid::JSValue;
using JSException = hybrid::JSException;
using UserFunctionCallback = hybrid::UserFunctionCallback;
using JSInspectorSession = hybrid::JSInspectorSession;
using JSInspectorClient = hybrid::JSInspectorClient;

class JSEnv;

typedef JSEnv* (*get_jsenv_cb)(J2V8Handle, int);

class JSEnv {
 public:
  enum { kFlagUseUTF8 = 1 };

  virtual int GetVersion() const = 0;

  virtual void* DispatchJSEnvCommand(int cmd, void* data) = 0;

  virtual bool HasException() const = 0;
  virtual JSException GetException() const = 0;
  virtual void SetException(JSException exception) = 0;
  virtual void ClearException() = 0;
  void SetException(int type, const char* message) {
    SetException(JSException(type, message));
  }

  virtual JSInspectorSession* CreateInspectorSession(
      JSInspectorClient* client,
      int context_group_id,
      const char* state,
      int is_jscontext_recreated) = 0;

  // object: nullptr 代表全局
  // 其他值，来自j2v8 内的 V8Object 的 nativeHandle
  virtual bool RegisterCallbackOnObject(J2V8ObjectHandle object,
                                        const char* domain,
                                        UserFunctionCallback callback,
                                        void* user_data,
                                        uint32_t flags) = 0;

  virtual bool ExecuteScript(const JSValue* code,
                             JSValue* presult,
                             const char* file_name = nullptr,
                             int start_lineno = 0) = 0;
  template <typename TCHAR>
  bool ExecuteScript(const TCHAR* code,
                     size_t code_size,
                     JSValue* presult,
                     const char* file_name = nullptr,
                     int start_lineno = 0) {
    JSValue code_value;
    code_value.Set(code, code_size);
    return ExecuteScript(&code_value, presult, file_name, start_lineno);
  }

  static JSEnv* GetInstance(J2V8Handle handle,
                            int version = JSENV_VERSION_V1000,
                            void* j2v8_so_handle = nullptr) {
    if (j2v8_so_handle == nullptr) {
      const char* jsenv_so_name = getenv(JSENV_SO_NAME);
      if (!jsenv_so_name) {
        jsenv_so_name = JSENV_DEFAULT_SO_NAME;
      }

      j2v8_so_handle = dlopen(jsenv_so_name, RTLD_NOW);
    }

    get_jsenv_cb get_env =
        reinterpret_cast<get_jsenv_cb>(dlsym(j2v8_so_handle, JSENV_ENTRY));

    if (!get_env) {
      return nullptr;
    }

    JSEnv* env = get_env(handle, version);
    if (env) {
      env->AddReference();
    }
    return env;
  }

  static void Release(JSEnv* env) {
    if (env) {
      env->Release();
    }
  }

 protected:
  virtual ~JSEnv() {}

  virtual void AddReference() = 0;
  virtual void Release() = 0;
};

}  // namespace hybrid_v1000

#endif  // J2V8_JSENV_H_
