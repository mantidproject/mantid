// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "ADARAPacketBuilders.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidLiveData/SNSLiveEventDataListener.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::LiveData;

// ---------------------------------------------------------------------------
// Test subclass — accesses protected fields and hooks without a live SMS
// connection.  Fields used here (m_mutex, m_pendingTransition, etc.) are
// protected in SNSLiveEventDataListener so no friend declaration is needed.
// ---------------------------------------------------------------------------
class TestableSNSListener : public SNSLiveEventDataListener {
public:
  int beginRunCount{0};
  int endRunCount{0};
  int pauseCount{0};
  bool lastPauseArg{false};

  /// When true the overridden hooks just increment counters without calling
  /// the base implementation.  Set this before tests that would throw from
  /// onBeginRun() due to a missing m_deferredRunDetailsPkt.
  bool m_stubHooks{false};

  // --- Helpers to inject protected base-class state -------------------------

  void injectPendingTransition(ILiveListener::RunStatus r) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pendingTransition.has_value())
      throw std::runtime_error("TestableSNSListener::injectPendingTransition: slot already occupied");
    m_pendingTransition = r;
  }

  void setWorkspaceInitialized(bool b) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_workspaceInitialized = b;
  }

  void setAdaraRunStatus(ILiveListener::RunStatus s) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_adaraRunStatus = s;
  }

  void setInstrumentData(const std::string &xml, const std::string &name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_instrumentXML = xml;
    m_instrumentName = name;
  }

  std::string readInstrumentXML() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_instrumentXML;
  }

  std::string readInstrumentName() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_instrumentName;
  }

  std::optional<ILiveListener::RunStatus> readPendingTransition() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingTransition;
  }

  bool readPauseNetRead() const {
    // m_pauseNetRead is std::atomic<bool>; load with the same acquire
    // semantics the bg loop uses (run() in SNSLiveEventDataListener.cpp).
    return m_pauseNetRead.load(std::memory_order_acquire);
  }

  /// Feed raw ADARA packet bytes through the inherited Parser::bufferParse()
  /// without a network connection.  Used to drive production rxPacket()
  /// overloads from unit tests.
  int callBufferParse(const std::vector<uint8_t> &bytes) {
    if (static_cast<unsigned int>(bytes.size()) > bufferFillLength())
      throw std::runtime_error("TestableSNSListener::callBufferParse: packet larger than parser buffer");
    std::copy(bytes.begin(), bytes.end(), bufferFillAddress());
    bufferBytesAppended(static_cast<unsigned int>(bytes.size()));
    std::string logInfo;
    return bufferParse(logInfo);
  }

  void injectBackgroundException(const std::string &msg) {
    m_backgroundException = std::make_shared<std::runtime_error>(msg);
  }

  void callOnBeginRun() { onBeginRun(); }
  void callOnEndRun() { onEndRun(); }
  void callOnRunPause(bool p) { onRunPause(p); }
  void callOnBeforeExtract() { onBeforeExtract(); }
  void callOnAfterExtract() { onAfterExtract(); }

  ILiveListener::RunStatus readAdaraRunStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_adaraRunStatus;
  }

  void seedRequiredLog(const std::string &id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requiredLogs.push_back(id);
  }
  std::vector<std::string> readRequiredLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requiredLogs;
  }

protected:
  void onBeginRun() override {
    ++beginRunCount;
    if (!m_stubHooks)
      SNSLiveEventDataListener::onBeginRun();
  }
  void onEndRun() override {
    ++endRunCount;
    if (!m_stubHooks)
      SNSLiveEventDataListener::onEndRun();
  }
  void onRunPause(bool p) override {
    ++pauseCount;
    lastPauseArg = p;
    SNSLiveEventDataListener::onRunPause(p);
  }
};

// ---------------------------------------------------------------------------

/**
 * Unit tests for SNSLiveEventDataListener that do not require a live SMS
 * network connection.
 *
 * Tests that require a running SMS server live in SNSLiveEventDataListenerTest.h.
 */
class SNSLiveEventDataListenerNoNetworkTest : public CxxTest::TestSuite {
public:
  static SNSLiveEventDataListenerNoNetworkTest *createSuite() { return new SNSLiveEventDataListenerNoNetworkTest(); }
  static void destroySuite(SNSLiveEventDataListenerNoNetworkTest *suite) { delete suite; }

