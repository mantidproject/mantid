// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#pragma once
#ifndef _WIN32

// INTEGRATION TESTS.  Exercises SNSLiveEventDataListener through the public
// algorithm API (LoadLiveData, MonitorLiveData), using a real in-process
// MockSMSServer over a Unix-domain socket.  Does NOT require SMS or any
// external network resource.  Linux/macOS only — compiles to an empty suite
// on Windows.
//
// test_LoadLiveData_standalone_no_deadlock
//   Regression test for the pre-subspec07 deadlock and the snapshot-too-early
//   race.  Before subspec07, LoadLiveData standalone never called runStatus(),
//   so m_pauseNetRead was never cleared.  After subspec07 + m_bgThreadCaughtUp
//   fix, onBeforeExtract() throws NotYet if m_bgThreadCaughtUp is false (bg
//   thread mid-bufferParse), ensuring BeginRun is never missed.  Once the
//   bg thread exits bufferParse (caughtUp=true), the snapshot is safe and
//   always captures BeginRun → correct run_number and events.
//
// test_MonitorLiveData_workspace_renaming_unchanged
//   Backward-compatibility test: drives a two-run sequence and asserts that
//   MonitorLiveData produces the expected EndRun rename suffixes
//   (_<runNumber>) with no regression from the new listener API.
//
// test_MonitorLiveData_BeginRun_post_rename
//   Regression test for the lastTransition() BeginRun erasure bug: a NEW_RUN
//   that arrives without a preceding END_RUN must produce a "_post"-suffixed
//   rename clone.  Before the fix, onAfterExtract() cleared m_lastTransition
//   before MonitorLiveData could read it; the "_post" clone was silently
//   dropped.

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/FacilityHelper.h"
#include "MantidKernel/ConfigService.h"

#include "MantidLiveData/ADARA/ADARA.h"

#include "ADARAPackets.h" // every byte-array fixture
#include "MockSMSServer.h"

#include <Poco/ActiveResult.h>
#include <Poco/TemporaryFile.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

using namespace Mantid;
using namespace Mantid::LiveData;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace {

constexpr const char *kAIInstrumentName = "DUM";

inline const std::string &kAIMinimalIDF() {
  static const std::string idf = []() {
    auto &config = ConfigService::Instance();
    const std::filesystem::path path =
        std::filesystem::path(config.getInstrumentDirectory()) / "unit_testing" / "DUM_Definition.xml";
    std::ifstream in(path);
    if (!in) {
      throw std::runtime_error("SNSLiveEventDataListenerAlgorithmIntegrationTest: failed to open IDF at " +
                               path.string());
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
  }();
  return idf;
}

template <typename Pred>
bool aiWaitFor(Pred pred, std::chrono::milliseconds timeout = std::chrono::seconds{30},
               std::chrono::milliseconds poll = std::chrono::milliseconds{50}) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!pred()) {
    if (std::chrono::steady_clock::now() >= deadline) {
      TS_FAIL("aiWaitFor timed out");
      return false;
    }
    std::this_thread::sleep_for(poll);
  }
  return true;
}

} // namespace

class SNSLiveEventDataListenerAlgorithmIntegrationTest : public CxxTest::TestSuite {
public:
  static SNSLiveEventDataListenerAlgorithmIntegrationTest *createSuite() {
    return new SNSLiveEventDataListenerAlgorithmIntegrationTest();
  }
  static void destroySuite(SNSLiveEventDataListenerAlgorithmIntegrationTest *s) { delete s; }

  SNSLiveEventDataListenerAlgorithmIntegrationTest() : m_testFacility("unit_testing/UnitTestFacilities.xml", "TEST") {
    FrameworkManager::Instance();
  }

  void setUp() override {
    AnalysisDataService::Instance().clear();

    m_sockFileHandle = std::make_unique<Poco::TemporaryFile>();
    m_sockPath = m_sockFileHandle->path();
    if (m_sockPath.size() >= 100) {
      TS_FAIL("UDS path too long for sun_path (>= 100 chars): " + m_sockPath);
      return;
    }
    std::filesystem::remove(m_sockPath);

    auto &cfg = ConfigService::Instance();
    m_savedKeepPausedEvents = cfg.getString("SNSLiveEventDataListener.keepPausedEvents");
    m_savedReadRetryInterval = cfg.getString("LiveData.readRetryInterval");
    cfg.setString("LiveData.readRetryInterval", "0.25");

    m_server = std::make_unique<Testing::MockSMSServer>(m_sockPath);
    m_watchdog = std::make_unique<Testing::TestWatchdog>(std::chrono::seconds{60},
                                                         "SNSLiveEventDataListenerAlgorithmIntegrationTest");
  }

