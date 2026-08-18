// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#include <cstdlib> // for std::abort
#include <ctime>
#include <exception>
#include <sstream> // for ostringstream
#include <string>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Events.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WriteLock.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/SNSLiveEventDataListener.h"

// Includes for parsing the XML device descriptions
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/NodeList.h>

#include <Poco/Net/NetException.h>
#include <Poco/Net/PollSet.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Timestamp.h>

#include <Poco/Runnable.h>
#include <Poco/Thread.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

namespace { // anonymous namespace
// Receive timeout on the socket (seconds).  PollSet drives readiness; this
// is a backstop against spurious POLL_READ wakeups that would otherwise block
// receiveBytes() indefinitely.  1 s is orders of magnitude longer than any
// legitimate poll-to-recv interval, but short enough to keep shutdown prompt.
const int64_t RECV_TIMEOUT = 1;

// Names for a couple of time series properties
const std::string PAUSE_PROPERTY("pause");
const std::string SCAN_PROPERTY("scan_index");
const std::string PROTON_CHARGE_PROPERTY("proton_charge");

// These are names for some string properties (not time series)
const std::string RUN_TITLE_PROPERTY("run_title");
const std::string EXPERIMENT_ID_PROPERTY("experiment_identifier");

// Helper function to get a DateAndTime value from an ADARA packet header
Mantid::Types::Core::DateAndTime timeFromPacket(const ADARA::PacketHeader &hdr) {
  const auto seconds = static_cast<uint32_t>(hdr.pulseId() >> 32);
  const uint32_t nanoseconds = hdr.pulseId() & 0xFFFFFFFF;

  // Make sure we pick the correct constructor (the Mac gets an ambiguous error)
  return DateAndTime(static_cast<int64_t>(seconds), static_cast<int64_t>(nanoseconds));
}

} // anonymous namespace

