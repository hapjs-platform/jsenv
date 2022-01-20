/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#define TAG "JSENVTEST"
#include "JSEnv.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "hybrid-log.h"
#include "test_help.h"

#include <initializer_list>

using hybrid::JSEnv;

namespace hybrid {
JSEnv* g_jsenv = nullptr;
}  // namespace hybrid

static void InitJSEnv() {
  hybrid::g_jsenv = JSEnv::GetInstance(reinterpret_cast<hybrid::J2V8Handle>(-1),
                                       JSENV_VERSION, nullptr);
}

namespace hybrid {

static void InitClassObject(JSEnv* jsenv,
                            const JSClassDefinition* class_define) {
  JSClass clazz = jsenv->CreateClass(class_define, nullptr);

  JSObject object = jsenv->NewInstance(clazz);

  jsenv->SetGlobalValue(class_define->class_name, object);
}

static void InitClass(JSEnv* jsenv,
                      const JSClassDefinition* class_define,
                      JSClass super = nullptr) {
  JSClass clazz = jsenv->CreateClass(class_define, super);

  JSObject function = jsenv->GetClassConstructorFunction(clazz);

  if (function == nullptr) {
    ALOGE("JSENV", "Create Function Constructor: %s faield",
          class_define->class_name);
    return;
  }

  jsenv->SetGlobalValue(class_define->class_name, function);
}

static bool mirror_func(JSEnv* env,
                        void* user_data,
                        JSObject self,
                        const JSValue* argv,
                        int argc,
                        JSValue* presult) {
  if (argc >= 1) {
    *presult = argv[0];
  }
  return true;
}

template <int N>
static bool user_data_func(JSEnv* env,
                           void* user_data,
                           JSObject self,
                           const JSValue* argv,
                           int argc,
                           JSValue* presult) {
  int n = static_cast<int>(reinterpret_cast<intptr_t>(user_data));

  EXPECT_EQ(N, n) << "user_data test failed";
  return true;
}

template <bool IS_PRIVATE = true>
static bool test_set_private_data(JSEnv* env,
                                  void* user_data,
                                  JSObject self,
                                  const JSValue* argv,
                                  int argc,
                                  JSValue* presult) {
  if (argc <= 0 || argv[0].IsInt()) {
    return false;
  }

  int v = argv[0].IntVal();

  v <<= 2;

  void* private_data = reinterpret_cast<void*>(v);

  if (IS_PRIVATE) {
    env->SetObjectPrivateData(self, private_data);
  } else {
    env->SetObjectPrivateExtraData(self, private_data);
  }

  return true;
}

template <bool IS_PRIVATE = true>
static bool test_get_private_data(JSEnv* env,
                                  void* user_data,
                                  JSObject self,
                                  const JSValue* argv,
                                  int argc,
                                  JSValue* presult) {
  void* private_data = nullptr;
  if (IS_PRIVATE) {
    private_data = env->GetObjectPrivateData(self);
  } else {
    private_data = env->GetObjectPrivateExtraData(self);
  }

  int v = static_cast<int>(reinterpret_cast<intptr_t>(private_data));

  v >>= 2;

  presult->Set(v);
  return true;
}

static bool test_get_global_value(JSEnv* env,
                                  void* user_data,
                                  JSObject self,
                                  const JSValue* argv,
                                  int argc,
                                  JSValue* presult) {
  JSValue ival;
  JSValue bval;
  JSValue dval;
  JSValue strval;

  env->GetGlobalValue("js_var_int", &ival);
  env->GetGlobalValue("js_var_bool", &bval);
  env->GetGlobalValue("js_var_float", &dval);
  env->GetGlobalValue("js_var_str", &strval, JSEnv::kFlagUseUTF8);

  EXPECT_EQ(ival.Type(), JSValue::kInt) << "js_var_int type is not int";
  EXPECT_EQ(ival.IntVal(), 100) << "js_var_int value is not 100";

  EXPECT_EQ(bval.Type(), JSValue::kBoolean) << "js_var_bool is not boolean";
  EXPECT_EQ(bval.Boolean(), true) << "js_var_bool is not true";

  EXPECT_EQ(dval.Type(), JSValue::kFloat) << "js_var_float is not float";
  EXPECT_PRIMARY_EQ(dval.FloatVal(), 2.71828) << "js_var_float is not 2.71828";

  EXPECT_EQ(strval.Type(), JSValue::kUTF8String) << "js_var_str is utf8string";
  EXPECT_EQ(std::string(strval.UTF8Str()), std::string("hello world!"))
      << "js_var_str is not \'hello world!\'";

  return true;
}

static bool test_get_object_properties(JSEnv* jsenv,
                                       void* user_data,
                                       JSObject self,
                                       const JSValue* argv,
                                       int argc,
                                       JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_get_object_properties need a object arg");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_get_object_properties args 0 must be a object");
    return false;
  }

  JSObject object = argv[0].Object();

  JSValue value;
  bool bret;

  bret = jsenv->GetObjectPropertyValue(object, "ival", &value);
  EXPECT_EQ(bret, true) << "test_get_object_properties get property";
  EXPECT_EQ(value.IsInt(), true) << "test_get_object_properties get ival";
  EXPECT_EQ(value.IntVal(), 100) << "test_get_object_properties get 100";

  bret = jsenv->GetObjectPropertyValue(object, "dval", &value);
  EXPECT_EQ(bret, true) << "test_get_object_properties get property";
  EXPECT_EQ(value.IsFloat(), true) << "test_get_object_properties get float";
  EXPECT_EQ(value.FloatVal(), 3.1415926)
      << "test_get_object_properties 3.1415926";

  bret = jsenv->GetObjectPropertyValue(object, "strval", &value,
                                       JSEnv::kFlagUseUTF8);
  EXPECT_EQ(bret, true) << "test_get_object_properties get property";
  EXPECT_EQ(value.IsUTF8String(), true)
      << "test_get_object_properties get utf8string";
  EXPECT_EQ(std::string(value.UTF8Str()), std::string("mi quickapp"))
      << "test_get_object_properties \"mi quickapp\"";

  bret = jsenv->GetObjectPropertyValue(object, "bval", &value,
                                       JSEnv::kFlagUseUTF8);
  EXPECT_EQ(bret, true) << "test_get_object_properties get property";
  EXPECT_EQ(value.IsBoolean(), true) << "test_get_object_properties get bool";
  EXPECT_EQ(value.Boolean(), true) << "test_get_object_properties get true";

  return true;
}

