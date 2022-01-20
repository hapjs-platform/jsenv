/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#define TAG "JSENV_IMPL"
#include "jsenv-impl.h"

#include <dlfcn.h>
#include <libplatform/libplatform.h>
#include <sstream>
#include <string>

#include "hybrid-log.h"
#include "inspector-js-api.h"
#include "inspector-proxy.h"
#include "jsvalue_impl.h"
#include "logcat-console.h"

using v8::Array;
using v8::ArrayBuffer;
using v8::BigInt64Array;
using v8::BigUint64Array;
using v8::Context;
using v8::EscapableHandleScope;
using v8::Float32Array;
using v8::Float64Array;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Int16Array;
using v8::Int32Array;
using v8::Int8Array;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Locker;
using v8::Message;
using v8::NonCopyablePersistentTraits;
using v8::Object;
using v8::ObjectTemplate;
using v8::Persistent;
using v8::PersistentBase;
using v8::Promise;
using v8::Script;
using v8::ScriptOrigin;
using v8::StartupData;
using v8::String;
using v8::TryCatch;
using v8::TypedArray;
using v8::Uint16Array;
using v8::Uint32Array;
using v8::Uint8Array;
using v8::Uint8ClampedArray;
using v8::Value;
using v8::WeakCallbackInfo;
using v8::WeakCallbackType;
using v8::Function;
using v8::Number;
using v8::PromiseRejectMessage;
using v8::MaybeLocal;
using v8::NewStringType;


namespace {

#define NATIVEJS_SNAPSHOT_ENTRY "get_nativejs_blob"

typedef const void* (*get_nativejs_blob_cb)();

}  // namespace

namespace hybrid_v1000 {
void* CreateJSEnvImpl(hybrid::J2V8Runtime* runtime);
}  // namespace hybrid_v1000

