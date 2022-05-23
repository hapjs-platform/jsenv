/*
 * Copyright (c) 2022, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "inspector.h"

#include "src/inspector/inspected-context.h"
#include "src/inspector/v8-inspector-impl.h"
#include "src/inspector/v8-inspector-session-impl.h"
#include "src/inspector/v8-runtime-agent-impl.h"

namespace v8 {

namespace wrapper {

void InspectorReportExecutionContextCreated(
      int context_group_id,
      v8::Local<v8::Context> context,
      v8_inspector::V8Inspector* inspector,
      v8_inspector::V8InspectorSession* session) {

  v8_inspector::V8InspectorImpl* inspector_impl =
        reinterpret_cast<v8_inspector::V8InspectorImpl*>(inspector);

  v8_inspector::V8InspectorSessionImpl* session_impl =
      reinterpret_cast<v8_inspector::V8InspectorSessionImpl*>(session);

  if (session_impl && session_impl->runtimeAgent()) {
    int context_id = v8_inspector::InspectedContext::contextId(context);
    v8_inspector::InspectedContext* inspector_context =
        inspector_impl->getContext(context_group_id, context_id);
    session_impl->runtimeAgent()->reportExecutionContextCreated(
        inspector_context);
  }
}

bool InspectorEnableRuntimeAgent(
      v8::Local<v8::Context> context,
      v8_inspector::V8InspectorSession* session) {
  v8_inspector::V8InspectorSessionImpl* session_impl =
      reinterpret_cast<v8_inspector::V8InspectorSessionImpl*>(session);
  if (session_impl && session_impl->runtimeAgent()) {
    session_impl->runtimeAgent()->enable();
    return true;
  }
  return false;
}


}  // namespace wrapper
}  // namespace v8

