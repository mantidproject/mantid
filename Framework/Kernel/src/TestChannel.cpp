// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/TestChannel.h"

namespace Mantid {

void TestChannel::log(const Poco::Message &msg) { _msgList.emplace_back(msg); }

TestChannel::MsgList &TestChannel::list() { return _msgList; }

void TestChannel::clear() { _msgList.clear(); }
} // namespace Mantid