  void tearDown() override {
    // Strict order: algorithm (which owns the listener) must be cancelled/done
    // before the server socket is destroyed.  Both test methods join their
    // algorithms before returning, so by the time tearDown runs the listener
    // bg thread has already exited.
    m_server.reset();
    m_sockFileHandle.reset();
    m_watchdog.reset();

    auto &cfg = ConfigService::Instance();
    cfg.setString("SNSLiveEventDataListener.keepPausedEvents", m_savedKeepPausedEvents);
    cfg.setString("LiveData.readRetryInterval", m_savedReadRetryInterval);

    AnalysisDataService::Instance().clear();
  }

  // -------------------------------------------------------------------------
  // Test 1: LoadLiveData standalone — no deadlock regression, correct workspace.
  //
  // Before subspec07: m_pauseNetRead was set by rxPacket(RunStatusPkt NEW_RUN)
  // and cleared only inside runStatus().  LoadLiveData standalone never called
  // runStatus(), so doExtractData() returned NotYet indefinitely.
  //
  // After subspec07 + m_bgThreadCaughtUp fix: onBeforeExtract() throws NotYet
  // if m_bgThreadCaughtUp is false (bg thread inside bufferParse).  Once the bg
  // thread exits its bufferParse iteration (caughtUp=true), the snapshot is safe
  // and always captures BeginRun.  The PktWaitForExtract gate makes this
  // deterministic: it holds the server after NEW_RUN until the test confirms the
  // bg thread has parsed through (scriptIndex 5), then releases so the bg thread
  // can drain the remaining packets.
  //
  // Script ordering:
  //   STATE + Geometry + Beamline + NEW_RUN — bg thread parses these;
  //     m_workspaceInitialized=true (from Geom+Beamline), then NEW_RUN (normal
  //     path): m_pendingTransition=BeginRun, m_pauseNetRead=true,
  //     m_bgThreadCaughtUp=true (pause loop entered).
  //   PktWaitForExtract — server blocks; test waits for scriptIndex 5, then
  //     calls releaseExtractGate().
  //   Geometry + Beamline — re-initialise workspace after onBeginRun() clears it.
  //   BankedEvent — one event on pixel 1 (mapped by DUM IDF).
  //   Disconnect.
  //
  // onBeforeExtract() waits for m_bgThreadCaughtUp=true, snapshots BeginRun,
  // dispatches onBeginRun() (writes run_number, clears m_workspaceInitialized,
  // clears m_pauseNetRead).  releaseExtractGate() lets bg thread drain the
  // remaining packets → m_workspaceInitialized=true with run_number and events.
  // Algorithm finishes well within the 10 s tryWait bound.
  // -------------------------------------------------------------------------
  void test_LoadLiveData_standalone_no_deadlock() {
    constexpr uint32_t kRunNum = 12345u;
    constexpr uint64_t kPulseId = 0x0000000100000000ULL;

    // Script ordering:
    //   STATE + Geometry + Beamline + NEW_RUN — bg thread parses these and enters
    //     the pause loop (m_bgThreadCaughtUp=true, m_pendingTransition=BeginRun).
    //   PktWaitForExtract — holds the server here.  bg thread has completed its
    //     bufferParse (caughtUp=true) and is in the pause loop.  onBeforeExtract()
    //     sees caughtUp=true, snapshots BeginRun, dispatches onBeginRun() (writes
    //     run_number, clears m_workspaceInitialized), clears m_pauseNetRead.
    //     bg thread exits pause, enters receiveBytes.
    //   releaseExtractGate() — server sends 2nd Geometry + Beamline + BankedEvent +
    //     Disconnect.  bg thread parses → m_workspaceInitialized=true, events appended.
    //   doExtractData() returns the workspace with run_number and events.
    m_server->script({
        Testing::buildRunStatusPkt(ADARA::RunStatus::STATE, /*runNum=*/0, kPulseId),
        Testing::buildGeometryPkt(kAIMinimalIDF()),
        Testing::buildBeamlineInfoPkt(kAIInstrumentName),
        Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, kRunNum, kPulseId),
        Testing::PktWaitForExtract{}, // gate (index 4 → scriptIndex 5 when server blocks)
        // onBeginRun() clears m_workspaceInitialized; re-init needs these:
        Testing::buildGeometryPkt(kAIMinimalIDF()),
        Testing::buildBeamlineInfoPkt(kAIInstrumentName),
        Testing::buildBankedEventPkt(kPulseId, /*chargePc=*/1000.0, {{/*tof=*/100u, /*pixel=*/1u}}),
        Testing::PktDisconnect{},
    });
    m_server->start();

