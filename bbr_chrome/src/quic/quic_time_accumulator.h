// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_QUIC_TIME_ACCUMULATOR_H_
#define QUICHE_QUIC_CORE_QUIC_TIME_ACCUMULATOR_H_

#include "src/quic/quic_time.h"


// QuicTimeAccumulator accumulates elapsed times between Start(s) and Stop(s).
class  QuicTimeAccumulator {
  // TODO(wub): Switch to a data member called kNotRunningSentinel after c++17.
  static  QuicTime NotRunningSentinel() {
    return QuicTime::Infinite();
  }

 public:
  // True if Started and not Stopped.
  bool IsRunning() const { return last_start_time_ != NotRunningSentinel(); }

  void Start(QuicTime now) {
    last_start_time_ = now;
  }

  void Stop(QuicTime now) {
    if (now > last_start_time_) {
      total_elapsed_ = total_elapsed_ + (now - last_start_time_);
    }
    last_start_time_ = NotRunningSentinel();
  }

  // Get total elapsed time between COMPLETED Start/Stop pairs.
  QuicTime::Delta GetTotalElapsedTime() const { return total_elapsed_; }

  // Get total elapsed time between COMPLETED Start/Stop pairs, plus, if it is
  // running, the elapsed time between |last_start_time_| and |now|.
  QuicTime::Delta GetTotalElapsedTime(QuicTime now) const {
    if (!IsRunning()) {
      return total_elapsed_;
    }
    if (now <= last_start_time_) {
      return total_elapsed_;
    }
    return total_elapsed_ + (now - last_start_time_);
  }

 private:
  //
  //                                       |last_start_time_|
  //                                         |
  //                                         V
  // Start => Stop  =>  Start => Stop  =>  Start
  // |           |      |           |
  // |___________|  +   |___________|  =   |total_elapsed_|
  QuicTime::Delta total_elapsed_ = QuicTime::Delta::Zero();
  QuicTime last_start_time_ = NotRunningSentinel();
};


#endif  // QUICHE_QUIC_CORE_QUIC_TIME_ACCUMULATOR_H_