static bool test_set_object_properties(JSEnv* jsenv,
                                       void* user_data,
                                       JSObject self,
                                       const JSValue* argv,
                                       int argc,
                                       JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_set_object_properties need a object arg");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_set_object_properties args 0 must be a object");
    return false;
  }

  JSObject object = argv[0].Object();

  jsenv->SetObjectPropertyValue(object, "ival", 100);
  jsenv->SetObjectPropertyValue(object, "dval", 2.71828);
  jsenv->SetObjectPropertyValue(object, "strval", "hello mi quickapp");
  jsenv->SetObjectPropertyValue(object, "bval", true);

  return true;
}

static bool test_foreach_object(JSEnv* jsenv,
                                void* user_data,
                                JSObject self,
                                const JSValue* argv,
                                int argc,
                                JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_set_object_properties need a object arg");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_set_object_properties args 0 must be a object");
    return false;
  }

  JSObject object = argv[0].Object();

  bool ival_found = false;
  bool dval_found = false;
  bool strval_found = false;
  bool bval_found = false;

  JSObject names = jsenv->GetObjectPropertyNames(object);

  EXPECT_NE(names, nullptr) << "test_foreach_object get names";

  size_t len = jsenv->GetObjectLength(names);

  for (size_t i = 0; i < len; i++) {
    JSValue key;
    JSValue value;

    bool bret = jsenv->GetObjectAtIndex(names, static_cast<int>(i), &key,
                                        JSEnv::kFlagUseUTF8);
    EXPECT_EQ(bret, true) << "test_foreach_object get name at:" << i;
    EXPECT_EQ(key.IsUTF8String(), true) << "test_foreach_object key is utf8str";

    const char* str_key = key.UTF8Str();

    bret = jsenv->GetObjectProperty(object, &key, &value, JSEnv::kFlagUseUTF8);

    EXPECT_EQ(bret, true) << "test_foreach_object get value :" << str_key;

    if (strcmp(str_key, "ival") == 0) {
      ival_found = true;
      EXPECT_EQ(value.IsInt(), true) << "test_foreach_object ival";
      EXPECT_EQ(value.IntVal(), 100) << "test_foreach_object ival 100";
    } else if (strcmp(str_key, "dval") == 0) {
      dval_found = true;
      EXPECT_EQ(value.IsFloat(), true) << "test_foreach_object dval";
      EXPECT_EQ(value.FloatVal(), 2.71828)
          << "test_foreach_object dval 2.71828";
    } else if (strcmp(str_key, "strval") == 0) {
      strval_found = true;
      EXPECT_EQ(value.IsUTF8String(), true) << "test_foreach_object strval";
      EXPECT_EQ(std::string(value.UTF8Str()), std::string("hello mi quickapp"))
          << "test_foreach_object strval";
    } else if (strcmp(str_key, "bval") == 0) {
      bval_found = true;
      EXPECT_EQ(value.IsBoolean(), true) << "test_foreach_object bval";
      EXPECT_EQ(value.Boolean(), true) << "test_foreach_object bval true";
    }
  }

  EXPECT_EQ(ival_found, true) << "test_foreach_object ival_found";
  EXPECT_EQ(dval_found, true) << "test_foreach_object dval_found";
  EXPECT_EQ(strval_found, true) << "test_foreach_object strval_found";
  EXPECT_EQ(bval_found, true) << "test_foreach_object bval_found";

  return true;
}

static bool test_get_array(JSEnv* jsenv,
                           void* user_data,
                           JSObject self,
                           const JSValue* argv,
                           int argc,
                           JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_set_object_properties need a object arg");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_set_object_properties args 0 must be a object");
    return false;
  }

  JSObject object = argv[0].Object();

  size_t len = jsenv->GetObjectLength(object);

  for (size_t i = 0; i < len; i++) {
    JSValue value;
    bool bret = jsenv->GetObjectAtIndex(object, static_cast<int>(i), &value,
                                        JSEnv::kFlagUseUTF8);

    EXPECT_EQ(bret, true) << "test_get_array get " << i;
    int type = value.Type();

    switch (type) {
      case JSValue::kInt:
        EXPECT_EQ(value.IntVal(), 100) << "test_get_array get " << i;
        break;
      case JSValue::kFloat:
        EXPECT_EQ(value.FloatVal(), 3.1415926) << "test_get_array get " << i;
        break;
      case JSValue::kUTF8String:
        EXPECT_EQ(std::string(value.UTF8Str()),
                  std::string("hello mi quickapp"))
            << "test_get_array get " << i;
        break;
      case JSValue::kBoolean:
        EXPECT_EQ(value.Boolean(), true) << "test_get_array get " << i;
        break;
      default:
        EXPECT_EQ(true, false) << "test_get_array unsupport type get " << i;
        break;
    }
  }

  return true;
}

