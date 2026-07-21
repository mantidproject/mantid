// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#ifndef _WIN32

#include "MockSMSServer.h"
#include "MantidKernel/Logger.h" // g_log.fatal for abort path
#include <Poco/Net/NetException.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Thread.h>
#include <sys/socket.h>
#include <unistd.h> // ::unlink

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
Mantid::Kernel::Logger g_log("MockSMSServer");
} // namespace

namespace Mantid::LiveData::Testing {

// ---------------------------------------------------------------------------
// MockSMSServer::Impl
// ---------------------------------------------------------------------------

struct MockSMSServer::Impl {
  std::string m_path;
  std::vector<ScriptEntry> m_script;
  std::chrono::seconds m_watchdog{30};

  // Connection state (protected by m_mutex)
  mutable std::mutex m_mutex;
  bool m_clientConnected{false};
  std::size_t m_bytesSent{0};
  std::size_t m_scriptIndex{0};

  // Extract gate (condition variable for PktWaitForExtract)
  std::condition_variable m_extractCV;
  bool m_extractReady{false};

  // Set by start() before the thread is launched; enforces the pre-start append contract.
  std::atomic<bool> m_started{false};
  // Stop flag for destructor
  std::atomic<bool> m_stop{false};

  // Sockets
  Poco::Net::ServerSocket m_listenSocket;
  Poco::Net::StreamSocket m_clientSocket;

  // Background thread
  std::thread m_thread;

  explicit Impl(std::string path) : m_path(std::move(path)) {}

  // Receive exactly 'len' bytes from the client socket.
  // Returns false on EOF or network error; true if all bytes were read.
  bool recvAll(void *buf, std::size_t len) {
    auto *ptr = static_cast<char *>(buf);
    std::size_t remaining = len;
    while (remaining > 0) {
      int n = m_clientSocket.receiveBytes(ptr, static_cast<int>(remaining));
      if (n <= 0)
        return false;
      ptr += n;
      remaining -= static_cast<std::size_t>(n);
    }
    return true;
  }

  // Receive one complete ADARA packet (16-byte header + payload).
  // Returns the full packet bytes on success, or an empty vector on read error
  // (error is logged internally).
  std::vector<uint8_t> recvPacket() {
    uint8_t hdr[16]{};
    if (!recvAll(hdr, sizeof(hdr))) {
      g_log.warning() << "MockSMSServer: failed to read packet header from listener\n";
      return {};
    }
    uint32_t payloadLen = static_cast<uint32_t>(hdr[0]) | (static_cast<uint32_t>(hdr[1]) << 8) |
                          (static_cast<uint32_t>(hdr[2]) << 16) | (static_cast<uint32_t>(hdr[3]) << 24);
    std::vector<uint8_t> pkt(sizeof(hdr) + payloadLen);
    std::memcpy(pkt.data(), hdr, sizeof(hdr));
    if (payloadLen > 0 && !recvAll(pkt.data() + sizeof(hdr), payloadLen)) {
      g_log.warning() << "MockSMSServer: failed to read packet payload from listener\n";
      return {};
    }
    return pkt;
  }

  // Check that packet 'pkt' carries the expected ADARA base packet type.
  // base type = (typeWord >> 8), where typeWord occupies bytes 4-7 (little-endian).
  // Returns false if the packet is too short or the base type does not match.
  static bool checkPktType(const std::vector<uint8_t> &pkt, uint16_t expectedBaseType) {
    if (pkt.size() < 8)
      return false;
    uint32_t typeWord = static_cast<uint32_t>(pkt[4]) | (static_cast<uint32_t>(pkt[5]) << 8) |
                        (static_cast<uint32_t>(pkt[6]) << 16) | (static_cast<uint32_t>(pkt[7]) << 24);
    return static_cast<uint16_t>(typeWord >> 8) == expectedBaseType;
  }