namespace Mantid::LiveData {
DECLARE_LISTENER(SNSLiveEventDataListener)

namespace {

/// static logger
Kernel::Logger g_log("SNSLiveEventDataListener");

} // namespace

/// Constructor
SNSLiveEventDataListener::SNSLiveEventDataListener()
    : LiveListener(), ADARA::Parser(), m_socket()
// ADARA::Parser() will accept values for buffer size and max packet size,
// but the defaults will work fine
{

  // Perform all the workspace initialization steps (including actually creating
  // the workspace) that need to happen prior to receiving any packets.
  initWorkspacePart1();

  // Initialize m_keepPausedEvents from the config file.
  auto keepPausedEvents = ConfigService::Instance().getValue<bool>("SNSLiveEventDataListener.keepPausedEvents");

  // If the property hasn't been set, assume false
  m_keepPausedEvents = keepPausedEvents.value_or(false);
}

/// Destructor
SNSLiveEventDataListener::~SNSLiveEventDataListener() {
  // Stop the background thread
  if (m_thread.isRunning()) {
    // Ask the thread to exit.  run()'s loop re-checks m_stopThread every
    // iteration between bounded waits (100 ms pause-loop sleep, 100 ms
    // pollSet.poll() timeout, 1 s RECV_TIMEOUT on receiveBytes()), so it
    // should notice within about a second.
    m_stopThread.store(true, std::memory_order_release);

    // Unblock the thread immediately rather than waiting out those timeouts:
    // shut down both directions of the socket.  This is safe to call
    // concurrently with the background thread's in-flight receiveBytes() /
    // poll() on the same StreamSocket — shutdown() only issues shutdown(fd,
    // SHUT_RDWR) on the shared descriptor and does not touch Poco's
    // reference-counted SocketImpl, unlike close() (which would race on that
    // refcount).  A shut-down socket becomes read-ready with EOF, so poll()
    // wakes and receiveBytes() returns 0, which run() already treats as a
    // clean peer-close.  If the socket was never connected or is already
    // closed, shutdown() throws; there is nothing left to unblock.
    try {
      m_socket.shutdown();
    } catch (...) {
    }

    // Wait for the thread to actually exit before returning — i.e. before any
    // member is destroyed.  Never proceed while the thread is still running:
    // doing so previously let the background thread read freed memory after
    // a join() timeout ("Chose segfault - at least that's obvious").  Because
    // the socket shutdown above removes the only wait that could exceed the
    // per-iteration bound, the first tryJoin() below is expected to succeed;
    // the retry cap and abort() are a fail-loud backstop for the "should
    // never happen" case where the thread is still wedged regardless.
    constexpr int JOIN_TIMEOUT_MS = 5000;
    constexpr int MAX_JOIN_ATTEMPTS = 6; // ~30 s total, then a controlled crash
    for (int attempt = 1; !m_thread.tryJoin(JOIN_TIMEOUT_MS); ++attempt) {
      if (attempt >= MAX_JOIN_ATTEMPTS) {
        // Log first: g_log.fatal() writes synchronously, so the diagnostic is
        // emitted before abort() raises SIGABRT.  A controlled crash here is
        // preferable to both a silent use-after-free and an unbounded hang.
        g_log.fatal() << "SNSLiveEventDataListener: background thread did not exit within "
                      << (JOIN_TIMEOUT_MS * MAX_JOIN_ATTEMPTS / 1000)
                      << " s of socket shutdown; aborting to avoid use-after-free.\n";
        std::abort();
      }
      g_log.warning() << "SNSLiveEventDataListener: still waiting for the background thread "
                         "to exit after socket shutdown (attempt "
                      << attempt << " of " << MAX_JOIN_ATTEMPTS << ")...\n";
    }
  }
}

/// Connect to the SMS daemon.

/// Attempts to connect to the SMS daemon at the specified address.  Note:
/// if the address hasn't been set, it uses the "testaddress" (useful for
/// debugging and testing).
/// @param address The address to attempt to connect to
/// @return Returns true if the connection succeeds.  False otherwise.
bool SNSLiveEventDataListener::connect(const Poco::Net::SocketAddress &address)
// The SocketAddress class will throw various exceptions if it encounters an
// error.  We're assuming the calling function will catch any exceptions
// that are important.
// Note: Right now, it's the factory class that actually calls connect(),
// and it doesn't check the return value.  (It does, however, trap the Poco
// exceptions.)
{
  // If we don't have an address, make a connection to the test server running
  //   on localhost on the default port.
  if (address == Poco::Net::SocketAddress()) {
    // WARNING: check the config setting: system admin may not allow loopback!
    const auto maybeTestAddress =
        ConfigService::Instance().getValue<std::string>("SNSLiveEventDataListener.testAddress");
    if (!maybeTestAddress.has_value())
      throw std::runtime_error("SNSLiveEventDataListener: 'testAddress' is not set in `Config`");
    Poco::Net::SocketAddress testAddress(maybeTestAddress.value());

    try {
      m_socket.connect(testAddress); // BLOCKING connect
    } catch (...) {
      g_log.error() << "Connection to " << testAddress.toString() << " failed.\n";
      return false;
    }
  } else {
    try {
      m_socket.connect(address); // BLOCKING connect
    } catch (const Poco::Exception &e) {
      g_log.error() << "POCO Exception in connect(): " << e.displayText();
      return false;
    } catch (const std::exception &e) {
      g_log.error() << "STD Exception in connect(): " << e.what() << ": "
                    << " type: " << typeid(e).name();
      return false;
    } catch (...) {
      g_log.error() << "Unknown exception in connect()";
      return false;
    }
  }

  m_socket.setReceiveTimeout(Poco::Timespan(RECV_TIMEOUT, 0)); // POCO timespan is seconds, microseconds
  g_log.debug() << "Connected to " << m_socket.address().toString() << '\n';
  m_isConnected.store(true);

  return true;
}

/// Test to see if the object has connected to the SMS daemon

/// Test to see if the object has connected to the SMS daemon
/// @return Returns true if connected.  False otherwise.
bool SNSLiveEventDataListener::isConnected() { return m_isConnected.load(); }

/// Start the background thread

/// Starts the background thread which reads data from the network, parses it
/// and stores the resulting events in a temporary workspace.
/// @param startTime Specifies how much historical data the SMS should send
/// before continuing the current 'live' data.  Use 0 to indicate no
/// historical data.
void SNSLiveEventDataListener::start(const Types::Core::DateAndTime startTime) {
  // Save the startTime and kick off the background thread (Can't really do anything else until we send the hello packet
  // and the SMS sends us back the various metadata packets)
  m_startTime = startTime;

  if (m_startTime.totalNanoseconds() == 1000000000) {
    // 1 billion nanoseconds - ie: 1 second past the EPOCH
    // Start live listener sends us this when it wants to start processing at the start of the previous run (and it
    // doesn't know when the previous run started).  This value for a start time will cause the SMS to replay all of its
    // historical data and it will be up to us to filter out everything before the start of the previous run. See the
    // description of the 'Client Hello' packet in the SNS DAS design doc for more details
    m_filterUntilRunStart = true;
  }
  // make sure all monitors are considered ok
  m_badMonitors.clear();

  m_thread.start(*this);
}

int SNSLiveEventDataListener::sendHelloPacket(const void *buf, int size) {
  const auto *p = static_cast<const char *>(buf);
  int remaining = size;
  while (remaining > 0) {
    int n = m_socket.sendBytes(p, remaining);
    if (n <= 0)
      return size - remaining; // short-write; caller detects
    p += n;
    remaining -= n;
  }
  return size;
}

/// The main function for the background thread

/// Loops until the forground thread requests it to stop.  Reads data from the
/// network, parses it and stores the resulting events (and other metadata) in
/// a temporary workspace.
void SNSLiveEventDataListener::run() {
  // Centralised side-effect helper: sets m_isConnected=false and writes
  // m_backgroundException (first-writer-wins) under m_mutex.  Called from
  // fatal() and from every outer catch arm so the locking + first-writer-wins
  // idiom lives in one place.
  auto setFatalState = [this](std::shared_ptr<std::runtime_error> ex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isConnected.store(false);
    if (!m_backgroundException)
      m_backgroundException = std::move(ex);
  };

  // Fatal-socket-failure helper.  Logs, sets fatal state, then throws
  // std::runtime_error so the outer catch cascade lands the thread cleanly.
  auto fatal = [&setFatalState](const std::string &msg) {
    g_log.fatal() << msg << '\n';
    setFatalState(std::make_shared<std::runtime_error>(msg));
    throw std::runtime_error(msg);
  };

  try {
    if (!m_isConnected.load()) // sanity check
    {
      throw std::runtime_error("SNSLiveEventDataListener: No connection to SMS server.");
    }

    // First thing to do is send a hello packet.
    uint32_t typeVal = ADARA_PKT_TYPE(ADARA::PacketType::Type::CLIENT_HELLO_TYPE, 0);
    uint32_t helloPkt[5] = {4, typeVal, 0, 0, 0};
    // TODO: The packet version should be bumped to 1 and we should add
    // the extra flags field.  This will have to wait until we're ready
    // to update the StartLiveListener GUI, though.

    Poco::Timestamp now;
    uint32_t now_usec = static_cast<uint32_t>(now.epochMicroseconds() % 1000000);

    helloPkt[2] = static_cast<uint32_t>(now.epochTime() - ADARA::EPICS_EPOCH_OFFSET);
    helloPkt[3] = now_usec * 1000;
    helloPkt[4] = static_cast<uint32_t>(m_startTime.totalNanoseconds() /
                                        1000000000); // divide by a billion to get time in seconds

    if (sendHelloPacket(helloPkt, static_cast<int>(sizeof(helloPkt))) != static_cast<int>(sizeof(helloPkt))) {
      fatal("SNSLiveEventDataListener: short write on client hello packet.");
    }

    Poco::Net::PollSet pollSet;
    pollSet.add(m_socket, Poco::Net::PollSet::POLL_READ);

    // bg-thread-local parse-pending flag (plain bool — only this thread touches
    // it; no atomic needed).  Set whenever bufferParse() should run on the
    // next iteration: fresh bytes were appended, the parse buffer is full and
    // must be drained before more bytes can be read, or a prior parse was
    // interrupted and bytes remain stranded in the parser's internal buffer
    // where poll() cannot see them.
    bool needParse = false;

    while (!m_stopThread.load(std::memory_order_acquire)) // loop until the foreground thread tells us to stop
    {
      // Gate: only honour m_pauseNetRead when no parse is in flight.
      // Short-circuit evaluation enforces the invariant — when
      // m_bgThreadCaughtUp is false (i.e. we are mid-bufferParse) the pause
      // check is skipped entirely, preventing a race where the foreground
      // clears m_pauseNetRead while we are still mutating state.
      while (m_bgThreadCaughtUp.load(std::memory_order_acquire) && m_pauseNetRead.load(std::memory_order_acquire) &&
             !m_stopThread.load(std::memory_order_acquire)) {
        // foreground thread doesn't want us to process any more packets until
        // it's ready.  See comments in rxPacket( const ADARA::RunStatusPkt &pkt)
        Poco::Thread::sleep(100); // 100 milliseconds
      }

      if (m_stopThread.load(std::memory_order_acquire)) {
        // it's possible that a stop request came in while we were sleeping...
        break;
      }

      // Skip poll() when needParse is already set — the pending bytes are in
      // the parser's internal buffer, not the kernel's socket buffer, so
      // poll() cannot see them.  Go straight to parse; the pause condition
      // is re-checked at the top of the next iteration.
      if (!needParse) {
        // Wait up to 100 ms for socket readiness.  Short timeout keeps
        // shutdown latency low and lets m_stopThread be checked promptly.
        auto ready = pollSet.poll(Poco::Timespan(0, 100000));

        // bufferParse() is only useful when bytes have just been appended or
        // when the parse buffer is full and must be drained to make room.
        // All other cases — poll timeout, spurious POLL_READ — are no-ops
        // that would waste CPU and spam the log string.
        if (!ready.empty()) {
          auto it = ready.find(m_socket);
          if (it != ready.end()) {
            const int mode = it->second;

            // POLL_READ first: on Linux a peer close with pending bytes
            // delivers EPOLLIN|EPOLLHUP together; drain bytes before treating
            // the hangup as fatal so the final END_RUN packet is not discarded.
            // On macOS kqueue, EV_ERROR (kevent changelist failure) can
            // co-occur with EVFILT_READ (valid pending data) after a
            // back-pressure pause; successfulRead below suppresses the
            // POLL_ERROR fatal in that case.
            bool successfulRead = false;
            if (mode & Poco::Net::PollSet::POLL_READ) {
              unsigned int bufFillLen = bufferFillLength();
              if (bufFillLen) {
                uint8_t *bufFillAddr = bufferFillAddress();
                int bytesRead = -1;
                try {
                  bytesRead = m_socket.receiveBytes(bufFillAddr, bufFillLen);
                } catch (Poco::TimeoutException &) {
                  // Spurious POLL_READ (rare on epoll, not zero) — transient.
                  // Skip this iteration; next poll() will re-evaluate.
                  // Do NOT log here: the loop fires every ~100 ms and would flood.
                  bytesRead = -1;
                } catch (Poco::Net::NetException &e) {
                  fatal(std::string("SNSLiveEventDataListener: network read failed: ") + e.name());
                }

                if (bytesRead == 0) {
                  // Clean peer-close.  Not an error — set Disconnected state
                  // and exit the bg thread cleanly.  doExtractData() will
                  // surface this to consumers via its disconnect checks.
                  // Set m_bgThreadCaughtUp so onBeforeExtract() can proceed
                  // past its NotYet gate even if we exit before bufferParse().
                  m_isConnected.store(false);
                  m_bgThreadCaughtUp.store(true, std::memory_order_release);
                  return;
                }
                if (bytesRead > 0) {
                  bufferBytesAppended(bytesRead);
                  needParse = true; // new bytes may complete one or more packets
                  successfulRead = true;
                }
                // bytesRead < 0 (TimeoutException): no new bytes; needParse unchanged.
              } else {
                // Buffer is full; we cannot read more bytes until the parser
                // drains some complete packets.  Signal parse so the loop
                // makes forward progress instead of spinning on POLL_READ.
                needParse = true;
                successfulRead = true; // event handled; defer POLL_ERROR check
                                       // to the next iteration after buffer drains
              }
            }

            if ((mode & Poco::Net::PollSet::POLL_ERROR) && !successfulRead) {
              // Only fatal when POLL_READ did not successfully handle the event.
              // When POLL_READ already read bytes (successfulRead=true), any
              // concurrent POLL_ERROR is a transient kqueue artefact — the next
              // poll() iteration will clear it or fire a pure POLL_ERROR if the
              // socket is genuinely broken.
              fatal("SNSLiveEventDataListener: socket poll reported error.");
            }
          }
        }
        // If poll() timed out, needParse stays false — go straight back to poll.
        // CPU already yielded for 100 ms; no additional sleep needed.
      }

      if (needParse) {
        needParse = false; // consume the signal; re-set below if interrupted
        // Close the foreground snapshot window for the duration of bufferParse()
        // only — rxPacket() calls inside it may mutate m_pendingTransition and
        // m_pauseNetRead, so the foreground must not snapshot between calls.
        m_bgThreadCaughtUp.store(false, std::memory_order_release);
        int packetsParsed;
        {
          // Hold m_mutex for the entire parse iteration so every rxPacket()
          // override runs under the lock.  This makes "caller holds m_mutex"
          // an invariant for all rxPacket() bodies and their callees
          // (initWorkspacePart2(), appendEvent(), etc.), eliminating the
          // per-packet lock acquisitions that previously existed inside each
          // override.  Fixes Coverity CIDs 1663860 and 1663859 (accesses to
          // m_adaraRunStatus / m_pendingTransition without the lock in
          // initWorkspacePart2()'s tail) and the broader inconsistency where
          // six of the seven initWorkspacePart2() callers previously ran it
          // without holding m_mutex at all.
          //
          // m_bgThreadCaughtUp is stored outside this scope (atomic, not
          // mutex-guarded) — the foreground's NotYet gate reads it without
          // the lock, so it must not be inside the lock scope.
          //
          // setFatalState() / fatal() are only called from run()'s catch arms
          // and pre-parse code paths; they are not reachable from within
          // bufferParse(), so there is no recursive acquisition.
          std::lock_guard<std::mutex> lock(m_mutex);
          std::string bufferParseLog; // required arg: unused
          packetsParsed = bufferParse(bufferParseLog);
        }
        // Reopen the snapshot window — all rxPacket() calls for this iteration
        // have completed; m_pendingTransition and m_pauseNetRead are stable.
        m_bgThreadCaughtUp.store(true, std::memory_order_release);

        if (packetsParsed < 0) {
          // bufferParse() was interrupted by a callback returning true
          // (rxPacket(RunStatusPkt) sets m_pauseNetRead and returns true).
          // Remaining packets are in the parser's internal buffer; process
          // them after the pause loop releases — poll() won't wake for them
          // because they are not in the kernel's socket buffer.
          needParse = true;
        }
        // packetsParsed == 0: fragment only; the next iteration calls poll()
        // which already yields the CPU up to 100 ms — no sleep needed.
      }
    }

    // If we've gotten here, it's because the thread has thrown an otherwise
    // uncaught exception.  In such a case, the thread will exit and there's
    // nothing we can do about that.  We'll log an error and save a copy of the
    // exception object so that we can re-throw it from the foreground thread
    // (which will cause the algorithm to exit).
    // NOTE: For the default exception handler, we actually create a new
    // runtime_error object and throw that, since there's no exception object
    // passed in to the handler.
  } catch (ADARA::invalid_packet &e) { // exception handler for invalid packets
    // For now, log it and let the thread exit.  In the future, we might
    // try to recover from this.  (A bad event packet could probably just
    // be ignored, for example)
    g_log.fatal() << "Caught an invalid packet exception in "
                     "SNSLiveEventDataListener network read thread.\n"
                  << "Exception message is: " << e.what() << ".\n"
                  << "Thread is exiting.\n";
    setFatalState(std::make_shared<ADARA::invalid_packet>(e));
  } catch (std::runtime_error &e) { // exception handler for generic runtime
                                    // exceptions
    g_log.fatal() << "Caught a runtime exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit.\n";
    // No-op when this exception came from the fatal() helper,
    //  which already called setFatalState() before throwing
    setFatalState(std::make_shared<std::runtime_error>(e));
  } catch (std::invalid_argument &e) { // TimeSeriesProperty (and possibly some
                                       // other things) can throw these errors
    g_log.fatal() << "Caught an invalid argument exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit.\n";
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_workspaceInitialized = true; // see the comments in the default exception
                                     // handler for why we set this value.
    }
    std::string newMsg("Invalid argument exception thrown from the background thread: ");
    newMsg += e.what();
    setFatalState(std::make_shared<std::runtime_error>(newMsg));
  } catch (std::exception &e) { // exception handler for generic exceptions
    g_log.fatal() << "Caught an exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit.\n";
    setFatalState(std::make_shared<std::runtime_error>(e.what()));
  } catch (...) { // Default exception handler
    g_log.fatal("Uncaught exception in SNSLiveEventDataListener network read thread."
                " Thread is exiting.");
    setFatalState(std::make_shared<std::runtime_error>("Unknown error in background thread"));
  }
}

