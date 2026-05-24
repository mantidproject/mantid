// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#pragma once
#ifndef _WIN32

#include "ADARAPacketBuilders.h"
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Mantid::LiveData::Testing {

// One step in the server's playback script.
struct PktGarbage {
  std::vector<uint8_t> bytes;
};
struct PktDisconnect {};
struct PktWaitForExtract {}; // gate: blocks until the test signals
                             // (used to deterministically interleave
                             //  extractData() with packet delivery).

using ScriptEntry = std::variant<std::vector<uint8_t>, // raw packet bytes (preferred — built by
                                                       // the helpers in §5 of subspec02)
                                 PktGarbage, PktDisconnect, PktWaitForExtract>;

class MockSMSServer {
public:
  // path: absolute UDS path.  Any stale file at this path is unlinked by
  // start() before bind(), so callers need not pre-clean it.
  explicit MockSMSServer(std::string path);
  ~MockSMSServer(); // joins server thread; closes sockets; unlinks path.

  MockSMSServer(const MockSMSServer &) = delete;
  MockSMSServer &operator=(const MockSMSServer &) = delete;

  // Begin listening.  Returns immediately; accept() happens on bg thread.
  // Must be called BEFORE the listener calls connect().
  void start();

  // Append script entries.  ALL entries must be queued before the
  // listener's background thread begins reading (i.e. before start()).
  // Calling either method after start() throws std::logic_error.
  void script(std::initializer_list<ScriptEntry> entries);
  void scriptAppend(ScriptEntry entry);

  // Release the next PktWaitForExtract gate.  Called by the test
  // fixture immediately after extractData() returns.
  void releaseExtractGate();

  // Diagnostics for assertions.
  bool clientConnected() const;
  std::size_t bytesSent() const;
  std::size_t scriptIndex() const; // how many entries have been delivered

  // Self-watchdog deadline (default 30 s).  If the script is not
  // exhausted by then, the server closes its sockets so the client
  // observes EOF rather than hanging.
  void setWatchdog(std::chrono::seconds);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

/// RAII watchdog: if not disarmed within the deadline, calls
/// g_log.fatal and std::abort().  Arm at the top of every test that
/// drives the listener; disarmed by fixture tearDown.
class TestWatchdog {
public:
  explicit TestWatchdog(std::chrono::seconds deadline, std::string testName);
  ~TestWatchdog(); // disarms if still armed
  void disarm();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace Mantid::LiveData::Testing

#endif // !_WIN32
