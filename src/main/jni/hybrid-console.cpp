/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "hybrid-console.h"

namespace hybrid {

HybridConsole::HybridConsole(v8::debug::ConsoleDelegate* v8_console,
                             v8::debug::ConsoleDelegate* logcat_console)
    : v8_console_(v8_console), logcat_console_(logcat_console) {}

HybridConsole::~HybridConsole() {}

// implement by logcat_console and v8_console
void HybridConsole::Log(const v8::debug::ConsoleCallArguments& args,
                        const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Log(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->Log(args, console_context);
  }
}

void HybridConsole::Error(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Error(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->Error(args, console_context);
  }
}

void HybridConsole::Warn(const v8::debug::ConsoleCallArguments& args,
                         const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Warn(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->Warn(args, console_context);
  }
}

void HybridConsole::Info(const v8::debug::ConsoleCallArguments& args,
                         const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Info(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->Info(args, console_context);
  }
}

void HybridConsole::Debug(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Debug(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->Debug(args, console_context);
  }
}

void HybridConsole::Time(const v8::debug::ConsoleCallArguments& args,
                         const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Time(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->Time(args, console_context);
  }
}

void HybridConsole::TimeEnd(const v8::debug::ConsoleCallArguments& args,
                            const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->TimeEnd(args, console_context);
  }
  if (logcat_console_) {
    logcat_console_->TimeEnd(args, console_context);
  }
}

// implement by v8_console
void HybridConsole::Dir(const v8::debug::ConsoleCallArguments& args,
                        const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Dir(args, console_context);
  }
}
void HybridConsole::DirXml(const v8::debug::ConsoleCallArguments& args,
                           const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->DirXml(args, console_context);
  }
}
void HybridConsole::Table(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Table(args, console_context);
  }
}
void HybridConsole::Trace(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Trace(args, console_context);
  }
}
void HybridConsole::Group(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Group(args, console_context);
  }
}
void HybridConsole::GroupCollapsed(
    const v8::debug::ConsoleCallArguments& args,
    const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->GroupCollapsed(args, console_context);
  }
}
void HybridConsole::GroupEnd(const v8::debug::ConsoleCallArguments& args,
                             const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->GroupEnd(args, console_context);
  }
}
void HybridConsole::Clear(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Clear(args, console_context);
  }
}
void HybridConsole::Count(const v8::debug::ConsoleCallArguments& args,
                          const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Count(args, console_context);
  }
}
void HybridConsole::Assert(const v8::debug::ConsoleCallArguments& args,
                           const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Assert(args, console_context);
  }
}
void HybridConsole::MarkTimeline(
    const v8::debug::ConsoleCallArguments& args,
    const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    // v8_console_->MarkTimeline(args, console_context);
  }
}
void HybridConsole::Profile(const v8::debug::ConsoleCallArguments& args,
                            const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->Profile(args, console_context);
  }
}
void HybridConsole::ProfileEnd(
    const v8::debug::ConsoleCallArguments& args,
    const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->ProfileEnd(args, console_context);
  }
}
void HybridConsole::Timeline(const v8::debug::ConsoleCallArguments& args,
                             const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    // v8_console_->Timeline(args, console_context);
  }
}
void HybridConsole::TimelineEnd(
    const v8::debug::ConsoleCallArguments& args,
    const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    // v8_console_->TimelineEnd(args, console_context);
  }
}
void HybridConsole::TimeStamp(
    const v8::debug::ConsoleCallArguments& args,
    const v8::debug::ConsoleContext& console_context) {
  if (v8_console_) {
    v8_console_->TimeStamp(args, console_context);
  }
}

}  // namespace hybrid
