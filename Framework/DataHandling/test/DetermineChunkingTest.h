// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/DetermineChunking.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class DetermineChunkingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetermineChunkingTest *createSuite() { return new DetermineChunkingTest(); }
  static void destroySuite(DetermineChunkingTest *suite) { delete suite; }

  void test_Init() {
    DetermineChunking alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test_CNCS(bool events) {
    // Name of the output workspace.
    std::string outWSName("DetermineChunkingTest_OutputWS");

    DetermineChunking alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    if (events) {
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "CNCS_7860_event.nxs"));
    } else {
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "CNCS_7860_runinfo.xml"));
    }
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxChunkSize", 0.0005));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    DataObjects::TableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<DataObjects::TableWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    TS_ASSERT_EQUALS(ws->rowCount(), 11);
    TableRow r = ws->getFirstRow();
    int s;
    int n;
    r >> s >> n;
    TS_ASSERT_EQUALS(s, 1);
    TS_ASSERT_EQUALS(n, 11);
    r.next();
    r >> s >> n;
    TS_ASSERT_EQUALS(s, 2);
    TS_ASSERT_EQUALS(n, 11);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  void test_CNCS() {
    do_test_CNCS(true);
    do_test_CNCS(false);
  }
};