namespace hybrid {

const int kJSEnvIsolateSoltIndex = v8::Isolate::GetNumberOfDataSlots() - 1;

void handle(PromiseRejectMessage message) {
  auto promise = message.GetPromise();
  auto event = message.GetEvent();
  auto value = message.GetValue();

  Isolate* isolate = promise->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  Local<String> quickApp =
      String::NewFromUtf8(isolate, "QuickApp", NewStringType::kNormal)
          .ToLocalChecked();
  Local<String> p =
      String::NewFromUtf8(isolate, "unhandledrejection", NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::Object> global =
      context->Global()->GetPrototype().As<v8::Object>();

  v8::Local<v8::Value> quickAppGlobalMaybe;
  (void)global->Get(context, quickApp).ToLocal(&quickAppGlobalMaybe);
  if (quickAppGlobalMaybe.IsEmpty() || !quickAppGlobalMaybe->IsObject()) {
    return;
  }

  Local<v8::Object> quickAppGlobal = quickAppGlobalMaybe.As<Object>();

  v8::Local<v8::Value> rejectFunc;
  (void)quickAppGlobal->Get(context, p).ToLocal(&rejectFunc);

  if (rejectFunc->IsFunction()) {
    Local<Function> f = rejectFunc.As<Function>();
    Local<Value> type = Number::New(isolate, event);
    Local<Value> args[] = {type, promise, value};
    // sometime V8 can't get the value needed
    // See https://github.com/nodejs/node/pull/29513
    if(value.IsEmpty()){
      value = v8::Undefined(isolate);
    }
    v8::TryCatch try_catch(isolate);
    (void)f->Call(context, Undefined(isolate), 3, args);
    if (try_catch.HasCaught() && !try_catch.HasTerminated()) {
      fprintf(stderr, "Exception in PromiseRejectCallback:\n");
    }
  }
}

JSEnvImpl::JSEnvImpl(J2V8Runtime* runtime)
    : runtime_(runtime),
      isolate_(nullptr),
      ref_count_(1),
      quickapp_jsruntime_handle_(nullptr),
      jsenv_v1000_(this) {
  isolate_ = J2V8RuntimeGetIsolate(runtime_);

  isolate_->SetData(kJSEnvIsolateSoltIndex, this);

  logcat_console_.reset(LogcatConsole::Create(isolate_));
  logcat_console_->Attach(isolate_);
  // set PromiseRejection
  // isolate_->SetPromiseRejectCallback(handle);
}

JSEnvImpl::~JSEnvImpl() {
  if (isolate_) {
    isolate_->SetData(kJSEnvIsolateSoltIndex, nullptr);
  }
}

int JSEnvImpl::GetVersion() const {
  return JSENV_VERSION;
}

void* JSEnvImpl::DispatchJSEnvCommand(int cmd, void* data) {
  return nullptr;
}

bool JSEnvImpl::HasException() const {
  return exception_.type != JSException::kNoneException;
}

JSException JSEnvImpl::GetException() const {
  return JSException(exception_.type, exception_.message.c_str());
}

void JSEnvImpl::SetException(JSException exception) {
  exception_ = exception;
}

void JSEnvImpl::ClearException() {
  exception_.Clear();
}

void JSEnvImpl::Detach() {
  if (logcat_console_ && isolate_) {
    HandleScope handle_scope(isolate_);
    if (logcat_console_) {
      logcat_console_->Detach(isolate_);
    }
  }

  if (isolate_) {
    isolate_->SetData(kJSEnvIsolateSoltIndex, nullptr);
  }
  isolate_ = nullptr;
  Release();
}

void JSEnvImpl::AddReference() {
  ref_count_++;
}

void JSEnvImpl::Release() {
  if (--ref_count_ == 0) {
    delete this;
  }
}

void JSEnvImpl::ResetLogcat() {
  if (logcat_console_) {
    logcat_console_->Attach(isolate_);
  }
}

JSInspectorSession* JSEnvImpl::CreateInspectorSession(
    JSInspectorClient* client,
    int context_group_id,
    const char* state,
    int is_jscontext_recreated) {
  return new JSInspectorSessionImpl(this, client, context_group_id, state,
                                      is_jscontext_recreated);
}

JSEnvImpl* JSEnvImpl::From(J2V8Runtime* runtime) {
  if (runtime == nullptr) {
    return nullptr;
  }

  v8::Isolate* isolate = J2V8RuntimeGetIsolate(runtime);
  if (isolate == nullptr) {
    return nullptr;
  }

  JSEnvImpl* env = From(isolate);
  if (env != nullptr) {
    return env;
  }

  return new JSEnvImpl(runtime);
}

JSEnvImpl* JSEnvImpl::From(Isolate* isolate) {
  return reinterpret_cast<JSEnvImpl*>(isolate->GetData(kJSEnvIsolateSoltIndex));
}

bool JSEnvImpl::ExecuteScript(const JSValue* code_value,
                              JSValue* presult,
                              const char* file_name /*   = nullptr */,
                              int start_lineno /* = 0 */,
                              uint32_t flags /* = 0*/) {
  HandleScope handle_scope(isolate_);
  TryCatch try_catch(isolate_);
  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Context::Scope context_scope(context);

  Local<Value> v8_code_value = ToV8Value(isolate_, code_value);

  if (v8_code_value.IsEmpty() || !v8_code_value->IsString()) {
    ALOGE(TAG, "ExecuteScript :Excecute a null value");
    return false;
  }

  Local<String> v8_code = Local<String>::Cast(v8_code_value);

  Local<String> v8_script_name;
  if (file_name) {
    if (!String::NewFromUtf8(isolate_, file_name, v8::NewStringType::kNormal)
             .ToLocal(&v8_script_name)) {
      // ignore
    }
  }

  if (v8_script_name.IsEmpty()) {
    v8_script_name = String::NewFromUtf8(isolate_, "").ToLocalChecked();
  }

  ScriptOrigin origin(v8_script_name, Integer::New(isolate_, start_lineno));

  Local<Script> script;

  if (!Script::Compile(context, v8_code, &origin).ToLocal(&script)) {
    ThrowException(&try_catch);
    return false;
  }

  Local<Value> result;

  if (!script->Run(context).ToLocal(&result)) {
    ThrowException(&try_catch);
  }

  if (presult) {
    return ToJSValue(isolate_, presult, result, flags);
  }

  return true;
}

void JSEnvImpl::ThrowException(v8::TryCatch* ptry_catch) {
  std::ostringstream out;

  String::Utf8Value exception(isolate_, ptry_catch->Exception());
  Handle<Message> message = ptry_catch->Message();

  ThrowExecutionException(runtime_, ptry_catch);

  if (message.IsEmpty()) {
    out << *exception;
  } else {
    String::Utf8Value filename(isolate_, message->GetScriptResourceName());
    int lineno =
        message->GetLineNumber(isolate_->GetCurrentContext()).FromMaybe(0);
    int start = message->GetStartPosition();
    int end = message->GetEndPosition();
    Local<String> v8_source_line;
    if (message->GetSourceLine(isolate_->GetCurrentContext())
            .ToLocal(&v8_source_line)) {
      String::Utf8Value source_line(isolate_, v8_source_line);
      out << *source_line << std::endl;
    } else {
      out << "No source line" << std::endl;
    }

    out << "@" << *filename << ":" << lineno << "(from " << start << " to "
        << end << " )" << std::endl;
    Local<Value> stack_trace;
    if (ptry_catch->StackTrace(isolate_->GetCurrentContext())
            .ToLocal(&stack_trace) &&
        stack_trace->IsString()) {
      String::Utf8Value stack_trace_string(isolate_,
                                           Local<String>::Cast(stack_trace));
      out << *stack_trace_string;
    }
  }

  exception_.Set(JSException::kJSException, out.str());
}

bool JSEnvImpl::RegisterCallbackOnObject(J2V8ObjectHandle object,
                                         const char* domain,
                                         UserFunctionCallback callback,
                                         void* user_data,
                                         uint32_t flags) {
  if (domain == nullptr || callback == nullptr) {
    return false;
  }

  // register function
  Isolate* isolate = J2V8RuntimeGetIsolate(runtime_);
  v8::Locker locker(isolate);
  Isolate::Scope isolate_scope(isolate);
  HandleScope handle_scope(isolate);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Object> v8_object;
  if (object == nullptr) {
    v8_object = J2V8RuntimeGetGlobalObject(runtime_);
  } else {
    v8_object = ToV8Object(isolate, object);
  }

  UserCallbackInfo user_callback(object, domain, callback, user_data, flags);

  size_t pos = user_callbacks_.size();

  user_callbacks_.push_back(user_callback);

  std::string owner_name;
  std::string func_name;

  ParseDomain(domain, &owner_name, &func_name);

  Local<Object> owner_object = CreateDomainObject(v8_object, owner_name);

  Local<String> v8_func_name;
  if (!String::NewFromUtf8(isolate, func_name.c_str(),
                           v8::NewStringType::kNormal)
           .ToLocal(&v8_func_name)) {
    user_callbacks_.pop_back();
    return false;
  }

  Local<Function> function;
  if (!Function::New(context, CallUserCallback,
                     Integer::New(isolate, static_cast<int>(pos)))
           .ToLocal(&function)) {
    return false;
  }

  v8::Maybe<bool> unusedResult =
      owner_object->Set(context, v8_func_name, function);
  if (unusedResult.IsNothing()) {
    return false;
  }

  return true;
}

void JSEnvImpl::CallUserCallback(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  JSEnvImpl* self = From(isolate);

  if (!self) {
    return;
  }

  Local<Integer> pos_value = Local<Integer>::Cast(args.Data());
  int pos = static_cast<int>(pos_value->Value());
  if (pos < 0 || pos >= static_cast<int>(self->user_callbacks_.size())) {
    ALOGE(TAG, "CallUserCallback failed, pos:%d", pos);
    return;
  }

  const UserCallbackInfo* pcallback = &(self->user_callbacks_[pos]);

  Arguments<> arguments(args.Length(), pcallback->flags);

  for (int i = 0; i < args.Length(); i++) {
    arguments.Add(isolate, args[i]);
  }

  JSValue result;

  bool bret =
      pcallback->callback(self, pcallback->user_data, pcallback->owner_handle,
                          arguments.args, arguments.argc, &result);

  // Check and throw exception
  if (self->ThrowExceptionToV8()) {
    return;
  }

  if (!bret) {
    return;
  }

  Local<Value> v8_result = ToV8Value(isolate, &result);
  args.GetReturnValue().Set(v8_result);
}

void JSEnvImpl::CallJSFunctionCallback(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();

  JSObject self = ToJSObject(args.This());

  JSEnvImpl* jsenv = From(isolate);

  if (!jsenv) {
    return;
  }

  Local<Integer> pos_value = Local<Integer>::Cast(args.Data());
  int pos = static_cast<int>(pos_value->Value());
  if (pos < 0 || pos >= static_cast<int>(jsenv->jsfunction_callbacks_.size())) {
    ALOGE(TAG, "CallJSFunctionCallback failed, pos:%d", pos);
    return;
  }

  const JSFunctionCallbackInfo* pcallback =
      &(jsenv->jsfunction_callbacks_[pos]);

  Arguments<> arguments(args.Length(), pcallback->flags);

  for (int i = 0; i < args.Length(); i++) {
    arguments.Add(isolate, args[i]);
  }

  JSValue result;

  bool bret = pcallback->callback(jsenv, pcallback->user_data, self,
                                  arguments.args, arguments.argc, &result);

  // Check and throw exception
  if (jsenv->ThrowExceptionToV8()) {
    return;
  }

  if (!bret) {
    return;
  }

  Local<Value> v8_result = ToV8Value(isolate, &result);

  args.GetReturnValue().Set(v8_result);
}

bool JSEnvImpl::ThrowExceptionToV8() {
  if (!HasException()) {
    return false;
  }

  isolate_->ThrowException(
      String::NewFromTwoByte(
          isolate_,
          reinterpret_cast<const uint16_t*>(exception_.message.c_str()),
          v8::NewStringType::kNormal,
          static_cast<int>(exception_.message.size()))
          .ToLocalChecked());

  ClearException();
  return true;
}

void JSEnvImpl::ParseDomain(const char* domain,
                            std::string* powner_name,
                            std::string* pfunc_name) {
  const char* dot = strrchr(domain, '.');
  if (dot == nullptr) {
    *powner_name = "";
    *pfunc_name = domain;
  } else {
    *pfunc_name = dot + 1;
    powner_name->clear();
    powner_name->insert(powner_name->begin(), domain, dot);
  }
}

Local<Object> JSEnvImpl::CreateDomainObject(Local<Object> object,
                                            const std::string owner_name) {
  if (owner_name.empty()) {
    return object;
  }

  const char* name = owner_name.c_str();
  const char* dot = name;
  do {
    dot = strchr(name, '.');
    Local<String> v8_name;
    if (String::NewFromUtf8(isolate_, name, v8::NewStringType::kNormal,
                            dot ? static_cast<int>(dot - name) : -1)
            .ToLocal(&v8_name)) {
      Local<Object> tmp = Object::New(isolate_);
      // object->Set(v8_name, tmp);//==wang==
      Local<Context> context = J2V8RuntimeGetContext(runtime_);
      v8::Maybe<bool> ret = object->Set(context, v8_name, tmp);
      ret.Check();

      object = tmp;
    }

    name = dot + 1;
  } while (dot);

  return object;
}

/////////////////////////////////////////////////////////////////////////////////
// version 1100
JSObject JSEnvImpl::J2V8ObjectHandleToJSObject(J2V8ObjectHandle j2v8_handle) {
  if (j2v8_handle == nullptr) {
    return nullptr;
  }

  InnerJ2V8ObjectHandle pinner_handle =
      reinterpret_cast<InnerJ2V8ObjectHandle>(j2v8_handle);

  return hybrid::ToJSObject(*pinner_handle);
}

// class support
JSClass JSEnvImpl::CreateClass(const JSClassDefinition* class_definition,
                               JSClass super) {
  if (class_definition == nullptr) {
    return nullptr;
  }

  if (class_definition->class_name != nullptr) {
    auto it = js_classes_.find(class_definition->class_name);
    if (it != js_classes_.end()) {
      JSClassTemplate* tmpl = it->second.get();
      return tmpl->ToJSClass();
    }
  }

  // register the class
  HandleScope handle_scope(isolate_);

  JSClassTemplate* new_class_tmpl = JSClassTemplate::Create(
      isolate_, class_definition, JSClassTemplate::From(super));

  if (new_class_tmpl == nullptr) {
    return nullptr;
  }

  js_classes_[new_class_tmpl->GetName()].reset(new_class_tmpl);

  return new_class_tmpl->ToJSClass();
}

JSClass JSEnvImpl::GetClass(const char* class_name) {
  if (class_name == nullptr) {
    return nullptr;
  }

  auto it = js_classes_.find(class_name);
  if (it != js_classes_.end()) {
    JSClassTemplate* tmpl = it->second.get();
    return tmpl->ToJSClass();
  }

  return nullptr;
}

JSObject JSEnvImpl::NewInstance(JSClass clazz) {
  JSClassTemplate* class_templ = JSClassTemplate::From(clazz);

  if (class_templ == nullptr) {
    return nullptr;
  }

  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Object> v8_object = class_templ->NewObject(context);

  return ToJSObject(escape_handle_scope.Escape(v8_object));
}

JSObject JSEnvImpl::NewInstanceWithConstructor(JSClass clazz,
                                               const JSValue* args,
                                               int argc) {
  EscapableHandleScope escape_handle_scope(isolate_);

  JSClassTemplate* clazz_templ = JSClassTemplate::From(clazz);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Function> function = clazz_templ->GetObjectConstructor(context);

  return ToJSObject(
      escape_handle_scope.Escape(CallConstructor(function, args, argc)));
}

JSObject JSEnvImpl::GetClassConstructorFunction(JSClass clazz) {
  EscapableHandleScope escape_handle_scope(isolate_);

  JSClassTemplate* clazz_templ = JSClassTemplate::From(clazz);

  if (!clazz_templ) {
    return nullptr;
  }

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Function> function = clazz_templ->GetObjectConstructor(context);

  return ToJSObject(escape_handle_scope.Escape(function));
}

JSObject JSEnvImpl::CallFunctionAsConstructor(JSObject function,
                                              const JSValue* argv,
                                              int argc) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Object> function_object = ToV8Object(isolate_, function);

  if (function_object.IsEmpty() || !function_object->IsFunction()) {
    return nullptr;
  }

  return ToJSObject(escape_handle_scope.Escape(
      CallConstructor(function_object.As<Function>(), argv, argc)));
}

// object type
int JSEnvImpl::GetObjectType(JSObject object) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty()) {
    return JSValue::kNull;
  }

  if (!v8_object->IsObject()) {
    return JSValue::kNull;
  }

  if (v8_object->IsFunction()) {
    return JSValue::kJSFunction;
  }

  if (v8_object->IsArray()) {
    return JSValue::kJSArray;
  }

  if (v8_object->IsTypedArray()) {
    return JSValue::kJSTypedArray;
  }

  if (v8_object->IsArrayBuffer()) {
    return JSValue::kJSArrayBuffer;
  }

  if (v8_object->IsDataView()) {
    return JSValue::kJSDataView;
  }

  if (v8_object->IsPromise()) {
    return JSValue::kJSPromise;
  }

  return JSValue::kJSObject;
}