  void run() {
    try {
      // Accept exactly one client connection
      m_clientSocket = m_listenSocket.acceptConnection();
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clientConnected = true;
      }

      // Watchdog timer: if script not exhausted within deadline, close socket
      auto deadline = std::chrono::steady_clock::now() + m_watchdog;

      // SNSLiveEventDataListener::run() sends a CLIENT_HELLO packet as its
      // very first action (SNSLiveEventDataListener.cpp:225).  Read and verify
      // it before starting the script playback sequence.
      {
        auto helloPkt = recvPacket();
        if (helloPkt.empty()) {
          // recvPacket() already logged the error
          return;
        }
        // ADARA::PacketType::Type::CLIENT_HELLO_TYPE = 0x4006
        constexpr uint16_t CLIENT_HELLO_BASE = 0x4006u;
        if (!checkPktType(helloPkt, CLIENT_HELLO_BASE)) {
          uint32_t typeWord = static_cast<uint32_t>(helloPkt[4]) | (static_cast<uint32_t>(helloPkt[5]) << 8) |
                              (static_cast<uint32_t>(helloPkt[6]) << 16) | (static_cast<uint32_t>(helloPkt[7]) << 24);
          g_log.warning() << "MockSMSServer: expected CLIENT_HELLO packet (base type 0x4006) "
                          << "but received base type 0x" << std::hex << static_cast<uint16_t>(typeWord >> 8)
                          << " — aborting script playback\n";
          return;
        }
      }

      for (std::size_t i = 0; i < m_script.size(); ++i) {
        if (m_stop.load())
          break;

        // Check watchdog
        if (std::chrono::steady_clock::now() > deadline) {
          g_log.warning() << "MockSMSServer watchdog fired at script entry " << i << " — closing client socket\n";
          try {
            m_clientSocket.close();
          } catch (...) {
          }
          return;
        }

        auto &entry = m_script[i];

        if (std::holds_alternative<std::vector<uint8_t>>(entry)) {
          const auto &data = std::get<std::vector<uint8_t>>(entry);
          if (!data.empty()) {
            m_clientSocket.sendBytes(data.data(), static_cast<int>(data.size()));
          }
          std::lock_guard<std::mutex> lock(m_mutex);
          m_bytesSent += data.size();
          m_scriptIndex = i + 1;

        } else if (std::holds_alternative<PktGarbage>(entry)) {
          const auto &garbage = std::get<PktGarbage>(entry);
          if (!garbage.bytes.empty()) {
            m_clientSocket.sendBytes(garbage.bytes.data(), static_cast<int>(garbage.bytes.size()));
          }
          std::lock_guard<std::mutex> lock(m_mutex);
          m_bytesSent += garbage.bytes.size();
          m_scriptIndex = i + 1;

        } else if (std::holds_alternative<PktDisconnect>(entry)) {
          try {
            m_clientSocket.close();
          } catch (...) {
          }
          std::lock_guard<std::mutex> lock(m_mutex);
          m_scriptIndex = i + 1;
          return;

        } else if (std::holds_alternative<PktWaitForExtract>(entry)) {
          // Wait until releaseExtractGate() is called, or until watchdog fires
          std::unique_lock<std::mutex> lock(m_mutex);
          m_scriptIndex = i + 1;
          auto remaining = deadline - std::chrono::steady_clock::now();
          if (remaining <= std::chrono::seconds{0}) {
            g_log.warning() << "MockSMSServer watchdog fired during PktWaitForExtract gate\n";
            try {
              m_clientSocket.close();
            } catch (...) {
            }
            return;
          }
          m_extractCV.wait_for(lock, remaining, [this] { return m_extractReady || m_stop.load(); });
          m_extractReady = false;
          if (m_stop.load())
            return;
        }
      }
    } catch (Poco::Net::NetException &e) {
      if (!m_stop.load()) {
        g_log.warning() << "MockSMSServer network exception: " << e.displayText() << "\n";
      }
    } catch (std::exception &e) {
      if (!m_stop.load()) {
        g_log.warning() << "MockSMSServer exception: " << e.what() << "\n";
      }
    }
  }
};

// ---------------------------------------------------------------------------
// MockSMSServer public API
// ---------------------------------------------------------------------------

MockSMSServer::MockSMSServer(std::string path) : m_impl(std::make_unique<Impl>(std::move(path))) {}