static bool test_set_array(JSEnv* jsenv,
                           void* user_data,
                           JSObject self,
                           const JSValue* argv,
                           int argc,
                           JSValue* presult) {
  JSObject array = jsenv->NewArray(4);

  jsenv->SetObjectAtIndexValue(array, 0, 100);
  jsenv->SetObjectAtIndexValue(array, 1, 2.71828);
  jsenv->SetObjectAtIndexValue(array, 2, "hello from native mi quickapp");
  jsenv->SetObjectAtIndexValue(array, 3, true);

  presult->Set(array);

  return true;
}

static bool test_set_array_with_values(JSEnv* jsenv,
                                       void* user_data,
                                       JSObject self,
                                       const JSValue* argv,
                                       int argc,
                                       JSValue* presult) {
  JSValue array_values[4];
  array_values[0].Set(100);
  array_values[1].Set(2.71828);
  array_values[2].Set("hello from native mi quickapp");
  array_values[3].Set(true);

  JSObject array = jsenv->NewArrayWithValues(array_values, 4);

  presult->Set(array);
  return true;
}

static bool test_functions(JSEnv* jsenv,
                           void* user_data,
                           JSObject self,
                           const JSValue* argv,
                           int argc,
                           JSValue* presult) {
  JSValue func_obj;
  JSValue result;

  EXPECT_EQ(jsenv->GetGlobalValue("test_func_get_ival", &func_obj), true)
      << "test_functions get test_func_get_ival";
  EXPECT_EQ(
      jsenv->CallFunction(func_obj.Object(), nullptr, nullptr, 0, &result),
      true)
      << "test_functions call test_func_get_ival";
  EXPECT_EQ(result.IsInt(), true) << "test_functions test_func_get_ival";
  EXPECT_EQ(result.IntVal(), 300) << "test_functions test_func_get_ival 300";

  EXPECT_EQ(jsenv->GetGlobalValue("test_func_get_dval", &func_obj), true)
      << "test_functions get test_func_get_dval";
  EXPECT_EQ(
      jsenv->CallFunction(func_obj.Object(), nullptr, nullptr, 0, &result),
      true)
      << "test_functions call test_func_get_dval";
  EXPECT_PRIMARY_EQ(result.IsFloat(), true)
      << "test_functions test_func_get_dval";
  EXPECT_PRIMARY_EQ(result.FloatVal(), 3.1415926)
      << "test_functions test_func_get_dval 3.1415926";

  EXPECT_EQ(jsenv->GetGlobalValue("test_func_get_bval", &func_obj), true)
      << "test_functions get test_func_get_bval";
  EXPECT_EQ(
      jsenv->CallFunction(func_obj.Object(), nullptr, nullptr, 0, &result),
      true)
      << "test_functions call test_func_get_bval";
  EXPECT_EQ(result.IsBoolean(), true) << "test_functions test_func_get_bval";
  EXPECT_EQ(result.Boolean(), true) << "test_functions test_func_get_bval true";

  EXPECT_EQ(jsenv->GetGlobalValue("test_func_get_strval", &func_obj), true)
      << "test_functions get test_func_get_strval";
  EXPECT_EQ(jsenv->CallFunction(func_obj.Object(), nullptr, nullptr, 0, &result,
                                JSEnv::kFlagUseUTF8),
            true)
      << "test_functions call test_func_get_strval";
  EXPECT_EQ(result.IsUTF8String(), true) << "test_functions test_func_get_bval";
  EXPECT_EQ(std::string(result.UTF8Str()), std::string("hello js mi quickapp"))
      << "test_functions test_func_get_strval";

  // test_func_set_values
  EXPECT_EQ(jsenv->GetGlobalValue("test_func_set_values", &func_obj), true)
      << "test_functions test_func_set_values";
  JSValue array_values[4];
  array_values[0].Set(100);
  array_values[1].Set(2.78128);
  array_values[2].Set("hello from native mi quickapp");
  array_values[3].Set(true);
  EXPECT_EQ(
      jsenv->CallFunction(func_obj.Object(), nullptr, array_values, 4, nullptr),
      true)
      << "test_functions test_func_set_values";

  return true;
}

static bool test_function_ctr(JSEnv* jsenv,
                              void* user_data,
                              JSObject self,
                              const JSValue* argv,
                              int argc,
                              JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_function_ctr need a object arg");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_function_ctr args 0 must be a function");
    return false;
  }

  JSObject function = argv[0].Object();

  JSValue array_values[4];
  array_values[0].Set(100);
  array_values[1].Set(2.78128);
  array_values[2].Set("hello from native mi quickapp");
  array_values[3].Set(true);

  JSObject object = jsenv->CallFunctionAsConstructor(function, array_values, 4);

  EXPECT_NE(object, nullptr) << "test_function_ctr new";

  presult->Set(object);

  return true;
}

