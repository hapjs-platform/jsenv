/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

$TEST(JSEnvTest, ClassTest1)$

gtest.eq(test1.mirror(10), 10, "test mirror 10");
gtest.eq(test1.mirror(200), 200, "test mirror 200");
gtest.eq(test1.mirror(3.1415), 3.1415, "test mirror 3.1415");
gtest.eq(test1.mirror("hello world"), "hello world", "test mirror hello world");

test1.user_data_100();
test1.user_data_200();

gtest.eq(test1.set_private_data(100), test1.get_private_data(), "test private data 100");
gtest.eq(test1.set_private_extra_data(200), test1.get_private_extra_data(), "test private extra data 200");

$TEST(JSEnvTest, GlobalTest)$

gtest.eq(global_var_int, 100, "global_var_int 100");
gtest.eq(global_var_str, "hello world", "global_var_str \"hello word\")");
gtest.eq(global_var_bool, true, "global_var_bool true");
gtest.eq(global_var_float, 3.1415926, "globa_var_float 3.1415926");


js_var_int = 100;
js_var_bool = true;
js_var_float = 2.71828;
js_var_str = "hello world!";
test1.test_get_global_value();

$TEST(JSEnvTest, ConstructorTest)$
// test native_create_foo
gtest.eq(foo_constructor, true, "native_create_foo constructor called");
native_create_foo.set(200);
native_create_foo.set(3.1415926);
native_create_foo.set("mi quickapp");
native_create_foo.set(true);
gtest.eq(foo_set_int, true,  "native_create_foo set int called");
gtest.eq(foo_set_float, true,  "native_create_foo set float called");
gtest.eq(foo_set_bool, true,  "native_create_foo set bool called");
gtest.eq(foo_set_str, true,  "native_create_foo set str called");

foo_constructor = false;

// create Foo from js
const f = new Foo(100);
f.set(200);
f.set(3.1415926);
f.set("mi quickapp");
f.set(true);

gtest.eq(foo_constructor, true,  "Foo constructor called");
gtest.eq(foo_set_int, true,  "Foo set int called");
gtest.eq(foo_set_float, true,  "Foo set float called");
gtest.eq(foo_set_bool, true,  "Foo set bool called");
gtest.eq(foo_set_str, true,  "Foo set str called");


$TEST(JSEnvTest, PropertyTest)$
const test_prop_foo = new Foo(100);
test_prop_foo.ival = 1000;
test_prop_foo.dval = 2.71828;
test_prop_foo.strval = "foo mi quickapp";
test_prop_foo.bval = true;

gtest.eq(test_prop_foo.ival, 1000,  "Foo ival");
gtest.eq(test_prop_foo.dval, 2.71828,  "Foo dval");
gtest.eq(test_prop_foo.strval, "foo mi quickapp",  "Foo strval");
gtest.eq(test_prop_foo.bval, true,  "Foo bval");


$TEST(JSEnvTest, InheritTest)$

const test_driver = new DriverClass();

print("super value", test_driver.getSuperValue());
print("driver value", test_driver.getDriverValue());
gtest.eq(driver_constructor_called, true, "driver class constructor called");
gtest.eq(test_driver.getSuperValue(), 3000, "test_driver get super value");
gtest.eq(test_driver.getDriverValue(), 5000, "test_driver get driver value");
gtest.eq(test_driver.superProp, 3.1415926, "test_driver get super prop");
gtest.eq(test_driver.driverProp, 2.78128, "test_driver get driver prop");


$TEST(JSEnvTest, GetObjectPropertiesTest)$
const test_get_properties_obj = {
  'ival' : 100,
  'dval' : 3.1415926,
  'strval' : 'mi quickapp',
  'bval' : true
};

test1.test_get_object_properties(test_get_properties_obj);

const test_set_properties_obj = {};
test1.test_set_object_properties(test_set_properties_obj);
gtest.eq(test_set_properties_obj.ival, 100, "test_set_object_properties ival");
gtest.eq(test_set_properties_obj.dval, 2.71828, "test_set_object_properties dval");
gtest.eq(test_set_properties_obj.strval, "hello mi quickapp", "test_set_object_properties strval");
gtest.eq(test_set_properties_obj.bval, true, "test_set_object_properties bval");

print("test_set_properties_obj:" + JSON.stringify(test_set_properties_obj));

test1.test_foreach_object(test_set_properties_obj);


$TEST(JSEnvTest, ArrayTest)$

test1.test_get_array([100, 3.1415926, "hello mi quickapp", true]);

const test_set_array = test1.test_set_array();

gtest.eq(test_set_array[0], 100, "test_set_array 0");
gtest.eq(test_set_array[1], 2.71828, "test_set_array 1");
gtest.eq(test_set_array[2], "hello from native mi quickapp", "test_set_array 2");
gtest.eq(test_set_array[3], true, "test_set_array 3");

