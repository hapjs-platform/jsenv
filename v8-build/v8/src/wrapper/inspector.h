/*
 * Copyright (c) 2022, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef V8_WRAPPER_INSPECTOR_H_
#define V8_WRAPPER_INSPECTOR_H_

#include "v8.h"
#include "v8-inspector.h"

namespace v8 {

namespace wrapper {

void InspectorReportExecutionContextCreated(
      int context_group_id,
      v8::Local<v8::Context> context,
      v8_inspector::V8Inspector* inspector,
      v8_inspector::V8InspectorSession* session);

bool InspectorEnableRuntimeAgent(
      v8::Local<v8::Context> context,
      v8_inspector::V8InspectorSession* session);

}  // namepsace wrapper

}  // namespace v8
#endif  // V8_WRAPPER_INSPECTOR_H_