template <typename T>
static void check_typed_array(JSEnv* jsenv,
                              JSObject typed_array,
                              int type,
                              const char* info,
                              std::initializer_list<T> l) {
  size_t count = jsenv->GetTypedArrayCount(typed_array);
  int arr_type = jsenv->GetTypedArrayType(typed_array);
  T* parr = reinterpret_cast<T*>(jsenv->GetTypedArrayPointer(typed_array, 0));

  EXPECT_EQ(arr_type, type) << info << " array type";
  EXPECT_EQ(count, l.size()) << info << " array size";

  const T* pcheck_val = l.begin();

  for (int i = 0; i < static_cast<int>(l.size()); i++) {
    EXPECT_PRIMARY_EQ(parr[i], pcheck_val[i]) << info << " array [" << i << "]";
  }
}

static bool test_typed_arraies(JSEnv* jsenv,
                               void* user_data,
                               JSObject self,
                               const JSValue* argv,
                               int argc,
                               JSValue* presult) {
  EXPECT_EQ(argc, 9) << "test_typed_arraies need 9 arraies";

  EXPECT_EQ(argv[0].IsObject(), true) << "test_typed_arraies at 0";
  check_typed_array<int8_t>(jsenv, argv[0].Object(), JSValue::kJSInt8Array,
                            "test_typed_arraies Int8Array", {1, 2, 126, -126});

  EXPECT_EQ(argv[1].IsObject(), true) << "test_typed_arraies at 1";
  check_typed_array<uint8_t>(jsenv, argv[1].Object(), JSValue::kJSUint8Array,
                             "test_typed_arraies Uint8Array", {1, 255, 0, 22});

  EXPECT_EQ(argv[2].IsObject(), true) << "test_typed_arraies at 2";
  check_typed_array<uint8_t>(
      jsenv, argv[2].Object(), JSValue::kJSUint8ClampedArray,
      "test_typed_arraies Uint8ClampedArray", {34, 255, 0, 2});

  EXPECT_EQ(argv[3].IsObject(), true) << "test_typed_arraies at 3";
  check_typed_array<int16_t>(jsenv, argv[3].Object(), JSValue::kJSInt16Array,
                             "test_typed_arraies Int16Array",
                             {1, 642, -2339, 2334});

  EXPECT_EQ(argv[4].IsObject(), true) << "test_typed_arraies at 4";
  check_typed_array<uint16_t>(jsenv, argv[4].Object(), JSValue::kJSUint16Array,
                              "test_typed_arraies Uint16Array",
                              {1, 65534, 6444, 22});

  EXPECT_EQ(argv[5].IsObject(), true) << "test_typed_arraies at 5";
  check_typed_array<int32_t>(jsenv, argv[5].Object(), JSValue::kJSInt32Array,
                             "test_typed_arraies Int32Array",
                             {1, 300203, 32030034, 30033});

  EXPECT_EQ(argv[6].IsObject(), true) << "test_typed_arraies at 6";
  check_typed_array<uint32_t>(jsenv, argv[6].Object(), JSValue::kJSUint32Array,
                              "test_typed_arraies Uint32Array",
                              {1, 300302032, 309433434, 4443443});

  EXPECT_EQ(argv[7].IsObject(), true) << "test_typed_arraies at 7";
  check_typed_array<float>(jsenv, argv[7].Object(), JSValue::kJSFloat32Array,
                           "test_typed_arraies Float32Array",
                           {1.034, -2300.3902, 32032.32, 0.0399234});

  EXPECT_EQ(argv[8].IsObject(), true) << "test_typed_arraies at 8";
  check_typed_array<double>(
      jsenv, argv[8].Object(), JSValue::kJSFloat64Array,
      "test_typed_arraies Float64Array",
      {2.300234, -239293943.3292934, 2399239323.39923, 1.9});

  return true;
}

template <typename T>
static void new_typed_array(JSEnv* jsenv,
                            JSObject arr_obj,
                            int idx,
                            int arr_type,
                            std::initializer_list<T> l) {
  JSObject typed_array = jsenv->NewTypedArray(arr_type, l.size());

  T* parr = reinterpret_cast<T*>(jsenv->GetTypedArrayPointer(typed_array, 0));

  const T* pbegin = l.begin();
  const T* pend = l.end();
  if (parr) {
    for (int i = 0; pbegin < pend; pbegin++, i++) {
      parr[i] = *pbegin;
    }
  }

  jsenv->SetObjectAtIndexValue(arr_obj, idx, typed_array);
}

static bool get_typed_array(JSEnv* jsenv,
                            void* user_data,
                            JSObject self,
                            const JSValue* argv,
                            int argc,
                            JSValue* presult) {
  JSObject result_array = jsenv->NewArray(9);

  new_typed_array<int8_t>(jsenv, result_array, 0, JSValue::kJSInt8Array,
                          {1, 2, 126, -126});
  new_typed_array<uint8_t>(jsenv, result_array, 1, JSValue::kJSUint8Array,
                           {1, 255, 0, 22});
  new_typed_array<uint8_t>(jsenv, result_array, 2,
                           JSValue::kJSUint8ClampedArray, {34, 255, 0, 2});
  new_typed_array<int16_t>(jsenv, result_array, 3, JSValue::kJSInt16Array,
                           {1, 642, -2339, 2334});
  new_typed_array<uint16_t>(jsenv, result_array, 4, JSValue::kJSUint16Array,
                            {1, 65534, 6444, 22});
  new_typed_array<int32_t>(jsenv, result_array, 5, JSValue::kJSInt32Array,
                           {1, 300203, 32030034, 30033});
  new_typed_array<uint32_t>(jsenv, result_array, 6, JSValue::kJSUint32Array,
                            {1, 300302032, 309433434, 4443443});
  new_typed_array<float>(jsenv, result_array, 7, JSValue::kJSFloat32Array,
                         {1.034, -2300.3902, 32032.32, 0.0399234});
  new_typed_array<double>(
      jsenv, result_array, 8, JSValue::kJSFloat64Array,
      {2.300234, -239293943.3292934, 2399239323.39923, 1.9});

  presult->Set(result_array);
  return true;
}

