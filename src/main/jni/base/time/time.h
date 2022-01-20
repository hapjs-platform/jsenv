// Copyright (c) 2021, the hapjs-platform Project Contributors
// SPDX-License-Identifier: EPL-1.0

#ifndef BASE_TIME_TIME_H_
#define BASE_TIME_TIME_H_

#include <stdint.h>
#include <limits>

#include "base_export.h"
#include "macros.h"

#if !defined(__APPLE__)
#include <sys/timerfd.h>
#endif  // !defined(__APPLE__)

namespace base {

class BASE_EXPORT TimeDelta {
 public:
  static constexpr TimeDelta FromDays(int days);
  static constexpr TimeDelta FromHours(int hours);
  static constexpr TimeDelta FromMinutes(int minutes);
  static constexpr TimeDelta FromSeconds(int64_t seconds);
  static constexpr TimeDelta FromMilliseconds(int64_t ms);
  static constexpr TimeDelta FromMicroseconds(int64_t us);
  static constexpr TimeDelta FromSecondsD(double seconds);
  static constexpr TimeDelta FromMillisecondsD(double ms);
  static constexpr TimeDelta FromMicrosecondsD(double us);

  // Returns the maximum time delta, which should be greater than any reasonable
  // time delta we might compare it to. Adding or subtracting the maximum time
  // delta to a time or another time delta has an undefined result.
  static constexpr TimeDelta Max();

  // Returns the minimum time delta, which should be less than than any
  // reasonable time delta we might compare it to. Adding or subtracting the
  // minimum time delta to a time or another time delta has an undefined result.
  static constexpr TimeDelta Min();

  constexpr TimeDelta() : delta_(0) {}
  TimeDelta(const TimeDelta& delta) = default;

  TimeDelta& operator=(TimeDelta other) {
    delta_ = other.delta_;
    return *this;
  }

  TimeDelta& operator+=(TimeDelta other) { return *this = (*this + other); }

  TimeDelta& operator-=(TimeDelta other) { return *this = *this - other; }

  TimeDelta operator+(TimeDelta other) const {
    return TimeDelta(delta_ + other.delta_);
  }

  TimeDelta operator-(TimeDelta other) const {
    return TimeDelta(delta_ - other.delta_);
  }

  constexpr TimeDelta operator-() const { return TimeDelta(-delta_); }

  constexpr bool operator==(TimeDelta other) const {
    return delta_ == other.delta_;
  }

  constexpr bool operator>=(TimeDelta other) const {
    return delta_ >= other.delta_;
  }

  constexpr bool operator<=(TimeDelta other) const {
    return delta_ <= other.delta_;
  }

  constexpr bool operator>(TimeDelta other) const {
    return delta_ > other.delta_;
  }

  constexpr bool operator<(TimeDelta other) const {
    return delta_ < other.delta_;
  }

  constexpr bool is_max() const {
    return delta_ == std::numeric_limits<int64_t>::max();
  }

  double InSecondsF() const;

  int64_t InSeconds() const;

  double InMillisecondsF() const;

  int64_t InMilliseconds() const;

  int64_t InMillisecondsRoundedUp() const;

  int64_t InMicroseconds() const;

 private:
  constexpr explicit TimeDelta(int64_t delta) : delta_(delta) {}

  // Time delta in microseconds.
  int64_t delta_;
};

template <class TimeClass>
class TimeBase {
 public:
  static const int64_t kHoursPerDay = 24;
  static const int64_t kMinutesPerHour = 60;
  static const int64_t kSecondsPerMinute = 60;
  static const int64_t kSecondsPerHour = kMinutesPerHour * kSecondsPerMinute;
  static const int64_t kSecondsPerDay = kSecondsPerHour * kHoursPerDay;
  static const int64_t kMillisecondsPerSecond = 1000;
  static const int64_t kMicrosecondsPerMillisecond = 1000;
  static const int64_t kNanosecondsPerMicrosecond = 1000;
  static const int64_t kMicrosecondsPerSecond =
      kMillisecondsPerSecond * kMicrosecondsPerMillisecond;
  static const int64_t kNanosecondsPerSecond =
      kMicrosecondsPerSecond * kNanosecondsPerMicrosecond;

  // Returns true if this object represents the maximum/minimum time.
  bool is_max() const { return us_ == std::numeric_limits<int64_t>::max(); }
  bool is_min() const { return us_ == std::numeric_limits<int64_t>::min(); }

