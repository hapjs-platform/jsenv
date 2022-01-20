/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#include "wrapper/console.h"
#include "src/debug/interface-types.h"
#include "src/debug/debug-interface.h"
#include "src/execution/isolate.h"

using v8::debug::ConsoleCallArguments;
using v8::debug::ConsoleContext;

namespace v8 {

namespace wrapper {

class WrapperConsoleDelegate : public v8::debug::ConsoleDelegate {
 public:
  WrapperConsoleDelegate(v8::wrapper::ConsoleDelegate* console)
    : console_(console) {
  }

  virtual void Debug(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    console_->Debug(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Debug(args, context);
    }
  }
  virtual void Error(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    console_->Error(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Error(args, context);
    }
  }
  virtual void Info(const ConsoleCallArguments& args,
                    const ConsoleContext& context) {
    console_->Info(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Info(args, context);
    }
  }
  virtual void Log(const ConsoleCallArguments& args,
                   const ConsoleContext& context) {
    console_->Log(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Log(args, context);
    }
  }
  virtual void Warn(const ConsoleCallArguments& args,
                    const ConsoleContext& context) {
    console_->Warn(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Warn(args, context);
    }
  }
  virtual void Dir(const ConsoleCallArguments& args,
                   const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->Dir(args, context);
    }
  }
  virtual void DirXml(const ConsoleCallArguments& args,
                      const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->DirXml(args, context);
    }
  }
  virtual void Table(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->Table(args, context);
    }
  }
  virtual void Trace(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    console_->Trace(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Trace(args, context);
    }
  }
  virtual void Group(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->Group(args, context);
    }
  }
  virtual void GroupCollapsed(const ConsoleCallArguments& args,
                              const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->GroupCollapsed(args, context);
    }
  }
  virtual void GroupEnd(const ConsoleCallArguments& args,
                        const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->GroupEnd(args, context);
    }
  }
  virtual void Clear(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->Clear(args, context);
    }
  }
  virtual void Count(const ConsoleCallArguments& args,
                     const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->Count(args, context);
    }
  }
  virtual void CountReset(const ConsoleCallArguments& args,
                          const ConsoleContext& context) {
    if (v8_console_) {
      v8_console_->CountReset(args, context);
    }
  }
  virtual void Assert(const ConsoleCallArguments& args,
                      const ConsoleContext& context) {
    console_->Assert(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Assert(args, context);
    }
  }
  virtual void Profile(const ConsoleCallArguments& args,
                       const ConsoleContext& context) {
    console_->Profile(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Profile(args, context);
    }
  }
  virtual void ProfileEnd(const ConsoleCallArguments& args,
                          const ConsoleContext& context) {
    console_->ProfileEnd(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->ProfileEnd(args, context);
    }
  }
  virtual void Time(const ConsoleCallArguments& args,
                    const ConsoleContext& context) {
    console_->Time(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->Time(args, context);
    }
  }
  virtual void TimeLog(const ConsoleCallArguments& args,
                       const ConsoleContext& context) {
    console_->TimeLog(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->TimeLog(args, context);
    }
  }
  virtual void TimeEnd(const ConsoleCallArguments& args,
                       const ConsoleContext& context) {
    console_->TimeEnd(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->TimeEnd(args, context);
    }
  }
  virtual void TimeStamp(const ConsoleCallArguments& args,
                         const ConsoleContext& context) {
    console_->TimeStamp(To(args), context.id(), context.name());
    if (v8_console_) {
      v8_console_->TimeStamp(args, context);
    }
  }

  void SetV8Console(v8::debug::ConsoleDelegate* console) {
    v8_console_ = console;
  }

  v8::debug::ConsoleDelegate* GetV8Console() {
    return v8_console_;
  }

 private:
  const v8::FunctionCallbackInfo<v8::Value>& To(const ConsoleCallArguments& args) {
    return *reinterpret_cast<const v8::FunctionCallbackInfo<v8::Value>*>(
        reinterpret_cast<const void*>(&args));
  }

  v8::wrapper::ConsoleDelegate* console_;
  v8::debug::ConsoleDelegate* v8_console_;
};

void ConsoleDelegate::Attach(v8::Isolate* isolate) {
  if (delegate_ == nullptr) {
    delegate_ = new WrapperConsoleDelegate(this);
  }

  v8::internal::Isolate* i_isolate = reinterpret_cast<v8::internal::Isolate*>(isolate);
  v8::debug::ConsoleDelegate* v8_delegate = i_isolate->console_delegate();
  if (v8_delegate == delegate_) {
    return;
  }

  delegate_->SetV8Console(v8_delegate);
  v8::debug::SetConsoleDelegate(isolate, delegate_);
}

void ConsoleDelegate::Restore(v8::Isolate* isolate) {
  v8::internal::Isolate* i_isolate = reinterpret_cast<v8::internal::Isolate*>(isolate);
  if (i_isolate->console_delegate() != delegate_) {
    return ;
  }

  v8::debug::ConsoleDelegate* delegate = nullptr;
  if (delegate_) {
    delegate = delegate_->GetV8Console();
  }

  v8::debug::SetConsoleDelegate(isolate, delegate);
}

void ConsoleDelegate::Detach(v8::Isolate* isolate) {
  v8::internal::Isolate* i_isolate = reinterpret_cast<v8::internal::Isolate*>(isolate);
  if (i_isolate->console_delegate() == delegate_) {
    v8::debug::SetConsoleDelegate(isolate, nullptr);
  }
}

ConsoleDelegate::~ConsoleDelegate() {
  if (delegate_) {
    delete delegate_;
  }
}

}  // namespace wrapper
}  // namespace v8