int JSEnvImpl::GetTypedArrayType(JSObject object) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty() || !v8_object->IsTypedArray()) {
    return JSValue::kJSNotTypedArray;
  }

  if (v8_object->IsInt8Array()) {
    return JSValue::kJSInt8Array;
  }

  if (v8_object->IsInt16Array()) {
    return JSValue::kJSInt16Array;
  }

  if (v8_object->IsInt32Array()) {
    return JSValue::kJSInt32Array;
  }

  if (v8_object->IsUint8Array()) {
    return JSValue::kJSUint8Array;
  }

  if (v8_object->IsUint8ClampedArray()) {
    return JSValue::kJSUint8ClampedArray;
  }

  if (v8_object->IsUint16Array()) {
    return JSValue::kJSUint16Array;
  }

  if (v8_object->IsUint32Array()) {
    return JSValue::kJSUint32Array;
  }

  if (v8_object->IsFloat32Array()) {
    return JSValue::kJSFloat32Array;
  }

  if (v8_object->IsFloat64Array()) {
    return JSValue::kJSFloat64Array;
  }

  return JSValue::kJSNotTypedArray;
}

// object reference
JSObject JSEnvImpl::NewObjectReference(JSObject object) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty()) {
    return nullptr;
  }

  NonCopyablePersistentTraits<Object>::NonCopyablePersistent global_ref(
      isolate_, v8_object);

  return ToJSObject(global_ref);
}

