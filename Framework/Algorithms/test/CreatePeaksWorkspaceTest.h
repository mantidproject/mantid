// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CreatePeaksWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("CreatePeaksWorkspaceTest_OutputWS");

    Workspace2D_sptr instws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InstrumentWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(instws)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfPeaks", 13));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName));
    // Check that this is a PeaksWorkspace
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 13);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_leanElasticPeakWorkspace() {
    // Name of the output workspace.
    std::string outWSName("CreatePeaksWorkspaceTest_OutputWS2");

    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfPeaks", 13));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "LeanElasticPeak"));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    LeanElasticPeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>(outWSName));
    // Check that this is a LeanElasticPeaksWorkspace
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 13);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};
