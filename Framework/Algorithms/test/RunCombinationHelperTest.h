#ifndef MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_
#define MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::RunCombinationHelper;
using Mantid::Algorithms::GroupWorkspaces;
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

  void setUp() override {
    m_reference =
        create2DWorkspaceWithFullInstrument(2, 3, true, false, true, "test");
    setUnits(m_reference);
    m_testee.setReferenceProperties(m_reference);
  }

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

  void testCompatible() {
    MatrixWorkspace_sptr ws = m_reference->clone();
    TS_ASSERT(m_testee.checkCompatibility(ws).empty());
  }

  void testIncompatibleInstrument() {
    MatrixWorkspace_sptr ws =
        create2DWorkspaceWithFullInstrument(2, 3, true, false, true, "other");
    setUnits(ws);
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws),
                     "different instrument names; ");
  }

  void testIncompatibleNumHistograms() {
    MatrixWorkspace_sptr ws =
        create2DWorkspaceWithFullInstrument(3, 3, true, false, true, "test");
    setUnits(ws);
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws, true),
                     "different number of histograms; ");
    TS_ASSERT(m_testee.checkCompatibility(ws).empty());
  }

  void testIncompatibleDataType() {
    MatrixWorkspace_sptr ws =
        create2DWorkspaceWithFullInstrument(2, 3, true, false, false, "test");
    setUnits(ws);
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws),
                     "different distribution or histogram type; ");
  }

  void testIncompatibleXUnits() {
    MatrixWorkspace_sptr ws = m_reference->clone();
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("Energy");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws), "different X units; ");
  }

  void testIncompatibleYUnits() {
    MatrixWorkspace_sptr ws = m_reference->clone();
    ws->setYUnit("Frequency");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws), "different Y units; ");
  }

  void testIncompatibleSpectrumAxisUnits() {
    MatrixWorkspace_sptr ws = m_reference->clone();
    ws->getAxis(1)->unit() = UnitFactory::Instance().create("QSquared");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws),
                     "different spectrum axis units; ");
  }

  void testIncompatibleMultiple() {
    MatrixWorkspace_sptr ws = m_reference->clone();
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("Energy");
    ws->getAxis(1)->unit() = UnitFactory::Instance().create("QSquared");
    TS_ASSERT_EQUALS(m_testee.checkCompatibility(ws),
                     "different X units; different spectrum axis units; ");
  }

private:
  void setUnits(MatrixWorkspace_sptr ws) {
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    ws->getAxis(1)->unit() = UnitFactory::Instance().create("Momentum");
    ws->setYUnit("Counts");
  }

  RunCombinationHelper m_testee;
  MatrixWorkspace_sptr m_reference;
};

#endif /* MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_ */
