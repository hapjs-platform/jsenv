/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "logcat-console.h"

#include <android/log.h>
#include <stdint.h>
#include <string.h>
#include <regex>
#include <string>
#include <sstream>

#define TAG "LOGCAT_CONSOLE"
#define DELIM "%s"

#define ALOG(TAG, LEVEL, ...)                                   \
  do {                                                          \
    __android_log_print(ANDROID_LOG_##LEVEL, TAG, __VA_ARGS__); \
  } while (0)

#define ALOGV(TAG, ...) ALOG(TAG, VERBOSE, __VA_ARGS__)
#define ALOGD(TAG, ...) ALOG(TAG, DEBUG, __VA_ARGS__)
#define ALOGI(TAG, ...) ALOG(TAG, INFO, __VA_ARGS__)
#define ALOGW(TAG, ...) ALOG(TAG, WARN, __VA_ARGS__)
#define ALOGE(TAG, ...) ALOG(TAG, ERROR, __VA_ARGS__)

namespace hybrid {

namespace {

void vsPrintf(std::ostringstream& buf,
              const char* fmt,
              const std::vector<std::string>& args) {
  int idx = 1;
  int len = args.size();
  std::string s(fmt);
  std::regex e(DELIM);

  while (idx < len) {
    std::string result = std::regex_replace(
        s, e, args[idx], std::regex_constants::format_first_only);
    if (s == result) {
      break;
    }
    s = result;
    idx++;
  }
  buf << s;

  while (idx < len) {
    buf << " " << args[idx++];
  }
}

void write(const android_LogPriority level,
           const std::vector<std::string>& args) {
  std::ostringstream buf;
  int len = static_cast<int>(args.size());
  if (len <= 0) {
    return;
  }
  if (args[0].find(DELIM) == std::string::npos) {
    for (int i = 0; i < len; i++) {
      if (i > 0) {
        buf << ' ';
      }
      buf << args[i].c_str();
    }
  } else {
    vsPrintf(buf, args[0].c_str(), args);
  }
  __android_log_print(level, TAG, "%s", buf.str().c_str());
}

void ConsolePrint(const android_LogPriority level,
                  v8::Isolate* isolate,
                  const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::vector<std::string> v_args;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Value> arg = args[i];
    v8::Local<v8::String> str_obj;

    if (arg->IsSymbol())
      arg = v8::Local<v8::Symbol>::Cast(arg)->Description(isolate);
    if (!arg->ToString(isolate->GetCurrentContext()).ToLocal(&str_obj))
      return;

    v8::String::Utf8Value str(isolate, str_obj);
    std::string s(*str);
    v_args.push_back(s);
  }
  write(level, v_args);
}
}  // anonymous namespace

LogcatConsole::LogcatConsole(v8::Isolate* isolate) : isolate_(isolate) {
  default_timer_ = base::TimeTicks::Now();
}

LogcatConsole::~LogcatConsole() {}

void LogcatConsole::Log(const v8::FunctionCallbackInfo<v8::Value>& args,
                        int id, v8::Local<v8::Value> name) {
  ConsolePrint(ANDROID_LOG_DEBUG, isolate_, args);
}

void LogcatConsole::Error(const v8::FunctionCallbackInfo<v8::Value>& args,
                          int id, v8::Local<v8::Value> name) {
  ConsolePrint(ANDROID_LOG_ERROR, isolate_, args);
}

void LogcatConsole::Warn(const v8::FunctionCallbackInfo<v8::Value>& args,
                         int id, v8::Local<v8::Value> name) {
  ConsolePrint(ANDROID_LOG_WARN, isolate_, args);
}

void LogcatConsole::Info(const v8::FunctionCallbackInfo<v8::Value>& args,
                         int id, v8::Local<v8::Value> name) {
  ConsolePrint(ANDROID_LOG_INFO, isolate_, args);
}

void LogcatConsole::Debug(const v8::FunctionCallbackInfo<v8::Value>& args,
                          int id, v8::Local<v8::Value> name) {
  ConsolePrint(ANDROID_LOG_DEBUG, isolate_, args);
}

void LogcatConsole::Time(const v8::FunctionCallbackInfo<v8::Value>& args,
                         int id, v8::Local<v8::Value> name) {
  if (args.Length() == 0) {
    default_timer_ = base::TimeTicks::Now();
  } else {
    v8::Local<v8::Value> arg = args[0];
    v8::Local<v8::String> label;
    v8::TryCatch try_catch(isolate_);
    if (!arg->ToString(isolate_->GetCurrentContext()).ToLocal(&label))
      return;
    v8::String::Utf8Value utf8(isolate_, label);
    std::string string(*utf8);
    auto find = timers_.find(string);
    if (find != timers_.end()) {
      find->second = base::TimeTicks::Now();
    } else {
      timers_.insert(std::pair<std::string, base::TimeTicks>(
          string, base::TimeTicks::Now()));
    }
  }
}

void LogcatConsole::TimeEnd(const v8::FunctionCallbackInfo<v8::Value>& args,
                            int id, v8::Local<v8::Value> name) {
  base::TimeDelta delta;
  if (args.Length() == 0) {
    delta = base::TimeTicks::Now() - default_timer_;
    ALOGD(TAG, "console.timeEnd: default, %f\n", delta.InMillisecondsF());
  } else {
    base::TimeTicks now = base::TimeTicks::Now();
    v8::Local<v8::Value> arg = args[0];
    v8::Local<v8::String> label;
    v8::TryCatch try_catch(isolate_);
    if (!arg->ToString(isolate_->GetCurrentContext()).ToLocal(&label))
      return;
    v8::String::Utf8Value utf8(isolate_, label);
    std::string string(*utf8);
    auto find = timers_.find(string);
    if (find != timers_.end()) {
      delta = now - find->second;
    }
    ALOGD(TAG, "console.timeEnd: %s, %f\n", *utf8, delta.InMillisecondsF());
  }
}

}  // namespace hybrid