template <typename T>
static void new_typed_array_array_buffer(JSEnv* jsenv,
                                         JSObject arr_obj,
                                         int idx,
                                         int arr_type,
                                         std::initializer_list<T> l) {
  JSObject array_buffer = jsenv->NewArrayBuffer(l.size() * sizeof(T));

  JSObject typed_array =
      jsenv->NewTypedArrayArrayBuffer(arr_type, array_buffer, 0, l.size());

  T* parr = reinterpret_cast<T*>(jsenv->GetTypedArrayPointer(typed_array, 0));

  const T* pbegin = l.begin();
  const T* pend = l.end();
  if (parr) {
    for (int i = 0; pbegin < pend; pbegin++, i++) {
      parr[i] = *pbegin;
    }
  }

  jsenv->SetObjectAtIndexValue(arr_obj, idx, typed_array);
}

static bool get_typed_array_with_array_buffer(JSEnv* jsenv,
                                              void* user_data,
                                              JSObject self,
                                              const JSValue* argv,
                                              int argc,
                                              JSValue* presult) {
  JSObject result_array = jsenv->NewArray(9);

  new_typed_array_array_buffer<int8_t>(
      jsenv, result_array, 0, JSValue::kJSInt8Array, {1, 2, 126, -126});
  new_typed_array_array_buffer<uint8_t>(
      jsenv, result_array, 1, JSValue::kJSUint8Array, {1, 255, 0, 22});
  new_typed_array_array_buffer<uint8_t>(
      jsenv, result_array, 2, JSValue::kJSUint8ClampedArray, {34, 255, 0, 2});
  new_typed_array_array_buffer<int16_t>(
      jsenv, result_array, 3, JSValue::kJSInt16Array, {1, 642, -2339, 2334});
  new_typed_array_array_buffer<uint16_t>(
      jsenv, result_array, 4, JSValue::kJSUint16Array, {1, 65534, 6444, 22});
  new_typed_array_array_buffer<int32_t>(jsenv, result_array, 5,
                                        JSValue::kJSInt32Array,
                                        {1, 300203, 32030034, 30033});
  new_typed_array_array_buffer<uint32_t>(jsenv, result_array, 6,
                                         JSValue::kJSUint32Array,
                                         {1, 300302032, 309433434, 4443443});
  new_typed_array_array_buffer<float>(jsenv, result_array, 7,
                                      JSValue::kJSFloat32Array,
                                      {1.034, -2300.3902, 32032.32, 0.0399234});
  new_typed_array_array_buffer<double>(
      jsenv, result_array, 8, JSValue::kJSFloat64Array,
      {2.300234, -239293943.3292934, 2399239323.39923, 1.9});

  presult->Set(result_array);

  return true;
}

static bool new_array_buffer(JSEnv* jsenv,
                             void* user_data,
                             JSObject self,
                             const JSValue* argv,
                             int argc,
                             JSValue* presult) {
  JSObject array_buffer = jsenv->NewArrayBuffer(4);

  uint8_t* parr =
      reinterpret_cast<uint8_t*>(jsenv->GetArrayBufferPointer(array_buffer));

  parr[0] = 1;
  parr[1] = 2;
  parr[2] = 3;
  parr[3] = 4;

  presult->Set(array_buffer);

  return true;
}

static bool new_array_buffer_external(JSEnv* jsenv,
                                      void* user_data,
                                      JSObject self,
                                      const JSValue* argv,
                                      int argc,
                                      JSValue* presult) {
  static uint8_t arr[4];

  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  arr[3] = 4;

  JSObject array_buffer =
      jsenv->NewArrayBufferExternal(arr, sizeof(arr), nullptr, nullptr);

  presult->Set(array_buffer);

  return true;
}

static JSObject g_resolver = nullptr;

static void ResolverWeakReferenceDelete(const void* weak_data) {
  void* p_user_data = nullptr;

  ALOGD("JSENV", "ResolverWeakReferenceDelete called");
  g_jsenv->GetWeakReferenceCallbackInfo(weak_data, &p_user_data, nullptr);

  if (p_user_data) {
    JSObject* presolver = reinterpret_cast<JSObject*>(p_user_data);
    *presolver = nullptr;
  }
}

static bool create_promise(JSEnv* jsenv,
                           void* user_data,
                           JSObject self,
                           const JSValue* argv,
                           int argc,
                           JSValue* presult) {
  JSObject resolver = jsenv->CreateResolver();

  JSObject promise = jsenv->GetPromiseFromResolver(resolver);

  if (g_resolver) {
    jsenv->DeleteObjectReference(g_resolver);
  }

  g_resolver = jsenv->NewObjectWeakReference(
      resolver, ResolverWeakReferenceDelete, &g_resolver);

  presult->Set(promise);

  return true;
}

