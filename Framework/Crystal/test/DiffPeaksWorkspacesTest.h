// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_DIFFPEAKSWORKSPACESTEST_H_
#define MANTID_CRYSTAL_DIFFPEAKSWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/DiffPeaksWorkspaces.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Crystal::DiffPeaksWorkspaces;

class DiffPeaksWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiffPeaksWorkspacesTest *createSuite() {
    return new DiffPeaksWorkspacesTest();
  }
  static void destroySuite(DiffPeaksWorkspacesTest *suite) { delete suite; }

  void test_init() {
    DiffPeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_invalid_input() {
    DiffPeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    // Tolerance has to be positive.
    TS_ASSERT_THROWS(alg.setProperty("Tolerance", -1.0), const std::invalid_argument &)
  }

  // It shouldn't be a problem to subtract identical workspaces. You just get an
  // empty one out.
  void test_diff_identical_peaksworkspaces() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    DiffPeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  // In fact, it should be fine for the second one to be larger
  void test_diff_larger_rhs() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr lhsWS = WorkspaceCreationHelper::createPeaksWorkspace();
    PeaksWorkspace_sptr rhsWS =
        WorkspaceCreationHelper::createPeaksWorkspace(6);

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    DiffPeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", lhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", rhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_no_matches() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr lhsWS = WorkspaceCreationHelper::createPeaksWorkspace();
    PeaksWorkspace_sptr rhsWS = WorkspaceCreationHelper::createPeaksWorkspace();

    // Shift the rhs peaks in Q
    auto &rhsPeaks = rhsWS->getPeaks();
    rhsPeaks[0].setWavelength(rhsPeaks[0].getWavelength() * 1.01);
    rhsPeaks[1].setDetectorID(50);

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    DiffPeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", lhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", rhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.1))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 2)
    TS_ASSERT_EQUALS(ws->getPeak(0).getQLabFrame(),
                     lhsWS->getPeak(0).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(1).getQLabFrame(),
                     lhsWS->getPeak(1).getQLabFrame())

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_match_peaks_within_tolerance() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr lhsWS =
        WorkspaceCreationHelper::createPeaksWorkspace(6);
    PeaksWorkspace_sptr rhsWS =
        WorkspaceCreationHelper::createPeaksWorkspace(4);

    // Slightly adjust the peaks in one of the workspaces
    auto &rhsPeaks = rhsWS->getPeaks();
    auto &lhsPeaks = lhsWS->getPeaks();

    // Need to change a couple of detector IDs so that I can get peaks with
    // larger |Q_z| than |Q_x|
    lhsPeaks[2].setDetectorID(50);
    lhsPeaks[3].setDetectorID(51);
    rhsPeaks[2].setDetectorID(50);
    rhsPeaks[3].setDetectorID(51);

    // And need to shift some peaks in one workspace to test the delta checking
    // This one will fails to match in x & z
    rhsPeaks[0].setWavelength(rhsPeaks[0].getWavelength() * 1.01);
    // This one matches in z but not in x
    rhsPeaks[1].setWavelength(rhsPeaks[1].getWavelength() * 1.02);
    // This one matches in x but not z
    rhsPeaks[2].setWavelength(rhsPeaks[2].getWavelength() * 1.0335);
    // This one will be matched (to lhsPeaks[0]) and will not appear in the
    // output
    rhsPeaks[3].setWavelength(rhsPeaks[3].getWavelength() * 1.04);

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    DiffPeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", lhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", rhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.08145))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 5)
    TS_ASSERT_EQUALS(ws->getPeak(0).getQLabFrame(),
                     lhsWS->getPeak(0).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(1).getQLabFrame(),
                     lhsWS->getPeak(1).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(2).getQLabFrame(),
                     lhsWS->getPeak(2).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(3).getQLabFrame(),
                     lhsWS->getPeak(4).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(4).getQLabFrame(),
                     lhsWS->getPeak(5).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getInstrument()->baseInstrument(),
                     lhsWS->getInstrument()->baseInstrument())

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};

#endif /* MANTID_CRYSTAL_DIFFPEAKSWORKSPACESTEST_H_ */