static void WeakReferenceCallbackIgnore(const WeakCallbackInfo<void>& data) {
  // do nothing;
}

JSObject JSEnvImpl::NewObjectWeakReference(
    JSObject object,
    JSWeakReferenceCallback weak_callback /* = nullptr*/,
    void* user_data /* = nullptr*/) {
  HandleScope handle_scope(isolate_);
  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty()) {
    return nullptr;
  }

  WeakCallbackInfo<void>::Callback callback = WeakReferenceCallbackIgnore;

  if (weak_callback) {
    callback =
        reinterpret_cast<WeakCallbackInfo<void>::Callback>(weak_callback);
  }

  NonCopyablePersistentTraits<Object>::NonCopyablePersistent global_ref(
      isolate_, v8_object);

  global_ref.SetWeak(user_data, callback, WeakCallbackType::kInternalFields);

  return ToJSObject(global_ref);
}

void JSEnvImpl::GetWeakReferenceCallbackInfo(const void* weak_data,
                                             void** p_user_data,
                                             void** p_internal_fields) {
  const WeakCallbackInfo<void>* pweak_callback_info =
      reinterpret_cast<const WeakCallbackInfo<void>*>(weak_data);

  if (p_user_data) {
    *p_user_data = pweak_callback_info->GetParameter();
  }

  if (p_internal_fields) {
    p_internal_fields[0] = pweak_callback_info->GetInternalField(0);
    p_internal_fields[1] = pweak_callback_info->GetInternalField(1);
  }
}

void JSEnvImpl::DeleteObjectReference(JSObject object) {
  if (IsPersistentObject(object)) {
    uintptr_t val = reinterpret_cast<uintptr_t>(object) & ~1;
    if (val == 0) {
      return;
    }

    Persistent<Object> persist_obj;
    uintptr_t* p_obj_val = reinterpret_cast<uintptr_t*>(&persist_obj);
    *p_obj_val = val;
    persist_obj.Reset();  // Reset the object
  }
}

// private data access
void* JSEnvImpl::GetObjectPrivateData(JSObject object) {
  return GetJSObjectPrivateData(object, 0);
}

bool JSEnvImpl::SetObjectPrivateData(JSObject object, void* user_data) {
  return SetJSObjectPrivateData(object, 0, user_data);
}
void* JSEnvImpl::GetObjectPrivateExtraData(JSObject object) {
  return GetJSObjectPrivateData(object, 1);
}

bool JSEnvImpl::SetObjectPrivateExtraData(JSObject object, void* user_data) {
  return SetJSObjectPrivateData(object, 1, user_data);
}

void* JSEnvImpl::GetJSObjectPrivateData(JSObject object, int index) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty()) {
    return nullptr;
  }

  if (v8_object->InternalFieldCount() <= index) {
    return nullptr;
  }

  return v8_object->GetAlignedPointerFromInternalField(index);
}

bool JSEnvImpl::SetJSObjectPrivateData(JSObject object,
                                       int index,
                                       void* user_data) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty()) {
    return false;
  }

  if (v8_object->InternalFieldCount() <= index) {
    return false;
  }

  v8_object->SetAlignedPointerInInternalField(index, user_data);
  return true;
}

// global object
JSObject JSEnvImpl::GetGlobalObject() {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  return ToJSObject(escape_handle_scope.Escape(context->Global()));
}

bool JSEnvImpl::SetGlobal(const JSValue* pkey, const JSValue* pvalue) {
  if (pkey == nullptr || pvalue == nullptr) {
    ALOGE("JSENV", "SetGlobal key or value is null");
    return false;
  }

  HandleScope handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Value> v8_key = ToV8Value(isolate_, pkey);
  Local<Value> v8_value = ToV8Value(isolate_, pvalue);

  if (v8_key.IsEmpty()) {
    ALOGE("JSENV", "SetGlobal key is null");
    return false;
  }

  bool bret = false;
  if (context->Global()->Set(context, v8_key, v8_value).To(&bret)) {
    return true;
  }

  return bret;
}

bool JSEnvImpl::GetGlobal(const JSValue* pkey,
                          JSValue* pvalue,
                          uint32_t flags /* = 0*/) {
  if (pkey == nullptr || pvalue == nullptr) {
    return false;
  }

  EscapableHandleScope escape_handle_scope(isolate_);
  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Value> v8_key = ToV8Value(isolate_, pkey);

  if (v8_key.IsEmpty()) {
    return false;
  }

  Local<Value> v8_value;

  if (!context->Global()->Get(context, v8_key).ToLocal(&v8_value)) {
    return false;
  }

  return ToJSValue(isolate_, pvalue, escape_handle_scope.Escape(v8_value),
                   flags);
}

