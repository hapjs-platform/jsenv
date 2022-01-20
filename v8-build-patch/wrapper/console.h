/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef V8_WRAPPER_DEBUG_CONSOLE_H_
#define V8_WRAPPER_DEBUG_CONSOLE_H_

#include "v8.h"

namespace v8 {

namespace wrapper {
class WrapperConsoleDelegate;

class ConsoleDelegate {
 public:
  ConsoleDelegate() : delegate_(nullptr) {}

  virtual void Debug(const v8::FunctionCallbackInfo<v8::Value>& args,
                     int id, v8::Local<v8::Value> name) {}
  virtual void Error(const v8::FunctionCallbackInfo<v8::Value>& args,
                     int id, v8::Local<v8::Value> name) {}
  virtual void Info(const v8::FunctionCallbackInfo<v8::Value>& args,
                    int id, v8::Local<v8::Value> name) {}
  virtual void Log(const v8::FunctionCallbackInfo<v8::Value>& args,
                   int id, v8::Local<v8::Value> name) {}
  virtual void Warn(const v8::FunctionCallbackInfo<v8::Value>& args,
                    int id, v8::Local<v8::Value> name) {}
  virtual void Trace(const v8::FunctionCallbackInfo<v8::Value>& args,
                     int id, v8::Local<v8::Value> name) {}
  virtual void Assert(const v8::FunctionCallbackInfo<v8::Value>& args,
                      int id, v8::Local<v8::Value> name) {}
  virtual void Profile(const v8::FunctionCallbackInfo<v8::Value>& args,
                       int id, v8::Local<v8::Value> name) {}
  virtual void ProfileEnd(const v8::FunctionCallbackInfo<v8::Value>& args,
                          int id, v8::Local<v8::Value> name) {}
  virtual void Time(const v8::FunctionCallbackInfo<v8::Value>& args,
                    int id, v8::Local<v8::Value> name) {}
  virtual void TimeLog(const v8::FunctionCallbackInfo<v8::Value>& args,
                       int id, v8::Local<v8::Value> name) {}
  virtual void TimeEnd(const v8::FunctionCallbackInfo<v8::Value>& args,
                       int id, v8::Local<v8::Value> name) {}
  virtual void TimeStamp(const v8::FunctionCallbackInfo<v8::Value>& args,
                         int id, v8::Local<v8::Value> name) {}
  virtual ~ConsoleDelegate();

  void Attach(v8::Isolate* isolate);
  void Restore(v8::Isolate* isolate);
  void Detach(v8::Isolate* isolate);

 private:
  WrapperConsoleDelegate* delegate_;

};

}  // namespace wrapper
}  // namespace v8

#endif // V8_WRAPPER_DEBUG_CONSOLE_H_
