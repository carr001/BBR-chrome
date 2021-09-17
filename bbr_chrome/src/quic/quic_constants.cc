// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/quic/quic_constants.h"


const char* const kFinalOffsetHeaderKey = ":final-offset";

const char* const kEPIDGoogleFrontEnd = "GFE";
const char* const kEPIDGoogleFrontEnd0 = "GFE0";

QuicPacketNumber MaxRandomInitialPacketNumber() {
  static const QuicPacketNumber kMaxRandomInitialPacketNumber =
      QuicPacketNumber(0x7fffffff);
  return kMaxRandomInitialPacketNumber;
}

QuicPacketNumber FirstSendingPacketNumber() {
  static const QuicPacketNumber kFirstSendingPacketNumber = QuicPacketNumber(1);
  return kFirstSendingPacketNumber;
}

