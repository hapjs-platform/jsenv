/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_JSENV_IMPL_V1000_H_
#define HYBRID_JSENV_IMPL_V1000_H_

#include "JSEnv-v1000.h"

namespace hybrid {
class JSEnv;
}  // namespace hybrid

namespace hybrid_v1000 {

class JSEnvImpl : public JSEnv {
 public:
  JSEnvImpl(hybrid::JSEnv* impl) : jsenv_v1100_(impl) {}

  int GetVersion() const override { return 1000; }

  void* DispatchJSEnvCommand(int cmd, void* data) override {
    return jsenv_v1100_->DispatchJSEnvCommand(cmd, data);
  }

  bool HasException() const override { return jsenv_v1100_->HasException(); }
  JSException GetException() const override {
    return jsenv_v1100_->GetException();
  }
  void SetException(JSException exception) override {
    jsenv_v1100_->SetException(exception);
  }
  void ClearException() override { jsenv_v1100_->ClearException(); }

  JSInspectorSession* CreateInspectorSession(
      JSInspectorClient* client,
      int context_group_id,
      const char* state,
      int is_jscontext_recreated) override {
    return jsenv_v1100_->CreateInspectorSession(client, context_group_id, state,
                                                is_jscontext_recreated);
  }

  // object: nullptr 代表全局
  // 其他值，来自j2v8 内的 V8Object 的 nativeHandle
  bool RegisterCallbackOnObject(J2V8ObjectHandle object,
                                const char* domain,
                                UserFunctionCallback callback,
                                void* user_data,
                                uint32_t flags) override {
    return jsenv_v1100_->RegisterCallbackOnObject(object, domain, callback,
                                                  user_data, flags);
  }

  bool ExecuteScript(const JSValue* code,
                     JSValue* presult,
                     const char* file_name = nullptr,
                     int start_lineno = 0) override {
    return jsenv_v1100_->ExecuteScript(code, presult, file_name, start_lineno);
  }

 protected:
  void AddReference() override { jsenv_v1100_->AddReference(); }

  void Release() override { jsenv_v1100_->Release(); }

 private:
  hybrid::JSEnv* jsenv_v1100_;
};

}  // namespace hybrid_v1000

#endif  // HYBRID_JSENV_IMPL_V1000_H_
