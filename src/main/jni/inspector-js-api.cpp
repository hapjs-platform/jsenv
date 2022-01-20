/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "inspector-js-api.h"
#include "iostream"
#include "jsenv-impl.h"
#include "logcat-console.h"
#include "v8-inspector.h"
#include "v8.h"

#include <android/log.h>
#include <cassert>

using v8::Context;
using v8::Function;
using v8::FunctionCallback;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Global;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Object;
using v8::String;
using v8::Value;

namespace hybrid {

InspectorFrontEnd::InspectorFrontEnd(
    Local<Context> context,
    std::unique_ptr<InspectorSessionDelegate> delegate)
    : delegate_(std::move(delegate)) {
  isolate_ = context->GetIsolate();
}

void InspectorFrontEnd::sendResponse(
    int callId,
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  sendMessageToFrontend(message->string());
}

void InspectorFrontEnd::sendNotification(
    std::unique_ptr<v8_inspector::StringBuffer> message) {
  sendMessageToFrontend(message->string());
}

void InspectorFrontEnd::flushProtocolNotifications() {}

void InspectorFrontEnd::sendMessageToFrontend(
    const v8_inspector::StringView& string) {
  delegate_->SendMessageToFrontend(string);
}

InspectorClient::InspectorClient(
    Local<Context> context,
    std::unique_ptr<InspectorSessionDelegate> delegate)
    : delegate_(std::move(delegate)) {
  isolate_ = context->GetIsolate();
  channel_.reset(new InspectorFrontEnd(context, std::move(delegate_)));
  inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
  session_ = inspector_->connect(kContextGroupId, channel_.get(),
                                 v8_inspector::StringView());
  inspector_->contextCreated(v8_inspector::V8ContextInfo(
      context, kContextGroupId, v8_inspector::StringView()));

}

void InspectorClient::sendInspectorMessage(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  Local<Context> context = isolate->GetCurrentContext();

  Local<String> message = args[0]->ToString(context).ToLocalChecked();
  int length = message->Length();
  std::unique_ptr<uint16_t[]> buffer(new uint16_t[length]);
  message->Write(isolate, buffer.get(), 0, length);

  v8_inspector::StringView message_view(buffer.get(), length);

  session_->dispatchProtocolMessage(message_view);
}

Isolate* InspectorClient::getIsolate() {
  return isolate_;
}

JSBindingConnection::JSBindingsSessionDelegate::JSBindingsSessionDelegate(
    JSBindingConnection* connection)
    : connection_(connection) {}

void JSBindingConnection::JSBindingsSessionDelegate::SendMessageToFrontend(
    const v8_inspector::StringView& string) {
  int length = static_cast<int>(string.length());
  Isolate* isolate_ = connection_->getInspector()->getIsolate();

  v8::HandleScope handleScope(isolate_);
  Local<String> message =
      v8::String::NewFromTwoByte(
          isolate_, reinterpret_cast<const uint16_t*>(string.characters16()),
          v8::NewStringType::kNormal, length)
          .ToLocalChecked();
  String::Utf8Value s(isolate_, message);

  Local<Value> v = message.As<Value>();
  connection_->OnMessage(v);
}

JSBindingConnection::JSBindingConnection(
    const v8::FunctionCallbackInfo<v8::Value>& info)
    : callback_(info.GetIsolate(), info[0].As<Function>()),
      persistent_(info.GetIsolate(), info.This()) {
  Wrap(info.This());
  client_ =
      new InspectorClient(info.GetIsolate()->GetCurrentContext(),
                          std::make_unique<JSBindingsSessionDelegate>(this));
}

void JSBindingConnection::OnMessage(Local<Value> v) {
  Local<Object> recv = object();

  Isolate* isolate_ = getInspector()->getIsolate();
  Local<Context> context_ = isolate_->GetCurrentContext();
  v8::Local<v8::Value> argv[1] = {v};
  (void)callback_.Get(isolate_)->Call(context_, recv, 1, argv);  // NOLINT
}

void JSBindingConnection::Disconnect() {
  delete this;
}

void JSBindingConnection::New(const FunctionCallbackInfo<Value>& info) {
  Isolate* isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  new JSBindingConnection(info);
}

void JSBindingConnection::Dispatch(const FunctionCallbackInfo<Value>& info) {
  Local<Object> object = info.This();
  JSBindingConnection* connection = static_cast<JSBindingConnection*>(
      object->GetAlignedPointerFromInternalField(0));
  if (connection == nullptr) {
    return;
  }

  InspectorClient* inspector = connection->getInspector();
  inspector->sendInspectorMessage(info);
}

void JSBindingConnection::Disconnect(const FunctionCallbackInfo<Value>& info) {
  Local<Object> object = info.This();
  JSBindingConnection* connection = static_cast<JSBindingConnection*>(
      object->GetAlignedPointerFromInternalField(0));
  if (connection != nullptr) {
    object->SetAlignedPointerInInternalField(0, nullptr);
    connection->Disconnect();
  }
}

void setMethod(Isolate* isolate,
               Local<FunctionTemplate> t,
               const char* name,
               FunctionCallback callback) {
  Local<String> dispatch =
      String::NewFromUtf8(isolate, name, NewStringType::kNormal)
          .ToLocalChecked();
  Local<FunctionTemplate> dispatchFt = FunctionTemplate::New(isolate, callback);
  t->PrototypeTemplate()->Set(dispatch, dispatchFt);
}

void JSBindingConnection::Init(JSEnvImpl* env) {
  Isolate* isolate = env->isolate();
  Local<FunctionTemplate> ft =
      FunctionTemplate::New(isolate, JSBindingConnection::New);
  Local<String> className =
      String::NewFromUtf8(isolate, "Connection", NewStringType::kNormal)
          .ToLocalChecked();
  ft->SetClassName(className);
  ft->InstanceTemplate()->SetInternalFieldCount(1);
  Local<Context> context = isolate->GetCurrentContext();

  setMethod(isolate, ft, "dispatch", JSBindingConnection::Dispatch);
  setMethod(isolate, ft, "disconnect", JSBindingConnection::Disconnect);

  context->Global()
      ->Set(context, className, ft->GetFunction(context).ToLocalChecked())
      .FromJust();  // NOLINT
}

}  // namespace hybrid
