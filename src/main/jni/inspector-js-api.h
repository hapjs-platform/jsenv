/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_INSPECTOR_JS_API_H_
#define HYBRID_INSPECTOR_JS_API_H_

#include <cassert>
#include "iostream"
#include "jsenv-impl.h"
#include "v8-inspector.h"
#include "v8.h"

namespace hybrid {

class InspectorSessionDelegate {
 public:
  virtual ~InspectorSessionDelegate() = default;
  virtual void SendMessageToFrontend(
      const v8_inspector::StringView& message) = 0;
};

class InspectorFrontEnd final : public v8_inspector::V8Inspector::Channel {
 public:
  explicit InspectorFrontEnd(
      v8::Local<v8::Context> context,
      std::unique_ptr<InspectorSessionDelegate> delegate);
  ~InspectorFrontEnd() override = default;

 private:
  void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;

  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;

  void flushProtocolNotifications() override;

  void sendMessageToFrontend(const v8_inspector::StringView& string);

  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  std::unique_ptr<InspectorSessionDelegate> delegate_;
};

class InspectorClient : public v8_inspector::V8InspectorClient {
 public:
  InspectorClient(v8::Local<v8::Context> context,
                  std::unique_ptr<InspectorSessionDelegate> delegate);

  void sendInspectorMessage(const v8::FunctionCallbackInfo<v8::Value>& args);

  v8::Isolate* getIsolate();

 private:
  static const int kContextGroupId = 1;

  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  std::unique_ptr<v8_inspector::V8Inspector> inspector_;
  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
  std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
  std::unique_ptr<InspectorSessionDelegate> delegate_;
};

class JSBindingConnection {
 public:
  class JSBindingsSessionDelegate : public InspectorSessionDelegate {
   public:
    JSBindingsSessionDelegate(JSBindingConnection* connection);

    void SendMessageToFrontend(
        const v8_inspector::StringView& message) override;

   private:
    JSBindingConnection* connection_;
  };

  JSBindingConnection(const v8::FunctionCallbackInfo<v8::Value>& info);

  virtual ~JSBindingConnection() {
    if (persistent_.IsEmpty()) {
      return;
    }

    persistent_.ClearWeak();
    persistent_.Reset();

    client_ = nullptr;
  }

  void OnMessage(v8::Local<v8::Value> value);
  void Disconnect();

  template <class T>
  inline v8::Local<T> ToLocal(v8::Persistent<T>& p_) {
    return *reinterpret_cast<v8::Local<T>*>(&p_);
  }

  inline v8::Local<v8::Object> object() {
    assert(!persistent_.IsEmpty());
    return ToLocal<v8::Object>(persistent_);
  }

  inline void Wrap(v8::Local<v8::Object> object) {
    assert(!persistent_.IsEmpty());
    assert(object->InternalFieldCount() > 0);
    object->SetAlignedPointerInInternalField(0, static_cast<void*>(this));
  }

  inline InspectorClient* getInspector() { return client_; }

  // for js api
  static void Init(JSEnvImpl* env);
  static void New(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Dispatch(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Disconnect(const v8::FunctionCallbackInfo<v8::Value>& info);

 private:
  InspectorClient* client_;
  v8::Global<v8::Function> callback_;
  v8::Persistent<v8::Object> persistent_;
};

}  // namespace hybrid

#endif  //  HYBRID_INSPECTOR_JS_API_H_