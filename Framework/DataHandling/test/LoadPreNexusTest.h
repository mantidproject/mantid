// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadPreNexus.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Timer.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadPreNexusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadPreNexusTest *createSuite() { return new LoadPreNexusTest(); }
  static void destroySuite(LoadPreNexusTest *suite) { delete suite; }

  void test_Init() {
    LoadPreNexus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("LoadPreNexusTest_OutputWS");

    LoadPreNexus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "CNCS_7860_runinfo.xml"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChunkNumber", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TotalChunks", 5));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // TODO: Check the results

    // Check the monitor workspace is in there
    TS_ASSERT(ws->monitorWorkspace());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};
