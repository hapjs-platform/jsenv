/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_INSPECTOR_PROXY_H_
#define HYBRID_INSPECTOR_PROXY_H_

#include "JSEnv.h"

#include "j2v8-runtime.h"
#include "v8-inspector.h"
#include "v8.h"

namespace hybrid {

class JSEnvImpl;

class JSInspectorSessionImpl : public JSInspectorSession,
                               public v8_inspector::V8InspectorClient,
                               public v8_inspector::V8Inspector::Channel {
 public:
  JSInspectorSessionImpl(JSEnvImpl* env,
                         JSInspectorClient* client,
                         int context_group_id,
                         const char* state = nullptr,
                         int is_jscontext_recreated = 0);
  ~JSInspectorSessionImpl() override;

  // JSInspectorSession
  void DispatchProtocolMessage(const jschar_t* message, size_t size) override;

  void OnFrontendReload() override;

  // for future
  void* DispatchSessionCommand(int cmd, void* data) override;

  bool CanDispatchMethod(const JSValue* method) override;

  bool GetStateJSON(JSValue* state, uint32_t flags) override;

  void SchedulePauseOnNextStatement(const JSValue* break_reason,
                                    const JSValue* break_details) override {}
  void CancelPauseOnNextStatement() override {}
  void BreakProgram(const JSValue* break_reason,
                    const JSValue* break_details) override {}
  void SetSkipAllPauses(bool) override {}
  void Resume() override {}
  void StepOver() override {}

  // V8Inspector::Channel
  void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override;
  void flushProtocolNotifications() override {}

  // V8InspectorClient
  void runMessageLoopOnPause(int contextGroupId) override;

  void quitMessageLoopOnPause() override;

  void runIfWaitingForDebugger(int contextGroupId) override;

  void muteMetrics(int contextGroupId) override;
  void unmuteMetrics(int contextGroupId) override;

  void beginUserGesture() override;
  void endUserGesture() override;
  void beginEnsureAllContextsInGroup(int contextGroupId) override;
  void endEnsureAllContextsInGroup(int contextGroupId) override;

 private:
  void OnJsContextReCreated(int is_jscontext_recreated);
  const jschar_t* GetString(const v8_inspector::StringView& str_view,
                            size_t* psize);

  JSEnvImpl* jsenv_;
  std::unique_ptr<v8_inspector::V8Inspector> v8_inspector_;
  std::unique_ptr<v8_inspector::V8InspectorSession> v8_session_;
  JSInspectorClient* client_;
  int context_group_id_;
};

}  // namespace hybrid

#endif  // HYBRID_INSPECTOR_PROXY_H_
