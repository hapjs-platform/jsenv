/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_JSCLASS_H_
#define HYBRID_JSCLASS_H_

#include "v8.h"

#include "j2v8-runtime.h"
#include "jsvalue_impl.h"

namespace hybrid {

class JSClassTemplate {
 public:
  static JSClassTemplate* Create(v8::Isolate* isolate,
                                 const JSClassDefinition* class_define,
                                 JSClassTemplate* parent) {
    if (class_define == nullptr) {
      return nullptr;
    }

    if (parent &&
        !(parent->IsFunctionTemplate() && class_define->constructor.function)) {
      return nullptr;
    }

    return new JSClassTemplate(isolate, class_define, parent);
  }

  ~JSClassTemplate() {
    if (properties_) {
      delete[] reinterpret_cast<uint8_t*>(properties_);
    } else {
      if (functions_) {
        delete[] reinterpret_cast<uint8_t*>(functions_);
      }
    }
  }

  bool IsFunctionTemplate() const { return constructor_.function != nullptr; }

  v8::Local<v8::FunctionTemplate> GetFunctionTemplate(v8::Isolate* isolate) {
    if (IsFunctionTemplate()) {
      v8::Local<v8::Template> tmpl =
          v8::Local<v8::Template>::New(isolate, template_);
      return tmpl.As<v8::FunctionTemplate>();
    }

    return v8::Local<v8::FunctionTemplate>();
  }

  v8::Local<v8::Function> GetObjectConstructor(v8::Local<v8::Context> context) {
    if (!IsFunctionTemplate()) {
      return v8::Local<v8::Function>();
    }

    v8::Local<v8::FunctionTemplate> func_templ =
        GetFunctionTemplate(context->GetIsolate());
    return !func_templ.IsEmpty()
               ? func_templ->GetFunction(context).ToLocalChecked()
               : v8::Local<v8::Function>();
  }

  v8::Local<v8::ObjectTemplate> GetObjectTemplate(v8::Isolate* isolate) {
    if (IsFunctionTemplate()) {
      return GetFunctionTemplate(isolate)->InstanceTemplate();
    }
    v8::Local<v8::Template> tmpl =
        v8::Local<v8::Template>::New(isolate, template_);
    return tmpl.As<v8::ObjectTemplate>();
  }

  v8::Local<v8::Object> NewObject(v8::Local<v8::Context> context);

  static inline JSClassTemplate* From(JSClass clazz) {
    return reinterpret_cast<JSClassTemplate*>(clazz);
  }

  static inline JSClass ToJSClass(JSClassTemplate* tmpl) {
    return reinterpret_cast<JSClass>(tmpl);
  }

  inline JSClass ToJSClass() { return ToJSClass(this); }

  const std::string& GetName() const { return class_name_; }

 private:
  JSClassTemplate(v8::Isolate* isolate,
                  const JSClassDefinition* class_define,
                  JSClassTemplate* parent);

  static void V8PropertyGetter(v8::Local<v8::String> property_name,
                               const v8::PropertyCallbackInfo<v8::Value>& info);

  static void V8PropertySetter(v8::Local<v8::String> property_name,
                               v8::Local<v8::Value> value,
                               const v8::PropertyCallbackInfo<void>& info);

  static void V8FunctionCallback(
      const v8::FunctionCallbackInfo<v8::Value>& info);

  static void V8FinalizeCallback(
      const v8::WeakCallbackInfo<JSClassTemplate>& info);

  template <typename T>
  static T* GetMemberInfo(v8::Local<v8::Value> data) {
    v8::Local<v8::External> external = v8::Local<v8::External>::Cast(data);

    return !external.IsEmpty() ? reinterpret_cast<T*>(external->Value())
                               : nullptr;
  }

  struct PropertyInfo {
    JSClassTemplate* self;
    JSPropertyGetCallback getter;
    JSPropertySetCallback setter;
    void* user_data;
    uint32_t flags;
  };

  struct FunctionInfo {
    JSClassTemplate* self;
    JSFunctionCallback function;
    void* user_data;
    uint32_t flags;
  };

  void InitMemberInfos(const JSClassDefinition* class_define);

  v8::Persistent<v8::Template> template_;
  JSFinalizeCallback finalize_;
  std::string class_name_;
  FunctionInfo constructor_;

  PropertyInfo* properties_;
  FunctionInfo* functions_;
};

template <int MAX = 16>
struct Arguments {
  JSValue* args;
  int argc;
  uint32_t flags;

  Arguments(int max, uint32_t flags) {
    if (max > MAX) {
      args = new JSValue[max];
    } else {
      args = baseargs;
    }

    argc = 0;
    this->flags = flags;
  }

  ~Arguments() {
    if (args != baseargs) {
      delete[] args;
    }
  }

  bool Add(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    return ToJSValue(isolate, &args[argc++], value, flags);
  }

 private:
  JSValue baseargs[MAX];
};

template <int MAX = 16>
struct V8Arguments {
  v8::Local<v8::Value>* args;
  int argc;
  v8::Local<v8::Value> baseargs[MAX];

  V8Arguments(v8::Isolate* isolate, const JSValue* args, int argc) {
    if (argc > MAX) {
      this->args = new v8::Local<v8::Value>[argc];
    } else {
      this->args = baseargs;
    }

    this->argc = argc;
    for (int i = 0; i < argc; i++) {
      this->args[i] = ToV8Value(isolate, &args[i]);
    }
  }

  ~V8Arguments() {
    if (args != baseargs) {
      delete[] args;
    }
  }
};

}  // namespace hybrid

#endif  // HYBRID_JSCLASS_H_