/// Parse a banked event packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::BankedEventPkt packets.  Parsed events are stored
/// in the temporary workspace until the forground thread retrieves
/// them.
/// @see extractData()
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::BankedEventPkt &pkt) {
  // Check to see if we should process this packet (depending on what
  // the user selected for start up options, the SMS might be replaying
  // historical data that we don't care about).
  if (ignorePacket(pkt)) {
    return false;
  }

  // Assuming we're going to process this packet, check to see if we need
  // to finish our initialization steps
  // We should already be initialized by the time we get here, but
  // we'll give it one last try anyway since we'll have to throw
  // the packet away if we can't initialize.
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }

    // If we weren't ready to init, or the init failed, that's an error and
    // we can't process this packet at all.
    if (!m_workspaceInitialized) {
      g_log.error("Cannot process BankedEventPacket because workspace isn't "
                  "initialized.");
      // Note: One error message per BankedEventPkt is likely to absolutely
      // flood the error log.
      // Might want to think about rate limiting this somehow...

      return false; // We still return false (ie: "no error") because there's no
                    // reason to stop parsing the data stream
    }
  }

  // A counter that we use for logging purposes
  unsigned totalEvents = 0;

  // First, check to see if the run has been paused.  We don't process
  // the events if we're paused unless the user has specifically overridden
  // this behavior with the livelistener.keeppausedevents property.
  if (m_isDasPaused.load() && (!m_keepPausedEvents)) {
    return false;
  }

  // Append the events
  g_log.debug() << "----- Pulse ID: " << pkt.pulseId() << " -----\n";
  // Timestamp for the events
  Mantid::Types::Core::DateAndTime eventTime = timeFromPacket(pkt);

  // Save the pulse charge in the logs (*10 because we want the units to be
  // picoCulombs, and ADARA sends them out in units of 10pC)
  m_eventBuffer->mutableRun()
      .getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)
      ->addValue(eventTime, pkt.pulseCharge() * 10);

  // Iterate through each event
  const ADARA::Event *event = pkt.firstEvent();
  unsigned lastBankID = pkt.curBankId();
  // A counter that we use for logging purposes
  unsigned eventsPerBank = 0;
  while (event != nullptr) {
    eventsPerBank++;
    totalEvents++;
    if (lastBankID < 0xFFFFFFFE) // Bank ID -1 & -2 are special cases and are
                                 // not valid pixels
    {
      // appendEvent needs tof to be in units of microseconds, but it comes
      // from the ADARA stream in units of 100ns.
      if (pkt.getSourceCORFlag()) {
        appendEvent(event->pixel, event->tof / 10.0, eventTime);
      } else {
        appendEvent(event->pixel, (event->tof + pkt.getSourceTOFOffset()) / 10.0, eventTime);
      }
    }

    event = pkt.nextEvent();
    if (pkt.curBankId() != lastBankID) {
      g_log.debug() << "BankID " << lastBankID << " had " << eventsPerBank << " events\n";

      lastBankID = pkt.curBankId();
      eventsPerBank = 0;
    }
  }

  g_log.debug() << "Total Events: " << totalEvents << "\n";
  g_log.debug("-------------------------------");

  return false;
}

/// Parse a beam monitor event packet

/** Overrides the default function defined in ADARA::Parser and processes data from ADARA::BeamMonitorPkt packets.
 * Parsed events are counted and the counts are accumulated in the temporary workspace until the foreground thread
 * retrieves them.
 *
 * @see extractData()
 * @param pkt The packet to be parsed
 * @return false if no problems, true if error occurred and packet parsing should be interrupted
 */
bool SNSLiveEventDataListener::rxPacket(const ADARA::BeamMonitorPkt &pkt) {
  // Check to see if we should process this packet (depending on what the user selected for start up options, the SMS
  // might be replaying historical data that we don't care about).
  if (ignorePacket(pkt)) {
    return false;
  }

  if (!m_eventBuffer->monitorWorkspace())
    return false;

  auto monitorBuffer = std::static_pointer_cast<DataObjects::EventWorkspace>(m_eventBuffer->monitorWorkspace());
  const auto pktTime = timeFromPacket(pkt);

  while (pkt.nextSection()) {
    const detid_t monitorID = static_cast<detid_t>(pkt.getSectionMonitorID());

    if (monitorID > 5) {
      // Currently, we only handle monitors 0-5.  At the present time, that's sufficient.
      g_log.error() << "Mantid cannot handle monitor ID's higher than 5.  If " << monitorID
                    << " is actually valid, then an appropriate entry must be made to the ADDABLE list at the top of "
                       "Framework/API/src/Run.cpp\n";
    } else {
      std::string monName("monitor");
      monName += static_cast<char>(monitorID + 48); // The +48 converts to the ASCII character
      monName += "_counts";
      // Note: The monitor name must exactly match one of the entries in the ADDABLE list at the top of Run.cpp!

      int events = pkt.getSectionEventCount();
      if (m_eventBuffer->run().hasProperty(monName)) {
        events += m_eventBuffer->run().getPropertyValueAsType<int>(monName);
      } else {
        // First time we've received this monitor.  Add it to our list
        m_monitorLogs.emplace_back(monName);
      }

      // Update the property value (overwriting the old value if there was one)
      m_eventBuffer->mutableRun().addProperty<int>(monName, events, true);

      const auto it = m_monitorIndexMap.find(-1 * monitorID); // Monitor IDs are negated in Mantid IDFs
      if (it != m_monitorIndexMap.end()) {
        bool risingEdge;
        uint32_t cycle, tof;
        while (pkt.nextEvent(risingEdge, cycle, tof)) {
          // Add the event. Note that they're in units of 100 ns in the packet, need to change to microseconds.
          monitorBuffer->getSpectrum(it->second).addEventQuickly(Types::Event::TofEvent(tof / 10.0, pktTime));
        }
      } else {
        // only notify about the first bad monitor id
        if (!m_badMonitors.contains(monitorID)) {
          m_badMonitors.insert(monitorID);
          g_log.error() << "Event from unknown monitor ID (" << monitorID << ") seen.\n";
        }
      }
    }
  }

  return false;
}

