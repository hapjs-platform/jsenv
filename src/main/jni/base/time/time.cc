// Copyright (c) 2021, the hapjs-platform Project Contributors
// SPDX-License-Identifier: EPL-1.0

#include "time.h"

#include <time.h>
#include <limits>

#if !defined(__APPLE__)
#include <sys/timerfd.h>
#endif

namespace base {

namespace {

int64_t ClockNow(clockid_t clk_id) {
  timespec ts;
  // Use monotonic time.
  clock_gettime(clk_id, &ts);
  int64_t us = ts.tv_sec;
  us *= TimeTicks::kMicrosecondsPerSecond;
  us += ts.tv_nsec / TimeTicks::kNanosecondsPerMicrosecond;
  return us;
}

}  // namespace

// TimeTicks implementations.
// static
TimeTicks TimeTicks::Now() {
  return TimeTicks() + TimeDelta::FromMicroseconds(ClockNow(CLOCK_MONOTONIC));
}

#if !defined(__APPLE__)
struct itimerspec TimeTicks::ToITimerSpec() const {
  uint64_t nano_secs = InMicroseconds() * kNanosecondsPerMicrosecond;

  struct itimerspec spec = {};
  spec.it_value.tv_sec = (time_t)(nano_secs / kNanosecondsPerSecond);
  spec.it_value.tv_nsec = nano_secs % kNanosecondsPerSecond;
  spec.it_interval = spec.it_value;

  return spec;
}
#endif  // !defined(__APPLE__)

// ThreadTicks implementations.
// static
ThreadTicks ThreadTicks::Now() {
  return ThreadTicks() +
         TimeDelta::FromMicroseconds(ClockNow(CLOCK_THREAD_CPUTIME_ID));
}

double TimeDelta::InSecondsF() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<double>::infinity();
  }
  return static_cast<double>(delta_) / TimeTicks::kMicrosecondsPerSecond;
}

int64_t TimeDelta::InSeconds() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64_t>::max();
  }
  return delta_ / TimeTicks::kMicrosecondsPerSecond;
}

double TimeDelta::InMillisecondsF() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<double>::infinity();
  }
  return static_cast<double>(delta_) / TimeTicks::kMicrosecondsPerMillisecond;
}

int64_t TimeDelta::InMilliseconds() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64_t>::max();
  }
  return delta_ / TimeTicks::kMicrosecondsPerMillisecond;
}

int64_t TimeDelta::InMillisecondsRoundedUp() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64_t>::max();
  }
  int64_t result = delta_ / TimeTicks::kMicrosecondsPerMillisecond;
  int64_t remainder =
      delta_ - (result * TimeTicks::kMicrosecondsPerMillisecond);
  if (remainder > 0) {
    ++result;  // Use ceil(), not trunc() rounding behavior.
  }
  return result;
}

int64_t TimeDelta::InMicroseconds() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64_t>::max();
  }
  return delta_;
}

}  // namespace base
