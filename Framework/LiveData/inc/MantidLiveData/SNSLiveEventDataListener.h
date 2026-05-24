// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/LiveListener.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidLiveData/ADARA/ADARAParser.h"
#include "MantidLiveData/DllConfig.h"

#include <Poco/Net/StreamSocket.h>
#include <Poco/Runnable.h>
#include <Poco/Timer.h>
#include <atomic>
#include <optional>
#include <set>

namespace Mantid {
namespace LiveData {

/** An implementation of ILiveListener for use at SNS.
 * Connects to the Stream Management Service and receives events from it.
 */
class MANTID_LIVEDATA_DLL SNSLiveEventDataListener : public API::LiveListener,
                                                     public Poco::Runnable,
                                                     public ADARA::Parser {
public:
  SNSLiveEventDataListener();
  ~SNSLiveEventDataListener() override;

  std::string name() const override { return "SNSLiveEventDataListener"; }
  bool supportsHistory() const override { return true; }
  bool buffersEvents() const override { return true; }

  bool connect(const Poco::Net::SocketAddress &address) override;
  void start(const Types::Core::DateAndTime startTime = Types::Core::DateAndTime()) override;
  std::shared_ptr<API::Workspace> doExtractData() override;

  RunStatus runState() const override;
  bool isPaused() const override;
  API::ListenerState listenerState() const override;
  std::optional<RunStatus> lastTransition() const override;

  int runNumber() const override { return m_runNumber; };

  bool isConnected() override;

  void run() override; // the background thread.  What gets executed when we
                       // call POCO::Thread::start()
protected:
  using ADARA::Parser::rxPacket;
  // virtual bool rxPacket( const ADARA::Packet &pkt);

  /// Called from LiveListener::extractData() before doExtractData().
  /// Dequeues any pending run-state transition and dispatches to onBeginRun()
  /// or onEndRun() as appropriate.
  void onBeforeExtract() override;

  /// Called from LiveListener::extractData() after doExtractData() returns
  /// normally. Clears the committed transition edge after a successful
  /// workspace hand-off.
  void onAfterExtract() override;

  /// Called from onBeforeExtract() when a BeginRun transition is dequeued.
  /// Acquires m_mutex itself.
  ///
  /// Two paths depending on m_adaraRunStatus at entry:
  ///   - JoiningRun: workspace was bootstrapped while joining mid-run; run
  ///     details were already applied in rxPacket(NEW_RUN).  Simply advances
  ///     to Running without resetting any workspace state.
  ///   - Otherwise (NoRun): transition path; resets workspace-initialisation
  ///     state, applies run details from the deferred RunStatusPkt, and
  ///     advances to Running.  Throws std::runtime_error if
  ///     m_deferredRunDetailsPkt is null (invariant violation in the producer).
  virtual void onBeginRun();

  /// Called from onBeforeExtract() when an EndRun transition is dequeued.
  /// Acquires m_mutex itself.  Resets workspace-initialisation state.
  virtual void onEndRun();

  /// Called from rxPacket(AnnotationPkt) under m_mutex when a PAUSE or RESUME
  /// annotation is received.  Sets m_isDasPaused without touching m_adaraRunStatus.
  /// @param paused  true for PAUSE, false for RESUME.
  virtual void onRunPause(bool paused);
  bool rxPacket(const ADARA::AnnotationPkt &pkt) override;
  bool rxPacket(const ADARA::BankedEventPkt &pkt) override;
  bool rxPacket(const ADARA::BeamlineInfoPkt &pkt) override;
  bool rxPacket(const ADARA::BeamMonitorPkt &pkt) override;
  bool rxPacket(const ADARA::DeviceDescriptorPkt &pkt) override;
  bool rxPacket(const ADARA::GeometryPkt &pkt) override;
  bool rxPacket(const ADARA::RunInfoPkt &pkt) override;
  bool rxPacket(const ADARA::RunStatusPkt &pkt) override;
  bool rxPacket(const ADARA::VariableU32Pkt &pkt) override;
  bool rxPacket(const ADARA::VariableDoublePkt &pkt) override;
  bool rxPacket(const ADARA::VariableStringPkt &pkt) override;

private:
  // Workspace initialization needs to happen in 2 steps.  Part 1 must happen
  // before we receive *any* packets.
  void initWorkspacePart1();

  // We need data from both the geometry packet and the run status packet in
  // order to run the second part of the initialization.  Since I don't know
  // what order the packets will arrive in, I've put the necessary code in
  // this function.  Both rxPacket() functions will check to see if all the
  // data is available and call this function if it is.
  void initWorkspacePart2();

  void initMonitorWorkspace();

  // Check to see if all the conditions we need for initWorkspacePart2() have
  // been met.  Making this a function because it's starting to get a little
  // complicated and I didn't want to be repeating the same tests in several
  // places...
  bool readyForInitPart2() {
    if (m_instrumentXML.empty())
      return false;
    if (m_instrumentName.empty())
      return false;
    if (m_dataStartTime == Types::Core::DateAndTime())
      return false;

    return haveRequiredLogs();
  }

  // Returns true if we've got a value for every log listed in m_requiredLogs
  bool haveRequiredLogs();

  void appendEvent(const uint32_t pixelId, const double tof, const Mantid::Types::Core::DateAndTime pulseTime);
  // tof is "Time Of Flight" and is in units of microsecondss relative to the
  // start of the pulse
  // (There's some documentation that says nanoseconds, but Russell Taylor
  // assures me it's really is microseconds!)
  // pulseTime is the start of the pulse relative to Jan 1, 1990.
  // Both values are designed to be passed straight into the TofEvent
  // constructor.

  int m_runNumber{0};
  DataObjects::EventWorkspace_sptr m_eventBuffer;
  ///< Used to buffer events between calls to extractData()

  std::string m_wsName;
  detid2index_map m_indexMap;        // maps pixel id's to workspace indexes
  detid2index_map m_monitorIndexMap; // Same as above for the monitor workspace

  // Names of any monitor logs (these must be manually removed during the call
  // to extractData())
  std::vector<std::string> m_monitorLogs;

  Poco::Net::StreamSocket m_socket;
  bool m_isConnected{false};

  Poco::Thread m_thread;
  /// Background thread checks this periodically. If true, the thread exits.
  /// Atomic because the bg loop reads it without holding m_mutex while the
  /// destructor writes it; same rationale as m_pauseNetRead.
  std::atomic<bool> m_stopThread{false};

  Types::Core::DateAndTime m_startTime; // The requested start time for the data
                                        // stream (needed by the run() function)

  // Used to initialize the scan_index property if we haven't received a packet
  // with the 'real' value by the time we call initWorkspacePart2.  (We can't
  // delay the call to initWorkspacePart2 because we might never receive a
  // 'real' value for that property.
  Types::Core::DateAndTime m_dataStartTime;

  bool m_keepPausedEvents{false}; // Set from a configuration property

  // --- Data structures necessary for handling all the process variable info
  // ---

  // maps a pair<device id, variable id> to its Processing Variable's name
  // (variable names are unique, so we don't need to worry about device names.)
  using NameMapType = std::map<std::pair<unsigned int, unsigned int>, std::string>;
  NameMapType m_nameMap;

  // ---------------------------------------------------------------------------

  // In cases where we're replaying historical data from the SMS, we're likely
  // to get multiple value packets for various values, but we only want to
  // process the most recent one.  Unfortunately, the only way to do this is to
  // hold the packets in a cache until the SMS works its way through the older
  // data and starts sending out the data we actual want.  At that point, we
  // need to parse whatever variable value packets we have in order to set the
  // state of the system properly.

  // Maps the pair<device ID, variable ID> to the actual packet. Using a map
  // means we will only keep one packet (the most recent one) for each variable
  using VariableMapType = std::map<std::pair<unsigned int, unsigned int>, std::shared_ptr<ADARA::Packet>>;
  VariableMapType m_variableMap;

  /// Process all the variable value packets stored in m_variableMap and then clear this cache
  ///
  /// The method is triggered through the packet processing chain:
  /// - rxPacket() → ignorePacket() → replayVariableCache()
  void replayVariableCache();

  // ---------------------------------------------------------------------------
  bool m_ignorePackets{false}; // used by filterPacket() below...
  bool m_filterUntilRunStart{false};

  // Called by the rxPacket() functions to determine if the packet should be
  // processed. (Depending on when it last indexed its data, SMS might send us
  // packets that are older than we requested.)
  // Returns false if the packet should be processed, true if is should be
  // ignored
  bool ignorePacket(const ADARA::PacketHeader &hdr, const ADARA::RunStatus::Enum status = ADARA::RunStatus::NO_RUN);
  void setRunDetails(const ADARA::RunStatusPkt &pkt);

  // We have to defer calling setRunDetails() at the start of a run until the
  // foreground thread has called extractData() and retrieved the last data
  // from the previous state (which was probably NO_RUN).
  // This holds a copy of the RunStatusPkt until we can call setRunDetails().
  std::shared_ptr<ADARA::RunStatusPkt> m_deferredRunDetailsPkt;

  /// list of monitors that were seen on the stream but are not in the IDF
  std::set<detid_t> m_badMonitors;

protected:
  // ---------------------------------------------------------------------------
  // Fields in protected: so that test subclasses can inject state without
  // requiring friend declarations (which cannot name a class defined in a test
  // header).  All fields here are guarded by m_mutex unless otherwise noted.
  // ---------------------------------------------------------------------------
  /// Protects m_eventBuffer, m_adaraRunStatus, m_pendingTransition,
  /// m_lastTransition, m_isDasPaused, m_instrumentXML, m_instrumentName,
  /// m_nameMap, m_requiredLogs, m_deferredRunDetailsPkt, and m_workspaceInitialized.
  /// Does NOT protect m_pauseNetRead, m_stopThread, or m_bgThreadCaughtUp —
  /// those are std::atomic<bool> and are accessed lock-free.
  mutable std::mutex m_mutex;
  /// Back-pressure flag set true by rxPacket(RunStatusPkt) on NEW_RUN/END_RUN
  /// to halt the bg-thread read loop until the foreground calls extractData().
  /// Atomic because the bg loop reads it without holding m_mutex (see run()).
  /// Acquire/release ordering pairs with the bg loop's read in run(), matching
  /// the pattern already used for m_bgThreadCaughtUp. m_mutex does NOT
  /// protect this field.
  std::atomic<bool> m_pauseNetRead{false};

  // These strings are needed to initialize m_eventBuffer.  Placed in
  // protected: so test subclasses can inject instrument data directly.
  std::string m_instrumentName;
  std::string m_instrumentXML;
  /// Logfile IDs required before readyForInitPart2() returns true.
  /// Populated by rxPacket(GeometryPkt); cleared by initWorkspacePart2() after
  /// a successful init and by onBeginRun()/onEndRun() at run boundaries to
  /// prevent stale entries from blocking the next run.
  std::vector<std::string> m_requiredLogs;
  /// False iff the background thread is currently inside bufferParse() (i.e. a
  /// parse iteration is in flight and rxPacket() calls may be mutating shared
  /// state).  The bg thread stores false at the start of bufferParse() and
  /// true at the end; receiveBytes() does NOT flip it, so a bg thread blocked
  /// in receiveBytes() (e.g. at a test gate) presents as caught-up.
  /// onBeforeExtract() throws NotYet immediately if this is false; the
  /// bg-thread pause loop gates on this flag (short-circuit) so it only
  /// honours m_pauseNetRead when no parse is in flight.
  /// onEndRun() resets the flag to false so that the first extractData() of
  /// each subsequent run is gated on that run's first bufferParse(), exactly
  /// as the construction-time initial value gates the very first run.
  std::atomic<bool> m_bgThreadCaughtUp{false};

  /// Stable, committed DAS run state read by runState().  Only ever takes
  /// the values {NoRun, JoiningRun, Running} — never BeginRun or EndRun.
  /// The BeginRun/EndRun *edges* are delivered to consumers via
  /// m_lastTransition (read by lastTransition()) and the legacy
  /// runStatus() shim, not via this field.  Written from rxPacket(NEW_RUN)
  /// on the joining path, and from the onBeginRun() / onEndRun() hooks
  /// dispatched by onBeforeExtract() / onAfterExtract().
  ILiveListener::RunStatus m_adaraRunStatus{RunStatus::NoRun};
  std::optional<RunStatus> m_pendingTransition;
  /// The run-state edge committed by the most recent successful extractData().
  /// Set in onBeforeExtract() for BeginRun, in onAfterExtract() for EndRun.
  /// Cleared at the start of the next onBeforeExtract() after both NotYet
  /// gates pass, guarded by m_previousExtractCompleted (C1 fix: a NotYet
  /// retry leaves m_previousExtractCompleted false, so the edge survives).
  /// BeginRun and EndRun are symmetric: both are observable to the caller
  /// (e.g. MonitorLiveData) between the extractData() that committed them
  /// and the next successful extractData() call.
  std::optional<RunStatus> m_lastTransition;
  /// Set true at the end of onAfterExtract(); consumed (cleared) at the start
  /// of the next onBeforeExtract() after both NotYet gates pass.  Gates the
  /// deferred m_lastTransition clear so the edge survives NotYet retries.
  bool m_previousExtractCompleted{false};

  bool m_workspaceInitialized{false};

  // These 2 determine whether or not we filter out events that arrive when
  // the run is paused.
  bool m_isDasPaused{false}; // Set to true or false when we receive a
                             // pause/resume marker in an annotation packet. (See
                             // rxPacket( const ADARA::AnnotationPkt &pkt))

  // Holds on to any exceptions that were thrown in the background thread so
  // that we can re-throw them in the foreground thread
  std::shared_ptr<std::runtime_error> m_backgroundException;
};

} // namespace LiveData
} // namespace Mantid