/// Parse a geometry packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::GeometryPkt packets.  The data is used to initialize
/// the temporary workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::GeometryPkt &pkt) {
  // Note: Deliberately NOT calling ignorePacket() because we always parse
  // these packets

  // TODO: For now, I'm assuming that we only need to process one of these
  // packets the first time it comes in and we can ignore any others.
  if (!m_workspaceInitialized) {
    m_instrumentXML = pkt.info(); // save the xml so we can pass it to the
                                  // LoadInstrument algorithm

    // Now parse the XML for required logfile parameters (we can't call the
    // LoadInstrument alg until we received a value for such parameters).
    //
    // What we're looking for is a node called "parameter" that has a child
    // node called "logfile".  We need to save the id attribute on the
    // logfile node.

    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> doc = parser.parseString(m_instrumentXML);

    const Poco::AutoPtr<Poco::XML::NodeList> nodes = doc->getElementsByTagName("parameter");
    // Oddly, NodeLists don't seem to have any provision for iterators.  Also,
    // the length() function actually traverses the list to get the count,
    // so we should probably call it once and store it in a variable instead
    // of putting it at the top of a for loop...
    long unsigned nodesLength = nodes->length();
    for (long unsigned i = 0; i < nodesLength; i++) {
      Poco::XML::Node *node = nodes->item(i);
      const Poco::AutoPtr<Poco::XML::NodeList> childNodes = node->childNodes();

      long unsigned childNodesLength = childNodes->length();
      for (long unsigned j = 0; j < childNodesLength; j++) {
        Poco::XML::Node *childNode = childNodes->item(j);
        if (childNode->nodeName() == "logfile") {
          // Found one!
          Poco::AutoPtr<Poco::XML::NamedNodeMap> attr = childNode->attributes();
          long unsigned attrLength = attr->length();
          for (long unsigned k = 0; k < attrLength; k++) {
            Poco::XML::Node *attrNode = attr->item(k);
            if (attrNode->nodeName() == "id") {
              m_requiredLogs.emplace_back(attrNode->nodeValue());
            }
          }
        }
      }
    }
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }
  return false;
}

/// Parse a beamline info packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::BeamlineInfoPkt packets.  The data is used to initialize
/// the temporary workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::BeamlineInfoPkt &pkt) {
  // Note: Deliberately NOT calling ignorePacket() because we always parse
  // these packets

  // We only need to process a beamlineinfo packet once
  if (!m_workspaceInitialized) {
    // We need the instrument name
    m_instrumentName = pkt.longName();
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }
  return false;
}

/// Parse a run status packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::RunStatusPkt packets.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::RunStatusPkt &pkt) {
  // grab the time from the packet - we'll use it down in
  // initializeWorkspacePart2()
  // Note that we need this value even if we otherwise ignore the packet
  m_dataStartTime = timeFromPacket(pkt);

  // Check to see if we should process the rest of this packet (depending
  // on what the user selected for start up options, the SMS might be
  // replaying historical data that we don't care about).
  if (ignorePacket(pkt, pkt.status())) {
    return false;
  }

  const bool haveRunNumber = m_eventBuffer->run().hasProperty("run_number");

  if (pkt.status() == ADARA::RunStatus::NEW_RUN) {
    // Starting a new run.

    if (m_adaraRunStatus != NoRun) {
      // Previous status should have been NoRun.  Spit out a warning if it's not.
      g_log.warning() << "Unexpected start of run.  Run status should have been " << NoRun << " (NoRun), but was "
                      << m_adaraRunStatus << '\n';
    }

    if (m_workspaceInitialized) {
      // Transition case: NEW_RUN arrived while the workspace from the previous
      // state is already live (e.g., SMS sends NEW_RUN after a STATE-packet init
      // completed without an intervening END_RUN).  Apply back-pressure so the
      // bg thread stops and the foreground can harvest the prior workspace before
      // onBeginRun() resets it.  Run details are deferred to onBeginRun().
      if (m_pendingTransition.has_value()) {
        throw std::runtime_error("SNSLiveEventDataListener: pending run-state transition was not "
                                 "consumed before a new BeginRun arrived — back-pressure invariant "
                                 "violation.");
      }
      m_pendingTransition = BeginRun;
      // Save a copy of the packet so we can call setRunDetails() in onBeginRun()
      // after extractData() has fetched any data remaining from before this run.
      m_deferredRunDetailsPkt = std::shared_ptr<ADARA::RunStatusPkt>(new ADARA::RunStatusPkt(pkt));
      m_pauseNetRead.store(true, std::memory_order_release);
    } else {
      // Joining case: NEW_RUN arrived before workspace initialisation completed.
      // This is the normal startup path: every run whose NEW_RUN arrives while
      // m_workspaceInitialized is false takes this branch (because onEndRun()
      // resets m_workspaceInitialized and m_dataStartTime, so initWorkspacePart2()
      // cannot complete until the next run's NEW_RUN restores m_dataStartTime).
      //
      // Transition to JoiningRun immediately — runState() now truthfully reports
      // that the DAS is in a run.  Apply run details now so they are in place
      // when initWorkspacePart2() completes.  We do NOT set m_pauseNetRead: the
      // bg thread must keep reading to receive the Geometry and Beamline packets
      // required for initialisation.  The BeginRun commit edge is queued by
      // initWorkspacePart2() once it succeeds; onBeginRun() then advances from
      // JoiningRun to Running without resetting any workspace state.
      if (m_adaraRunStatus == JoiningRun) {
        throw std::runtime_error("SNSLiveEventDataListener: received NEW_RUN while already in JoiningRun "
                                 "— two consecutive NEW_RUN packets without an intervening END_RUN.  "
                                 "The data stream is malformed.");
      }
      m_adaraRunStatus = JoiningRun;
      setRunDetails(pkt);
    }

  } else if (pkt.status() == ADARA::RunStatus::END_RUN) {
    // Run has ended: queue the EndRun transition edge, record run_end, and
    // set the back-pressure flag to stop parsing network packets so the
    // foreground can harvest the finishing run's events before onEndRun()
    // commits the state transition (m_adaraRunStatus is advanced to NoRun
    // from onEndRun(), not here — see comments below for why we pause
    // network reads).
    if ((m_adaraRunStatus != Running) && (m_adaraRunStatus != JoiningRun) && (m_pendingTransition != BeginRun)) {
      // Previous status should have been Running (or JoiningRun for a run that
      // ended before init completed).  Spit out a warning if it's not.
      // (If m_pendingTransition == BeginRun, the single-slot check below applies.)
      g_log.warning() << "Unexpected end of run.  Run status should have been " << Running << " (Running), but was "
                      << m_adaraRunStatus << '\n';
    }
    if (m_pendingTransition.has_value()) {
      if (m_pendingTransition == BeginRun && m_adaraRunStatus == JoiningRun) {
        // The run ended before the BeginRun commit edge was consumed by the
        // foreground (run started and ended during the joining/init window).
        // Collapse: drop the partial-run BeginRun edge and replace with EndRun.
        // The foreground will see an EndRun transition but no preceding BeginRun,
        // which matches the behaviour of the former "white-lie" code path.
        m_pendingTransition.reset();
      } else {
        throw std::runtime_error("SNSLiveEventDataListener: pending run-state transition was not "
                                 "consumed before EndRun arrived — back-pressure invariant "
                                 "violation.");
      }
    }
    m_pendingTransition = EndRun;

    // Add the run_end property
    m_eventBuffer->mutableRun().addProperty("run_end", timeFromPacket(pkt).toISO8601String());

    // Set the flag to make us stop reading from the network.
    // Stopping network reads solves a number of problems:
    // 1) We don't need to manage a second buffer in order to keep the events
    // in the just ended run separate from the events in the next run.
    // 2) We don't have to deal with the case where the next run has already
    // started but extractData() hasn't been called to fetch the last events
    // from the previous run.
    // 3) We don't have to worry about the case where more than one run has
    // started and finished between calls to extractData()  (ie: 5 second runs
    // and a 10 second update interval.  Yes, that's an operator error, but I
    // still don't want to worry about it.)
    //
    // Because of this, however, if extractData() isn't called at least once
    // per run, the network packets may start to back up and SMS may eventually
    // disconnect us.
    // This flag will be cleared in onEndRun() (called from onAfterExtract()),
    // which is guaranteed to be called after extractData() has returned data.

    m_pauseNetRead.store(true, std::memory_order_release);

    // Set the run number & start time if we don't already have it
    if (!haveRunNumber) {
      setRunDetails(pkt);
    }
  } else if (pkt.status() == ADARA::RunStatus::STATE && !haveRunNumber) {
    // A packet status of STATE and no run number means we've just connected
    // to the SMS.  Specifically, this is the RunStatus packet that SMS
    // initially sends out when a client hasn't set the flag to request
    // historical data.  We may or may not actually be in a run right now.
    // If we are, then we need to set the run details.  If not, there's
    // nothing we need to do with this packet.
    if (pkt.runNumber() != 0) {
      setRunDetails(pkt);
    }
  }

  // Note: all other possibilities for pkt.status() can be ignored

  // Check to see if we can/should complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return m_pauseNetRead.load(std::memory_order_acquire);
  // If we've set m_pauseNetRead, it means we want to stop processing packets.
  // In that case, we need to return true so that we'll break out of the read()
  // loop in the packet parser.
}

