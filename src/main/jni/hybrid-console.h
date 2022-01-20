/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef HYBRID_HYBRID_CONSOLE_H_
#define HYBRID_HYBRID_CONSOLE_H_

#include "src/base/platform/time.h"
#include "src/debug/debug-interface.h"

namespace hybrid {

class HybridConsole : public v8::debug::ConsoleDelegate {
 public:
  static HybridConsole* Create(v8::debug::ConsoleDelegate* v8_console,
                               v8::debug::ConsoleDelegate* logcat_console) {
    if (v8_console && logcat_console) {
      return new HybridConsole(v8_console, logcat_console);
    }
    return nullptr;
  }

  void Log(const v8::debug::ConsoleCallArguments& args,
           const v8::debug::ConsoleContext&) override;
  void Error(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void Warn(const v8::debug::ConsoleCallArguments& args,
            const v8::debug::ConsoleContext&) override;
  void Info(const v8::debug::ConsoleCallArguments& args,
            const v8::debug::ConsoleContext&) override;
  void Debug(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void Time(const v8::debug::ConsoleCallArguments& args,
            const v8::debug::ConsoleContext&) override;
  void TimeEnd(const v8::debug::ConsoleCallArguments& args,
               const v8::debug::ConsoleContext&) override;
  void Dir(const v8::debug::ConsoleCallArguments& args,
           const v8::debug::ConsoleContext&) override;
  void DirXml(const v8::debug::ConsoleCallArguments& args,
              const v8::debug::ConsoleContext&) override;
  void Table(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void Trace(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void Group(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void GroupCollapsed(const v8::debug::ConsoleCallArguments& args,
                      const v8::debug::ConsoleContext&) override;
  void GroupEnd(const v8::debug::ConsoleCallArguments& args,
                const v8::debug::ConsoleContext&) override;
  void Clear(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void Count(const v8::debug::ConsoleCallArguments& args,
             const v8::debug::ConsoleContext&) override;
  void Assert(const v8::debug::ConsoleCallArguments& args,
              const v8::debug::ConsoleContext&) override;
  void MarkTimeline(const v8::debug::ConsoleCallArguments& args,
                    const v8::debug::ConsoleContext&);
  void Profile(const v8::debug::ConsoleCallArguments& args,
               const v8::debug::ConsoleContext&) override;
  void ProfileEnd(const v8::debug::ConsoleCallArguments& args,
                  const v8::debug::ConsoleContext&) override;
  void Timeline(const v8::debug::ConsoleCallArguments& args,
                const v8::debug::ConsoleContext&);
  void TimelineEnd(const v8::debug::ConsoleCallArguments& args,
                   const v8::debug::ConsoleContext&);
  void TimeStamp(const v8::debug::ConsoleCallArguments& args,
                 const v8::debug::ConsoleContext&) override;

  ~HybridConsole() override;

 private:
  explicit HybridConsole(v8::debug::ConsoleDelegate* v8_console,
                         v8::debug::ConsoleDelegate* logcat_console);

  v8::debug::ConsoleDelegate* v8_console_;
  v8::debug::ConsoleDelegate* logcat_console_;
  std::map<std::string, v8::base::TimeTicks> timers_;
  v8::base::TimeTicks default_timer_;
};

}  // namespace hybrid

#endif  // HYBRID_HYBRID_CONSOLE_H_