MockSMSServer::~MockSMSServer() {
  m_impl->m_stop.store(true);

  // Release any waiting gate so the thread can observe the stop flag
  {
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    m_impl->m_extractReady = true;
  }
  m_impl->m_extractCV.notify_all();

  // Close sockets to break blocked accept() or send()
  try {
    m_impl->m_listenSocket.close();
  } catch (...) {
  }
  try {
    m_impl->m_clientSocket.close();
  } catch (...) {
  }

  // Join with 5-second timeout via a helper thread
  if (m_impl->m_thread.joinable()) {
    std::atomic<bool> joinDone{false};
    std::thread joiner([this, &joinDone] {
      m_impl->m_thread.join();
      joinDone.store(true);
    });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (!joinDone.load() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    if (joinDone.load()) {
      joiner.join();
    } else {
      joiner.detach();
      g_log.fatal() << "MockSMSServer: background thread did not exit within 5 s — aborting\n";
      std::abort();
    }
  }

  // Remove the socket file
  ::unlink(m_impl->m_path.c_str());
}

void MockSMSServer::start() {
  // Remove any stale UDS file left by a prior aborted test run so that bind()
  // does not fail with EADDRINUSE.  ENOENT is the expected case; any other
  // failure will surface clearly when ServerSocket below tries to bind.
  ::unlink(m_impl->m_path.c_str());
  m_impl->m_listenSocket = Poco::Net::ServerSocket(
      Poco::Net::SocketAddress(Poco::Net::AddressFamily::UNIX_LOCAL, m_impl->m_path), /*backlog=*/1);
  m_impl->m_started.store(true);
  m_impl->m_thread = std::thread([this] { m_impl->run(); });
}

void MockSMSServer::script(std::initializer_list<ScriptEntry> entries) {
  if (m_impl->m_started.load())
    throw std::logic_error("MockSMSServer::script() called after start(); "
                           "all entries must be queued before start()");
  for (const auto &e : entries)
    m_impl->m_script.push_back(e);
}

void MockSMSServer::scriptAppend(ScriptEntry entry) {
  if (m_impl->m_started.load())
    throw std::logic_error("MockSMSServer::scriptAppend() called after start(); "
                           "all entries must be queued before start()");
  m_impl->m_script.push_back(std::move(entry));
}

void MockSMSServer::releaseExtractGate() {
  {
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    m_impl->m_extractReady = true;
  }
  m_impl->m_extractCV.notify_one();
}

bool MockSMSServer::clientConnected() const {
  std::lock_guard<std::mutex> lock(m_impl->m_mutex);
  return m_impl->m_clientConnected;
}

std::size_t MockSMSServer::bytesSent() const {
  std::lock_guard<std::mutex> lock(m_impl->m_mutex);
  return m_impl->m_bytesSent;
}

std::size_t MockSMSServer::scriptIndex() const {
  std::lock_guard<std::mutex> lock(m_impl->m_mutex);
  return m_impl->m_scriptIndex;
}

void MockSMSServer::setWatchdog(std::chrono::seconds deadline) { m_impl->m_watchdog = deadline; }

// ---------------------------------------------------------------------------
// TestWatchdog::Impl
// ---------------------------------------------------------------------------

struct TestWatchdog::Impl {
  std::chrono::seconds m_deadline;
  std::string m_testName;
  std::atomic<bool> m_armed{true};
  std::thread m_thread;

  Impl(std::chrono::seconds deadline, std::string testName) : m_deadline(deadline), m_testName(std::move(testName)) {
    m_thread = std::thread([this] {
      auto end = std::chrono::steady_clock::now() + m_deadline;
      while (m_armed.load()) {
        if (std::chrono::steady_clock::now() >= end) {
          if (m_armed.load()) {
            g_log.fatal() << "TestWatchdog fired for test '" << m_testName << "' after " << m_deadline.count()
                          << " s — aborting\n";
            std::abort();
          }
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
      }
    });
  }
};

// ---------------------------------------------------------------------------
// TestWatchdog public API
// ---------------------------------------------------------------------------

TestWatchdog::TestWatchdog(std::chrono::seconds deadline, std::string testName)
    : m_impl(std::make_unique<Impl>(deadline, std::move(testName))) {}

TestWatchdog::~TestWatchdog() { disarm(); }

void TestWatchdog::disarm() {
  if (m_impl) {
    m_impl->m_armed.store(false);
    if (m_impl->m_thread.joinable())
      m_impl->m_thread.join();
  }
}

} // namespace Mantid::LiveData::Testing

#endif // !_WIN32
