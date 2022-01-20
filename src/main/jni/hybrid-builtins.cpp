/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "j2v8-runtime.h"

#include <string>
#include "hybrid-log.h"
#include "jsvalue_impl.h"

#include "v8.h"

#define TAG "HYBRID_BUILTINS"

using v8::Context;
using v8::External;
using v8::Function;
using v8::FunctionCallback;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Script;
using v8::ScriptOrigin;
using v8::String;
using v8::TryCatch;
using v8::Value;

namespace hybrid {

static J2V8Runtime* J2V8RuntimeFromCallback(
    const FunctionCallbackInfo<Value>& info) {
  Local<External> external = Local<External>::Cast(info.Data());
  return reinterpret_cast<J2V8Runtime*>(external->Value());
}

static void CompileAndRunScript(const FunctionCallbackInfo<Value>& info);

static struct {
  const char* name;
  FunctionCallback callback;
} builtin_funcs[] = {{"compileAndRunScript", CompileAndRunScript}};

static bool RegisterFunction(Isolate* isolate,
                             Local<Context> context,
                             Local<Object> exports,
                             const char* function_name,
                             FunctionCallback callback,
                             J2V8Runtime* runtime) {
  Local<String> v8_name;
  v8_name = String::NewFromUtf8(isolate, function_name).ToLocalChecked();

  Local<External> external = External::New(isolate, runtime);
  Local<Function> function =
      Function::New(context, callback, external).ToLocalChecked();

  v8::Maybe<bool> b1 = exports->Set(context, v8_name, function);  // modified
  if (b1.IsNothing()) {
    return false;
  }

  return true;
}

bool RegisterBuiltins(J2V8Runtime* runtime,
                      Isolate* isolate,
                      Local<Context> context) {
  Local<Object> exports = J2V8RuntimeGetGlobalObject(runtime);

  for (size_t i = 0; i < sizeof(builtin_funcs) / sizeof(builtin_funcs[0]);
       i++) {
    if (!RegisterFunction(isolate, context, exports, builtin_funcs[i].name,
                          builtin_funcs[i].callback, runtime)) {
      std::string error("Cannot Register builtin function:");
      error += builtin_funcs[i].name;
      ThrowRuntimeException(runtime, error.c_str());
      return false;
    }
  }

  return true;
}

/////////////////////////////////////////////////
static void CompileAndRunScript(const FunctionCallbackInfo<Value>& args) {
  J2V8Runtime* runtime = J2V8RuntimeFromCallback(args);

  if (args.Length() < 2) {
    ThrowRuntimeException(runtime,
                          "compileAndRunScript need 2 arguments at least");
    return;
  }

  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  Local<String> v8_code = Local<String>::Cast(args[0]);
  Local<String> v8_script_name = Local<String>::Cast(args[1]);

  if (v8_code.IsEmpty()) {
    return;  // run noting
  }

  TryCatch try_catch(isolate);

  ScriptOrigin origin(v8_script_name);
  Local<Script> script;

  if (!Script::Compile(context, v8_code, &origin).ToLocal(&script)) {
    if (try_catch.HasCaught()) {
      ThrowExecutionException(runtime, &try_catch);
    } else {
      ThrowRuntimeException(runtime,
                            "compileAndRunScript compile script faield");
    }
    return;
  }

  Local<Value> result;
  if (!script->Run(context).ToLocal(&result)) {
    if (try_catch.HasCaught()) {
      ThrowExecutionException(runtime, &try_catch);
    } else {
      ThrowRuntimeException(runtime, "compileAndRunScript run script faield");
    }
    return;
  }

  args.GetReturnValue().Set(result);
}

}  // namespace hybrid
