// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/AddPeak.h"
#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class AddPeakTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("AddPeakTest_PeakWS");

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
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 13);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_with_incorrect_instrument() {
    // Name of the output workspace.
    std::string outWSName("AddPeakTest_PeakWS");

    Workspace2D_sptr instws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InstrumentWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(instws)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(outWSName));
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 1);

    auto run_ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10, false, false, true, "something_else");
    AddPeak add_alg;
    add_alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(add_alg.initialize())
    TS_ASSERT(add_alg.isInitialized())
    add_alg.setProperty("PeaksWorkspace", ws);
    add_alg.setProperty("RunWorkspace", run_ws);
    auto err_string = "The peak from " + run_ws->getName() + " comes from a different instrument (" +
                      run_ws->getInstrument()->getName() +
                      ") to the peaks "
                      "already in the table (" +
                      ws->getInstrument()->getName() + "). It could not be added.";
    TS_ASSERT_THROWS_EQUALS(add_alg.execute(), std::runtime_error & e, e.what(), err_string)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().clear();
  }
};