const test_set_array_with_values = test1.test_set_array_with_values();

gtest.eq(test_set_array_with_values[0], 100, "test_set_array_with_values 0");
gtest.eq(test_set_array_with_values[1], 2.71828, "test_set_array_with_values 1");
gtest.eq(test_set_array_with_values[2], "hello from native mi quickapp", "test_set_array_with_values 2");
gtest.eq(test_set_array_with_values[3], true, "test_set_array_with_values 3");


$TEST(JSEnvTest, FunctionTest)$

function test_func_get_ival() { return 300; }
function test_func_get_dval() { return 3.1415926; }
function test_func_get_strval() { return "hello js mi quickapp"; }
function test_func_get_bval() { return true; }

function test_func_set_values(ival, dval, strval, bval) {
  gtest.eq(ival, 100, "test_func_set_values ival");
  gtest.eq(dval, 2.78128, "test_func_set_values dval");
  gtest.eq(bval, true, "test_func_set_values bval");
  gtest.eq(strval, "hello from native mi quickapp", "test_func_set_values strval");
}

test1.test_functions();


$TEST(JSEnvTest, FunctionNewTest)$

function test_func_as_ctr(ival, dval, strval, bval) {
  this.ival = ival;
  this.dval = dval;
  this.strval = strval;
  this.bval = bval;
}

const new_obj_func_as_ctr = test1.test_function_ctr(test_func_as_ctr);

print("new_obj_func_as_ctr :" + JSON.stringify(new_obj_func_as_ctr));
gtest.eq(new_obj_func_as_ctr.ival, 100, "test_func_set_values ival");
gtest.eq(new_obj_func_as_ctr.dval, 2.78128, "test_func_set_values dval");
gtest.eq(new_obj_func_as_ctr.bval, true, "test_func_set_values bval");
gtest.eq(new_obj_func_as_ctr.strval, "hello from native mi quickapp", "test_func_set_values strval");


$TEST(JSEnvTest, TypedArray)$

test1.test_typed_arraies(
    new Int8Array([1,2,126,-126]),
    new Uint8Array([1, 255, 0, 22]),
    new Uint8ClampedArray([34,255,0,2]),
    new Int16Array([1, 642,-2339, 2334]),
    new Uint16Array([1, 65534, 6444, 22]),
    new Int32Array([1,300203,32030034, 30033]),
    new Uint32Array([1, 300302032, 309433434,4443443]),
    new Float32Array([1.034, -2300.3902, 32032.32, 0.0399234]),
    new Float64Array([2.300234, -239293943.3292934, 2399239323.39923, 1.9]));

function check_typed_arries(arr, str) {
  gtest.eq(arr[0] instanceof Int8Array, true, str + " Int8Array");
  gtest.eq(arr[0][0], 1, str + " Int8Array [0]");
  gtest.eq(arr[0][1], 2, str + " Int8Array [1]");
  gtest.eq(arr[0][2], 126, str + " Int8Array [2]");
  gtest.eq(arr[0][3], -126, str + " Int8Array [3]");

  gtest.eq(arr[1] instanceof Uint8Array, true, str + " Uint8Array");
  gtest.eq(arr[1][0], 1, str + " Uint8Array [0]");
  gtest.eq(arr[1][1], 255, str + " Uint8Array [1]");
  gtest.eq(arr[1][2], 0, str + " Uint8Array [2]");
  gtest.eq(arr[1][3], 22, str + " Uint8Array [3]");


  gtest.eq(arr[2] instanceof Uint8ClampedArray, true, str + " Uint8ClampedArray");
  gtest.eq(arr[2][0], 34, str + " Uint8ClampedArray [0]");
  gtest.eq(arr[2][1], 255, str + " Uint8ClampedArray [1]");
  gtest.eq(arr[2][2], 0, str + " Uint8ClampedArray [2]");
  gtest.eq(arr[2][3], 2, str + " Uint8ClampedArray [3]");

  gtest.eq(arr[3] instanceof Int16Array, true, str + " Int16Array");
  gtest.eq(arr[3][0], 1, str + " Int16Array [0]");
  gtest.eq(arr[3][1], 642, str + " Int16Array [1]");
  gtest.eq(arr[3][2], -2339, str + " Int16Array [2]");
  gtest.eq(arr[3][3], 2334, str + " Int16Array [3]");

  gtest.eq(arr[4] instanceof Uint16Array, true, str + " Uint16Array");
  gtest.eq(arr[4][0], 1, str + " Uint16Array [0]");
  gtest.eq(arr[4][1], 65534, str + " Uint16Array [1]");
  gtest.eq(arr[4][2], 6444, str + " Uint16Array [2]");
  gtest.eq(arr[4][3], 22, str + " Uint16Array [3]");

  gtest.eq(arr[5] instanceof Int32Array, true, str + " Int32Array");
  gtest.eq(arr[5][0], 1, str + " Int32Array [0]");
  gtest.eq(arr[5][1], 300203, str + " Int32Array [1]");
  gtest.eq(arr[5][2], 32030034, str + " Int32Array [2]");
  gtest.eq(arr[5][3], 30033, str + " Int32Array [3]");

  gtest.eq(arr[6] instanceof Uint32Array, true, str + " Uint32Array");
  gtest.eq(arr[6][0], 1, str + " Uint32Array [0]");
  gtest.eq(arr[6][1], 300302032, str + " Uint32Array [1]");
  gtest.eq(arr[6][2], 309433434, str + " Uint32Array [2]");
  gtest.eq(arr[6][3], 4443443, str + " Uint32Array [3]");

  gtest.eq(arr[7] instanceof Float32Array, true, str + " Float32Array");
  gtest.eq(arr[7][0], 1.034, str + " Float32Array [0]");
  gtest.eq(arr[7][1], -2300.3902, str + " Float32Array [1]");
  gtest.eq(arr[7][2], 32032.32, str + " Float32Array [2]");
  gtest.eq(arr[7][3], 0.0399234, str + " Float32Array [3]");

  gtest.eq(arr[8] instanceof Float64Array, true, str + " Float64Array");
  gtest.eq(arr[8][0], 2.300234, str + " Float64Array [0]");
  gtest.eq(arr[8][1], -239293943.3292934, str + " Float64Array [1]");
  gtest.eq(arr[8][2], 2399239323.39923, str + " Float64Array [2]");
  gtest.eq(arr[8][3], 1.9, str + " Float64Array [3]");
}

