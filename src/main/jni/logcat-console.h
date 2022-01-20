/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_LOGCAT_CONSOLE_H_
#define HYBRID_LOGCAT_CONSOLE_H_

#include <map>

#include "wrapper/console.h"
#include "base/time/time.h"

namespace hybrid {

class LogcatConsole : public v8::wrapper::ConsoleDelegate {
 public:
  static LogcatConsole* Create(v8::Isolate* isolate) {
    return new LogcatConsole(isolate);
  }
  void Log(const v8::FunctionCallbackInfo<v8::Value>& args,
           int id, v8::Local<v8::Value> name) override;
  void Error(const v8::FunctionCallbackInfo<v8::Value>& args,
             int id, v8::Local<v8::Value> name) override;
  void Warn(const v8::FunctionCallbackInfo<v8::Value>& args,
            int id, v8::Local<v8::Value> name) override;
  void Info(const v8::FunctionCallbackInfo<v8::Value>& args,
            int id, v8::Local<v8::Value> name) override;
  void Debug(const v8::FunctionCallbackInfo<v8::Value>& args,
             int id, v8::Local<v8::Value> name) override;
  void Time(const v8::FunctionCallbackInfo<v8::Value>& args,
            int id, v8::Local<v8::Value> name) override;
  void TimeEnd(const v8::FunctionCallbackInfo<v8::Value>& args,
               int id, v8::Local<v8::Value> name) override;

  ~LogcatConsole() override;

 private:
  explicit LogcatConsole(v8::Isolate* isolate);
  v8::Isolate* isolate_;
  std::map<std::string, base::TimeTicks> timers_;
  base::TimeTicks default_timer_;
};

}  // namespace hybrid

#endif  // HYBRID_LOGCAT_CONSOLE_H_
