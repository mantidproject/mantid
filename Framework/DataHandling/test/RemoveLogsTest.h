// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidDataHandling/RemoveLogs.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

class RemoveLogsTest : public CxxTest::TestSuite {
public:
  static RemoveLogsTest *createSuite() { return new RemoveLogsTest(); }
  static void destroySuite(RemoveLogsTest *suite) { delete suite; }

  RemoveLogsTest() {}

  /**
   * Creates a sample workspace in ADS.
   */
  void setUp() override {
    m_sampleWorkspace = "__remove_logs_test_ws";
    createSampleWorkspace();
  }

  /**
   * Removes the sample workspace from ADS.
   */
  void tearDown() override { AnalysisDataService::Instance().remove(m_sampleWorkspace); }

  /**
   * Tests creation and initialisation of the algorithm.
   */
  void test_init() {
    TS_ASSERT(!m_remover.isInitialized());
    TS_ASSERT_THROWS_NOTHING(m_remover.initialize());
    TS_ASSERT(m_remover.isInitialized());
  }

  /**
   * Tests removal of all logs from the workspace.
   */
  void test_removeAllLogs() {
    // Get the sample workspace from ADS
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_sampleWorkspace));

    // Make sure it has log data
    TS_ASSERT_DIFFERS(output->run().getLogData().size(), 0);

    // Remove it's logs
    TS_ASSERT_THROWS_NOTHING(m_remover.initialize());
    TS_ASSERT_THROWS_NOTHING(m_remover.setPropertyValue("Workspace", m_sampleWorkspace));
    TS_ASSERT_THROWS_NOTHING(m_remover.execute());
    TS_ASSERT(m_remover.isExecuted());

    // Ensure it's no longer has any log data
    TS_ASSERT_EQUALS(output->run().getLogData().size(), 0);
  }

  /**
   * Tests keeping certain logs in the workspace.
   */
  void test_keepLogs() {
    // Get the sample workspace from ADS
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_sampleWorkspace));

    // Make sure it has log data
    TS_ASSERT_DIFFERS(output->run().getLogData().size(), 0);

    // Remove it's logs
    TS_ASSERT_THROWS_NOTHING(m_remover.initialize());
    TS_ASSERT_THROWS_NOTHING(m_remover.setPropertyValue("Workspace", m_sampleWorkspace));
    TS_ASSERT_THROWS_NOTHING(m_remover.setPropertyValue("KeepLogs", "Ei, scan_index"));
    TS_ASSERT_THROWS_NOTHING(m_remover.execute());
    TS_ASSERT(m_remover.isExecuted());

    // Ensure it has the correct log data
    TS_ASSERT_DIFFERS(output->run().getLogData().size(), 0);

    TS_ASSERT_THROWS(output->run().getLogData("some_prop"), const std::runtime_error &);
    TS_ASSERT_THROWS(output->run().getLogData("T0"), const std::runtime_error &);

    TS_ASSERT_THROWS_NOTHING(output->run().getLogData("Ei"));
    TS_ASSERT_THROWS_NOTHING(output->run().getLogData("scan_index"));
  }

private:
  /**
   * Creates a sample workspace with various types of log entries
   */
  void createSampleWorkspace() {
    // Create the workspace
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 100);

    // Add some log entries to it
    std::vector<DateAndTime> times;
    std::vector<int> index;
    std::vector<double> dbl1, dbl2;
    DateAndTime startTime("2010-01-01T00:00:00");
    for (int i = 0; i < 100; ++i) {
      times.emplace_back(startTime + i * 10.0);
      index.emplace_back(i);
      dbl1.emplace_back(i * 0.1);
      dbl2.emplace_back(6.0);
    }

    auto scan_index = new TimeSeriesProperty<int>("scan_index");
    scan_index->addValues(times, index);
    ws->mutableRun().addProperty(scan_index);

    auto dbl_prop1 = new TimeSeriesProperty<double>("some_prop");
    auto dbl_prop2 = new TimeSeriesProperty<double>("some_other_prop");
    dbl_prop1->addValues(times, dbl1);
    dbl_prop2->addValues(times, dbl2);

    ws->mutableRun().addProperty(dbl_prop1);
    ws->mutableRun().addProperty(dbl_prop2);

    ws->mutableRun().addProperty("Ei", 42.);
    ws->mutableRun().addProperty("T0", 42.);

    // Store it in ADS
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(m_sampleWorkspace, ws));
  }

  RemoveLogs m_remover;
  std::string m_sampleWorkspace;
};
