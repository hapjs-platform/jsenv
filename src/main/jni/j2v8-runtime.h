/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_J2V8_RUNTIME_H_
#define HYBRID_J2V8_RUNTIME_H_

#include "JSEnv.h"
#include "v8.h"

namespace hybrid {

class J2V8Runtime;

v8::Isolate* J2V8RuntimeGetIsolate(J2V8Runtime* runtime);

v8::Local<v8::Context> J2V8RuntimeGetContext(J2V8Runtime* runtime);

v8::Local<v8::Object> J2V8RuntimeGetGlobalObject(J2V8Runtime* runtime);

void ThrowRuntimeException(J2V8Runtime* runtime, const char* message);

void ThrowExecutionException(J2V8Runtime* runtime, v8::TryCatch* trycatch);

}  // namespace hybrid
#endif  // HYBRID_J2V8_RUNTIME_H_