    auto loadAlg = AlgorithmManager::Instance().create("LoadLiveData");
    loadAlg->initialize();
    loadAlg->setPropertyValue("Instrument", "TestDataListener");
    loadAlg->setPropertyValue("Listener", "SNSLiveEventDataListener");
    loadAlg->setPropertyValue("Address", m_sockPath);
    loadAlg->setPropertyValue("AccumulationMethod", "Replace");
    loadAlg->setProperty("PreserveEvents", true);
    loadAlg->setPropertyValue("OutputWorkspace", "ts1_load_no_deadlock");

    Poco::ActiveResult<bool> res = loadAlg->executeAsync();

    // Wait for the server to have sent the first 4 packets (STATE + Geom + Beamline
    // + NEW_RUN) and to be blocking at the PktWaitForExtract gate.  By this point
    // the bg thread has parsed through NEW_RUN and is in the pause loop
    // (m_bgThreadCaughtUp=true), so onBeforeExtract() will snapshot BeginRun.
    // Then release the gate so the bg thread can drain the remaining packets.
    if (aiWaitFor([&] { return m_server->scriptIndex() >= 5; }, std::chrono::seconds{10})) {
      m_server->releaseExtractGate();
    }

    const bool finished = res.tryWait(10000); // ms
    TSM_ASSERT("LoadLiveData did not complete within 10 s — deadlock regression", finished);
    if (!finished) {
      loadAlg->cancel();
      res.wait();
      return;
    }

    TS_ASSERT(loadAlg->isExecuted());

    auto &ads = AnalysisDataService::Instance();
    TS_ASSERT(ads.doesExist("ts1_load_no_deadlock"));

