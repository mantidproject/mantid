// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.hxx"
#include "MantidAlgorithms/ChangePulsetime2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

namespace {
EventWorkspace_sptr execute_change_of_pulse_times(const EventWorkspace_sptr &in_ws, const std::string &timeOffset,
                                                  const std::string &workspaceIndexList) {
  // Create and run the algorithm
  ChangePulsetime2 alg;
  alg.initialize();
  alg.setRethrows(true);
  alg.setChild(true);
  alg.setWorkspaceInputProperties<EventWorkspace>("InputWorkspace", in_ws, IndexType::WorkspaceIndex,
                                                  workspaceIndexList);
  alg.setProperty("InputWorkspace", in_ws);
  alg.setPropertyValue("OutputWorkspace", "out_ws");
  alg.setPropertyValue("TimeOffset", timeOffset);
  alg.execute();

  // Get the result and return it
  EventWorkspace_sptr out_ws = alg.getProperty("OutputWorkspace");
  return out_ws;
}
} // namespace

//---------------------------------------------------------------------------------
// Unit Tests
//---------------------------------------------------------------------------------
class ChangePulsetime2Test : public CxxTest::TestSuite {
public:
  void test_Init() {
    ChangePulsetime2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test(const std::string &in_ws_name, const std::string &out_ws_name, const std::string &WorkspaceIndexList) {
    ChangePulsetime2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    EventWorkspace_sptr in_ws, out_ws;
    in_ws = WorkspaceCreationHelper::createEventWorkspace2(100, 100);
    AnalysisDataService::Instance().addOrReplace(in_ws_name, in_ws);

    alg.setWorkspaceInputProperties<EventWorkspace>("InputWorkspace", in_ws_name, IndexType::WorkspaceIndex,
                                                    WorkspaceIndexList);
    alg.setPropertyValue("OutputWorkspace", out_ws_name);
    alg.setPropertyValue("TimeOffset", "1000.0");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        out_ws = std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(out_ws_name)));
    TS_ASSERT(out_ws);
    if (!out_ws)
      return;

    for (size_t wi = 10; wi < 20; wi++) {
      double secs;
      secs = DateAndTime::secondsFromDuration(out_ws->getSpectrum(wi).getEvent(0).pulseTime() -
                                              DateAndTime("2010-01-01T00:00:00"));
      TS_ASSERT_DELTA(secs, 1000.0, 1e-5);
      secs = DateAndTime::secondsFromDuration(out_ws->getSpectrum(wi).getEvent(2).pulseTime() -
                                              DateAndTime("2010-01-01T00:00:00"));
      TS_ASSERT_DELTA(secs, 1001.0, 1e-5);
    }

    // If only modifying SOME spectra, check that the others did not change
    if (WorkspaceIndexList != "") {
      double secs;
      secs = DateAndTime::secondsFromDuration(out_ws->getSpectrum(0).getEvent(2).pulseTime() -
                                              DateAndTime("2010-01-01T00:00:00"));
      TS_ASSERT_DELTA(secs, 1.0, 1e-5);
      secs = DateAndTime::secondsFromDuration(out_ws->getSpectrum(30).getEvent(2).pulseTime() -
                                              DateAndTime("2010-01-01T00:00:00"));
      TS_ASSERT_DELTA(secs, 1.0, 1e-5);
    }

    // If not inplace, then the original did not change
    if (in_ws_name != out_ws_name) {
      double secs;
      secs = DateAndTime::secondsFromDuration(in_ws->getSpectrum(0).getEvent(2).pulseTime() -
                                              DateAndTime("2010-01-01T00:00:00"));
      TS_ASSERT_DELTA(secs, 1.0, 1e-5);
    }

    AnalysisDataService::Instance().remove(in_ws_name);
    AnalysisDataService::Instance().remove(out_ws_name);
  }

  void test_exec_allSpectra_copying_the_workspace() {
    do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_out_ws", "");
  }

  void test_exec_allSpectra_inplace() { do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_ws", ""); }

  void test_exec_someSpectra_copying_the_workspace() {
    do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_out_ws", "10-20");
  }

  void test_exec_someSpectra_inplace() { do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_ws", "10-20"); }
};

//---------------------------------------------------------------------------------
// Performance Test
//---------------------------------------------------------------------------------
class ChangePulsetime2TestPerformance : public CxxTest::TestSuite {
private:
  EventWorkspace_sptr m_workspace;

public:
  void setUp() override {
    EventWorkspace_sptr in_ws = WorkspaceCreationHelper::createEventWorkspace2(30000, 30000);
    m_workspace = in_ws;
  }

  void test_change_of_pulse_time() { execute_change_of_pulse_times(m_workspace, "1000", ""); }
};
