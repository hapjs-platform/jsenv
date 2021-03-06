/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "inspector-proxy.h"

#include "hybrid-log.h"
#include "jsenv-impl.h"
#include "wrapper/inspector.h"

#define TAG "INSPECTOR_PROXY"

using v8_inspector::StringBuffer;
using v8_inspector::StringView;
using v8_inspector::V8ContextInfo;
using v8_inspector::V8Inspector;
using v8_inspector::V8InspectorSession;

namespace hybrid {

JSInspectorSessionImpl::JSInspectorSessionImpl(JSEnvImpl* env,
                                               JSInspectorClient* client,
                                               int context_group_id,
                                               const char* state,
                                               int is_jscontext_recreated)
    : jsenv_(env),
      v8_inspector_(nullptr),
      v8_session_(nullptr),
      client_(client),
      context_group_id_(context_group_id) {
  v8::Isolate* isolate = jsenv_->isolate();
  v8::Locker locker(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = jsenv_->context();
  v8::Context::Scope context_scope(context);

  v8_inspector_ = V8Inspector::create(isolate, this);
  StringView str_state(reinterpret_cast<const uint8_t*>(state ? state : ""),
                       (state ? strlen(state) : 0));
  v8_session_ = v8_inspector_->connect(context_group_id_, this, str_state);
  V8ContextInfo context_info(context, context_group_id_, str_state);
  v8_inspector_->contextCreated(context_info);
  OnJsContextReCreated(is_jscontext_recreated);

  jsenv_->ResetLogcat();
}

JSInspectorSessionImpl::~JSInspectorSessionImpl() {
  if (v8_inspector_) {
    v8::Isolate* isolate = jsenv_->isolate();
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    jsenv_->ResetLogcat();

    v8::Local<v8::Context> context = jsenv_->context();

    v8_inspector_->contextDestroyed(context);
  }
}

void JSInspectorSessionImpl::DispatchProtocolMessage(const jschar_t* message,
                                                     size_t size) {
  if (v8_session_) {
    StringView message_view(reinterpret_cast<const uint16_t*>(message), size);
    v8_session_->dispatchProtocolMessage(message_view);
  }
}

/*
 * ??????????????????????????????????????????, ?????????V8?????????????????????????????????, ?????????????????????
 * ??????????????????
 * 1 .??????: v8?????????stetho??????????????????, ??????????????????.
 * 2. v8??????: v8??????, ???stetho?????????, ??????????????????Runtime.enable ?????????
 * RuntimeAgent. ???OnJsContextReCreated?????????.
 * 3. page??????: ?????????page??????, stetho??????, ???v8????????????, ???????????????
 * Runtime.executionContextCreated ???????????????devtools, ??????devtools??? console
 * ??????, ???OnFrontendReload?????????.
 */
void JSInspectorSessionImpl::OnFrontendReload() {
  v8::Isolate* isolate = jsenv_->isolate();
  v8::Locker locker(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = jsenv_->context();
  v8::Context::Scope context_scope(context);
  if (v8_session_ && v8_inspector_) {
    v8::wrapper::InspectorReportExecutionContextCreated(
        context_group_id_, context, v8_inspector_.get(), v8_session_.get());
  }
}
// for future
void* JSInspectorSessionImpl::DispatchSessionCommand(int cmd, void* data) {
  return nullptr;
}

bool JSInspectorSessionImpl::CanDispatchMethod(const JSValue* method) {
  return true;
}

bool JSInspectorSessionImpl::GetStateJSON(JSValue* state, uint32_t flags) {
  return true;
}

// V8InspectorClient
void JSInspectorSessionImpl::runMessageLoopOnPause(int contextGroupId) {
  if (client_) {
    client_->RunMessageLoopOnPause(contextGroupId);
  }
}

void JSInspectorSessionImpl::quitMessageLoopOnPause() {
  if (client_) {
    client_->QuitMessageLoopOnPause();
  }
}

void JSInspectorSessionImpl::runIfWaitingForDebugger(int contextGroupId) {
  if (client_) {
    client_->RunIfWaitingForDebugger(contextGroupId);
  }
}

void JSInspectorSessionImpl::muteMetrics(int contextGroupId) {
  if (client_) {
    client_->MuteMetrics(contextGroupId);
  }
}
void JSInspectorSessionImpl::unmuteMetrics(int contextGroupId) {
  if (client_) {
    client_->UnmuteMetrics(contextGroupId);
  }
}

void JSInspectorSessionImpl::beginUserGesture() {
  if (client_) {
    client_->BeginUserGesture();
  }
}
void JSInspectorSessionImpl::endUserGesture() {
  if (client_) {
    client_->EndUserGesture();
  }
}
void JSInspectorSessionImpl::beginEnsureAllContextsInGroup(int contextGroupId) {
  if (client_) {
    client_->BeginEnsureAllContextsInGroup(contextGroupId);
  }
}
void JSInspectorSessionImpl::endEnsureAllContextsInGroup(int contextGroupId) {
  if (client_) {
    client_->EndEnsureAllContextsInGroup(contextGroupId);
  }
}

void JSInspectorSessionImpl::OnJsContextReCreated(int is_jscontext_recreated) {
  if (v8_session_ && is_jscontext_recreated != 0) {
    v8::wrapper::InspectorEnableRuntimeAgent(jsenv_->context(),
                                             v8_session_.get());
  }
}

void JSInspectorSessionImpl::sendResponse(
    int callId,
    std::unique_ptr<StringBuffer> message) {
  if (client_ && message) {
    const StringView& message_view = message->string();
    size_t length = 0;
    const jschar_t* data = GetString(message_view, &length);
    client_->SendResponse(callId, data, length);
  }
}

void JSInspectorSessionImpl::sendNotification(
    std::unique_ptr<StringBuffer> message) {
  if (client_ && message) {
    size_t length = 0;
    const jschar_t* data = GetString(message->string(), &length);
    client_->SendNotification(data, length);
  }
}

const jschar_t* JSInspectorSessionImpl::GetString(const StringView& str_view,
                                                  size_t* psize) {
  if (str_view.is8Bit()) {
    // TODO translate to jschar_t
    return nullptr;
  }

  if (psize) {
    *psize = str_view.length();
  }

  return reinterpret_cast<const jschar_t*>(str_view.characters16());
}

}  // namespace hybrid