void SNSLiveEventDataListener::setRunDetails(const ADARA::RunStatusPkt &pkt) {
  m_runNumber = pkt.runNumber();
  m_eventBuffer->mutableRun().addProperty("run_number", Strings::toString<int>(pkt.runNumber()));
  g_log.notice() << "Run number is " << m_runNumber << '\n';

  // runStart() is in the EPICS epoch - ie Jan 1, 1990.  Convert to Unix epoch
  time_t runStartTime = pkt.runStart() + ADARA::EPICS_EPOCH_OFFSET;

  // Add the run_start property
  char timeString[64]; // largest the string should end up is 20 (plus a null
                       // terminator)
  strftime(timeString, 64, "%Y-%m-%dT%H:%M:%SZ", gmtime(&runStartTime));
  // addProperty() wants the time as an ISO 8601 string
  m_eventBuffer->mutableRun().addProperty("run_start", std::string(timeString));
}

/// Parse a variable value packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::VariableU32Pkt packets.  The extracted value is stored
/// in the sample log of the temporary workspace.
/// @warning The specified variable must have already been described in a
/// ADARA::DeviceDescriptorPkt packet.
/// @see SNSLiveEventDataListener::rxPacket( const ADARA::DeviceDescriptorPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::VariableU32Pkt &pkt) {
  unsigned devId = pkt.devId();
  unsigned pvId = pkt.varId();

  // Check to see if we should process this packet now.  If not, add it to the
  // variable map because we might need to process it later
  if (ignorePacket(pkt)) {
    m_variableMap.emplace(std::make_pair(devId, pvId), std::make_shared<ADARA::VariableU32Pkt>(pkt));
  } else {
    // Look up the name of this variable
    NameMapType::const_iterator it = m_nameMap.find(std::make_pair(devId, pvId));

    if (it == m_nameMap.end()) {
      g_log.error() << "Ignoring variable value packet for device " << devId << ", variable " << pvId
                    << " because we haven't received a device descriptor "
                       "packet for it.\n";
    } else {
      m_eventBuffer->mutableRun().getTimeSeriesProperty<int>((*it).second)->addValue(timeFromPacket(pkt), pkt.value());
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a variable value packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::VariableDoublePkt packets.  The extracted value is stored
/// in the sample log of the temporary workspace.
/// @warning The specified variable must have already been described in a
/// ADARA::DeviceDescriptorPkt packet.
/// @see SNSLiveEventDataListener::rxPacket( const ADARA::DeviceDescriptorPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::VariableDoublePkt &pkt) {
  unsigned devId = pkt.devId();
  unsigned pvId = pkt.varId();

  // Check to see if we should process this packet now.  If not, add it to the
  // variable map because we might need to process it later
  if (ignorePacket(pkt)) {
    m_variableMap.emplace(std::make_pair(devId, pvId), std::make_shared<ADARA::VariableDoublePkt>(pkt));
  } else {
    // Look up the name of this variable
    NameMapType::const_iterator it = m_nameMap.find(std::make_pair(devId, pvId));

    if (it == m_nameMap.end()) {
      g_log.error() << "Ignoring variable value packet for device " << devId << ", variable " << pvId
                    << " because we haven't received a device descriptor "
                       "packet for it.\n";
    } else {
      m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<double>((*it).second)
          ->addValue(timeFromPacket(pkt), pkt.value());
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a variable value packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::VariableStringPkt packets.  The extracted value is stored
/// in the sample log of the temporary workspace.
/// @warning The specified variable must have already been described in a
/// ADARA::DeviceDescriptorPkt packet.
/// @see SNSLiveEventDataListener::rxPacket( const ADARA::DeviceDescriptorPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
/// @remarks As of Februrary 2013, the SMS does not actually send out packets
/// of this type.  As such, this particular function has received very little
/// testing.
bool SNSLiveEventDataListener::rxPacket(const ADARA::VariableStringPkt &pkt) {
  unsigned devId = pkt.devId();
  unsigned pvId = pkt.varId();

  // Check to see if we should process this packet now.  If not, add it to the
  // variable map because we might need to process it later
  if (ignorePacket(pkt)) {
    m_variableMap.emplace(std::make_pair(devId, pvId), std::make_shared<ADARA::VariableStringPkt>(pkt));
  } else {
    // Look up the name of this variable
    NameMapType::const_iterator it = m_nameMap.find(std::make_pair(devId, pvId));

    if (it == m_nameMap.end()) {
      g_log.error() << "Ignoring variable value packet for device " << devId << ", variable " << pvId
                    << " because we haven't received a device descriptor "
                       "packet for it.\n";
    } else {
      m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<std::string>((*it).second)
          ->addValue(timeFromPacket(pkt), pkt.value());
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a device decriptor packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::DeviceDecriptorPkt packets.  These packets contain
/// XML text decribing variables that will be received in subsequent
/// ADARA::VariableU32Pkt, ADARA::VariableDoublePkt and
/// ADARA::VariableStringPkt packets.
/// @see rxPacket( const ADARA::VariableU32Pkt &)
/// @see rxPacket( const ADARA::VariableDoublePkt &)
/// @see rxPacket( const ADARA::VariableStringPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::DeviceDescriptorPkt &pkt) {
  // Note: Deliberately NOT calling ignorePacket() because we always parse
  // these packets

  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> doc = parser.parseMemory(pkt.description().c_str(), pkt.description().length());
  const Poco::XML::Node *deviceNode = doc->firstChild();

  // The 'device' should be the root element of the document.  I'm just being
  // paranoid here.
  while (deviceNode && deviceNode->nodeName() != "device") {
    deviceNode = deviceNode->nextSibling();
  }

  if (!deviceNode) {
    g_log.error("Device descriptor packet did not contain a device element!!  "
                "This should never happen!");
    return false;
  }

  // Find the process_variables element
  // Note: for now, I'm ignoring the 'device_name' & 'enumeration' elements
  // because I don't think I need them

  const Poco::XML::Node *node = deviceNode->firstChild();
  while (node && node->nodeName() != "process_variables") {
    node = node->nextSibling();
  }

  if (!node) {
    g_log.warning("Device descriptor packet did not contain a "
                  "process_variables element.");
    return false;
  }

  node = node->firstChild();
  while (node) {
    // iterate through each individual variable...
    if (node->nodeName() == "process_variable") {
      // we need the name, ID and type
      const Poco::XML::Node *pvNode = node->firstChild();
      std::string pvName;
      std::string pvId;
      unsigned pvIdNum;
      std::string pvUnits;
      std::string pvType;
      while (pvNode) {
        const Poco::XML::Node *textElement = pvNode->firstChild();
        if (textElement) {
          if (pvNode->nodeName() == "pv_name") {
            pvName = textElement->nodeValue();
          } else if (pvNode->nodeName() == "pv_id") {
            pvId = textElement->nodeValue();
            std::istringstream(pvId) >> pvIdNum;
          } else if (pvNode->nodeName() == "pv_type") {
            pvType = textElement->nodeValue();
          } else if (pvNode->nodeName() == "pv_units") {
            pvUnits = textElement->nodeValue();
          }
        }

        pvNode = pvNode->nextSibling();
      }

      // We need at least the name, id & type before we can create the property
      // (Units are optional)
      if (pvName.empty() || pvId.empty() || pvType.empty()) {
        if (pvName.empty()) {
          pvName = "<UNKNOWN>";
        }
        g_log.warning() << "Ignoring process variable " << pvName << " because it was missing required fields.\n";
      } else {
        // Check the nameMap - we may have already received a description for
        // this
        // device.  (SMS will re-send DeviceDescriptor packets under certain
        // circumstances.)
        NameMapType::const_iterator it = m_nameMap.find(std::make_pair(pkt.devId(), pvIdNum));
        if (it == m_nameMap.end()) {
          // create the property in the workspace - this is a little bit kludgy
          // because
          // the type is specified as a string in the XML, but we pass the
          // actual keyword
          // to the template declaration.  Hense all the if...else if...else
          // stuff...
          Property *prop = nullptr;
          if (pvType == "double") {
            prop = new TimeSeriesProperty<double>(pvName);
          } else if ((pvType == "integer") || (pvType == "unsigned") || (pvType == "unsigned integer") ||
                     (pvType.compare(0, 5, "enum_") == 0))
          // Note: Mantid doesn't currently support unsigned int properties
          // Note: We're treating enums as ints (at least for now)
          // Note: ADARA doesn't currently define an integer variable value
          // packet (only unsigned)
          {
            prop = new TimeSeriesProperty<int>(pvName);
          } else if (pvType == "string") {
            prop = new TimeSeriesProperty<std::string>(pvName);
          } else {
            // invalid type string
            g_log.warning() << "Ignoring process variable " << pvName << " because it had an unrecognized type ("
                            << pvType << ").\n";
          }

          if (prop) {
            if (!pvUnits.empty()) {
              prop->setUnits(pvUnits);
            }
            if (m_eventBuffer->run().hasProperty(pvName)) {
              g_log.error() << "Ignoring duplicate process variable " << pvName << " for devId=" << pkt.devId()
                            << ", pvId=" << pvIdNum << "; skipping.\n";
              delete prop;
            } else {
              m_eventBuffer->mutableRun().addLogData(prop);

              // Add the pv id, device id and pv name to the name map so we can
              // find the name when we process the variable value packets
              m_nameMap[std::make_pair(pkt.devId(), pvIdNum)] = pvName;
            }
          }
        }
      }
    }

    node = node->nextSibling();
  }

  return false;
}

/// Parse a stream annotation packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::AnnotationPkt packets.  Scan start, Scan stop, Pause
/// & Resume annotations are stored as time series properties in the
/// workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::AnnotationPkt &pkt) {
  // Check to see if we should process this packet (depending on what
  // the user selected for start up options, the SMS might be replaying
  // historical data that we don't care about).
  if (ignorePacket(pkt)) {
    return false;
  }

  switch (pkt.marker_type()) {

  case ADARA::MarkerType::GENERIC:
    // Do nothing.  We log the comment field below for all types
    break;

  case ADARA::MarkerType::SCAN_START:
    m_eventBuffer->mutableRun()
        .getTimeSeriesProperty<int>(SCAN_PROPERTY)
        ->addValue(timeFromPacket(pkt), pkt.scanIndex());
    g_log.information() << "Scan Start: " << pkt.scanIndex() << '\n';
    break;

  case ADARA::MarkerType::SCAN_STOP:
    m_eventBuffer->mutableRun().getTimeSeriesProperty<int>(SCAN_PROPERTY)->addValue(timeFromPacket(pkt), 0);
    g_log.information() << "Scan Stop:  " << pkt.scanIndex() << '\n';
    break;

  case ADARA::MarkerType::PAUSE:
    m_eventBuffer->mutableRun().getTimeSeriesProperty<int>(PAUSE_PROPERTY)->addValue(timeFromPacket(pkt), 1);
    g_log.information() << "Run paused\n";
    onRunPause(true);
    break;

  case ADARA::MarkerType::RESUME:
    m_eventBuffer->mutableRun().getTimeSeriesProperty<int>(PAUSE_PROPERTY)->addValue(timeFromPacket(pkt), 0);
    g_log.information() << "Run resumed\n";
    onRunPause(false);
    break;

  case ADARA::MarkerType::OVERALL_RUN_COMMENT:
    // Do nothing.  We log the comment field below for all types
    break;

  case ADARA::MarkerType::SYSTEM:
    // Do nothing.  We log the comment field below for all types
    break;
  }

  // if there's a comment in the packet, log it at the info level
  const std::string &comment = pkt.comment();
  if (!comment.empty()) {
    g_log.information() << "Annotation: " << comment << '\n';
  }

  return false;
}

/// Parse a Run Information packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::RunInfoPkt packets.  Specifically, it looks for the
/// proposal id and run title and stores those values in properties in the
/// workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::RunInfoPkt &pkt) {

  // RunInfoPkts are mostly just blocks of XML.
  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> doc = parser.parseString(pkt.info());
  const Poco::XML::Node *runInfoNode = doc->firstChild();

  // The root of the XML should be "runinfo".
  while (runInfoNode && runInfoNode->nodeName() != "runinfo") {
    runInfoNode = runInfoNode->nextSibling();
  }

  if (!runInfoNode) {
    g_log.error("Run info packet did not contain a 'runinfo' element!!  "
                "This should never happen!");
    return false;
  }

  // The two elements we're looking for (proposal_id and run_title) should
  // be children of the runInfoNode.  (Note that run_number is also in there,
  // but we already get that from the RunStatusPkt.)
  std::string proposalID;
  std::string runTitle;
  const Poco::XML::Node *node = runInfoNode->firstChild();
  while (node) {
    // iterate through each individual variable...
    if (node->nodeName() == "proposal_id") {
      const Poco::XML::Node *textElement = node->firstChild();
      if (textElement) {
        proposalID = textElement->nodeValue();
      }
    } else if (node->nodeName() == "run_title") {
      const Poco::XML::Node *textElement = node->firstChild();
      if (textElement) {
        runTitle = textElement->nodeValue();
      }
    }

    // If we've got everything we need, we can break out of the while loop
    if (proposalID.length() && runTitle.length()) {
      break;
    }

    node = node->nextSibling();
  }

  if (proposalID.length()) {
    Property *prop = m_eventBuffer->mutableRun().getProperty(EXPERIMENT_ID_PROPERTY);

    // Sanity check: We're likely to get multiple RunInfo packets in a
    // run, but the values shouldn't change mid-run...
    std::string prevPropVal = prop->value();
    if (prevPropVal.length() && prevPropVal != proposalID) {
      g_log.error("Proposal ID in the current run info packet has changed!  "
                  "This shouldn't happen!  (Keeping new ID value.)");
    }
    prop->setValue(proposalID);
  } else {
    g_log.warning("Run info packet did not contain a proposal ID.  "
                  "Property will be empty.");
  }

  if (runTitle.length()) {
    Property *prop = m_eventBuffer->mutableRun().getProperty(RUN_TITLE_PROPERTY);

    // Sanity check
    std::string prevPropVal = prop->value();
    if (prevPropVal.length() && prevPropVal != runTitle) {
      g_log.error("The run title in the current run info packet has changed!"
                  "  This shouldn't happen!  (Keeping new title value.)");
    }
    prop->setValue(runTitle);
  } else {
    g_log.warning("Run info packet did not contain a run title.  "
                  "Property will be empty.");
  }

  return false;
}

/// First part of the workspace initialization

/// Performs various initialization steps that can (and, in some
/// cases, must) be done prior to receiving any packets from the SMS daemon.
void SNSLiveEventDataListener::initWorkspacePart1() {
  m_eventBuffer = std::static_pointer_cast<DataObjects::EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 1, 1, 1));
  // The numbers in the create() function don't matter - they'll get overwritten
  // down in initWorkspacePart2() when we load the instrument definition.

  // We also know we'll need 3 time series properties on the workspace.  Create
  // them now. (We may end up adding values to the pause and scan properties
  // before we can call initWorkspacePart2().)
  Property *prop = new TimeSeriesProperty<int>(PAUSE_PROPERTY);
  m_eventBuffer->mutableRun().addLogData(prop);
  prop = new TimeSeriesProperty<int>(SCAN_PROPERTY);
  m_eventBuffer->mutableRun().addLogData(prop);
  prop = new TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY);
  prop->setUnits("picoCoulomb");
  m_eventBuffer->mutableRun().addLogData(prop);

  // Same for a couple of other properties (that are not time series)
  prop = new PropertyWithValue<std::string>(RUN_TITLE_PROPERTY, "");
  m_eventBuffer->mutableRun().addLogData(prop);
  prop = new PropertyWithValue<std::string>(EXPERIMENT_ID_PROPERTY, "");
  m_eventBuffer->mutableRun().addLogData(prop);
}

/// Second part of the workspace initialization

/// Finishes the workspace initialization using data from
/// various packets received from the SMS daemon
void SNSLiveEventDataListener::initWorkspacePart2() {
  // Use the LoadEmptyInstrument algorithm to create a proper workspace
  // for whatever beamline we're on
  std::shared_ptr<Algorithm> loadInst = Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
  loadInst->initialize();
  loadInst->setChild(true); // keep the workspace out of the ADS
  loadInst->setProperty("InstrumentXML", m_instrumentXML);
  loadInst->setProperty("InstrumentName", m_instrumentName);
  loadInst->setProperty("Workspace", m_eventBuffer);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(false));

  // Wrap LoadInstrument so a malformed Geometry/BeamlineInfo packet does
  // not just kill the background thread silently: produce a context-rich
  // background exception that extractData() will surface to the caller.
  // SAXParseException::what() typically yields just "SAXParseException", so
  // the InstrumentName / XML-length context here is what makes the failure
  // diagnosable in tests and production.
  try {
    loadInst->execute();
  } catch (std::exception &e) {
    // Re-throw with context: InstrumentName + XML length make the failure
    // diagnosable.  Do NOT acquire m_mutex here — the caller always holds it
    // (guaranteed by the outer lock around bufferParse() in run()), and
    // std::mutex is non-recursive.
    // The outer catch arms in run() call setFatalState() with this message.
    std::ostringstream msg;
    msg << "SNSLiveEventDataListener: LoadInstrument failed during workspace "
           "initialization (InstrumentName='"
        << m_instrumentName << "', InstrumentXML length=" << m_instrumentXML.size() << " bytes): " << e.what();
    throw std::runtime_error(msg.str());
  }

  m_requiredLogs.clear();
  // Cleared here after a successful init (entries have served their purpose)
  // and also at run boundaries — see onBeginRun() / onEndRun() — to prevent
  // stale entries from blocking readyForInitPart2() on the next run.

  auto tmp =
      createWorkspace<DataObjects::EventWorkspace>(m_eventBuffer->getInstrument()->getDetectorIDs(false).size(), 2, 1);
  WorkspaceFactory::Instance().initializeFromParent(*m_eventBuffer, *tmp, true);
  if (m_eventBuffer->getNumberHistograms() != tmp->getNumberHistograms()) {
    // need to generate the spectra to detector map
    tmp->rebuildSpectraMapping();
  }
  m_eventBuffer = std::move(tmp);

  // Set the units
  m_eventBuffer->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_eventBuffer->setYUnit("Counts");

  m_indexMap = m_eventBuffer->getDetectorIDToWorkspaceIndexMap(true /* bool throwIfMultipleDets */);

  // We always want to have at least one value for the scan index time
  // series.  We may have already gotten a scan start packet by the time we
  // get here and therefor don't need to do anything.  If not, we need to put
  // a 0 into the time series.
  if (m_eventBuffer->mutableRun().getTimeSeriesProperty<int>(SCAN_PROPERTY)->size() == 0) {
    m_eventBuffer->mutableRun().getTimeSeriesProperty<int>(SCAN_PROPERTY)->addValue(m_dataStartTime, 0);
  }

  initMonitorWorkspace();

  // Compose the workspace title from the instrument name and run number, now
  // that both are known: readyForInitPart2() guarantees m_instrumentName is
  // non-empty, and m_runNumber is already set on both the joining path
  // (rxPacket(NEW_RUN) -> setRunDetails()) and the transition path
  // (onBeginRun() -> setRunDetails(), which runs before initWorkspacePart2()
  // is reached again for that run).  Applied directly to m_eventBuffer here,
  // same as the geometry / Y-unit / monitor-workspace setup above: every
  // subsequent initializeFromParent() copy in doExtractData() propagates
  // this title forward, so no per-extraction handling is needed there.
  m_wsName = m_instrumentName + std::to_string(m_runNumber.load());
  m_eventBuffer->setTitle(m_wsName);

  m_workspaceInitialized = true;

  // If we completed initialisation while joining mid-run, queue the BeginRun
  // commit edge so the foreground advances JoiningRun -> Running on its next
  // extractData() call.  No m_pauseNetRead: the bg thread keeps reading (there
  // is no prior run's events to protect against mixing).
  if (m_adaraRunStatus == JoiningRun) {
    m_pendingTransition = BeginRun;
  }
}

