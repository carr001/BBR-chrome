// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/quic/quic_bandwidth.h"

#include <string>



std::string QuicBandwidth::ToDebuggingValue() const {

  return std::to_string(bits_per_second_);
}

