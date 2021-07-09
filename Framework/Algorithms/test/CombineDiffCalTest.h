// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CombineDiffCal.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataHandling/ApplyDiffCal.h"
#include "MantidDataHandling/GroupDetectors2.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Algorithms::CombineDiffCal;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::API::TableRow;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;

class CombineDiffCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CombineDiffCalTest *createSuite() { return new CombineDiffCalTest(); }
  static void destroySuite(CombineDiffCalTest *suite) { delete suite; }

  CombineDiffCalTest() { FrameworkManager::Instance(); }

  DataObjects::TableWorkspace_sptr createPixelCalibrationTableUnsorted() {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = std::make_shared<DataObjects::TableWorkspace>();
    table->addColumn("int", "detid");
    table->addColumn("double", "difc");
    table->addColumn("double", "difa");
    table->addColumn("double", "tzero");

    // fill the values
    //      new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
    TableRow newRow = table->appendRow();
    newRow << 103 << 1101.0 << 4.0 << 0.0;

    newRow = table->appendRow();
    newRow << 100 << 1000.0 << 1.0 << 0.0;

    newRow = table->appendRow();
    newRow << 101 << 1001.0 << 2.0 << 0.0;

    newRow = table->appendRow();
    newRow << 102 << 1099.0 << 3.0 << 0.0;

    return table;
  }

  DataObjects::TableWorkspace_sptr createPixelCalibrationTable() {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = std::make_shared<DataObjects::TableWorkspace>();
    table->addColumn("int", "detid");
    table->addColumn("double", "difc");
    table->addColumn("double", "difa");
    table->addColumn("double", "tzero");

    // fill the values
    //      new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
    TableRow newRow = table->appendRow();
    newRow << 100 << 1000.0 << 1.0 << 0.0;

    newRow = table->appendRow();
    newRow << 101 << 1001.0 << 2.0 << 0.0;

    newRow = table->appendRow();
    newRow << 102 << 1099.0 << 3.0 << 0.0;

    newRow = table->appendRow();
    newRow << 103 << 1101.0 << 4.0 << 0.0;

    return table;
  }

  DataObjects::TableWorkspace_sptr createGroupedCalibrationTable() {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = std::make_shared<DataObjects::TableWorkspace>();
    table->addColumn("int", "detid");
    table->addColumn("double", "difc");
    table->addColumn("double", "difa");
    table->addColumn("double", "tzero");

    // fill the values
    //      new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
    TableRow newRow = table->appendRow();
    newRow << 100 << 1000.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 101 << 1001.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 102 << 1110.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 103 << 1110.0 << 0.0 << 0.0;

    return table;
  }

  DataObjects::TableWorkspace_sptr createCalibrationTableArgs() {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = std::make_shared<DataObjects::TableWorkspace>();
    table->addColumn("int", "detid");
    table->addColumn("double", "difc");
    table->addColumn("double", "difa");
    table->addColumn("double", "tzero");

    // fill the values
    //      new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
    TableRow newRow = table->appendRow();
    newRow << 100 << 1000.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 101 << 1000.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 102 << 1100.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 103 << 1100.0 << 0.0 << 0.0;

    return table;
  }

  MatrixWorkspace_sptr getInstrumentWorkspace() {
    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "outWSName");
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr instrumentWS = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    GroupDetectors2 groupDetectorsAlgo;
    groupDetectorsAlgo.setChild(true);
    groupDetectorsAlgo.initialize();
    groupDetectorsAlgo.setProperty("InputWorkspace", instrumentWS);
    groupDetectorsAlgo.setProperty("GroupingPattern", "0+1,2+3");
    groupDetectorsAlgo.setPropertyValue("OutputWorkspace", "outWSName");
    groupDetectorsAlgo.execute();

    instrumentWS = groupDetectorsAlgo.getProperty("OutputWorkspace");
    return instrumentWS;
  }

  MatrixWorkspace_sptr createCalibrationWorkspace() {
    MatrixWorkspace_sptr instrumentWS = getInstrumentWorkspace();
    const auto calibrationArgsTable = createCalibrationTableArgs();

    std::string testWorkspaceName = "TestWorkspace";
    AnalysisDataService::Instance().add(testWorkspaceName, instrumentWS);
    const auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(testWorkspaceName);

    ApplyDiffCal applyDiffCalAlgo;
    applyDiffCalAlgo.setChild(true);
    applyDiffCalAlgo.initialize();
    applyDiffCalAlgo.setProperty("InstrumentWorkspace", testWorkspaceName);
    applyDiffCalAlgo.setProperty("CalibrationWorkspace", calibrationArgsTable);
    applyDiffCalAlgo.execute();

    AnalysisDataService::Instance().remove(testWorkspaceName);
    return outWS;
  }

  DataObjects::MaskWorkspace_sptr getMaskWorkspace() {
    MatrixWorkspace_sptr instrumentWS = getInstrumentWorkspace();
    // In case of MaskWorkspace
    DataObjects::MaskWorkspace_sptr maskWS = std::make_shared<DataObjects::MaskWorkspace>(instrumentWS);

    maskWS->setMasked(100, true);
    maskWS->setMasked(101, true);

    return maskWS;
  }

  Mantid::API::IAlgorithm_sptr setupAlg(DataObjects::TableWorkspace_sptr difCalPixelCalibration,
                                        DataObjects::TableWorkspace_sptr difCalGroupedCalibration,
                                        MatrixWorkspace_sptr diffCalCalibrationWs) {
    // set up algorithm
    auto alg = std::make_shared<CombineDiffCal>();
    alg->setChild(true); // Don't put output in ADS by default
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("PixelCalibration", difCalPixelCalibration));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("GroupedCalibration", difCalGroupedCalibration));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("CalibrationWorkspace", diffCalCalibrationWs));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "_unused_for_child"));
    return alg;
  }

  void confirmResults(DataObjects::TableWorkspace_sptr output) {
    // double difc = (difcPD /difcArb) * difcPrev;
    // double difa = ((difcPD / difcArb) * (difcPD / difcArb)) * difaPrev;
    auto difc = output->getColumn("difc");
    TS_ASSERT(difc);
    TS_ASSERT_EQUALS(difc->toDouble(0), (1000. / 1000.) * 1000.);
    TS_ASSERT_EQUALS(difc->toDouble(1), (1001. / 1000.) * 1001.);
    TS_ASSERT_EQUALS(difc->toDouble(2), (1110. / 1100.) * 1099.);
    TS_ASSERT_EQUALS(difc->toDouble(3), (1110. / 1100.) * 1101.);

    auto difa = output->getColumn("difa");
    TS_ASSERT_EQUALS(difa->toDouble(0), ((1000. / 1000.) * (1000. / 1000.)) * 1.);
    TS_ASSERT_EQUALS(difa->toDouble(1), ((1001. / 1000.) * (1001. / 1000.)) * 2.);
    TS_ASSERT_EQUALS(difa->toDouble(2), ((1110. / 1100.) * (1110. / 1100.)) * 3.);
    TS_ASSERT_EQUALS(difa->toDouble(3), ((1110. / 1100.) * (1110. / 1100.)) * 4.);
  }

  void confirmMaskedResults(DataObjects::TableWorkspace_sptr output) {
    // double difc = (difcPD /difcArb) * difcPrev;
    // double difa = ((difcPD / difcArb) * (difcPD / difcArb)) * difaPrev;
    auto difc = output->getColumn("difc");
    TS_ASSERT(difc);

    // 1st and 2nd are both masked, since they are grouped together, will take groupcalibration val instead
    TS_ASSERT_EQUALS(difc->toDouble(0), 1000.);
    TS_ASSERT_EQUALS(difc->toDouble(1), 1001);

    TS_ASSERT_EQUALS(difc->toDouble(2), (1110. / 1100.) * 1099.);
    TS_ASSERT_EQUALS(difc->toDouble(3), (1110. / 1100.) * 1101.);

    auto difa = output->getColumn("difa");
    // 1st and 2nd are both masked, since they are grouped together, will take groupcalibration val instead
    TS_ASSERT_EQUALS(difa->toDouble(0), 0);
    TS_ASSERT_EQUALS(difa->toDouble(1), 0);

    TS_ASSERT_EQUALS(difa->toDouble(2), ((1110. / 1100.) * (1110. / 1100.)) * 3.);
    TS_ASSERT_EQUALS(difa->toDouble(3), ((1110. / 1100.) * (1110. / 1100.)) * 4.);
  }

  void test_init() {
    CombineDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // cases to cover (can be in the same dataset)
    // single pixel with pixel==group==arb
    // single pixel with pixel==arb!=group
    // single pixel with pixel==arb!=group
    // grouped with arb==group
    // grouped with arb!=group

    // test input

    // fake data to simulate the output of cross correlate PixelCalibration
    const auto difCalPixelCalibration = createPixelCalibrationTable();

    // fake data to simulate the output of PDCalibration GroupedCalibration
    const auto difCalGroupedCalibration = createGroupedCalibrationTable();

    // fake data to simulate CalibrationWorkspace
    const auto diffCalCalibrationWs = createCalibrationWorkspace();

    // set up algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    TS_ASSERT(output);

    confirmResults(output);
  }

  void testUnsorted() {
    // test input

    // fake data to simulate the output of cross correlate PixelCalibration
    const auto difCalPixelCalibration = createPixelCalibrationTableUnsorted();

    // fake data to simulate the output of PDCalibration GroupedCalibration
    const auto difCalGroupedCalibration = createGroupedCalibrationTable();

    // fake data to simulate CalibrationWorkspace
    const auto diffCalCalibrationWs = createCalibrationWorkspace();

    // set up algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    TS_ASSERT(output);

    confirmResults(output);
  }

  void testMasked() {

    // test input

    // fake data to simulate the output of cross correlate PixelCalibration
    const auto difCalPixelCalibration = createPixelCalibrationTable();

    // fake data to simulate the output of PDCalibration GroupedCalibration
    const auto difCalGroupedCalibration = createGroupedCalibrationTable();

    // fake data to simulate CalibrationWorkspace
    const auto diffCalCalibrationWs = createCalibrationWorkspace();

    const auto maskWorkspace = getMaskWorkspace();

    TS_ASSERT(maskWorkspace->isMasked(100));

    // set up algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("MaskWorkspace", maskWorkspace));

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    TS_ASSERT(output);

    confirmMaskedResults(output);
  }
};