/// Creates a monitor workspace sized to the number of monitors, with the
/// monitor IDs set
void SNSLiveEventDataListener::initMonitorWorkspace() {
  auto monitors = m_eventBuffer->getInstrument()->getMonitorIDs();

  // don't create monitor workspace if there are no monitors
  if (monitors.size() == 0)
    return;

  auto monitorsBuffer = WorkspaceFactory::Instance().create("EventWorkspace", monitors.size(), 1, 1);
  WorkspaceFactory::Instance().initializeFromParent(*m_eventBuffer, *monitorsBuffer, true);
  // Set the id numbers
  for (size_t i = 0; i < monitors.size(); ++i) {
    monitorsBuffer->getSpectrum(i).setDetectorID(monitors[i]);
  }

  m_monitorIndexMap = monitorsBuffer->getDetectorIDToWorkspaceIndexMap(true);

  m_eventBuffer->setMonitorWorkspace(monitorsBuffer);
}

// Check to see if we have data for all of the logs listed in m_requiredLogs.
// NOTE: This function does not lock the mutex!  The calling function must
// ensure that m_eventBuffer won't change while the function runs (either by
// locking the mutex, or by the simple fact of never calling it once the
// workspace has been initialized...)
bool SNSLiveEventDataListener::haveRequiredLogs() {
  bool allFound = true;
  const Run &experimentRun = m_eventBuffer->run();
  auto it = m_requiredLogs.begin();
  while (it != m_requiredLogs.end() && allFound) {
    if (!experimentRun.hasProperty(*it)) {
      allFound = false;
    } else if (experimentRun.getProperty(*it)->size() == 0) {
      allFound = false;
    }

    it++;
  }

  return allFound;
}