check_typed_arries(test1.get_typed_array(), "get_typed_array");
check_typed_arries(test1.get_typed_array_with_array_buffer(), "get_typed_array");


$TEST(JSEnvTest, ArrayBufferTest)$

const array_buffer = test1.new_array_buffer();
const array_buffer_external = test1.new_array_buffer_external();

const int8_arr = new Int8Array(array_buffer);
const int8_arr_external = new Int8Array(array_buffer_external);
gtest.eq(int8_arr[0], 1, "array_buffer 0");
gtest.eq(int8_arr[1], 2, "array_buffer 1");
gtest.eq(int8_arr[2], 3, "array_buffer 2");
gtest.eq(int8_arr[4], 4, "array_buffer 3");

gtest.eq(int8_arr_external[0], 1, "array_buffer_external 0");
gtest.eq(int8_arr_external[1], 2, "array_buffer_external 1");
gtest.eq(int8_arr_external[2], 3, "array_buffer_external 2");
gtest.eq(int8_arr_external[4], 4, "array_buffer_external 3");


$TEST(JSEnvTest, PromiseTest)$
const promise_from_native1 = test1.create_promise();
promise_from_native1.then((val) => {
  print("promise resolved:" + val);
  gtest.eq(val, 100, "Promise_from_native then");
});

test1.test_promise(promise_from_native1);
test1.test_resolve();
test1.test_resolve_state(promise_from_native1);

const promise_from_native2 = test1.create_promise();
promise_from_native2.catch((err) => {
  print("promise reject: " + err);
  gtest.eq(err, "promise reject", "Promise_from_native catch");
});
test1.test_promise(promise_from_native2);
test1.test_reject();
test1.test_reject_state(promise_from_native2);

$TEST(JSEnvTest, PromiseThenCatchTest)$

test1.test_promise_then_catch((val) => {
  // resolve function
  print("promise_then_catch test: val=" + val);
  gtest.eq(val, 200, "promise test then");
}, (err) => {
  // reject function
  print("promise_then_catch test: error=" + error);
  gtest.eq(err, "promise reject", "promise test catch");
});

// test NewFunction
$TEST(JSEnvTest, NewFunctionTest)$
const new_func1 = test1.new_func()
gtest.eq(typeof (new_func1), 'function', 'new_func1 is a function')
gtest.eq(new_func1(100), 100, 'new_func1(100) result is 100')
gtest.eq(new_func1(true), true, 'new_func1(true) result is true')
gtest.eq(new_func1('test new function'), 'test new function',
                'new_func1("test new function") ok')

const new_func2 = test1.new_func2()
gtest.eq(typeof(new_func2), 'function', 'new_func2 is a function')
gtest.eq(new_func2(100), 100, 'new_func2(100) result is 100')
gtest.eq(new_func2(true), true, 'new_func2(true) result is true')

function new_func3_callback() {
  console.log('pass func as param');
}
const new_func3 = test1.new_func(new_func3_callback)
gtest.eq(new_func3(new_func3_callback) == new_func3_callback,
    true, 'pass func as param')
