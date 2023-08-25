/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

namespace hybrid {
class J2V8Runtime;
typedef struct J2V8ObjectHandle_* J2V8ObjectHandle;

// implement the function need by jsenv
v8::Isolate* J2V8RuntimeGetIsolate(J2V8Runtime* runtime) {
  return reinterpret_cast<V8Runtime*>(runtime)->isolate;
}

v8::Local<v8::Context> J2V8RuntimeGetContext(J2V8Runtime* runtime) {
  V8Runtime* v8rt = reinterpret_cast<V8Runtime*>(runtime);
  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(v8rt->isolate, v8rt->context_);
  return context;
}

v8::Local<v8::Object> J2V8RuntimeGetGlobalObject(J2V8Runtime* runtime) {
  V8Runtime* v8rt = reinterpret_cast<V8Runtime*>(runtime);
  if (v8rt->globalObject) {
    v8::Local<v8::Object> object =
        v8::Local<v8::Object>::New(v8rt->isolate, *(v8rt->globalObject));
    return object;
  } else {
    v8::Local<v8::Context> context =
        v8::Local<v8::Context>::New(v8rt->isolate, v8rt->context_);
    return context->Global();
  }
}

void ThrowRuntimeException(J2V8Runtime* runtime, const char* message) {
  JNIEnv* env;
  V8Runtime* v8rt = reinterpret_cast<V8Runtime*>(runtime);
  if (v8rt->globalObject == nullptr) {  // for test
    return;
  }
  getJNIEnv(env);

  Local<String> v8_message =
      String::NewFromUtf8(v8rt->isolate, message).ToLocalChecked();

  v8::String::Value message_value(v8rt->isolate, v8_message);
  throwV8RuntimeException(env, &message_value);
}

void ThrowExecutionException(J2V8Runtime* runtime, v8::TryCatch* trycatch) {
  JNIEnv* env;
  V8Runtime* v8rt = reinterpret_cast<V8Runtime*>(runtime);
  if (v8rt->globalObject == nullptr) {  // for test
    return;
  }
  getJNIEnv(env);
  v8::Local<v8::Context> context = J2V8RuntimeGetContext(runtime);
  throwExecutionException(env, context, v8rt->isolate, trycatch,
                          reinterpret_cast<jlong>(runtime));
}

// declare function called in j2v8
const v8::StartupData* GetCustomJsSnapshot(
    const char* nativejs_snapshot_so_name);

bool OnCreateIsolate(J2V8Runtime* runtime);

void OnDestroyIsolate(J2V8Runtime* runtime);

}  // namespace hybrid