static bool test_promise(JSEnv* jsenv,
                         void* user_data,
                         JSObject self,
                         const JSValue* argv,
                         int argc,
                         JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_promise need promise");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_promise argv[0] should be promise");
    return false;
  }

  JSObject promise = argv[0].Object();

  EXPECT_EQ(jsenv->GetPromiseState(promise), kJSPromiseStatePending)
      << "test_promise state";
  EXPECT_EQ(jsenv->PromiseHasHandler(promise), true)
      << "test_promise hashandler";

  return true;
}

static bool test_resolve(JSEnv* jsenv,
                         void* user_data,
                         JSObject self,
                         const JSValue* argv,
                         int argc,
                         JSValue* presult) {
  if (g_resolver) {
    jsenv->ResolveValue(g_resolver, 100);
  }

  return true;
}

static bool test_reject(JSEnv* jsenv,
                        void* user_data,
                        JSObject self,
                        const JSValue* argv,
                        int argc,
                        JSValue* presult) {
  if (g_resolver) {
    jsenv->RejectValue(g_resolver, "promise reject");
  }
  return true;
}

static bool test_resolve_state(JSEnv* jsenv,
                               void* user_data,
                               JSObject self,
                               const JSValue* argv,
                               int argc,
                               JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_resolve_state need promise");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_resolve_state argv[0] should be promise");
    return false;
  }

  JSObject promise = argv[0].Object();

  EXPECT_EQ(jsenv->GetPromiseState(promise), kJSPromiseStateFulfilled)
      << "test_resolve_state resolved";
  JSValue value;
  EXPECT_EQ(jsenv->GetPromiseResult(promise, &value), true)
      << "test_resolve_state get promise result";
  EXPECT_EQ(value.IsInt(), true) << "test_resolve_state promise result int";
  EXPECT_EQ(value.IntVal(), 100) << "test_resolve_state promise result 100";

  return true;
}

static bool test_reject_state(JSEnv* jsenv,
                              void* user_data,
                              JSObject self,
                              const JSValue* argv,
                              int argc,
                              JSValue* presult) {
  if (argc <= 0) {
    ALOGE("JSENV", "test_reject_state need promise");
    return false;
  }

  if (!argv[0].IsObject()) {
    ALOGE("JSENV", "test_reject_state argv[0] should be promise");
    return false;
  }

  JSObject promise = argv[0].Object();
  EXPECT_EQ(jsenv->GetPromiseState(promise), kJSPromiseStateRejected)
      << "test_reject_state reject";
  JSValue value;
  EXPECT_EQ(jsenv->GetPromiseResult(promise, &value, JSEnv::kFlagUseUTF8), true)
      << "test_reject_state get promise result";
  EXPECT_EQ(value.IsUTF8String(), true)
      << "test_resolve_state promise result int";
  EXPECT_EQ(std::string(value.UTF8Str()), std::string("promise reject"))
      << "test_reject_state promise result";

  return true;
}

static bool test_promise_then_catch(JSEnv* jsenv,
                                    void* user_data,
                                    JSObject self,
                                    const JSValue* argv,
                                    int argc,
                                    JSValue* presult) {
  if (argc < 2) {
    ALOGE("JSENV", "test_promise_then_catch need 2 args");
    return false;
  }

  if (!argv[0].IsObject() || !argv[1].IsObject()) {
    ALOGE("JSENV", "test_promise_then_catch args must be object");
    return false;
  }

  JSObject fn_then = argv[0].Object();
  JSObject fn_catch = argv[0].Object();

  JSObject resolver = jsenv->CreateResolver();
  JSObject promise = jsenv->GetPromiseFromResolver(resolver);
  jsenv->SetPromiseThen(promise, fn_then);
  jsenv->ResolveValue(resolver, 200);

  ///////////////////////////////////
  // Test catch

  resolver = jsenv->CreateResolver();
  promise = jsenv->GetPromiseFromResolver(resolver);
  jsenv->SetPromiseCatch(promise, fn_catch);
  jsenv->RejectValue(resolver, "promise reject");

  return true;
}

// NewFunction Test
static bool NewFuncCallback(JSEnv* env,
                            void* user_data,
                            JSObject self,
                            const JSValue* argv,
                            int argc,
                            JSValue* presult) {
  if (argc < 1) {
    ALOGE("JSENV", "need at least one param for NewFuncCallback");
    return false;
  }

  *presult = argv[0];
  return true;
}

template <const int N>
static bool new_func(JSEnv* env,
                     void* user_data,
                     JSObject self,
                     const JSValue* argv,
                     int argc,
                     JSValue* presult) {
  int n = static_cast<int>(reinterpret_cast<intptr_t>(user_data));
  EXPECT_EQ(N, n) << "pass new_func user_data error";
  JSObject my_function =
      env->NewFunction(NewFuncCallback, user_data, JSEnv::kFlagUseUTF8);
  presult->Set(my_function);
  return true;
}