// object property
bool JSEnvImpl::GetObjectProperty(JSObject object,
                                  const JSValue* pkey,
                                  JSValue* pvalue,
                                  uint32_t flags /* = 0*/) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);
  Isolate* isolate = isolate_;

  if (pkey == nullptr || pvalue == nullptr) {
    return false;
  }

  Local<Object> v8_object = ToV8Object(isolate, object);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return false;
  }

  Local<Value> key = ToV8Value(isolate, pkey);

  if (key.IsEmpty()) {
    return false;
  }

  Local<Value> value;
  if (!v8_object->Get(context, key).ToLocal(&value)) {
    return false;
  }

  return ToJSValue(isolate, pvalue, escape_handle_scope.Escape(value), flags);
}

bool JSEnvImpl::SetObjectProperty(JSObject object,
                                  const JSValue* pkey,
                                  const JSValue* pvalue) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);
  Isolate* isolate = isolate_;

  if (pkey == nullptr || pvalue == nullptr) {
    return false;
  }

  Local<Object> v8_object = ToV8Object(isolate, object);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return false;
  }

  Local<Value> key = ToV8Value(isolate, pkey);

  if (key.IsEmpty()) {
    return false;
  }

  Local<Value> v8_value = ToV8Value(isolate, pvalue);

  bool bret = false;
  if (v8_object->Set(context, key, escape_handle_scope.Escape(v8_value))
          .To(&bret)) {
    return false;
  }

  return bret;
}

JSObject JSEnvImpl::GetObjectPropertyNames(JSObject object) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);
  Isolate* isolate = isolate_;

  Local<Object> v8_object = ToV8Object(isolate, object);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return nullptr;
  }

  Local<Array> property_names;
  if (!v8_object->GetPropertyNames(context).ToLocal(&property_names)) {
    return nullptr;
  }

  return ToJSObject(escape_handle_scope.Escape(property_names));
}

// array access
size_t JSEnvImpl::GetObjectLength(JSObject object) {
  HandleScope handle_scope(isolate_);

  Isolate* isolate = isolate_;

  Local<Object> v8_object = ToV8Object(isolate, object);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return 0;
  }

  if (v8_object->IsArray()) {
    return v8_object.As<Array>()->Length();
  } else if (v8_object->IsTypedArray()) {
    return v8_object.As<TypedArray>()->Length();
  } else if (v8_object->IsArrayBuffer()) {
    return v8_object.As<ArrayBuffer>()->ByteLength();
  }

  return 0;
}

bool JSEnvImpl::GetObjectAtIndex(JSObject object,
                                 int index,
                                 JSValue* pvalue,
                                 uint32_t flags /* = 0*/) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);
  Isolate* isolate = isolate_;

  Local<Object> v8_object = ToV8Object(isolate, object);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return false;
  }

  Local<Value> v8_value;

  if (!v8_object->Get(context, static_cast<uint32_t>(index))
           .ToLocal(&v8_value)) {
    return false;
  }

  return ToJSValue(isolate, pvalue, escape_handle_scope.Escape(v8_value),
                   flags);
}

bool JSEnvImpl::SetObjectAtIndex(JSObject object,
                                 int index,
                                 const JSValue* pvalue) {
  HandleScope handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);
  Isolate* isolate = isolate_;

  Local<Object> v8_object = ToV8Object(isolate, object);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return false;
  }

  Local<Value> v8_value = ToV8Value(isolate, pvalue);

  bool bret = false;

  if (!v8_object->Set(context, static_cast<uint32_t>(index), v8_value)
           .To(&bret)) {
    return false;
  }

  return bret;
}

// function call
JSObject JSEnvImpl::NewFunction(JSFunctionCallback callback,
                                void* user_data,
                                uint32_t flags /* = 0 */) {
  if (callback == nullptr) {
    return nullptr;
  }
  Isolate* isolate = isolate_;
  v8::Locker locker(isolate);
  EscapableHandleScope escape_handle_scope(isolate);
  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  JSFunctionCallbackInfo jsfunction_callback(callback, user_data, flags);

  size_t pos = jsfunction_callbacks_.size();

  jsfunction_callbacks_.push_back(jsfunction_callback);

  Local<Function> function;
  if (!Function::New(context, CallJSFunctionCallback,
                     Integer::New(isolate, static_cast<int>(pos)))
           .ToLocal(&function)) {
    return nullptr;
  }
  return ToJSObject(escape_handle_scope.Escape(function));
}

bool JSEnvImpl::CallFunction(JSObject function,
                             JSObject self,
                             const JSValue* argv,
                             int argc,
                             JSValue* presult,
                             uint32_t flags /* = 0*/) {
  Isolate* isolate = isolate_;
  EscapableHandleScope escape_handle_scope(isolate);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Object> v8_function_object = ToV8Object(isolate, function);

  if (v8_function_object.IsEmpty() || !v8_function_object->IsFunction()) {
    return false;
  }

  Local<Function> v8_function = v8_function_object.As<Function>();

  Local<Object> v8_self = ToV8Object(isolate, self);

  if (!v8_self.IsEmpty() && !v8_self->IsObject()) {
    return false;
  }

  if (v8_self.IsEmpty()) {
    v8_self = context->Global();
  }

  TryCatch try_catch(isolate);

  V8Arguments<> arguments(isolate, argv, argc);

  Local<Value> result;

  if (!v8_function->Call(context, v8_self, arguments.argc, arguments.args)
           .ToLocal(&result)) {
    return false;
  }

  if (try_catch.HasCaught()) {
    ThrowException(&try_catch);
    return false;
  }

  if (presult) {
    return ToJSValue(isolate, presult, escape_handle_scope.Escape(result),
                     flags);
  }
  return true;
}

static size_t GetTypedArrayElementSize(int array_type) {
  switch (array_type) {
    case JSValue::kJSInt8Array:
    case JSValue::kJSUint8Array:
    case JSValue::kJSUint8ClampedArray:
      return 1;
    case JSValue::kJSInt16Array:
    case JSValue::kJSUint16Array:
      return 2;
    case JSValue::kJSInt32Array:
    case JSValue::kJSUint32Array:
    case JSValue::kJSFloat32Array:
      return 4;
    case JSValue::kJSInt64Array:
    case JSValue::kJSUint64Array:
    case JSValue::kJSFloat64Array:
      return 8;
    default:
      return 0;
  }
}

