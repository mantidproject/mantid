// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CLONEWORKSPACETEST_H_
#define CLONEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::MDEventsTestHelper::makeMDEW;

class CloneWorkspaceTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(cloner.name(), "CloneWorkspace"); }

  void testVersion() { TS_ASSERT_EQUALS(cloner.version(), 1); }

  void testCategory() {
    TS_ASSERT_EQUALS(cloner.category(), "Utility\\Workspaces");
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(cloner.initialize());
    TS_ASSERT(cloner.isInitialized());
  }

  void testExec() {
    if (!cloner.isInitialized())
      cloner.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace", "in");
    loader.execute();

    MatrixWorkspace_sptr in =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("in");
    // Input file does not contain Dx, we just add it for testing.
    HistogramData::PointStandardDeviations dx(in->readX(0).size() - 1);
    for (size_t i = 0; i < in->getNumberHistograms(); ++i)
      in->setPointStandardDeviations(i, dx);

    TS_ASSERT_THROWS_NOTHING(cloner.setPropertyValue("InputWorkspace", "in"));
    TS_ASSERT_THROWS_NOTHING(cloner.setPropertyValue("OutputWorkspace", "out"));

    TS_ASSERT(cloner.execute());

    // Check Dx vectors are shared on both the input and output workspaces
    // Check sharing of the Dx vectors has not been broken
    TSM_ASSERT_EQUALS("Dx vectors should be shared between spectra by default "
                      "(after a LoadRaw)",
                      in->sharedDx(0), in->sharedDx(1));
    MatrixWorkspace_const_sptr out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    // Check sharing of the Dx vectors has not been broken
    TSM_ASSERT_EQUALS(
        "Dx vectors should remain shared between spectra after CloneWorkspace",
        out->sharedDx(0), out->sharedDx(1));

    // Best way to test this is to use the CompareWorkspaces algorithm
    Mantid::Algorithms::CompareWorkspaces checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1", "in");
    checker.setPropertyValue("Workspace2", "out");
    checker.execute();

    TS_ASSERT(checker.getProperty("Result"));
  }

  void testExecEvent() {
    // First make the algorithm
    EventWorkspace_sptr ew =
        WorkspaceCreationHelper::createEventWorkspace(100, 60, 50);
    AnalysisDataService::Instance().addOrReplace("in_event", ew);

    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "in_event");
    alg.setPropertyValue("OutputWorkspace", "out_event");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Best way to test this is to use the CompareWorkspaces algorithm
    Mantid::Algorithms::CompareWorkspaces checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1", "in_event");
    checker.setPropertyValue("Workspace2", "out_event");
    checker.execute();

    TS_ASSERT(checker.getProperty("Result"));
  }

  /** Test is not full, see CloneMDWorkspaceTest */
  void test_exec_MDEventWorkspace() {
    MDEventWorkspace3Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<3>(5, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("inWS", ws);
    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "inWS");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  /** Build a test PeaksWorkspace
   * @return PeaksWorkspace   */
  PeaksWorkspace_sptr buildPW() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    PeaksWorkspace *pw = new PeaksWorkspace();
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return PeaksWorkspace_sptr(pw);
  }

  /** Test is not full, see PeaksWorkspaceTest */
  void test_exec_PeaksWorkspace() {
    PeaksWorkspace_sptr ws = buildPW();
    AnalysisDataService::Instance().addOrReplace("inWS", ws);
    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "inWS");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  void test_group() {
    WorkspaceGroup_const_sptr ingroup =
        WorkspaceCreationHelper::createWorkspaceGroup(3, 1, 1, "grouptoclone");
    Mantid::Algorithms::CloneWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "grouptoclone");
    alg.setPropertyValue("OutputWorkspace", "clonedgroup");
    TS_ASSERT(alg.execute())

    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    Workspace_sptr out;
    TS_ASSERT_THROWS_NOTHING(out = ads.retrieve("clonedgroup"))
    WorkspaceGroup_const_sptr outgroup;
    TS_ASSERT(outgroup = boost::dynamic_pointer_cast<WorkspaceGroup>(out))
    TS_ASSERT_EQUALS(outgroup->size(), 3)
    TS_ASSERT_DIFFERS(outgroup, ingroup)
    Workspace_sptr out1, out2, out3;
    // Try to get the first member
    TS_ASSERT_THROWS_NOTHING(out1 = outgroup->getItem(0))
    // Check its name
    TS_ASSERT_EQUALS(out1->getName(), "clonedgroup_1")
    // Check it is indeed a different workspace
    TS_ASSERT_DIFFERS(out1, ingroup->getItem(0))
    // Try to get the second member
    TS_ASSERT_THROWS_NOTHING(out2 = outgroup->getItem(1))
    // Check its name
    TS_ASSERT_EQUALS(out2->getName(), "clonedgroup_2")
    // Check it is indeed a different workspace
    TS_ASSERT_DIFFERS(out2, ingroup->getItem(1))
    // Try to get the third member
    TS_ASSERT_THROWS_NOTHING(out3 = outgroup->getItem(2))
    // Check its name
    TS_ASSERT_EQUALS(out3->getName(), "clonedgroup_3")
    // Check it is indeed a different workspace
    TS_ASSERT_DIFFERS(out3, ingroup->getItem(2))

    ads.clear();
  }

  /** Test cloning a TableWorkspace
   */
  void test_exec_TableWorkspace() {
    // 1. Create input table workspace
    auto inpWS = boost::make_shared<TableWorkspace>();

    inpWS->addColumn("str", "Name");
    inpWS->addColumn("double", "Value");
    inpWS->addColumn("int", "Step");

    TableRow row0 = inpWS->appendRow();
    row0 << "S1" << 12.34 << 0;
    TableRow row1 = inpWS->appendRow();
    row1 << "S2" << 23.45 << 1;
    TableRow row2 = inpWS->appendRow();
    row2 << "S3" << 34.56 << 2;

    AnalysisDataService::Instance().addOrReplace("TestTableIn", inpWS);

    // 2. Execute
    Algorithms::CloneWorkspace alg;

    alg.initialize();

    alg.setProperty("InputWorkspace", "TestTableIn");
    alg.setProperty("OutputWorkspace", "TestTableOut");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outWS = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("TestTableOut"));
    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(inpWS->columnCount(), outWS->columnCount());
    TS_ASSERT_EQUALS(inpWS->rowCount(), outWS->rowCount());

    TS_ASSERT_EQUALS(inpWS->cell<std::string>(0, 0), "S1");
    TS_ASSERT_DELTA(inpWS->cell<double>(1, 1), 23.45, 0.000001);
    TS_ASSERT_EQUALS(inpWS->cell<int>(2, 2), 2);

    return;
  }

private:
  Mantid::Algorithms::CloneWorkspace cloner;
};

#endif /*CLONEWORKSPACETEST_H_*/
