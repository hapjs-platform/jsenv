/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_JSVALUE_IMPL_H_
#define HYBRID_JSVALUE_IMPL_H_

#include <v8.h>
#include <string>

#include "JSEnv.h"

namespace hybrid {

typedef v8::Persistent<v8::Object>* InnerJ2V8ObjectHandle;

static inline v8::Local<v8::String> ToV8String(v8::Isolate* isolate,
                                               const char* cstr) {
  v8::Local<v8::String> v8_str;
  if (v8::String::NewFromUtf8(isolate, cstr, v8::NewStringType::kNormal)
          .ToLocal(&v8_str)) {
    return v8_str;
  }

  return v8::Local<v8::String>();
}

static inline v8::Local<v8::String> ToV8String(v8::Isolate* isolate,
                                               const std::string str) {
  return ToV8String(isolate, str.c_str());
}

static inline v8::Local<v8::Object> ToV8Object(v8::Isolate* isolate,
                                               J2V8ObjectHandle handle) {
  return v8::Local<v8::Object>::New(
      isolate, *reinterpret_cast<InnerJ2V8ObjectHandle>(handle));
}

static inline v8::Local<v8::Object> ToV8Object(v8::Isolate* isolate,
                                               JSObject object) {
  v8::Local<v8::Object> js_object;
  uintptr_t val = reinterpret_cast<uintptr_t>(object);

  if ((val & ~1) == 0) {
    return v8::Local<v8::Object>();
  }

  if ((val & 1) == 1) {
    v8::Persistent<v8::Object> persist_obj;
    uintptr_t* pobj_val = reinterpret_cast<uintptr_t*>(&persist_obj);
    *pobj_val = (val & ~1);

    return persist_obj.Get(isolate);
  }

  JSObject* pjs_object = reinterpret_cast<JSObject*>(&js_object);
  *pjs_object = object;
  return js_object;
}

static inline JSObject ToJSObject(v8::Local<v8::Object> object) {
  return reinterpret_cast<JSObject>(*object);
}

static inline JSObject ToJSObject(
    const v8::PersistentBase<v8::Object>& object) {
  uintptr_t val = 0;
  // memcpy(&val, &object, sizeof(uintptr_t));
  val = *(reinterpret_cast<const uintptr_t*>(&object));

  return reinterpret_cast<JSObject>(val | 1);
}

static inline bool IsPersistentObject(JSObject object) {
  uintptr_t val = reinterpret_cast<uintptr_t>(object);

  return (val & 1) == 1;
}

static inline bool ToJSValue(v8::Isolate* isolate,
                             JSValue* pjs_value,
                             v8::Local<v8::Value> v8_value,
                             uint32_t flags = 0) {
  if (v8_value.IsEmpty() || v8_value->IsNullOrUndefined()) {
    pjs_value->SetNull();
    return true;
  }

  if (v8_value->IsTrue()) {
    pjs_value->Set(true);
    return true;
  }
  if (v8_value->IsFalse()) {
    pjs_value->Set(false);
    return true;
  }
  if (v8_value->IsString()) {
    if (flags & JSEnv::kFlagUseUTF8) {
      v8::String::Utf8Value str_val(isolate,
                                    v8::Local<v8::String>::Cast(v8_value));
      pjs_value->Set(*str_val, str_val.length(), true);
    } else {
      v8::String::Value str_val(isolate, v8::Local<v8::String>::Cast(v8_value));
      pjs_value->Set(reinterpret_cast<jschar_t*>(*str_val), str_val.length(),
                     true);
    }
    return true;
  }

  if (v8_value->IsInt32()) {
    pjs_value->Set(v8::Local<v8::Int32>::Cast(v8_value)->Value());
    return true;
  }

  if (v8_value->IsBoolean()) {
    pjs_value->Set(v8::Local<v8::Boolean>::Cast(v8_value)->Value());
    return true;
  }

  if (v8_value->IsNumber()) {
    pjs_value->Set(v8::Local<v8::Number>::Cast(v8_value)->Value());
    return true;
  }

  if (v8_value->IsObject()) {
    pjs_value->Set(ToJSObject(v8_value.As<v8::Object>()));
    return true;
  }

  return false;
}

static inline v8::Local<v8::Value> ToV8Value(v8::Isolate* isolate,
                                             const JSValue* pjs_value) {
  switch (pjs_value->type) {
    case JSValue::kInt:
      return v8::Integer::New(isolate, pjs_value->IntVal());
    case JSValue::kUInt:
      return v8::Integer::New(isolate, pjs_value->UintVal());
    case JSValue::kFloat:
      return v8::Number::New(isolate, pjs_value->FloatVal());
    case JSValue::kBoolean:
      return v8::Boolean::New(isolate, pjs_value->Boolean());
    case JSValue::kUTF8String:
      return v8::String::NewFromUtf8(isolate, pjs_value->UTF8Str(),
                                     v8::NewStringType::kNormal,
                                     static_cast<int>(pjs_value->Length()))
          .ToLocalChecked();
    case JSValue::kUTF16String:
      return v8::String::NewFromTwoByte(
                 isolate,
                 reinterpret_cast<const uint16_t*>(pjs_value->UTF16Str()),
                 v8::NewStringType::kNormal,
                 static_cast<int>(pjs_value->Length()))
          .ToLocalChecked();
      break;
    case JSValue::kJSObject:
    case JSValue::kJSFunction:
    case JSValue::kJSArray:
    case JSValue::kJSTypedArray:
    case JSValue::kJSArrayBuffer:
    case JSValue::kJSDataView:
    case JSValue::kJSPromise:
    case JSValue::kJSResolver:
      return ToV8Object(isolate, pjs_value->Object());
  }

  return v8::Local<v8::Value>();
}

}  // namespace hybrid

#endif  // HYBRID_JSVALUE_IMPL_H_