static Local<TypedArray> CreateTypedArray(Isolate* isolate,
                                          int array_type,
                                          Local<ArrayBuffer> array_buffer,
                                          size_t byte_offset,
                                          size_t length) {
  switch (array_type) {
    case JSValue::kJSInt8Array:
      return Int8Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSInt16Array:
      return Int16Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSInt32Array:
      return Int32Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSInt64Array:
      return BigInt64Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSUint8Array:
      return Uint8Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSUint8ClampedArray:
      return Uint8ClampedArray::New(array_buffer, byte_offset, length);
    case JSValue::kJSUint16Array:
      return Uint16Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSUint32Array:
      return Uint32Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSUint64Array:
      return BigUint64Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSFloat32Array:
      return Float32Array::New(array_buffer, byte_offset, length);
    case JSValue::kJSFloat64Array:
      return Float64Array::New(array_buffer, byte_offset, length);
  }

  return Local<TypedArray>();
}

JSObject JSEnvImpl::NewObject() {
  Local<Object> object = Object::New(isolate_);

  return ToJSObject(object);
}

JSObject JSEnvImpl::NewArray(size_t length) {
  Local<Object> array = Array::New(isolate_, length);

  return ToJSObject(array);
}

JSObject JSEnvImpl::NewArrayWithValues(const JSValue* argv, int argc) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Object> array = Array::New(isolate_, argc);

  if (argv != nullptr) {
    for (int i = 0; i < argc; i++) {
      Local<Value> value = ToV8Value(isolate_, &argv[i]);
      bool bret;
      if (array->Set(context, static_cast<uint32_t>(i), value).To(&bret)) {
      }
    }
  }

  return ToJSObject(escape_handle_scope.Escape(array));
}

// TypedArray access
JSObject JSEnvImpl::NewTypedArray(int array_type, size_t element_count) {
  Isolate* isolate = isolate_;

  EscapableHandleScope escape_handle_scope(isolate);

  if (element_count <= 0) {
    return nullptr;
  }

  size_t element_size = GetTypedArrayElementSize(array_type);

  if (element_size == 0) {
    return nullptr;
  }

  size_t length = element_size * element_count;

  Local<ArrayBuffer> array_buffer = ArrayBuffer::New(isolate, length);

  if (array_buffer.IsEmpty()) {
    return nullptr;
  }

  Local<TypedArray> typed_array =
      CreateTypedArray(isolate, array_type, array_buffer, 0, length);

  return ToJSObject(escape_handle_scope.Escape(typed_array));
}

JSObject JSEnvImpl::NewTypedArrayArrayBuffer(int array_type,
                                             JSObject array_buffer,
                                             size_t element_offset,
                                             size_t element_count) {
  Isolate* isolate = isolate_;

  EscapableHandleScope escape_handle_scope(isolate);

  Local<ArrayBuffer> v8_array_buffer =
      ToV8Object(isolate, array_buffer).As<ArrayBuffer>();

  if (v8_array_buffer.IsEmpty()) {
    return nullptr;
  }

  if (element_count <= 0) {
    return nullptr;
  }

  size_t element_size = GetTypedArrayElementSize(array_type);

  if (element_size == 0) {
    return nullptr;
  }

  size_t byte_offset = element_size * element_offset;
  size_t length = element_size * element_count;

  Local<TypedArray> typed_array = CreateTypedArray(
      isolate, array_type, v8_array_buffer, byte_offset, length);

  return ToJSObject(escape_handle_scope.Escape(typed_array));
}

size_t JSEnvImpl::GetTypedArrayCount(JSObject typed_array) {
  HandleScope handle_scope(isolate_);

  Local<Object> object = ToV8Object(isolate_, typed_array);

  if (object.IsEmpty() || !object->IsTypedArray()) {
    return 0;
  }

  return object.As<TypedArray>()->Length();
}

void* JSEnvImpl::GetTypedArrayPointer(JSObject typed_array,
                                      size_t element_offset) {
  HandleScope handle_scope(isolate_);

  Local<Object> object = ToV8Object(isolate_, typed_array);

  if (object.IsEmpty()) {
    return nullptr;
  }

  if (!object->IsTypedArray()) {
    return nullptr;
  }

  Local<TypedArray> v8_typed_array = object.As<TypedArray>();

  if (v8_typed_array->Length() <= element_offset) {
    return nullptr;
  }

  Local<ArrayBuffer> v8_array_buffer = v8_typed_array->Buffer();

  size_t byte_offset = v8_typed_array->ByteOffset();

  ArrayBuffer::Contents contents = v8_array_buffer->GetContents();

  uint8_t* pdata = reinterpret_cast<uint8_t*>(contents.Data());

  if (pdata == nullptr) {
    return nullptr;
  }

  return pdata + byte_offset;
}

JSObject JSEnvImpl::GetTypedArrayArrayBuffer(JSObject typed_array) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Object> object = ToV8Object(isolate_, typed_array);

  if (object.IsEmpty() || !object->IsTypedArray()) {
    return nullptr;
  }

  Local<TypedArray> v8_typed_array = object.As<TypedArray>();

  Local<ArrayBuffer> v8_array_buffer = v8_typed_array->Buffer();

  return ToJSObject(escape_handle_scope.Escape(v8_array_buffer));
}

// ArrayBuffer access
JSObject JSEnvImpl::NewArrayBuffer(size_t length) {
  if (length <= 0) {
    return nullptr;
  }

  EscapableHandleScope escape_handle_scope(isolate_);

  Local<ArrayBuffer> array_buffer = ArrayBuffer::New(isolate_, length);

  return ToJSObject(escape_handle_scope.Escape(array_buffer));
}

JSObject JSEnvImpl::NewArrayBufferExternal(
    void* byte,
    size_t length,
    JSArrayBufferReleaseExteranlCallback release_callback,
    void* user_data) {
  if (byte == nullptr || length <= 0) {
    return nullptr;
  }

  EscapableHandleScope escape_handle_scope(isolate_);

  Local<ArrayBuffer> array_buffer = ArrayBuffer::New(isolate_, byte, length);

  if (array_buffer.IsEmpty()) {
    return nullptr;
  }

  // Make Weak Reference
  if (release_callback) {
    NonCopyablePersistentTraits<Object>::NonCopyablePersistent weak_ref(
        isolate_, array_buffer);
    WeakCallbackInfo<void>::Callback weak_callback =
        reinterpret_cast<WeakCallbackInfo<void>::Callback>(release_callback);

    weak_ref.SetWeak(user_data, weak_callback, WeakCallbackType::kParameter);
  }

  return ToJSObject(escape_handle_scope.Escape(array_buffer));
}

size_t JSEnvImpl::GetArrayBufferLength(JSObject object) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty() || !v8_object->IsArrayBuffer()) {
    return 0;
  }

  Local<ArrayBuffer> array_buffer = v8_object.As<ArrayBuffer>();

  return array_buffer->ByteLength();
}