  // -------------------------------------------------------------------------
  // Sub-spec 05 regression: field rename did not invert pause flag
  // -------------------------------------------------------------------------

  /** Verify that the m_isDasPaused rename did not invert the pause flag.
   *  A freshly created listener (without connecting) must report "not paused".
   */
  void test_field_rename_does_not_break_pause_handling() {
    Mantid::Kernel::LiveListenerInfo info("SNSLiveEventDataListener");
    auto listener = LiveListenerFactory::Instance().create(info, /*connect=*/false);
    TS_ASSERT(listener);
    TS_ASSERT(!listener->isPaused());
  }

  // -------------------------------------------------------------------------
  // Sub-spec 06 regression: hooks callable and correctly guarded
  // -------------------------------------------------------------------------

  /** onBeginRun() on the transition path (m_adaraRunStatus==NoRun) must throw
   *  when m_deferredRunDetailsPkt is null — invariant: rxPacket(NEW_RUN) on
   *  the transition path must always stash the RunStatusPkt before queuing.
   */
  void test_onBeginRun_throws_when_deferred_run_details_missing() {
    TestableSNSListener listener;
    // A freshly-constructed listener has m_adaraRunStatus==NoRun and no deferred
    // pkt → transition path → must throw.
    TS_ASSERT_THROWS(listener.callOnBeginRun(), const std::runtime_error &);
    // The override counted the call before delegating to super.
    TS_ASSERT_EQUALS(1, listener.beginRunCount);
  }

  /** onRunPause() must be dispatched for PAUSE and RESUME without
   *  affecting any other observable state (the test checks it is callable
   *  and toggles correctly).
   */
  void test_onRunPause_is_callable_and_toggles() {
    TestableSNSListener listener;
    TS_ASSERT_THROWS_NOTHING(listener.callOnRunPause(true));
    TS_ASSERT_EQUALS(1, listener.pauseCount);
    TS_ASSERT_EQUALS(true, listener.lastPauseArg);

    TS_ASSERT_THROWS_NOTHING(listener.callOnRunPause(false));
    TS_ASSERT_EQUALS(2, listener.pauseCount);
    TS_ASSERT_EQUALS(false, listener.lastPauseArg);
  }

  // -------------------------------------------------------------------------
  // Sub-spec 07: pure-getter tests
  // -------------------------------------------------------------------------

  /** runState() is a pure getter: calling it 100× must not change any
   *  observable state and must always return the initial value (NoRun).
   */
  void test_runState_pure_getter_does_not_mutate() {
    TestableSNSListener listener;
    for (int i = 0; i < 100; ++i) {
      TS_ASSERT_EQUALS(ILiveListener::NoRun, listener.runState());
    }
    // Counters untouched: no hook fired.
    TS_ASSERT_EQUALS(0, listener.beginRunCount);
    TS_ASSERT_EQUALS(0, listener.endRunCount);
  }

  /** A freshly constructed (not connected) listener should report Disconnected. */
  void test_listenerState_initially_disconnected() {
    TestableSNSListener listener;
    TS_ASSERT_EQUALS(ListenerState::Disconnected, listener.listenerState());
  }

  /** lastTransition() must be nullopt before any extractData() call. */
  void test_lastTransition_initially_null() {
    TestableSNSListener listener;
    TS_ASSERT(!listener.lastTransition().has_value());
  }

  /** isPaused() must be false on a freshly-constructed listener. */
  void test_isPaused_initially_false() {
    TestableSNSListener listener;
    TS_ASSERT(!listener.isPaused());
  }

  /** isPaused() is orthogonal to runState(): onRunPause toggles isPaused() but
   *  must not alter m_adaraRunStatus.
   */
  void test_isPaused_orthogonal_to_runState() {
    TestableSNSListener listener;
    TS_ASSERT_EQUALS(ILiveListener::NoRun, listener.runState());
    TS_ASSERT(!listener.isPaused());

    listener.callOnRunPause(true);
    TS_ASSERT_EQUALS(ILiveListener::NoRun, listener.runState()); // unchanged
    TS_ASSERT(listener.isPaused());

    listener.callOnRunPause(false);
    TS_ASSERT_EQUALS(ILiveListener::NoRun, listener.runState()); // still unchanged
    TS_ASSERT(!listener.isPaused());
  }

