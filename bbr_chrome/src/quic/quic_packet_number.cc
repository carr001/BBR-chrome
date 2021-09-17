// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/quic/quic_packet_number.h"

#include <algorithm>
#include <limits>

// #include "absl/strings/str_cat.h"

void QuicPacketNumber::Clear() {
  packet_number_ = UninitializedPacketNumber();
}

void QuicPacketNumber::UpdateMax(QuicPacketNumber new_value) {
  if (!new_value.IsInitialized()) {
    return;
  }
  if (!IsInitialized()) {
    packet_number_ = new_value.ToUint64();
  } else {
    packet_number_ = std::max(packet_number_, new_value.ToUint64());
  }
}

uint64_t QuicPacketNumber::Hash() const {
  return packet_number_;
}

uint64_t QuicPacketNumber::ToUint64() const {
  return packet_number_;
}

bool QuicPacketNumber::IsInitialized() const {
  return packet_number_ != UninitializedPacketNumber();
}

QuicPacketNumber& QuicPacketNumber::operator++() {
  packet_number_++;
  return *this;
}

QuicPacketNumber QuicPacketNumber::operator++(int) {
  QuicPacketNumber previous(*this);
  packet_number_++;
  return previous;
}

QuicPacketNumber& QuicPacketNumber::operator--() {
  packet_number_--;
  return *this;
}

QuicPacketNumber QuicPacketNumber::operator--(int) {
  QuicPacketNumber previous(*this);
  packet_number_--;
  return previous;
}

QuicPacketNumber& QuicPacketNumber::operator+=(uint64_t delta) {
  packet_number_ += delta;
  return *this;
}

QuicPacketNumber& QuicPacketNumber::operator-=(uint64_t delta) {
  packet_number_ -= delta;
  return *this;
}

std::string QuicPacketNumber::ToString() const {
  if (!IsInitialized()) {
    return "uninitialized";
  }
  // return absl::StrCat(ToUint64());
  return std::to_string(ToUint64());
}

std::ostream& operator<<(std::ostream& os, const QuicPacketNumber& p) {
  os << p.ToString();
  return os;
}