/// Adds an event to the workspace
void SNSLiveEventDataListener::appendEvent(const uint32_t pixelId, const double tof,
                                           const Mantid::Types::Core::DateAndTime pulseTime)
// NOTE: This function does NOT lock the mutex!  Make sure you do that
// before calling this function!
{
  // It'd be nice to use operator[], but we might end up inserting a value....
  // Have to use find() instead.
  const auto it = m_indexMap.find(pixelId);
  if (it != m_indexMap.end()) {
    const std::size_t workspaceIndex = it->second;
    Types::Event::TofEvent event(tof, pulseTime);
    m_eventBuffer->getSpectrum(workspaceIndex).addEventQuickly(event);
  } else {
    g_log.warning() << "Invalid pixel ID: " << pixelId << " (TofF: " << tof << " microseconds)\n";
  }
}

/// Retrieve buffered data

/// Called by the foreground thread to fetch data that's accumulated in
/// the temporary workspace.  The temporary workspace is left empty and
/// ready to receive more data.
/// @return shared pointer to a workspace containing the accumulated data
std::shared_ptr<Workspace> SNSLiveEventDataListener::doExtractData() {
  // Wait for the background thread to initialise the workspace (requires
  // geometry + beamline packets from the SMS).  Poll for up to 10 s, then
  // throw NotYet so the caller can cancel or retry.
  double startupTimeout = 10.0;
  if (const auto v = ConfigService::Instance().getValue<double>("SNSLiveEventDataListener.startupTimeout"))
    startupTimeout = std::max(0.001, *v);

  const DateAndTime endTime = DateAndTime::getCurrentTime() + startupTimeout;
  while (true) {
    bool initialized;
    std::shared_ptr<std::runtime_error> bgEx;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      initialized = m_workspaceInitialized;
      bgEx = m_backgroundException;
    }
    if (initialized)
      break;
    // Surface any fatal exception (e.g. LoadInstrument failure) so the
    // caller sees the real cause instead of waiting out the timeout.
    if (bgEx)
      throw std::runtime_error(bgEx->what());
    // Check 1: clean peer-close before workspace was ever initialised.
    if (!m_isConnected.load())
      throw std::runtime_error("SNSLiveEventDataListener: stream ended before workspace initialization.");
    if (DateAndTime::getCurrentTime() >= endTime)
      break;
    Poco::Thread::sleep(100); // 100 milliseconds
  }
  using namespace DataObjects;

  // From here through the workspace swap below, hold m_mutex as a single
  // critical section.  Everything in this region reads or mutates state that
  // the background thread also touches under the parse lock inside
  // bufferParse(): m_backgroundException, m_workspaceInitialized,
  // m_ignorePackets, m_eventBuffer (including its Run / TimeSeriesProperty
  // internals via initializeFromParent()), m_monitorLogs, and
  // m_pendingTransition.  A narrower lock here previously left several of
  // those reads unguarded.  No deadlock risk: the background thread only ever
  // acquires m_mutex inside bufferParse() and never blocks on the foreground
  // while holding it, so this simply serializes extract-vs-parse (the
  // intended back-pressure).  NOTE: this lock must NOT extend back over the
  // init-wait sleep loop above — holding it there would starve the background
  // thread that sets m_workspaceInitialized.
  EventWorkspace_sptr temp;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_backgroundException)
      throw std::runtime_error(m_backgroundException->what());
    if (!m_workspaceInitialized)
      throw Exception::NotYet("The workspace has not yet been initialized.");

    // Throw if the request was for data from the start of a run, but we're
    // not yet in a run.
    if (m_ignorePackets.load()) // This variable is (un)set in ignorePacket()
    {
      throw Exception::NotYet("Waiting for a run to start.");
    }

    // Make a brand new EventWorkspace
    temp = std::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", m_eventBuffer->getNumberHistograms(), 2, 1));

    // Copy geometry and title over.
    API::WorkspaceFactory::Instance().initializeFromParent(*m_eventBuffer, *temp, false);

    // Clear out the old logs, except for the most recent entry
    temp->mutableRun().clearOutdatedTimeSeriesLogValues();

    // Clear out old monitor logs
    for (auto &monitorLog : m_monitorLogs) {
      temp->mutableRun().removeProperty(monitorLog);
    }
    m_monitorLogs.clear();

    // Create a fresh monitor workspace and insert into the new 'main' workspace
    auto monitorBuffer = m_eventBuffer->monitorWorkspace();
    if (monitorBuffer) {
      auto newMonitorBuffer =
          WorkspaceFactory::Instance().create("EventWorkspace", monitorBuffer->getNumberHistograms(), 1, 1);
      WorkspaceFactory::Instance().initializeFromParent(*monitorBuffer, *newMonitorBuffer, false);
      temp->setMonitorWorkspace(newMonitorBuffer);
    }

    // Check 2: if the bg thread exited cleanly (clean disconnect) and there is
    // nothing left to deliver — no detector events, no monitor events, and no
    // pending run-state transition — throw so consumers don't spin on empty
    // workspaces indefinitely.  Monitor events live in the separate
    // EventWorkspace returned by m_eventBuffer->monitorWorkspace(), so
    // m_eventBuffer->getNumberEvents() alone would silently discard a final
    // batch that contains only monitor data.
    const auto monitorEventsBuffer = std::dynamic_pointer_cast<EventWorkspace>(monitorBuffer);
    const std::size_t monitorEvents = monitorEventsBuffer ? monitorEventsBuffer->getNumberEvents() : 0;
    if (!m_isConnected.load() && !m_pendingTransition && m_eventBuffer->getNumberEvents() == 0 && monitorEvents == 0) {
      throw std::runtime_error("SNSLiveEventDataListener: stream ended after server disconnect.");
    }

    // Swap the workspaces
    std::swap(m_eventBuffer, temp);
  } // mutex automatically unlocks here

  return temp;
}

// ---------------------------------------------------------------------------
// Pure state getters
// ---------------------------------------------------------------------------

ILiveListener::RunStatus SNSLiveEventDataListener::runState() const {
  std::lock_guard<std::mutex> scopedLock(m_mutex);
  if (m_backgroundException)
    throw(*m_backgroundException);
  return m_adaraRunStatus;
}

bool SNSLiveEventDataListener::isPaused() const { return m_isDasPaused.load(); }