  static TimeClass Max() {
    return TimeClass(std::numeric_limits<int64_t>::max());
  }

  static TimeClass Min() {
    return TimeClass(std::numeric_limits<int64_t>::min());
  }

  TimeBase() : us_(0) {}

  bool operator==(TimeClass other) const { return us_ == other.us_; }

  bool operator!=(TimeClass other) const { return us_ != other.us_; }

  bool operator>=(TimeClass other) const { return us_ >= other.us_; }

  bool operator<=(TimeClass other) const { return us_ <= other.us_; }

  bool operator>(TimeClass other) const { return us_ > other.us_; }

  bool operator<(TimeClass other) const { return us_ < other.us_; }

  TimeClass operator+(TimeDelta delta) const {
    if (is_max() || delta.is_max())
      return Max();
    return TimeClass(us_ + delta.InMicroseconds());
  }

  TimeClass operator-(TimeDelta delta) const {
    return TimeClass(us_ - delta.InMicroseconds());
  }

  TimeDelta operator-(TimeClass other) const {
    return TimeDelta::FromMicroseconds(us_ - other.us_);
  }

  int64_t InMicroseconds() const { return us_; }

 protected:
  constexpr explicit TimeBase(int64_t us) : us_(us) {}

  // Mono clock in microseconds.
  int64_t us_;
};

// TimeTicks is a monotonically increasing time based on CLOCK_MONO.
class BASE_EXPORT TimeTicks : public TimeBase<TimeTicks> {
 public:
  TimeTicks() : TimeBase(0) {}
  static TimeTicks Now();

#if !defined(__APPLE__)
  struct itimerspec ToITimerSpec() const;
#endif  // !defined(__APPLE__)

 private:
  friend class TimeBase<TimeTicks>;

  constexpr explicit TimeTicks(int64_t us) : TimeBase(us) {}
};

// Represents a clock, specific to a particular thread, that runs only while the
// thread is running.
class BASE_EXPORT ThreadTicks : public TimeBase<ThreadTicks> {
 public:
  ThreadTicks() : TimeBase(0) {}

  static ThreadTicks Now();

 private:
  friend class TimeBase<ThreadTicks>;

  constexpr explicit ThreadTicks(int64_t us) : TimeBase(us) {}
};

// TimeDelta implementations, constexpr functions should be in the header file.
// static
constexpr TimeDelta TimeDelta::FromMilliseconds(int64_t milliseconds) {
  return TimeDelta(milliseconds * TimeTicks::kMicrosecondsPerMillisecond);
}

// static
constexpr TimeDelta TimeDelta::FromMicroseconds(int64_t microseconds) {
  return TimeDelta(microseconds);
}

// static
constexpr TimeDelta TimeDelta::FromDays(int days) {
  return TimeDelta(days * TimeTicks::kSecondsPerDay *
                   TimeTicks::kMicrosecondsPerSecond);
}

// static
constexpr TimeDelta TimeDelta::FromHours(int hours) {
  return TimeDelta(hours * TimeTicks::kSecondsPerHour *
                   TimeTicks::kMicrosecondsPerSecond);
}

// static
constexpr TimeDelta TimeDelta::FromMinutes(int minutes) {
  return TimeDelta(minutes * TimeTicks::kSecondsPerMinute *
                   TimeTicks::kMicrosecondsPerSecond);
}

// static
constexpr TimeDelta TimeDelta::FromSeconds(int64_t seconds) {
  return TimeDelta(seconds * TimeTicks::kMicrosecondsPerSecond);
}

// static
constexpr TimeDelta TimeDelta::FromMillisecondsD(double ms) {
  return TimeDelta(
      static_cast<int64_t>(ms * TimeTicks::kMicrosecondsPerMillisecond));
}

// static
constexpr TimeDelta TimeDelta::FromMicrosecondsD(double us) {
  return TimeDelta(static_cast<int64_t>(us));
}

// static
constexpr TimeDelta TimeDelta::Max() {
  return TimeDelta(std::numeric_limits<int64_t>::max());
}

// static
constexpr TimeDelta TimeDelta::Min() {
  return TimeDelta(std::numeric_limits<int64_t>::min());
}

}  // namespace base

#endif  // BASE_TIME_TIME_H_
