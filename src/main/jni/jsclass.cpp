/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#define TAG "JSENV_IMPL"
#include "jsclass.h"

#include "hybrid-log.h"
#include "jsenv-impl.h"
#include "jsvalue_impl.h"

using v8::Context;
using v8::External;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::ObjectTemplate;
using v8::Persistent;
using v8::PropertyCallbackInfo;
using v8::String;
using v8::Template;
using v8::Value;
using v8::WeakCallbackInfo;
using v8::WeakCallbackType;

namespace hybrid {

JSClassTemplate::JSClassTemplate(Isolate* isolate,
                                 const JSClassDefinition* class_define,
                                 JSClassTemplate* parent) {
  Local<ObjectTemplate> object_templ;
  Local<Template> templ;

  if (class_define->class_name) {
    class_name_ = class_define->class_name;
  }
  finalize_ = class_define->finalize;
  constructor_.function = class_define->constructor.function;
  constructor_.user_data = class_define->constructor.user_data;
  constructor_.flags = class_define->constructor.flags;
  constructor_.self = this;

  if (class_define->constructor.function) {
    Local<FunctionTemplate> func_templ =
        FunctionTemplate::New(isolate, V8FunctionCallback,
                              External::New(isolate, &this->constructor_));

    if (parent) {
      func_templ->Inherit(parent->GetFunctionTemplate(isolate));
    }

    func_templ->InstanceTemplate()->SetInternalFieldCount(2);
    object_templ = func_templ->PrototypeTemplate();
    templ = func_templ;
  } else {
    object_templ = ObjectTemplate::New(isolate);
    object_templ->SetInternalFieldCount(2);
    templ = object_templ;
  }

  InitMemberInfos(class_define);

  // add the property define
  if (class_define->properties) {
    for (int i = 0;; i++) {
      JSPropertyDefinition& pd = class_define->properties[i];
      if (pd.name == nullptr) {
        break;
      }

      PropertyInfo* pinfo = &properties_[i];
      v8::AccessorGetterCallback getter = nullptr;
      v8::AccessorSetterCallback setter = nullptr;

      if (pd.getter) {
        getter = &V8PropertyGetter;
      }

      if (pd.setter) {
        setter = &V8PropertySetter;
      }

      object_templ->SetAccessor(ToV8String(isolate, pd.name), getter, setter,
                                External::New(isolate, pinfo));

      pinfo->getter = pd.getter;
      pinfo->setter = pd.setter;
      pinfo->user_data = pd.user_data;
      pinfo->flags = pd.flags;

      pinfo->self = this;
    }
  }

  // add the function define
  if (class_define->functions) {
    for (int i = 0;; i++) {
      JSFunctionDefinition& fd = class_define->functions[i];

      if (fd.name == nullptr) {
        break;
      }

      FunctionInfo* finfo = &functions_[i];

      object_templ->Set(ToV8String(isolate, fd.name),
                        FunctionTemplate::New(isolate, &V8FunctionCallback,
                                              External::New(isolate, finfo)));

      finfo->function = fd.function;
      finfo->user_data = fd.user_data;
      finfo->self = this;
      finfo->flags = fd.flags;
    }
  }

  template_.Reset(isolate, templ);
}

void JSClassTemplate::InitMemberInfos(const JSClassDefinition* class_def) {
  int prop_count = 0;
  int func_count = 0;

  if (class_def->properties) {
    while (class_def->properties[prop_count].name)
      prop_count++;
  }

  if (class_def->functions) {
    while (class_def->functions[func_count].name)
      func_count++;
  }

  properties_ = nullptr;
  functions_ = nullptr;

  if (prop_count <= 0 && func_count <= 0) {
    return;
  }

  uint8_t* ptr = new uint8_t[prop_count * sizeof(PropertyInfo) +
                             func_count * sizeof(FunctionInfo)];

  if (prop_count > 0) {
    properties_ = reinterpret_cast<PropertyInfo*>(ptr);
  }

  if (func_count > 0) {
    functions_ = reinterpret_cast<FunctionInfo*>(
        ptr + prop_count * sizeof(PropertyInfo));
  }
}

Local<Object> JSClassTemplate::NewObject(v8::Local<Context> context) {
  Local<ObjectTemplate> object_template =
      GetObjectTemplate(context->GetIsolate());

  if (object_template.IsEmpty()) {
    ALOGE("JSENV", "Get Null ObjectTemplate");
    return Local<Object>();
  }

  Local<Object> object;
  if (!object_template->NewInstance(context).ToLocal(&object)) {
    ALOGE("JSENV", "Create ObjectTemplate Null object");
    return Local<Object>();
  }

  if (!object.IsEmpty() && finalize_ != nullptr) {
    // Set WeakReference
    v8::NonCopyablePersistentTraits<Object>::NonCopyablePersistent weak_ref(
        context->GetIsolate(), object);

    weak_ref.SetWeak(this, &V8FinalizeCallback,
                     WeakCallbackType::kInternalFields);
    // ignore the weak handle.
  }
  return object;
}

void JSClassTemplate::V8PropertyGetter(
    Local<String> property_name,
    const PropertyCallbackInfo<Value>& info) {
  Isolate* isolate = info.GetIsolate();
  PropertyInfo* pinfo = GetMemberInfo<PropertyInfo>(info.Data());

  JSObject self = ToJSObject(info.This());

  JSEnvImpl* jsenv = JSEnvImpl::From(isolate);

  JSValue js_value;

  if (!pinfo->getter(jsenv, pinfo->user_data, self, &js_value)) {
    return;
  }

  Local<Value> v8_value = ToV8Value(isolate, &js_value);

  info.GetReturnValue().Set(v8_value);
}

void JSClassTemplate::V8PropertySetter(Local<String> property_name,
                                       Local<Value> value,
                                       const PropertyCallbackInfo<void>& info) {
  Isolate* isolate = info.GetIsolate();
  PropertyInfo* pinfo = GetMemberInfo<PropertyInfo>(info.Data());

  JSObject self = ToJSObject(info.This());

  JSValue js_value;

  JSEnvImpl* jsenv = JSEnvImpl::From(isolate);

  if (!ToJSValue(isolate, &js_value, value, pinfo->flags)) {
    return;
  }

  pinfo->setter(jsenv, pinfo->user_data, self, &js_value);
}

void JSClassTemplate::V8FunctionCallback(
    const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  FunctionInfo* finfo = GetMemberInfo<FunctionInfo>(args.Data());

  JSObject self = ToJSObject(args.This());

  JSEnvImpl* jsenv = JSEnvImpl::From(isolate);

  Arguments<> arguments(args.Length(), finfo->flags);

  for (int i = 0; i < args.Length(); i++) {
    arguments.Add(isolate, args[i]);
  }

  JSValue result;

  bool bret = finfo->function(jsenv, finfo->user_data, self, arguments.args,
                              arguments.argc, &result);

  if (jsenv->ThrowExceptionToV8()) {
    return;
  }

  if (!bret) {
    return;
  }

  Local<Value> v8_result = ToV8Value(isolate, &result);

  args.GetReturnValue().Set(v8_result);
}

void JSClassTemplate::V8FinalizeCallback(
    const WeakCallbackInfo<JSClassTemplate>& info) {
  JSClassTemplate* self_weak = info.GetParameter();

  if (self_weak && self_weak->finalize_) {
    self_weak->finalize_(info.GetInternalField(0), info.GetInternalField(1));
  }
}

}  // namespace hybrid