API::ListenerState SNSLiveEventDataListener::listenerState() const {
  std::lock_guard<std::mutex> scopedLock(m_mutex);
  if (m_backgroundException)
    return API::ListenerState::Error;
  if (!m_isConnected.load())
    return API::ListenerState::Disconnected;
  if (m_pauseNetRead.load(std::memory_order_acquire))
    return API::ListenerState::ReadWait;
  return API::ListenerState::Connected;
}

std::optional<ILiveListener::RunStatus> SNSLiveEventDataListener::lastTransition() const {
  std::lock_guard<std::mutex> scopedLock(m_mutex);
  if (m_backgroundException)
    throw(*m_backgroundException);
  return m_lastTransition;
}

// ---------------------------------------------------------------------------
// onBeforeExtract / onAfterExtract — extractData() commit points
// ---------------------------------------------------------------------------
// BeginRun is dispatched from onBeforeExtract() so the new run's workspace
// initialisation is in place before doExtractData() snapshots it.  EndRun is
// dispatched from onAfterExtract() so doExtractData() can harvest the
// finishing run's accumulated events before onEndRun() resets the buffer.

void SNSLiveEventDataListener::onBeforeExtract() {
  // Throw NotYet if the background thread is currently inside bufferParse().
  // m_bgThreadCaughtUp is false only during bufferParse() (it is true while
  // the bg thread is in receiveBytes(), in the pause loop, or between
  // iterations).  An immediate load avoids a 500 ms wait.
  // NoNetworkTest fixtures run without a bg thread, so m_thread.isRunning()
  // short-circuits the check for them.
  if (m_thread.isRunning() && !m_bgThreadCaughtUp.load(std::memory_order_acquire)) {
    throw Exception::NotYet("Background thread parse is in flight; snapshot would be unsafe.");
  }

  std::optional<RunStatus> pending;
  {
    std::lock_guard<std::mutex> scopedLock(m_mutex);
    // Re-check under the lock: between the fast-path above and acquiring
    // m_mutex the bg thread may have flipped m_bgThreadCaughtUp false and
    // entered bufferParse().  rxPacket() holds m_mutex while mutating
    // m_pendingTransition, so re-reading the flag here (while we hold the
    // lock) closes the TOCTOU window — if it is now false, bufferParse() is
    // in flight and the snapshot would be unsafe.  m_memory_order_acquire is
    // still required because the bg stores false outside m_mutex.
    if (m_thread.isRunning() && !m_bgThreadCaughtUp.load(std::memory_order_acquire)) {
      throw Exception::NotYet("Background thread parse is in flight; snapshot would be unsafe.");
    }
    // Both NotYet gates have passed: the prior extract's edge has been
    // delivered to the caller.  Clear it now so MonitorLiveData sees nullopt
    // on the *next* tick rather than re-processing a stale edge.
    // m_previousExtractCompleted is false during a NotYet retry (onAfterExtract
    // was never reached), so the edge survives across retries (C1 invariant).
    if (m_previousExtractCompleted) {
      m_lastTransition.reset();
      m_previousExtractCompleted = false;
    }
    pending = m_pendingTransition;
  }
  if (pending && *pending == BeginRun) {
    {
      std::lock_guard<std::mutex> scopedLock(m_mutex);
      m_pendingTransition.reset();
    }
    onBeginRun();
    {
      std::lock_guard<std::mutex> scopedLock(m_mutex);
      m_lastTransition = BeginRun;
    }
    m_pauseNetRead.store(false, std::memory_order_release);
  }
}

void SNSLiveEventDataListener::onAfterExtract() {
  std::optional<RunStatus> pending;
  {
    std::lock_guard<std::mutex> scopedLock(m_mutex);
    pending = m_pendingTransition;
    // m_lastTransition is NOT cleared here: clearing is deferred to the next
    // call's onBeforeExtract() after both NotYet gates pass (guarded by
    // m_previousExtractCompleted).  This makes BeginRun and EndRun symmetric:
    // both are observable via lastTransition() after the extractData() that
    // committed them returns.
  }
  if (pending && *pending == EndRun) {
    {
      std::lock_guard<std::mutex> scopedLock(m_mutex);
      m_pendingTransition.reset();
      m_lastTransition = EndRun;
    }
    onEndRun();
    m_pauseNetRead.store(false, std::memory_order_release);
  }
  {
    std::lock_guard<std::mutex> scopedLock(m_mutex);
    m_previousExtractCompleted = true;
  }
}

// ---------------------------------------------------------------------------
// Run-state transition hooks
// ---------------------------------------------------------------------------
// Hooks acquire m_mutex themselves.  onBeginRun() is called from
// onBeforeExtract(); onEndRun() is called from onAfterExtract().

void SNSLiveEventDataListener::onBeginRun() {
  std::lock_guard<std::mutex> scopedLock(m_mutex);

  if (m_adaraRunStatus == JoiningRun) {
    // Joining completion path: workspace initialisation finished while in
    // JoiningRun state.  Run details (run_number, run_start) were already
    // applied by rxPacket(NEW_RUN) when we entered JoiningRun; the init caches
    // (m_instrumentXML etc.) are exactly what we need — do not reset them.
    // Simply advance to Running.
    m_adaraRunStatus = Running;
    return;
  }

  // Transition path: NEW_RUN arrived while the previous workspace was live.
  // Reset workspace initialisation so that new geometry and device-descriptor
  // packets will be processed.
  m_workspaceInitialized = false;

  // Clear the caches that depend on instrument configuration.
  // Note: m_dataStartTime is NOT cleared here; it was set by rxPacket(RunStatusPkt)
  // for the NEW_RUN packet and already contains the correct value.
  m_instrumentXML.clear();
  m_instrumentName.clear();
  m_nameMap.clear();
  m_requiredLogs.clear(); // stale entries from previous geometry would block readyForInitPart2()
  m_wsName.clear();       // recomposed by initWorkspacePart2() once the new run's
                          // instrument name and run number are both known again

  initWorkspacePart1();

  if (!m_deferredRunDetailsPkt) {
    // Invariant: rxPacket(NEW_RUN) must have stashed the RunStatusPkt before
    // queuing a BeginRun transition on the transition path.  Reaching here
    // means the producer side has a bug.
    throw std::runtime_error("SNSLiveEventDataListener::onBeginRun(): "
                             "m_deferredRunDetailsPkt is null — invariant violation.");
  }
  setRunDetails(*m_deferredRunDetailsPkt);
  m_deferredRunDetailsPkt.reset();

  m_adaraRunStatus = Running;
}

void SNSLiveEventDataListener::onEndRun() {
  std::lock_guard<std::mutex> scopedLock(m_mutex);

  m_workspaceInitialized = false;

  m_instrumentXML.clear();
  m_instrumentName.clear();
  m_dataStartTime = Types::Core::DateAndTime(); // cleared on EndRun only
  m_nameMap.clear();
  m_requiredLogs.clear(); // stale entries from previous geometry would block readyForInitPart2()
  m_wsName.clear();       // recomposed by initWorkspacePart2() once the next run's
                          // instrument name and run number are both known

  initWorkspacePart1();

  // Re-arm the first-extract synchronisation gate for the next run, mirroring
  // the construction-time initial value.  Without this reset, the flag would
  // remain sticky-true across the boundary and the first extractData() of the
  // next run would bypass the "wait for first bufferParse" guarantee.
  m_bgThreadCaughtUp.store(false, std::memory_order_release);

  m_adaraRunStatus = NoRun;
}

void SNSLiveEventDataListener::onRunPause(bool paused) {
  // Pause state is orthogonal to run state: m_adaraRunStatus remains Running
  // while the DAS is paused.  m_isDasPaused is read by isPaused() and by
  // rxPacket(BankedEventPkt) to gate event appending.
  // Called from rxPacket(AnnotationPkt) which already holds m_mutex.
  m_isDasPaused.store(paused);
}

// Called by the rxPacket() functions to determine if the packet should be processed
// (Depending on when it last indexed its data, SNS might send us packets that are older than we requested)
// Returns false if the packet should be processed, true if it should be ignored
bool SNSLiveEventDataListener::ignorePacket(const ADARA::PacketHeader &hdr, const ADARA::RunStatus::Enum status) {
  // Since we're filtering based on time (either the absolute timestamp or nothing
  // before the start of the most recent run),
  // once we've determined a given packet should be processed,
  // we know all packets after that should also be processed.
  // Thus, we can reduce most calls to this function to a simple boolean test
  if (!m_ignorePackets) // don't ignore
    return false;

  // Are we looking for the start of the run?
  if (m_filterUntilRunStart) {
    if (hdr.base_type() == ADARA::PacketType::Type::RUN_STATUS_TYPE && status == ADARA::RunStatus::NEW_RUN) {
      // A new run is starting...
      m_ignorePackets = false;
    }
  } else // Filter based solely on time
  {
    if (timeFromPacket(hdr) >= m_startTime) {
      m_ignorePackets = false;
    }
  }

  // If we've just hit our start-up condition, then process
  // all the variable value packets we've been hanging on to.
  if (!m_ignorePackets) { // don't ignore
    replayVariableCache();
  }

  return m_ignorePackets;
}

// Process all the variable value packets stored in m_variableMap
void SNSLiveEventDataListener::replayVariableCache() {
  for (const auto &varPacketPair : m_variableMap) {
    rxPacket(*varPacketPair.second); // call rxPacket() on the stored packet
  }
  m_variableMap.clear(); // empty the map to save a little ram
}

} // namespace Mantid::LiveData