static JSFunctionDefinition test1_functions[] = {
    {"mirror", mirror_func, 0, 0},
    {"new_func", new_func<100>, reinterpret_cast<void*>(100), 0},
    {"new_func2", new_func<200>, reinterpret_cast<void*>(200), 0},
    {"user_data_100", user_data_func<100>, reinterpret_cast<void*>(100), 0},
    {"user_data_200", user_data_func<200>, reinterpret_cast<void*>(200), 0},
    {"set_private_data", test_set_private_data<true>, 0, 0},
    {"get_private_data", test_get_private_data<true>, 0, 0},
    {"set_private_extra_data", test_set_private_data<false>, 0, 0},
    {"get_private_extra_data", test_get_private_data<false>, 0, 0},
    {"test_get_global_value", test_get_global_value, 0, 0},
    {"test_get_object_properties", test_get_object_properties, 0, 0},
    {"test_set_object_properties", test_set_object_properties, 0, 0},
    {"test_foreach_object", test_foreach_object, 0, 0},
    {"test_set_array", test_set_array, 0, 0},
    {"test_get_array", test_get_array, 0, 0},
    {"test_set_array_with_values", test_set_array_with_values, 0, 0},
    {"test_functions", test_functions, 0, 0},
    {"test_function_ctr", test_function_ctr, 0, 0},
    {"test_typed_arraies", test_typed_arraies, 0, 0},
    {"get_typed_array", get_typed_array, 0, 0},
    {"get_typed_array_with_array_buffer", get_typed_array_with_array_buffer, 0,
     0},
    {"new_array_buffer", new_array_buffer, 0, 0},
    {"new_array_buffer_external", new_array_buffer_external, 0, 0},
    {"create_promise", create_promise, 0, 0},
    {"test_promise", test_promise, 0, 0},
    {"test_resolve", test_resolve, 0, 0},
    {"test_reject", test_reject, 0, 0},
    {"test_resolve_state", test_resolve_state, 0, 0},
    {"test_reject_state", test_reject_state, 0, 0},
    {"test_promise_then_catch", test_promise_then_catch, 0, 0},
    {0}};

static JSClassDefinition test1_class = {"test1",
                                        {nullptr},
                                        nullptr,
                                        nullptr,
                                        test1_functions};

///////////////////////////////////////////////////////////

struct FooData {
  int ival;
  bool bval;
  double dval;
  std::string strval;

  FooData() : ival(0), bval(false), dval(0.0) {}
};

static bool Foo_Ctr(JSEnv* jsenv,
                    void* user_data,
                    JSObject self,
                    const JSValue* argv,
                    int argc,
                    JSValue* presult) {
  if (argc <= 0) {
    return false;
  }

  if (!argv[0].IsInt()) {
    return false;
  }

  int n = argv[0].IntVal();

  EXPECT_EQ(n, 100) << "Foo constructor arg 100";

  jsenv->SetGlobalValue("foo_constructor", true);

  FooData* pfd = new FooData();

  jsenv->SetObjectPrivateData(self, pfd);

  return true;
}

static void Foo_Finalize(void* private_data, void* extra_data) {
  ALOGD("JSENV", "Foo Fianlize Called");
  if (private_data) {
    delete reinterpret_cast<FooData*>(private_data);
  }
}

static bool Foo_SetProp(JSEnv* jsenv,
                        void* user_data,
                        JSObject self,
                        const JSValue* pvalue) {
  FooData* foo_data =
      reinterpret_cast<FooData*>(jsenv->GetObjectPrivateData(self));

  if (!foo_data) {
    return false;
  }

  int type = pvalue->Type();

  switch (type) {
    case JSValue::kInt:
      foo_data->ival = pvalue->IntVal();
      break;
    case JSValue::kFloat:
      foo_data->dval = pvalue->FloatVal();
      break;
    case JSValue::kBoolean:
      foo_data->bval = pvalue->Boolean();
      break;
    case JSValue::kUTF8String:
      foo_data->strval = pvalue->UTF8Str();
      break;
    default:
      return false;
  }
  return true;
}

static bool Foo_GetProp(JSEnv* jsenv,
                        void* user_data,
                        JSObject self,
                        JSValue* pvalue) {
  FooData* foo_data =
      reinterpret_cast<FooData*>(jsenv->GetObjectPrivateData(self));

  if (!foo_data) {
    return false;
  }

  int type = pvalue->Type();

  switch (type) {
    case JSValue::kInt:
      pvalue->Set(foo_data->ival);
      break;
    case JSValue::kFloat:
      pvalue->Set(foo_data->dval);
      break;
    case JSValue::kBoolean:
      pvalue->Set(foo_data->bval);
      break;
    case JSValue::kUTF8String:
      pvalue->Set(foo_data->strval.c_str(), -1, false);
      break;
    default:
      return false;
  }
  return true;
}

static bool Foo_Set(JSEnv* jsenv,
                    void* user_data,
                    JSObject self,
                    const JSValue* argv,
                    int argc,
                    JSValue* presult) {
  if (argc <= 0) {
    return false;
  }

  int type = argv[0].Type();

  switch (type) {
    case JSValue::kInt:
      EXPECT_EQ(argv[0].IntVal(), 200) << "Foo Set value 200";
      jsenv->SetGlobalValue("foo_set_int", true);
      break;
    case JSValue::kFloat:
      EXPECT_PRIMARY_EQ(argv[0].FloatVal(), 3.1415926)
          << "Foo Set value 3.1415926";
      jsenv->SetGlobalValue("foo_set_float", true);
      break;
    case JSValue::kUTF8String:
      EXPECT_EQ(std::string(argv[0].UTF8Str()), std::string("mi quickapp"))
          << "Foo Set value \"mi quickapp\"";
      jsenv->SetGlobalValue("foo_set_str", true);
      break;
    case JSValue::kBoolean:
      EXPECT_EQ(argv[0].Boolean(), true) << "Foo Set value true";
      jsenv->SetGlobalValue("foo_set_bool", true);
      break;
    default:
      EXPECT_EQ(true, false) << "Foo Set is not acceptable type";
      break;
  }

  return true;
}

