// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REBINTOWORKSPACETEST_H_
#define REBINTOWORKSPACETEST_H_

//-------------------
// Includes
//--------------------
#include "MantidAlgorithms/RebinToWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <numeric>

class RebinToWorkspaceTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(rebinToWS.initialize());
    TS_ASSERT(rebinToWS.isInitialized());
  }

  void testPointDataWorkspacesAreRejected() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using Mantid::HistogramData::Points;

    Mantid::Algorithms::RebinToWorkspace alg;
    alg.initialize();

    // Creates a workspace with 10 points
    const int numYPoints(10);
    const int numSpectra(2);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspace123(
        numSpectra, numYPoints, false);
    // Reset the X data to something reasonable
    Points x(numYPoints);
    std::iota(begin(x), end(x), 0.0);
    for (int i = 0; i < numSpectra; ++i) {
      testWS->setPoints(i, x);
    }

    TS_ASSERT_EQUALS(testWS->isHistogramData(), false);
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty<MatrixWorkspace_sptr>("WorkspaceToMatch", testWS));
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty<MatrixWorkspace_sptr>("WorkspaceToRebin", testWS));
  }

  void testExec() {
    if (!rebinToWS.isInitialized())
      rebinToWS.initialize();

    // No properties have been set so should throw if executed
    TS_ASSERT_THROWS(rebinToWS.execute(), const std::runtime_error &);

    // Need to input workspaces to test this
    using namespace Mantid::DataObjects;
    Workspace2D_sptr rebinThis =
        WorkspaceCreationHelper::create2DWorkspaceBinned(10, 50, 5.0, 1.0);
    Workspace2D_sptr matchToThis =
        WorkspaceCreationHelper::create2DWorkspaceBinned(15, 30, 3.0, 2.5);
    // Register them with the DataService
    using namespace Mantid::API;
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().add("rbThis", rebinThis));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().add("matThis", matchToThis));

    // Set the properties for the algorithm
    rebinToWS.setPropertyValue("WorkspaceToRebin", "rbThis");
    rebinToWS.setPropertyValue("WorkspaceToMatch", "matThis");
    std::string outputSpace("testOutput");
    rebinToWS.setPropertyValue("OutputWorkspace", outputSpace);

    // Test that the properties are set correctly
    std::string result;
    TS_ASSERT_THROWS_NOTHING(result =
                                 rebinToWS.getPropertyValue("WorkspaceToRebin"))
    TS_ASSERT(result == "rbThis");

    TS_ASSERT_THROWS_NOTHING(result =
                                 rebinToWS.getPropertyValue("WorkspaceToMatch"))
    TS_ASSERT(result == "matThis");

    TS_ASSERT_THROWS_NOTHING(result =
                                 rebinToWS.getPropertyValue("OutputWorkspace"))
    TS_ASSERT(result == outputSpace);

    // Execute the algorithm, testing that it does not throw
    TS_ASSERT_THROWS_NOTHING(rebinToWS.execute());
    TS_ASSERT(rebinToWS.isExecuted());

    // Retrieved rebinned workspace
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(
        workspace = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(workspace);

    // Test x-vectors from this and "matchToThis" are the same
    TS_ASSERT_EQUALS(output2D->x(0).size(), matchToThis->x(0).size());
    TS_ASSERT_DIFFERS(output2D->x(0).size(), rebinThis->x(0).size());

    // Test a random x bin for matching value
    TS_ASSERT_EQUALS(output2D->x(0)[22], matchToThis->x(0)[22]);
  }

private:
  Mantid::Algorithms::RebinToWorkspace rebinToWS;
};

#endif // REBINTOWORKSPACETEST_H_