void* JSEnvImpl::GetArrayBufferPointer(JSObject object,
                                       size_t* plength /* = nullptr*/) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, object);

  if (v8_object.IsEmpty() || !v8_object->IsArrayBuffer()) {
    return 0;
  }

  Local<ArrayBuffer> array_buffer = v8_object.As<ArrayBuffer>();

  if (plength) {
    *plength = array_buffer->ByteLength();
  }

  ArrayBuffer::Contents contents = array_buffer->GetContents();

  return contents.Data();
}

// promise
JSObject JSEnvImpl::CreateResolver() {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Promise::Resolver> resolver;

  if (!Promise::Resolver::New(context).ToLocal(&resolver)) {
    return nullptr;
  }

  return ToJSObject(escape_handle_scope.Escape(resolver));
}

JSObject JSEnvImpl::GetPromiseFromResolver(JSObject resolver) {
  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, resolver);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return nullptr;
  }

  Local<Promise::Resolver> v8_resolver =
      Local<Promise::Resolver>::Cast(v8_object);

  if (v8_resolver.IsEmpty()) {
    return nullptr;
  }

  Local<Promise> promise = v8_resolver->GetPromise();

  return ToJSObject(escape_handle_scope.Escape(promise));
}

bool JSEnvImpl::Resolve(JSObject resolver, const JSValue* pvalue) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, resolver);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  if (v8_object.IsEmpty()) {
    return false;
  }

  Local<Promise::Resolver> v8_resolver =
      Local<Promise::Resolver>::Cast(v8_object);

  if (v8_resolver.IsEmpty()) {
    return false;
  }

  Local<Value> v8_value = ToV8Value(isolate_, pvalue);

  bool bret = false;
  if (v8_resolver->Resolve(context, v8_value).To(&bret)) {
    return bret;
  }

  return false;
}

bool JSEnvImpl::Reject(JSObject resolver, const JSValue* pvalue) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_object = ToV8Object(isolate_, resolver);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  if (v8_object.IsEmpty() || !v8_object->IsObject()) {
    return false;
  }

  Local<Promise::Resolver> v8_resolver =
      Local<Promise::Resolver>::Cast(v8_object);

  if (v8_resolver.IsEmpty()) {
    return false;
  }

  Local<Value> v8_value = ToV8Value(isolate_, pvalue);

  bool bret = false;
  if (v8_resolver->Reject(context, v8_value).To(&bret)) {
    return bret;
  }

  return false;
}

bool JSEnvImpl::SetPromiseThen(JSObject promise, JSObject function) {
  HandleScope handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Promise> v8_promise_tmp;
  Local<Object> v8_promise_object;
  Local<Promise> v8_promise;

  v8_promise_object = ToV8Object(isolate_, promise);

  if (v8_promise_object.IsEmpty() || !v8_promise_object->IsPromise()) {
    return false;
  }

  v8_promise = v8_promise_object.As<Promise>();

  Local<Object> v8_function_object = ToV8Object(isolate_, function);
  if (v8_function_object.IsEmpty() || !v8_function_object->IsFunction()) {
    return false;
  }

  Local<Function> v8_function = v8_function_object.As<Function>();

  if (v8_promise->Then(context, v8_function).ToLocal(&v8_promise_tmp)) {
    return true;
  }

  return false;
}

bool JSEnvImpl::SetPromiseCatch(JSObject promise, JSObject function) {
  HandleScope handle_scope(isolate_);

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  Local<Promise> v8_promise_tmp;
  Local<Object> v8_promise_object;
  Local<Promise> v8_promise;

  v8_promise_object = ToV8Object(isolate_, promise);

  if (v8_promise_object.IsEmpty() || !v8_promise_object->IsPromise()) {
    return false;
  }

  v8_promise = v8_promise_object.As<Promise>();

  Local<Object> v8_function_object = ToV8Object(isolate_, function);
  if (v8_function_object.IsEmpty() || !v8_function_object->IsFunction()) {
    return false;
  }

  Local<Function> v8_function = v8_function_object.As<Function>();

  if (v8_promise->Catch(context, v8_function).ToLocal(&v8_promise_tmp)) {
    return true;
  }

  return false;
}

bool JSEnvImpl::PromiseHasHandler(JSObject promise) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_promise_object;
  Local<Promise> v8_promise;

  v8_promise_object = ToV8Object(isolate_, promise);

  if (v8_promise_object.IsEmpty() || !v8_promise_object->IsPromise()) {
    return false;
  }

  v8_promise = v8_promise_object.As<Promise>();

  return v8_promise->HasHandler();
}

int JSEnvImpl::GetPromiseState(JSObject promise) {
  HandleScope handle_scope(isolate_);

  Local<Object> v8_promise_object;
  Local<Promise> v8_promise;

  v8_promise_object = ToV8Object(isolate_, promise);

  if (v8_promise_object.IsEmpty() || !v8_promise_object->IsPromise()) {
    return kJSPromiseStateNoState;
  }

  v8_promise = v8_promise_object.As<Promise>();

  return v8_promise->State();
}

bool JSEnvImpl::GetPromiseResult(JSObject promise,
                                 JSValue* pvalue,
                                 uint32_t flags /* = 0*/) {
  if (pvalue == nullptr) {
    return false;
  }

  EscapableHandleScope escape_handle_scope(isolate_);

  Local<Object> v8_promise_object;
  Local<Promise> v8_promise;

  v8_promise_object = ToV8Object(isolate_, promise);

  if (v8_promise_object.IsEmpty() || !v8_promise_object->IsPromise()) {
    return false;
  }

  v8_promise = v8_promise_object.As<Promise>();

  if (v8_promise->State() == Promise::kPending) {
    return false;
  }

  Local<Value> result = v8_promise->Result();

  if (result.IsEmpty()) {
    return false;
  }

  return ToJSValue(isolate_, pvalue, escape_handle_scope.Escape(result), flags);
}

struct JSEnvHandleScope {
  Locker locker;
  Isolate::Scope isolate_scope;
  HandleScope handle_scope;
  Context::Scope context_scope;

  JSEnvHandleScope(Isolate* isolate, J2V8Runtime* runtime)
      : locker(isolate),
        isolate_scope(isolate),
        handle_scope(isolate),
        context_scope(J2V8RuntimeGetContext(runtime)) {}

  ~JSEnvHandleScope() {}
};

// scope
void JSEnvImpl::PushScope() {
  JSEnvHandleScope* scope = new JSEnvHandleScope(isolate_, runtime_);

  handle_scopes_.push_back(scope);
}

