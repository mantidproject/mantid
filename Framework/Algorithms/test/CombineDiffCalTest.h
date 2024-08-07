// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <ctime>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CombineDiffCal.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

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

  /// create table with correct column names
  DataObjects::TableWorkspace_sptr createEmptyCalibrationTable() {
    DataObjects::TableWorkspace_sptr table = std::make_shared<DataObjects::TableWorkspace>();
    table->addColumn("int", "detid");
    table->addColumn("double", "difc");
    table->addColumn("double", "difa");
    table->addColumn("double", "tzero");

    return table;
  }

  DataObjects::TableWorkspace_sptr createPixelCalibrationTableUnsorted() {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = createEmptyCalibrationTable();

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
    DataObjects::TableWorkspace_sptr table = createEmptyCalibrationTable();

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

  DataObjects::TableWorkspace_sptr createGroupedCalibrationTable(const bool fullTable) {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = createEmptyCalibrationTable();

    // fill the values
    //      new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
    TableRow newRow = table->appendRow();
    if (fullTable) { // most tests have values for all pixels
      newRow << 100 << 1000.0 << 0.0 << 0.0;

      newRow = table->appendRow();
      newRow << 101 << 1001.0 << 0.0 << 0.0;
      newRow = table->appendRow();
    }

    newRow << 102 << 1110.0 << 0.0 << 0.0;

    newRow = table->appendRow();
    newRow << 103 << 1110.0 << 0.0 << 0.0;

    return table;
  }

  DataObjects::TableWorkspace_sptr createCalibrationTableArgs() {
    // create table with correct column names
    DataObjects::TableWorkspace_sptr table = createEmptyCalibrationTable();

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
    const auto difCalGroupedCalibration = createGroupedCalibrationTable(true);

    // fake data to simulate CalibrationWorkspace
    const auto diffCalCalibrationWs = createCalibrationWorkspace();

    // set up algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg->execute());
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
    const auto difCalGroupedCalibration = createGroupedCalibrationTable(true);

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
    const auto difCalGroupedCalibration = createGroupedCalibrationTable(true);

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

  void testSingleGroupedSpectrum() {
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
    // detids 100 and 102 will be missing
    const auto difCalGroupedCalibration = createGroupedCalibrationTable(false);

    // fake data to simulate CalibrationWorkspace
    const auto diffCalCalibrationWs = createCalibrationWorkspace();

    // set up algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);

    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());

    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    TS_ASSERT(output);

    // validate the output
    TS_ASSERT_EQUALS(output->rowCount(), 4);

    auto detid = output->getColumn("detid");
    TS_ASSERT_EQUALS(detid->toDouble(0), 100.);
    TS_ASSERT_EQUALS(detid->toDouble(1), 101.);
    TS_ASSERT_EQUALS(detid->toDouble(2), 102.);
    TS_ASSERT_EQUALS(detid->toDouble(3), 103.);

    // double difc = (difcPD /difcArb) * difcPrev;
    // double difa = ((difcPD / difcArb) * (difcPD / difcArb)) * difaPrev;
    auto difc = output->getColumn("difc");
    TS_ASSERT(difc);
    TS_ASSERT_EQUALS(difc->toDouble(0), 1000.);
    TS_ASSERT_EQUALS(difc->toDouble(1), 1001.);
    TS_ASSERT_EQUALS(difc->toDouble(2), (1110. / 1100.) * 1099.);
    TS_ASSERT_EQUALS(difc->toDouble(3), (1110. / 1100.) * 1101.);

    auto difa = output->getColumn("difa");
    TS_ASSERT_EQUALS(difa->toDouble(0), 1.);
    TS_ASSERT_EQUALS(difa->toDouble(1), 2.);
    TS_ASSERT_EQUALS(difa->toDouble(2), ((1110. / 1100.) * (1110. / 1100.)) * 3.);
    TS_ASSERT_EQUALS(difa->toDouble(3), ((1110. / 1100.) * (1110. / 1100.)) * 4.);
    // detids 100 and 102 will be copied from the pixel calibration table
  }

  float doTimedRunWithPixels(std::size_t bank_width) {
    // the detector panel will be square with bank_pixel_width X bank_pixel_width detectors
    std::size_t Npixels = bank_width * bank_width;
    double difcprev = 1.0, difcpd = 2.0, difcarb = 3.0;

    // fake data to simulate the output of cross correlate PixelCalibration
    DataObjects::TableWorkspace_sptr difCalPixelCalibration = createEmptyCalibrationTable();
    for (std::size_t i = 0; i < Npixels; i++) {
      TableRow newRow = difCalPixelCalibration->appendRow();
      // adding i to the DIFC value ensures a proper order for combining
      newRow << int(i + Npixels) << difcprev + double(i) << 0.0 << 0.0;
    }

    // fake data to simulate the output of PDCalibration GroupedCalibration
    DataObjects::TableWorkspace_sptr difCalGroupedCalibration = createEmptyCalibrationTable();
    for (std::size_t i = 0; i < Npixels; i++) {
      TableRow newRow = difCalGroupedCalibration->appendRow();
      // adding i to the DIFC value ensures a proper order for combining
      newRow << int(i + Npixels) << difcpd + double(i) << 0.0 << 0.0;
    }

    // fake data to simulate CalibrationWorkspace
    std::string calWsName = AnalysisDataService::Instance().uniqueName();
    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", calWsName);
    createSampleWorkspaceAlgo.setProperty("NumBanks", 1);
    createSampleWorkspaceAlgo.setProperty("BankPixelWidth", int(bank_width));
    createSampleWorkspaceAlgo.execute();
    // set the workspace's diffraction constants for comparison
    MatrixWorkspace_sptr diffCalCalibrationWs = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");
    auto experimentInfo = std::dynamic_pointer_cast<API::ExperimentInfo>(diffCalCalibrationWs);
    auto instrument = experimentInfo->getInstrument();
    auto &paramMap = experimentInfo->instrumentParameters();
    auto detids = instrument->getDetectorIDs();
    std::sort(detids.begin(), detids.end());
    for (std::size_t i = 0; i < Npixels; i++) {
      auto det = instrument->getDetector(detids[i]);
      // adding i to the DIFC value ensures a proper order for combining
      paramMap.addDouble(det->getComponentID(), "DIFC", difcarb + double(i));
      paramMap.addDouble(det->getComponentID(), "DIFA", 0.0);
      paramMap.addDouble(det->getComponentID(), "TZERO", 0.0);
    }

    // set up and run the algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);
    // run and time the algorithm
    clock_t start = clock();
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());
    clock_t end = clock();
    float total = static_cast<float>(end - start);
    TS_ASSERT_LESS_THAN(0.0, total);

    // check the difcnew values
    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    std::vector<double> difcout = output->getColVector<double>("difc");
    for (std::size_t i = 0; i < Npixels; i++) {
      double I = double(i);
      TS_ASSERT_EQUALS(difcout[i], (difcpd + I) / (difcarb + I) * (difcprev + I));
    }

    // return the measured time
    return total;
  }

  void test_time_scaling() {
    // run with increasing numbers of pixels
    // check that the time scales better than quadratic, i.e. better than O(N^2)
    constexpr std::size_t Npoints = 3;
    std::size_t x[Npoints]; // the number of pixels
    float y[Npoints];

    for (std::size_t i = 0; i < Npoints; i++) {
      std::size_t bank_width = (i + 1) * 100UL;
      x[i] = bank_width * bank_width;
      y[i] = doTimedRunWithPixels(bank_width);
    }

    // make a quadratic y = A(x-x0)^2 + C  (ignore linear term)
    // use it to predict the final time under O(N^2) scaling
    // ensure the final time is less than this
    double deltax = double(x[1] - x[0]), deltay = (y[1] - y[0]);
    double quadratic_C = y[0];
    double quadratic_A = deltay / deltax / deltax;
    deltax = double(x[Npoints - 1] - x[0]);
    double quadratic_prediction = quadratic_A * deltax * deltax + quadratic_C;
    TS_ASSERT_LESS_THAN(y[Npoints - 1], quadratic_prediction);
  }

  void test_correct_matching_detids() {
    // Ensure the correct detector IDs are being matched up
    // create workspaces with these detectors:
    //  ARB: 4, 5, 6, 7
    //   PD:    5,    7
    // PREV: 4, 5, 6, 7
    // in the result, we expect 5, 6, then the algo adds in 4, 7 from PREV

    std::size_t bank_width = 2;

    // Create the GroupedCalibration
    DataObjects::TableWorkspace_sptr difCalGroupedCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {5, 7}) {
      TableRow newRow = difCalGroupedCalibration->appendRow();
      newRow << int(i) << double(i) << 0.0 << 0.0;
    }

    // Create the PixelCalibration
    DataObjects::TableWorkspace_sptr difCalPixelCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {4, 5, 6, 7}) {
      TableRow newRow = difCalPixelCalibration->appendRow();
      newRow << int(i) << double(i) << 0.0 << 0.0;
    }

    // Create a CalibrationWorkspace
    std::string calWsName = AnalysisDataService::Instance().uniqueName();
    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", calWsName);
    createSampleWorkspaceAlgo.setProperty("NumBanks", 1);
    createSampleWorkspaceAlgo.setProperty("BankPixelWidth", int(bank_width));
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr diffCalCalibrationWs = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");
    auto experimentInfo = std::dynamic_pointer_cast<API::ExperimentInfo>(diffCalCalibrationWs);
    auto instrument = experimentInfo->getInstrument();
    auto &paramMap = experimentInfo->instrumentParameters();
    auto detids = instrument->getDetectorIDs();
    std::sort(detids.begin(), detids.end());
    for (detid_t detid : detids) {
      auto det = instrument->getDetector(detid);
      paramMap.addDouble(det->getComponentID(), "DIFC", double(detid));
      paramMap.addDouble(det->getComponentID(), "DIFA", 0.0);
      paramMap.addDouble(det->getComponentID(), "TZERO", 0.0);
    }

    // set up and run algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // check the difcnew values
    // difc_new = (difc_pd / difc_arb) * difc_prev = (detid / detid) * detid = detid
    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    std::vector<detid_t> detid_out = output->getColVector<detid_t>("detid");
    std::vector<detid_t> detid_exp{4, 5, 6, 7};
    std::vector<double> difc_out = output->getColVector<double>("difc");
    TS_ASSERT_EQUALS(detid_out.size(), detid_exp.size());
    for (std::size_t i = 0; i < detid_out.size(); i++) {
      TS_ASSERT_EQUALS(detid_out[i], detid_exp[i]);
      TS_ASSERT_EQUALS(difc_out[i], double(detid_exp[i]));
    }
  }

  void test_missing_pixel_cal_copies_group_cal() {
    // Ensure the algorithm will simply copy missing rows from GroupedCalibration
    // if they are not found inside PixelCalibraion
    // create workspaces with these detectors:
    //  ARB: 4, 5, 6, 7
    //   PD: 4, 5, 6, 7
    // PREV:    5, 6

    std::size_t bank_width = 2;
    double difc_pd = 2.0, difc_prev = 3.0, difc_arb = 4.0;
    double difc_new = (difc_pd / difc_arb) * difc_prev;

    // First create the GroupedCalibration
    DataObjects::TableWorkspace_sptr difCalGroupedCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {4, 5, 6, 7}) {
      TableRow newRow = difCalGroupedCalibration->appendRow();
      newRow << int(i) << difc_pd << 0.0 << 0.0;
    }

    // Create a compatible PixelCalibration
    DataObjects::TableWorkspace_sptr difCalPixelCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {5, 6}) {
      TableRow newRow = difCalPixelCalibration->appendRow();
      newRow << int(i) << difc_prev << 0.0 << 0.0;
    }

    // Create a CalibrationWorkspace
    std::string calWsName = AnalysisDataService::Instance().uniqueName();
    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", calWsName);
    createSampleWorkspaceAlgo.setProperty("NumBanks", 1);
    createSampleWorkspaceAlgo.setProperty("BankPixelWidth", int(bank_width));
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr diffCalCalibrationWs = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");
    auto experimentInfo = std::dynamic_pointer_cast<API::ExperimentInfo>(diffCalCalibrationWs);
    auto instrument = experimentInfo->getInstrument();
    auto &paramMap = experimentInfo->instrumentParameters();
    auto detids = instrument->getDetectorIDs();
    std::sort(detids.begin(), detids.end());
    for (detid_t detid : detids) {
      auto det = instrument->getDetector(detid);
      paramMap.addDouble(det->getComponentID(), "DIFC", difc_arb);
      paramMap.addDouble(det->getComponentID(), "DIFA", 0.0);
      paramMap.addDouble(det->getComponentID(), "TZERO", 0.0);
    }

    // set up and run algorithm
    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // check the difcnew values
    DataObjects::TableWorkspace_sptr output = alg->getProperty("OutputWorkspace");
    std::vector<detid_t> detid_out = output->getColVector<detid_t>("detid");
    std::vector<detid_t> detid_exp{4, 5, 6, 7};
    std::vector<double> difc_out = output->getColVector<double>("difc");
    std::vector<double> difc_exp{difc_pd, difc_new, difc_new, difc_pd};
    TS_ASSERT_EQUALS(detid_out.size(), detid_exp.size());
    for (std::size_t i = 0; i < detid_out.size(); i++) {
      TS_ASSERT_EQUALS(detid_out[i], detid_exp[i]);
      TS_ASSERT_EQUALS(difc_out[i], difc_exp[i]);
    }
  }

  void test_validateInputs_in_pixel_not_cal() {
    // Ensure the algorithm will fail early if the pixels in the PixelCalibration
    // are not present in the CalibrationWorkspace
    // create workspaces with these detectors:
    //  ARB:    4, 5, 6, 7
    //   PD:    4, 5, 6, 7
    // PREV: 1, 4, 5, 6, 7

    std::size_t bank_width = 2;

    // First create the GroupedCalibration
    DataObjects::TableWorkspace_sptr difCalGroupedCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {4, 5, 6, 7}) {
      TableRow newRow = difCalGroupedCalibration->appendRow();
      newRow << int(i) << 1.0 << 0.0 << 0.0;
    }

    // Create a compatible PixelCalibration
    DataObjects::TableWorkspace_sptr difCalPixelCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {1, 4, 5, 6, 7}) {
      TableRow newRow = difCalPixelCalibration->appendRow();
      newRow << int(i) << 1.0 << 0.0 << 0.0;
    }

    // Create a CalibrationWorkspace
    std::string calWsName = AnalysisDataService::Instance().uniqueName();
    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", calWsName);
    createSampleWorkspaceAlgo.setProperty("NumBanks", 1);
    createSampleWorkspaceAlgo.setProperty("BankPixelWidth", int(bank_width));
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr diffCalCalibrationWs = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    TS_ASSERT(!alg->isExecuted());
    auto result = alg->validateInputs();
    TS_ASSERT(!result.count("GroupedCalibration"));
    TS_ASSERT(result.count("PixelCalibration"));
    TS_ASSERT(result.count("CalibrationWorkspace"));
  }

  void test_validateInputs_in_grouped_not_cal() {
    // Ensure the algorithm will fail early if the pixels in the GroupedCalibration
    // are not present in the CalibrationWorkspace
    // create workspaces with these detectors:
    //  ARB:    4, 5, 6, 7
    //   PD: 1, 4, 5, 6, 7
    // PREV:    4, 5, 6, 7

    std::size_t bank_width = 2;

    // First create the GroupedCalibration
    DataObjects::TableWorkspace_sptr difCalGroupedCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {1, 4, 5, 6, 7}) {
      TableRow newRow = difCalGroupedCalibration->appendRow();
      newRow << int(i) << 1.0 << 0.0 << 0.0;
    }

    // Create a compatible PixelCalibration
    DataObjects::TableWorkspace_sptr difCalPixelCalibration = createEmptyCalibrationTable();
    for (std::size_t i : {4, 5, 6, 7}) {
      TableRow newRow = difCalPixelCalibration->appendRow();
      newRow << int(i) << 1.0 << 0.0 << 0.0;
    }

    // Create a CalibrationWorkspace with detector IDs 4, 5, 6, 7, and NOT 1
    std::string calWsName = AnalysisDataService::Instance().uniqueName();
    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", calWsName);
    createSampleWorkspaceAlgo.setProperty("NumBanks", 1);
    createSampleWorkspaceAlgo.setProperty("BankPixelWidth", int(bank_width));
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr diffCalCalibrationWs = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    auto alg = setupAlg(difCalPixelCalibration, difCalGroupedCalibration, diffCalCalibrationWs);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    TS_ASSERT(!alg->isExecuted());
    auto result = alg->validateInputs();
    TS_ASSERT(result.count("GroupedCalibration"));
    TS_ASSERT(!result.count("PixelCalibration"));
    TS_ASSERT(result.count("CalibrationWorkspace"));
  }
};