static JSFunctionDefinition Foo_functions[] = {
    {"set", Foo_Set, nullptr, JSEnv::kFlagUseUTF8},
    {0}};

static JSPropertyDefinition Foo_properties[] = {
    {"ival", Foo_GetProp, Foo_SetProp, nullptr, JSEnv::kFlagUseUTF8},
    {"dval", Foo_GetProp, Foo_SetProp, nullptr, JSEnv::kFlagUseUTF8},
    {"strval", Foo_GetProp, Foo_SetProp, nullptr, JSEnv::kFlagUseUTF8},
    {"bval", Foo_GetProp, Foo_SetProp, nullptr, JSEnv::kFlagUseUTF8},
    {0}};

static JSClassDefinition Foo_class = {
    "Foo",
    {nullptr, Foo_Ctr, 0, JSEnv::kFlagUseUTF8},
    Foo_Finalize,
    Foo_properties,
    Foo_functions};

static void InitGlobalValues(JSEnv* jsenv) {
  jsenv->SetGlobalValue("global_var_int", 100);
  jsenv->SetGlobalValue("global_var_str", "hello world");
  jsenv->SetGlobalValue("global_var_bool", true);
  jsenv->SetGlobalValue("global_var_float", 3.1415926);
}

static void InitFooConstructor(JSEnv* jsenv) {
  JSClass foo_class = jsenv->GetClass("Foo");

  EXPECT_NE(foo_class, nullptr) << "Foo class cannot be found";

  if (foo_class) {
    JSValue args[1];
    args[0].Set(100);
    JSObject object = jsenv->NewInstanceWithConstructor(foo_class, args, 1);

    EXPECT_NE(object, nullptr) << "create Foo object faield";

    jsenv->SetGlobalValue("native_create_foo", object);
  }
}

////////////////////////////
static bool Super_GetSuperValue(JSEnv* jsenv,
                                void* user_data,
                                JSObject self,
                                const JSValue* argv,
                                int argc,
                                JSValue* presult) {
  presult->Set(3000);

  return true;
}

static bool Driver_GetDriverValue(JSEnv* jsenv,
                                  void* user_data,
                                  JSObject self,
                                  const JSValue* argv,
                                  int argc,
                                  JSValue* presult) {
  presult->Set(5000);
  return true;
}

static bool Super_GetSuperProp(JSEnv* jsenv,
                               void* user_data,
                               JSObject self,
                               JSValue* pvalue) {
  pvalue->Set(3.1415926);
  return true;
}

static bool Driver_GetDriverProp(JSEnv* jsenv,
                                 void* user_data,
                                 JSObject self,
                                 JSValue* pvalue) {
  pvalue->Set(2.78128);
  return true;
}

static bool Null_Constructor(JSEnv* jsenv,
                             void* user_data,
                             JSObject self,
                             const JSValue* argv,
                             int argc,
                             JSValue* presult) {
  return true;
}

static bool Driver_Constructor(JSEnv* jsenv,
                               void* user_data,
                               JSObject self,
                               const JSValue* argv,
                               int argc,
                               JSValue* presult) {
  jsenv->SetGlobalValue("driver_constructor_called", true);
  return true;
}

static JSFunctionDefinition super_functions[] = {
    {"getSuperValue", Super_GetSuperValue, 0, 0},
    {0}};

static JSPropertyDefinition super_propertis[] = {
    {"superProp", Super_GetSuperProp, nullptr, 0, 0},
    {0}};

static JSFunctionDefinition driver_functions[] = {
    {"getDriverValue", Driver_GetDriverValue, 0, 0},
    {0}};

static JSPropertyDefinition driver_properties[] = {
    {"driverProp", Driver_GetDriverProp, nullptr, 0, 0},
    {0}};

static JSClassDefinition super_class = {"SuperClass",
                                        {nullptr, Null_Constructor, nullptr, 0},
                                        nullptr,
                                        super_propertis,
                                        super_functions};

static JSClassDefinition driver_class = {
    "DriverClass",
    {nullptr, Driver_Constructor, nullptr, 0},
    nullptr,
    driver_properties,
    driver_functions};

static void InitInheritClass(JSEnv* jsenv) {
  JSClass super_clazz = jsenv->CreateClass(&super_class, nullptr);
  JSClass driver_clazz = jsenv->CreateClass(&driver_class, super_clazz);

  JSObject object = jsenv->GetClassConstructorFunction(driver_clazz);

  EXPECT_NE(object, nullptr) << "Get DriverClass constructor";

  jsenv->SetGlobalValue("DriverClass", object);
}

static void InitClassesObjects(JSEnv* jsenv) {
  InitClassObject(jsenv, &test1_class);
  InitClass(jsenv, &Foo_class);
  InitGlobalValues(jsenv);

  // Test NewInstanceWithConstructor
  InitFooConstructor(jsenv);
  InitInheritClass(jsenv);
}

}  // namespace hybrid

int main(int argc, char** argv) {
  InitJSEnv();

  testing::InitGoogleTest(&argc, argv);

  hybrid::g_jsenv->PushScope();
  hybrid::InitTestModule(hybrid::g_jsenv);

  hybrid::InitClassesObjects(hybrid::g_jsenv);

  int return_value = RUN_ALL_TESTS();

  hybrid::g_jsenv->PopScope();

  JSEnv::Release(hybrid::g_jsenv);

  return return_value;
}