void JSEnvImpl::PopScope() {
  if (handle_scopes_.size() > 0) {
    JSEnvHandleScope* scope = handle_scopes_.back();
    if (scope) {
      delete scope;
    }
    handle_scopes_.pop_back();
  }
}

Local<Object> JSEnvImpl::CallConstructor(Local<Function> function,
                                         const JSValue* args,
                                         int argc) {
  Isolate* isolate = isolate_;

  Local<Context> context = J2V8RuntimeGetContext(runtime_);

  TryCatch try_catch(isolate);

  V8Arguments<> arguments(isolate, args, argc);

  Local<Value> result;

  if (!function->NewInstance(context, arguments.argc, arguments.args)
           .ToLocal(&result)) {
    return Local<Object>();
  }

  if (try_catch.HasCaught()) {
    ThrowException(&try_catch);
    return Local<Object>();
  }

  return result.As<Object>();
}

///////////////////////////////////////
bool RegisterBuiltins(J2V8Runtime* runtime,
                      Isolate* isolate,
                      Local<Context> context);

const StartupData* GetCustomJsSnapshot(const char* nativejs_snapshot_so_name) {
  if (nativejs_snapshot_so_name == NULL) {
    return nullptr;
  }
  void* nativejs_handle = dlopen(nativejs_snapshot_so_name, RTLD_NOW);
  if (!nativejs_handle) {
    ALOGE(TAG, "GetCustomJsSnapshot: dlopen error:%s",
          nativejs_snapshot_so_name);
    return nullptr;
  }
  get_nativejs_blob_cb get_nativejs_blob =
      reinterpret_cast<get_nativejs_blob_cb>(
          dlsym(nativejs_handle, NATIVEJS_SNAPSHOT_ENTRY));
  if (!get_nativejs_blob) {
    ALOGE(TAG, "GetCustomJsSnapshot: dlsym error:%s", NATIVEJS_SNAPSHOT_ENTRY);
    return nullptr;
  }
  return reinterpret_cast<const v8::StartupData*>(get_nativejs_blob());
}

// extern "C" void* QuickAppJSRuntimeInit(void* vm, void* context);
// extern "C" void QuickAppJSRuntimeDeInit(void* isolate);

bool OnCreateIsolate(J2V8Runtime* runtime) {
  Isolate* isolate = J2V8RuntimeGetIsolate(runtime);
  HandleScope handle_scope(isolate);
  Local<Context> context = J2V8RuntimeGetContext(runtime);
  Context::Scope context_scope(context);

  JSEnvImpl* jsenv = JSEnvImpl::From(runtime);

  // Init native api for js
  JSBindingConnection::Init(jsenv);

  // jsenv->SetQuickAppJSRuntimeHandle(MinaJSRuntimeInit(isolate, &context));

  return RegisterBuiltins(runtime, isolate, context);
}

void OnDestroyIsolate(J2V8Runtime* runtime) {
  Isolate* isolate = J2V8RuntimeGetIsolate(runtime);
  JSEnvImpl* js_env = JSEnvImpl::From(isolate);
  if (js_env) {
    // QuickAppJSRuntimeDeInit(js_env->GetMinaJSRuntimeHandle());
    js_env->Detach();
  }
}

/////////////////////////////////////
// export  get_jsenv

// only for test
struct V8Runtime {
  Isolate* isolate;
  Persistent<Context> context;
  Persistent<Object>* globalObject;
};

static void TestPrint(const FunctionCallbackInfo<Value>& args) {
  std::ostringstream oss;
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  for (int i = 0; i < args.Length(); i++) {
    Local<String> v8_str;
    if (args[i]->IsString()) {
      v8_str = args[i].As<String>();
    } else if (!args[i]->ToString(context).ToLocal(&v8_str)) {
      continue;
    }

    if (!v8_str.IsEmpty()) {
      String::Utf8Value utf8_str(isolate, v8_str);
      oss << *utf8_str << ",";
    }
  }
  ALOGD("JSENV PRINT", "%s", oss.str().c_str());
}

static J2V8Handle create_test_j2v8handle() {
  std::unique_ptr<v8::Platform> v8Platform;

  v8::V8::InitializeICUDefaultLocation("", nullptr);
  v8Platform = v8::platform::NewDefaultPlatform();

  v8::V8::InitializePlatform(v8Platform.get());
  v8::V8::Initialize();

  V8Runtime* runtime = new V8Runtime();
  runtime->globalObject = nullptr;

  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      ArrayBuffer::Allocator::NewDefaultAllocator();
  runtime->isolate = Isolate::New(create_params);

  Isolate::Scope isolate_scope(runtime->isolate);
  HandleScope handle_scope(runtime->isolate);
  Local<ObjectTemplate> globalObject = ObjectTemplate::New(runtime->isolate);
  globalObject->Set(ToV8String(runtime->isolate, "print"),
                    FunctionTemplate::New(runtime->isolate, TestPrint));

  Local<Context> context =
      Context::New(runtime->isolate, nullptr, globalObject);
  runtime->context.Reset(runtime->isolate, context);

  return reinterpret_cast<J2V8Handle>(runtime);
}

__attribute__((visibility("default"))) extern "C" void* get_jsenv(
    J2V8Handle handle,
    int version) {
  if (reinterpret_cast<intptr_t>(handle) == -1) {
    handle = create_test_j2v8handle();
  }

  if (version > JSENV_VERSION && !handle) {
    return nullptr;
  }

  JSEnvImpl* jsenv_impl =
      JSEnvImpl::From(reinterpret_cast<J2V8Runtime*>(handle));

  if (version <= 1000) {
    return jsenv_impl->GetJSEnvV1000();
  }
  return jsenv_impl;
}

#if defined(SUPPORT_J2V8RUNTIME)
extern "C" void GetJ2V8PlatformHandles(J2V8Runtime* runtime,
                                       void** pvm,
                                       void** pcontext) {
  if (pvm == nullptr && pcontext == nullptr) {
    return;
  }

  if (pvm) {
    *pvm = nullptr;
  }
  if (pcontext) {
    *pcontext = nullptr;
  }
  if (!runtime) {
    return;
  }

  if (pvm) {
    *pvm = reinterpret_cast<void*>(J2V8RuntimeGetIsolate(runtime));
  }

  if (pcontext) {
    *pcontext = reinterpret_cast<void*>(*J2V8RuntimeGetContext(runtime));
  }
}
#endif

}  // namespace hybrid
