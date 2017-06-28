#ifndef MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_
#define MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::RunCombinationHelper;
using Mantid::Algorithms::GroupWorkspaces;
using Mantid::Algorithms::CreateSampleWorkspace;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

class RunCombinationHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunCombinationHelperTest *createSuite() {
    return new RunCombinationHelperTest();
  }
  static void destroySuite(RunCombinationHelperTest *suite) { delete suite; }

  void testUnwraping() {

    MatrixWorkspace_sptr ws1 = create2DWorkspace(2, 3);
    MatrixWorkspace_sptr ws2 = create2DWorkspace(1, 4);
    MatrixWorkspace_sptr ws3 = create2DWorkspace(5, 6);

    storeWS("ws1", ws1);
    storeWS("ws2", ws2);
    storeWS("ws3", ws3);

    GroupWorkspaces grouper;
    grouper.initialize();
    grouper.setProperty("InputWorkspaces",
                        std::vector<std::string>{"ws1", "ws2"});
    grouper.setProperty("OutputWorkspace", "ws12");
    grouper.execute();

    auto flatVector =
        m_testee.unWrapGroups(std::vector<std::string>{"ws12", "ws3"});
    TS_ASSERT_EQUALS(flatVector[0], "ws1");
    TS_ASSERT_EQUALS(flatVector[1], "ws2");
    TS_ASSERT_EQUALS(flatVector[2], "ws3");

    removeWS("ws1");
    removeWS("ws2");
    removeWS("ws3");
  }

  void testCompatibility() {

    MatrixWorkspace_sptr ws1 =
        create2DWorkspaceWithFullInstrument(2, 3, true, false, true, "test");
    m_testee.setReferenceProperties(ws1);

    // compatible
    MatrixWorkspace_sptr ws2 = ws1->clone();
    TS_ASSERT(m_testee.checkCompatibility(ws2).empty());

    // incompatible instrument
    MatrixWorkspace_sptr ws3 =
        create2DWorkspaceWithFullInstrument(2, 3, true, false, true, "other");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws3),
                     "different instrument names; ");

    // incompatible number of histograms
    MatrixWorkspace_sptr ws4 =
        create2DWorkspaceWithFullInstrument(3, 3, true, false, true, "test");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws4, true),
                     "different number of histograms; ");
    TS_ASSERT(m_testee.checkCompatibility(ws4).empty());

    // point data
    MatrixWorkspace_sptr ws5 =
        create2DWorkspaceWithFullInstrument(2, 3, true, false, false, "test");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws5),
                     "different distribution or histogram type; ");

    // setup units of the reference
    ws1->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    ws1->getAxis(1)->unit() = UnitFactory::Instance().create("Momentum");
    ws1->setYUnit("Counts");
    m_testee.setReferenceProperties(ws1);

    // incompatible x-axis unit
    MatrixWorkspace_sptr ws6 = ws1->clone();
    ws6->getAxis(0)->unit() = UnitFactory::Instance().create("Energy");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws6), "different X units; ");

    // incompatible spectrum-axis unit
    MatrixWorkspace_sptr ws7 = ws1->clone();
    ws7->getAxis(1)->unit() = UnitFactory::Instance().create("QSquared");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws7),
                     "different spectrum axis units; ");

    // incompatible y unit
    MatrixWorkspace_sptr ws8 = ws1->clone();
    ws8->setYUnit("Frequency");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws8), "different Y units; ");
  }

  void test_scanning_workspaces_throw_no_error() {
    const auto scanWS1 = createSampleScanningWorkspace(2);
    const auto scanWS2 = createSampleScanningWorkspace(2);

    m_testee.setReferenceProperties(scanWS1);
    TS_ASSERT(m_testee.checkCompatibility(scanWS2).empty());
  }

  void test_mix_of_scanning_and_non_scanning_workspaces_throws_error() {
    const auto scanWS = createSampleScanningWorkspace(2);
    const auto nonScanWS = createSampleScanningWorkspace(1);

    m_testee.setReferenceProperties(scanWS);
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(nonScanWS),
                     "a mix of workspaces with and without detector scans; ");
  }

  void
  test_scanning_workspaces_with_different_numbers_of_detectors_throws_error() {
    const auto scanWS1 = createSampleScanningWorkspace(2);
    const auto scanWS2 = createSampleScanningWorkspace(2, 5);

    TS_ASSERT_EQUALS(scanWS1->detectorInfo().size(), 200)
    TS_ASSERT_EQUALS(scanWS2->detectorInfo().size(), 205)

    m_testee.setReferenceProperties(scanWS1);
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(scanWS2),
                     "workspaces with detectors scans have different number of "
                     "detectors; ");
  }

private:
  RunCombinationHelper m_testee;

  MatrixWorkspace_sptr createSampleScanningWorkspace(int nTimeIndexes,
                                                     int nMonitors = 0) {
    FrameworkManager::Instance();

    CreateSampleWorkspace wsAlg;
    wsAlg.initialize();
    wsAlg.setChild(true);
    wsAlg.setProperty("NumScanPoints", nTimeIndexes);
    wsAlg.setProperty("NumMonitors", nMonitors);
    wsAlg.setPropertyValue("OutputWorkspace", "__out");
    wsAlg.execute();

    return wsAlg.getProperty("OutputWorkspace");
  }
};

#endif /* MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_ */