  // -------------------------------------------------------------------------
  // Sub-spec 07: onBeforeExtract / onAfterExtract dispatch tests
  // -------------------------------------------------------------------------

  /** onBeforeExtract() with a queued BeginRun must dispatch exactly once and
   *  set lastTransition() == BeginRun.
   */
  void test_onBeforeExtract_dispatches_BeginRun_to_hook() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;
    listener.injectPendingTransition(ILiveListener::BeginRun);

    TS_ASSERT_THROWS_NOTHING(listener.callOnBeforeExtract());
    TS_ASSERT_EQUALS(1, listener.beginRunCount);
    TS_ASSERT_EQUALS(0, listener.endRunCount);
    TS_ASSERT(listener.lastTransition().has_value());
    TS_ASSERT_EQUALS(ILiveListener::BeginRun, *listener.lastTransition());
  }

  /** onAfterExtract() with a queued EndRun must dispatch exactly once and
   *  set lastTransition() == EndRun.  onBeforeExtract() must not dispatch
   *  EndRun (EndRun is deferred so doExtractData() can harvest the buffer).
   */
  void test_onAfterExtract_dispatches_EndRun_to_hook() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;
    listener.injectPendingTransition(ILiveListener::EndRun);

    // onBeforeExtract must NOT dispatch EndRun.
    TS_ASSERT_THROWS_NOTHING(listener.callOnBeforeExtract());
    TS_ASSERT_EQUALS(0, listener.beginRunCount);
    TS_ASSERT_EQUALS(0, listener.endRunCount);
    TS_ASSERT(!listener.lastTransition().has_value());

    // onAfterExtract dispatches EndRun and commits lastTransition.
    TS_ASSERT_THROWS_NOTHING(listener.callOnAfterExtract());
    TS_ASSERT_EQUALS(0, listener.beginRunCount);
    TS_ASSERT_EQUALS(1, listener.endRunCount);
    TS_ASSERT(listener.lastTransition().has_value());
    TS_ASSERT_EQUALS(ILiveListener::EndRun, *listener.lastTransition());
  }

  /** onBeforeExtract() with no pending transition must not dispatch any hook
   *  and must leave lastTransition() in its prior state.
   */
  void test_no_transition_no_hook() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;
    // Pre-condition: no pending transition, lastTransition stays null.
    TS_ASSERT_THROWS_NOTHING(listener.callOnBeforeExtract());
    TS_ASSERT_EQUALS(0, listener.beginRunCount);
    TS_ASSERT_EQUALS(0, listener.endRunCount);
    TS_ASSERT(!listener.lastTransition().has_value());
  }

  // -------------------------------------------------------------------------
  // Sub-spec 07: lastTransition lifecycle
  // -------------------------------------------------------------------------

  /** After a BeginRun transition is committed via onBeforeExtract(), a second
   *  onBeforeExtract() with no pending must leave lastTransition() as BeginRun
   *  (C1 fix: edge survives across NotYet retries).
   */
  void test_lastTransition_survives_NotYet_retry() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;

    // Commit BeginRun.
    listener.injectPendingTransition(ILiveListener::BeginRun);
    listener.callOnBeforeExtract();
    TS_ASSERT_EQUALS(ILiveListener::BeginRun, *listener.lastTransition());

    // Simulate NotYet retry: onBeforeExtract() again with no pending and no
    // successful extract — edge must survive (C1 fix).
    listener.callOnBeforeExtract();
    TS_ASSERT(listener.lastTransition().has_value());
    TS_ASSERT_EQUALS(ILiveListener::BeginRun, *listener.lastTransition());
  }

  /** BeginRun edge must be observable via lastTransition() after the full
   *  extractData() cycle that committed it (onBeforeExtract + onAfterExtract).
   *  It is cleared on the *next* successful cycle, not the same one.
   */
  void test_lastTransition_BeginRun_observable_after_successful_extract() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;

    // Full extract cycle: BeginRun committed in onBeforeExtract; onAfterExtract
    // must NOT erase it before the caller can observe it.
    listener.injectPendingTransition(ILiveListener::BeginRun);
    listener.callOnBeforeExtract();
    listener.callOnAfterExtract();
    TS_ASSERT(listener.lastTransition().has_value());
    TS_ASSERT_EQUALS(ILiveListener::BeginRun, *listener.lastTransition());

    // Second full cycle with no pending: edge is now cleared.
    listener.callOnBeforeExtract();
    listener.callOnAfterExtract();
    TS_ASSERT(!listener.lastTransition().has_value());
  }

  /** Cleared-after-success: BeginRun committed in call N, observable after
   *  call N, gone after the next successful call (call N+1).
   */
  void test_lastTransition_cleared_after_successful_extract() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;

    listener.injectPendingTransition(ILiveListener::BeginRun);
    listener.callOnBeforeExtract();
    listener.callOnAfterExtract();
    TS_ASSERT_EQUALS(ILiveListener::BeginRun, *listener.lastTransition());

    // Next successful cycle (no pending): edge must be gone.
    listener.callOnBeforeExtract();
    listener.callOnAfterExtract();
    TS_ASSERT(!listener.lastTransition().has_value());
  }

  /** Symmetric test for EndRun: edge is observable after the successful
   *  extract that committed it and gone after the following one.
   */
  void test_lastTransition_reports_EndRun_then_null_after_success() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;

    listener.injectPendingTransition(ILiveListener::EndRun);

    // onBeforeExtract() does not dispatch EndRun — lastTransition() stays null.
    listener.callOnBeforeExtract();
    TS_ASSERT(!listener.lastTransition().has_value());

    // onAfterExtract() commits EndRun; edge observable after this call returns.
    listener.callOnAfterExtract();
    TS_ASSERT(listener.lastTransition().has_value());
    TS_ASSERT_EQUALS(ILiveListener::EndRun, *listener.lastTransition());

    // Next successful extract (no pending): edge cleared.
    listener.callOnBeforeExtract();
    listener.callOnAfterExtract();
    TS_ASSERT(!listener.lastTransition().has_value());
  }

  // -------------------------------------------------------------------------
  // Sub-spec 07: legacy runStatus() shim
  // -------------------------------------------------------------------------

  /** The deprecated runStatus() shim must return the edge when lastTransition()
   *  is set, then fall back to runState() once the edge is cleared.
   */
  void test_legacy_runStatus_returns_edge_then_state() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;

    // Inject a BeginRun transition and commit it.
    listener.injectPendingTransition(ILiveListener::BeginRun);
    listener.callOnBeforeExtract();

    // runStatus() should report the edge.
    TS_ASSERT_EQUALS(ILiveListener::BeginRun, listener.runStatus());

    // Simulate success: the post-extract hook marks the edge as delivered.
    listener.callOnAfterExtract();

    // Under the new design, BeginRun persists until the *next* onBeforeExtract()
    // observes m_previousExtractCompleted == true and performs the deferred clear.
    listener.callOnBeforeExtract();

    // Now runStatus() must fall back to runState().
    // onBeginRun() was stubbed, so m_adaraRunStatus is still NoRun.
    TS_ASSERT_EQUALS(ILiveListener::NoRun, listener.runStatus());
  }

  // -------------------------------------------------------------------------
  // Sub-spec 07: background exception propagation
  // -------------------------------------------------------------------------

  /** When the background thread has thrown, runState(), lastTransition(), and
   *  runStatus() must all rethrow the stored exception.  listenerState() must
   *  return Error without throwing.
   */
  void test_background_exception_propagates_from_all_getters() {
    TestableSNSListener listener;
    listener.injectBackgroundException("synthetic background error");

    TS_ASSERT_THROWS(listener.runState(), const std::runtime_error &);
    TS_ASSERT_THROWS(listener.lastTransition(), const std::runtime_error &);
    TS_ASSERT_THROWS(listener.runStatus(), const std::runtime_error &);

    // listenerState() must NOT throw — it returns Error to allow callers to
    // distinguish "listener broken" from "listener not yet connected".
    TS_ASSERT_THROWS_NOTHING(listener.listenerState());
    TS_ASSERT_EQUALS(ListenerState::Error, listener.listenerState());
  }

  // -------------------------------------------------------------------------
  // Sub-spec 07: single-slot invariant — production throws
  //
  // The throw on the NEW_RUN transition path (m_workspaceInitialized==true,
  // slot occupied) is unreachable over a real socket because m_pauseNetRead
  // is set in the same rxPacket(RunStatusPkt) call that occupies the slot,
  // causing bufferParse() to stop parsing.
  // The throw on the END_RUN path fires when a pending transition (that is
  // NOT a BeginRun-in-JoiningRun collapse) is already queued.
  // These tests drive the production code directly by feeding pre-built
  // ADARA::RunStatusPkt bytes through the inherited Parser::bufferParse()
  // with state pre-injected via TestableSNSListener helpers.
  // -------------------------------------------------------------------------

  /** rxPacket(RunStatusPkt[NEW_RUN]) must throw on the transition path when
   *  m_workspaceInitialized is true and m_pendingTransition is already set.
   */
  void test_rxRunStatusPkt_newRun_throws_when_slot_occupied() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;
    listener.setWorkspaceInitialized(true);
    listener.injectPendingTransition(ILiveListener::BeginRun);
    auto bytes = Mantid::LiveData::Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, /*runNumber=*/1,
                                                              /*pulseId=*/0x0000000100000000ULL);
    TS_ASSERT_THROWS(listener.callBufferParse(bytes), const std::runtime_error &);
  }

  /** rxPacket(RunStatusPkt[NEW_RUN]) must throw on the joining path when
   *  m_adaraRunStatus is already JoiningRun (consecutive NEW_RUN without END_RUN).
   */
  void test_rxRunStatusPkt_newRun_joiningCase_throwsOnConsecutive() {
    TestableSNSListener listener;
    listener.setAdaraRunStatus(ILiveListener::JoiningRun);
    auto bytes = Mantid::LiveData::Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, /*runNumber=*/2,
                                                              /*pulseId=*/0x0000000200000000ULL);
    TS_ASSERT_THROWS(listener.callBufferParse(bytes), const std::runtime_error &);
  }

  /** rxPacket(RunStatusPkt[NEW_RUN]) on the joining path must set JoiningRun,
   *  call setRunDetails (run_number on m_eventBuffer), leave m_pendingTransition
   *  empty, and leave m_pauseNetRead false.
   */
  void test_rxRunStatusPkt_newRun_joiningCase_setsJoiningRun_without_pause() {
    TestableSNSListener listener;
    // m_workspaceInitialized defaults to false; m_adaraRunStatus to NoRun.
    auto bytes = Mantid::LiveData::Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, /*runNumber=*/42,
                                                              /*pulseId=*/0x0000000100000000ULL);
    TS_ASSERT_THROWS_NOTHING(listener.callBufferParse(bytes));
    TS_ASSERT_EQUALS(ILiveListener::JoiningRun, listener.readAdaraRunStatus());
    TS_ASSERT(!listener.readPendingTransition().has_value());
    TS_ASSERT(!listener.readPauseNetRead());
    // run_number must have been applied immediately (setRunDetails called in rxPacket).
    TS_ASSERT_EQUALS(42, listener.runNumber());
  }

  /** rxPacket(RunStatusPkt[END_RUN]) must throw when m_pendingTransition is
   *  already set and the state is NOT the JoiningRun-collapse case.
   */
  void test_rxRunStatusPkt_endRun_throws_when_slot_occupied() {
    TestableSNSListener listener;
    listener.m_stubHooks = true;
    // BeginRun pending with NoRun state → not the collapse case → throws.
    listener.injectPendingTransition(ILiveListener::BeginRun);
    auto bytes = Mantid::LiveData::Testing::buildRunStatusPkt(ADARA::RunStatus::END_RUN, /*runNumber=*/1,
                                                              /*pulseId=*/0x0000000100000000ULL);
    TS_ASSERT_THROWS(listener.callBufferParse(bytes), const std::runtime_error &);
  }

  /** rxPacket(RunStatusPkt[END_RUN]) must collapse (not throw) when
   *  m_pendingTransition==BeginRun and m_adaraRunStatus==JoiningRun.
   *  After collapse the slot must hold EndRun and m_pauseNetRead must be true.
   */
  void test_rxRunStatusPkt_endRun_joiningRunCollapse_setsEndRun() {
    TestableSNSListener listener;
    listener.setAdaraRunStatus(ILiveListener::JoiningRun);
    listener.injectPendingTransition(ILiveListener::BeginRun);
    auto bytes = Mantid::LiveData::Testing::buildRunStatusPkt(ADARA::RunStatus::END_RUN, /*runNumber=*/1,
                                                              /*pulseId=*/0x0000000200000000ULL);
    TS_ASSERT_THROWS_NOTHING(listener.callBufferParse(bytes));
    TS_ASSERT(listener.readPendingTransition().has_value());
    TS_ASSERT_EQUALS(ILiveListener::EndRun, *listener.readPendingTransition());
    TS_ASSERT(listener.readPauseNetRead());
  }

  /** onBeginRun() on the JoiningRun path must not reset instrument caches and
   *  must advance m_adaraRunStatus to Running without requiring a deferred pkt.
   */
  void test_onBeginRun_joiningPath_preserves_instrument_data() {
    TestableSNSListener listener;
    listener.setAdaraRunStatus(ILiveListener::JoiningRun);
    listener.setInstrumentData("<instrument/>", "TEST_INST");
    // No deferred packet — the JoiningRun path must not touch it.
    TS_ASSERT_THROWS_NOTHING(listener.callOnBeginRun());
    TS_ASSERT_EQUALS(ILiveListener::Running, listener.readAdaraRunStatus());
    TS_ASSERT_EQUALS("<instrument/>", listener.readInstrumentXML());
    TS_ASSERT_EQUALS("TEST_INST", listener.readInstrumentName());
  }

  // -------------------------------------------------------------------------
  // Sub-spec: m_requiredLogs cleared at run boundaries
  // -------------------------------------------------------------------------

  /** m_requiredLogs must be empty after onEndRun() and after the transition
   *  path of onBeginRun().  Stale entries from a previous geometry packet
   *  would prevent readyForInitPart2() from ever returning true on the next
   *  run, leaving the listener stuck in NotYet permanently.
   */
  void test_requiredLogs_cleared_on_run_boundaries() {
    // ---- Part 1: onEndRun() clears stale entries ----
    {
      TestableSNSListener listener;
      listener.seedRequiredLog("stale_log_a");
      TS_ASSERT(!listener.readRequiredLogs().empty());

      // Drive onEndRun() via the extract-cycle path: inject EndRun pending,
      // then run onBeforeExtract + onAfterExtract.  m_stubHooks is false
      // (default), so the real onEndRun() fires.
      listener.injectPendingTransition(ILiveListener::EndRun);
      TS_ASSERT_THROWS_NOTHING(listener.callOnBeforeExtract());
      TS_ASSERT_THROWS_NOTHING(listener.callOnAfterExtract());
      TS_ASSERT(listener.readRequiredLogs().empty());
    }

    // ---- Part 2: onBeginRun() transition path clears stale entries ----
    {
      TestableSNSListener listener;
      listener.seedRequiredLog("stale_log_b");
      TS_ASSERT(!listener.readRequiredLogs().empty());

      // Seed workspace-initialized state so the NEW_RUN packet takes the
      // transition path in rxPacket, storing m_deferredRunDetailsPkt (required
      // by onBeginRun()'s transition branch at SNSLiveEventDataListener.cpp:1649).
      listener.setWorkspaceInitialized(true);
      auto bytes = Mantid::LiveData::Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, /*runNum=*/1,
                                                                /*pulseId=*/0x0000000100000000ULL);
      TS_ASSERT_THROWS_NOTHING(listener.callBufferParse(bytes));

      // callOnBeginRun() takes the transition branch (m_adaraRunStatus == NoRun
      // != JoiningRun, m_deferredRunDetailsPkt is set) and must clear
      // m_requiredLogs.
      TS_ASSERT_THROWS_NOTHING(listener.callOnBeginRun());
      TS_ASSERT(listener.readRequiredLogs().empty());
    }
  }
};