    auto ws = ads.retrieveWS<DataObjects::EventWorkspace>("ts1_load_no_deadlock");
    TS_ASSERT_DIFFERS(ws, nullptr);
    if (ws) {
      TS_ASSERT_LESS_THAN(0, static_cast<int>(ws->getNumberEvents()));
      TS_ASSERT_EQUALS(ws->run().getPropertyValueAsType<std::string>("run_number"), std::string{"12345"});
    }
  }

  // -------------------------------------------------------------------------
  // Test 2: MonitorLiveData — workspace rename suffixes unchanged.
  //
  // Drives a two-run sequence (runs 10001 and 10002) and asserts that
  // MonitorLiveData produces the expected ADS clones at each EndRun boundary:
  //   EndRun 10001  → "ts2_monitor_10001"
  //   EndRun 10002  → "ts2_monitor_10002"
  //
  // The two-run sequence avoids the prevRunNumber=0 corner case (which
  // produces a "__post" clone on the first BeginRun) while demonstrating
  // that the new listener API integrates correctly with the unmodified
  // MonitorLiveData rename logic.
  //
  // Script ordering (white-lie path for both runs):
  //   Geometry + Beamline for run A — queued before NEW_RUN so they are
  //   parsed before m_dataStartTime is set; initWorkspacePart2() does not
  //   trigger until NEW_RUN arrives and sets m_dataStartTime.
  //   NEW_RUN 10001 — white-lie path (m_workspaceInitialized=false):
  //     sets m_dataStartTime, calls initWorkspacePart2() immediately, no pause.
  //   BankedEvent (run A, pixels 1..2).
  //   END_RUN 10001 — queued; processed by onAfterExtract() after MonitorLiveData
  //     calls extractData(); triggers rename to "ts2_monitor_10001".
  //   Geometry + Beamline for run B.
  //   NEW_RUN 10002 — white-lie path again (onEndRun clears m_workspaceInitialized).
  //   BankedEvent (run B, pixel 1).
  //   END_RUN 10002 — triggers rename to "ts2_monitor_10002".
  //   Disconnect.
  //
  // No PktWaitForExtract gates are needed: the single-slot m_pendingTransition
  // queue plus m_pauseNetRead serialise the two EndRun edges across separate
  // MonitorLiveData iterations without any test-side gating.
  // -------------------------------------------------------------------------
  void test_MonitorLiveData_workspace_renaming_unchanged() {
    constexpr uint32_t kRunA = 10001u;
    constexpr uint32_t kRunB = 10002u;
    constexpr uint64_t kPulseA = 0x0000000100000000ULL;
    constexpr uint64_t kPulseA2 = 0x0000000200000000ULL;
    constexpr uint64_t kPulseB = 0x0000000300000000ULL;
    constexpr uint64_t kPulseB2 = 0x0000000400000000ULL;

    m_server->script({
        // Run A
        Testing::buildGeometryPkt(kAIMinimalIDF()),
        Testing::buildBeamlineInfoPkt(kAIInstrumentName),
        Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, kRunA, kPulseA),
        Testing::buildBankedEventPkt(kPulseA, /*chargePc=*/1000.0,
                                     {{/*tof=*/100u, /*pixel=*/1u}, {/*tof=*/200u, /*pixel=*/2u}}),
        Testing::buildRunStatusPkt(ADARA::RunStatus::END_RUN, kRunA, kPulseA2),
        // Run B
        Testing::buildGeometryPkt(kAIMinimalIDF()),
        Testing::buildBeamlineInfoPkt(kAIInstrumentName),
        Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, kRunB, kPulseB),
        Testing::buildBankedEventPkt(kPulseB, /*chargePc=*/500.0, {{/*tof=*/300u, /*pixel=*/1u}}),
        Testing::buildRunStatusPkt(ADARA::RunStatus::END_RUN, kRunB, kPulseB2),
        Testing::PktDisconnect{},
    });
    m_server->start();

    auto monitorAlg = AlgorithmManager::Instance().create("MonitorLiveData", -1);
    monitorAlg->initialize();
    monitorAlg->setPropertyValue("Instrument", "TestDataListener");
    monitorAlg->setPropertyValue("Listener", "SNSLiveEventDataListener");
    monitorAlg->setPropertyValue("Address", m_sockPath);
    monitorAlg->setPropertyValue("UpdateEvery", "0.05");
    monitorAlg->setPropertyValue("AccumulationMethod", "Replace");
    monitorAlg->setPropertyValue("RunTransitionBehavior", "Rename");
    monitorAlg->setProperty("PreserveEvents", true);
    monitorAlg->setPropertyValue("OutputWorkspace", "ts2_monitor");

    Poco::ActiveResult<bool> active = monitorAlg->executeAsync();

    auto &ads = AnalysisDataService::Instance();

    // Wait for MonitorLiveData to produce the EndRun A clone.
    const bool gotA = aiWaitFor([&] { return ads.doesExist("ts2_monitor_10001"); }, std::chrono::seconds{30});
    TSM_ASSERT("ts2_monitor_10001 not created within 30 s (EndRun A)", gotA);

    // Wait for MonitorLiveData to produce the EndRun B clone.
    const bool gotB = aiWaitFor([&] { return ads.doesExist("ts2_monitor_10002"); }, std::chrono::seconds{30});
    TSM_ASSERT("ts2_monitor_10002 not created within 30 s (EndRun B)", gotB);

    monitorAlg->cancel();
    active.wait(20000); // ms

    // Assert EndRun A clone has events from run A.
    if (gotA) {
      auto wsA = ads.retrieveWS<DataObjects::EventWorkspace>("ts2_monitor_10001");
      TS_ASSERT_DIFFERS(wsA, nullptr);
      if (wsA) {
        TS_ASSERT_LESS_THAN(0, static_cast<int>(wsA->getNumberEvents()));
        TS_ASSERT_EQUALS(wsA->run().getPropertyValueAsType<std::string>("run_number"), std::string{"10001"});
      }
    }

    // Assert EndRun B clone has events from run B.
    if (gotB) {
      auto wsB = ads.retrieveWS<DataObjects::EventWorkspace>("ts2_monitor_10002");
      TS_ASSERT_DIFFERS(wsB, nullptr);
      if (wsB) {
        TS_ASSERT_LESS_THAN(0, static_cast<int>(wsB->getNumberEvents()));
        TS_ASSERT_EQUALS(wsB->run().getPropertyValueAsType<std::string>("run_number"), std::string{"10002"});
      }
    }
  }

  // -------------------------------------------------------------------------
  // Test 3: MonitorLiveData — BeginRun "_post" rename fires when a NEW_RUN
  // arrives without a preceding END_RUN.
  //
  // Regression test for the lastTransition() BeginRun erasure bug: before the
  // fix, onAfterExtract() unconditionally cleared m_lastTransition, so
  // MonitorLiveData would see nullopt and fall back to runState() == Running,
  // bypassing the BeginRun rename branch entirely.
  //
  // Script ordering:
  //   Run A (10001): NEW_RUN + BankedEvent + END_RUN.
  //     MonitorLiveData sees EndRun, sets prevRunNumber=10001, renames to
  //     "ts2_monitor2_10001".
  //   Run B (10002): Geometry + Beamline (because onEndRun clears
  //     m_workspaceInitialized) + NEW_RUN + BankedEvent.
  //     MonitorLiveData sees lastTransition() == BeginRun (requires the fix),
  //     prevRunNumber==10001, renames to "ts2_monitor2_10001_post".
  //   Disconnect (no END_RUN 10002 — the BeginRun rename must fire for run B).
  //
  // Without the fix the "_post" clone never appears; with the fix it does.
  // The "_10001" clone must also appear to confirm prevRunNumber was set.
  // -------------------------------------------------------------------------
  void test_MonitorLiveData_BeginRun_post_rename() {
    constexpr uint32_t kRunA = 10001u;
    constexpr uint32_t kRunB = 10002u;
    constexpr uint64_t kPulseA = 0x0000000100000000ULL;
    constexpr uint64_t kPulseA2 = 0x0000000200000000ULL;
    constexpr uint64_t kPulseB = 0x0000000300000000ULL;

    m_server->script({
        // Run A — ends normally so prevRunNumber is set before run B starts.
        Testing::buildGeometryPkt(kAIMinimalIDF()),
        Testing::buildBeamlineInfoPkt(kAIInstrumentName),
        Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, kRunA, kPulseA),
        Testing::buildBankedEventPkt(kPulseA, /*chargePc=*/1000.0, {{/*tof=*/100u, /*pixel=*/1u}}),
        Testing::buildRunStatusPkt(ADARA::RunStatus::END_RUN, kRunA, kPulseA2),
        // Run B — NEW_RUN with NO END_RUN.  The BeginRun edge must survive
        // through extractData() so MonitorLiveData can observe it and rename.
        Testing::buildGeometryPkt(kAIMinimalIDF()),
        Testing::buildBeamlineInfoPkt(kAIInstrumentName),
        Testing::buildRunStatusPkt(ADARA::RunStatus::NEW_RUN, kRunB, kPulseB),
        Testing::buildBankedEventPkt(kPulseB, /*chargePc=*/500.0, {{/*tof=*/200u, /*pixel=*/1u}}),
        Testing::PktDisconnect{},
    });
    m_server->start();

    auto monitorAlg = AlgorithmManager::Instance().create("MonitorLiveData", -1);
    monitorAlg->initialize();
    monitorAlg->setPropertyValue("Instrument", "TestDataListener");
    monitorAlg->setPropertyValue("Listener", "SNSLiveEventDataListener");
    monitorAlg->setPropertyValue("Address", m_sockPath);
    monitorAlg->setPropertyValue("UpdateEvery", "0.05");
    monitorAlg->setPropertyValue("AccumulationMethod", "Replace");
    monitorAlg->setPropertyValue("RunTransitionBehavior", "Rename");
    monitorAlg->setProperty("PreserveEvents", true);
    monitorAlg->setPropertyValue("OutputWorkspace", "ts2_monitor2");

    Poco::ActiveResult<bool> active = monitorAlg->executeAsync();

    auto &ads = AnalysisDataService::Instance();

    // EndRun A rename — confirms prevRunNumber is set before the BeginRun B tick.
    const bool gotEndRunA = aiWaitFor([&] { return ads.doesExist("ts2_monitor2_10001"); }, std::chrono::seconds{30});
    TSM_ASSERT("ts2_monitor2_10001 not created within 30 s (EndRun A)", gotEndRunA);

    // BeginRun B rename — the regression: requires the fix so that
    // lastTransition() == BeginRun is observable after extractData() returns.
    const bool gotPost = aiWaitFor([&] { return ads.doesExist("ts2_monitor2_10001_post"); }, std::chrono::seconds{30});
    TSM_ASSERT("ts2_monitor2_10001_post not created within 30 s (BeginRun B, requires fix)", gotPost);

    monitorAlg->cancel();
    active.wait(20000); // ms

    if (gotPost) {
      auto wsPost = ads.retrieveWS<DataObjects::EventWorkspace>("ts2_monitor2_10001_post");
      TS_ASSERT_DIFFERS(wsPost, nullptr);
    }
  }

private:
  FacilityHelper::ScopedFacilities m_testFacility;
  std::string m_savedKeepPausedEvents;
  std::string m_savedReadRetryInterval;
  std::unique_ptr<Poco::TemporaryFile> m_sockFileHandle;
  std::string m_sockPath;
  std::unique_ptr<Testing::MockSMSServer> m_server;
  std::unique_ptr<Testing::TestWatchdog> m_watchdog;
};

#else
// Windows stub
class SNSLiveEventDataListenerAlgorithmIntegrationTest : public CxxTest::TestSuite {};
#endif // !_WIN32
