#ifndef MANTID_ALGORITHMS_MUONWORKSPACECREATIONHELPERTEST_H_
#define MANTID_ALGORITHMS_MUONWORKSPACECREATIONHELPERTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MuonWorkspaceCreationHelper;
// using namespace Mantid::DataObjects;

/** Test class for the helpers in
 * MantidTestHelpers/MuonWorkspaceCreationHelper.h */
class MuonWorkspaceCreationHelperTest : public CxxTest::TestSuite {
public:
  void test_yDataAsymmetry_gives_expected_numbers() {
    auto yData = MuonWorkspaceCreationHelper::yDataAsymmetry();

    TS_ASSERT_DELTA(yData(0.0, 1), 24.78, 0.01);
    TS_ASSERT_DELTA(yData(0.5, 1), 0.000, 0.01);
    TS_ASSERT_DELTA(yData(1.0, 1), 10.55, 0.01);
    // spectra 5
    TS_ASSERT_DELTA(yData(0.0, 5), 70.54, 0.01);
    TS_ASSERT_DELTA(yData(0.5, 5), 0.000, 0.01);
    TS_ASSERT_DELTA(yData(1.0, 5), 38.21, 0.01);
  };

  void test_yDataAsymmetry_with_custom_amplitude_gives_expected_numbers() {
    auto yData = MuonWorkspaceCreationHelper::yDataAsymmetry(10.0, 0.1);

    TS_ASSERT_DELTA(yData(0.0, 1), 108.56, 0.01);
    TS_ASSERT_DELTA(yData(0.5, 1), 0.000, 0.01);
    TS_ASSERT_DELTA(yData(1.0, 1), 34.36, 0.01);
    // spectra 5
    TS_ASSERT_DELTA(yData(0.0, 5), 300.26, 0.01);
    TS_ASSERT_DELTA(yData(0.5, 5), 0.000, 0.01);
    TS_ASSERT_DELTA(yData(1.0, 5), 146.88, 0.01);
  };

  void test_yDataCounts_gives_expected_numbers() {
    auto yData = MuonWorkspaceCreationHelper::yDataCounts();

    // arguments of yData are not used
    TS_ASSERT_EQUALS(yData(0.0, 1), 0);
    TS_ASSERT_EQUALS(yData(0.0, 1), 1);
    TS_ASSERT_EQUALS(yData(0.0, 1), 2);
    TS_ASSERT_EQUALS(yData(0.0, 1), 3);
  };

  void test_createCountsWorkspace_number_histograms_correct() {
    MatrixWorkspace_sptr ws = createCountsWorkspace(25, 2, 0.0);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 25);
  }

  void test_createCountsWorkspace_detectorIDs_correct() {
    MatrixWorkspace_sptr ws = createCountsWorkspace(2, 10, 0.0);

    const std::set<detid_t> detids0 = ws->getSpectrum(0).getDetectorIDs();
    const std::set<detid_t> detids1 = ws->getSpectrum(1).getDetectorIDs();

    TS_ASSERT_EQUALS(*detids0.begin(), 1);
    TS_ASSERT_EQUALS(*detids1.begin(), 2);

    TS_ASSERT_EQUALS(detids0.size(), 1);
    TS_ASSERT_EQUALS(detids1.size(), 1);
  }

  void test_createCountsWorkspace_correct_XYE_values() {

    MatrixWorkspace_sptr ws = createCountsWorkspace(2, 10, 0.0);

    Mantid::MantidVec vecX1 = ws->getSpectrum(0).readX();
    Mantid::MantidVec vecX2 = ws->getSpectrum(1).readX();
    TS_ASSERT_DELTA(vecX1.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX1.at(9), 0.900, 0.001);
    TS_ASSERT_DELTA(vecX2.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX2.at(9), 0.900, 0.001);

    Mantid::MantidVec vecY1 = ws->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 0, 0.1);
    TS_ASSERT_DELTA(vecY1.at(9), 9, 0.1);
    TS_ASSERT_DELTA(vecY2.at(0), 10, 0.1);
    TS_ASSERT_DELTA(vecY2.at(9), 19, 0.1);

    Mantid::MantidVec vecE1 = ws->getSpectrum(0).readE();
    Mantid::MantidVec vecE2 = ws->getSpectrum(1).readE();
    TS_ASSERT_DELTA(vecE1.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE1.at(9), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(9), 0.005, 0.0001);
  }

  void test_createAsymmetryWorkspace_number_histograms_correct() {
    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(25, 2);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 25);
  }

  void test_createAsymmetryWorkspace_detectorIDs_correct() {
    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(2, 10);

    const std::set<detid_t> detids0 = ws->getSpectrum(0).getDetectorIDs();
    const std::set<detid_t> detids1 = ws->getSpectrum(1).getDetectorIDs();

    TS_ASSERT_EQUALS(*detids0.begin(), 1);
    TS_ASSERT_EQUALS(*detids1.begin(), 2);

    TS_ASSERT_EQUALS(detids0.size(), 1);
    TS_ASSERT_EQUALS(detids1.size(), 1);
  }

  void test_createAsymmetryWorkspace_correct_XYE_values() {

    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(2, 10);

    Mantid::MantidVec vecX1 = ws->getSpectrum(0).readX();
    Mantid::MantidVec vecX2 = ws->getSpectrum(1).readX();
    TS_ASSERT_DELTA(vecX1.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX1.at(9), 0.900, 0.001);
    TS_ASSERT_DELTA(vecX2.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX2.at(9), 0.900, 0.001);

    Mantid::MantidVec vecY1 = ws->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 12.46, 0.01);
    TS_ASSERT_DELTA(vecY1.at(9), 2.76, 0.01);
    TS_ASSERT_DELTA(vecY2.at(0), 24.78, 0.01);
    TS_ASSERT_DELTA(vecY2.at(9), 6.21, 0.01);

    Mantid::MantidVec vecE1 = ws->getSpectrum(0).readE();
    Mantid::MantidVec vecE2 = ws->getSpectrum(1).readE();
    TS_ASSERT_DELTA(vecE1.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE1.at(9), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(9), 0.005, 0.0001);
  }

  void test_createAsymmetryWorkspace_custom_generator() {

    yDataCounts yData = yDataCounts();
    MatrixWorkspace_sptr ws = createAsymmetryWorkspace(5, 10, yData);

    Mantid::MantidVec vecY1 = ws->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws->getSpectrum(4).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 0.0, 0.01);
    TS_ASSERT_DELTA(vecY1.at(9), 9.0, 0.01);
    TS_ASSERT_DELTA(vecY2.at(0), 40.0, 0.01);
    TS_ASSERT_DELTA(vecY2.at(9), 49.0, 0.01);
  }

  void test_createMultiPeriodWorkspace_names() {
    auto wsGroup = createMultiPeriodWorkspaceGroup(5, 2, 10, "muonGroup");

    std::vector<std::string> names = wsGroup->getNames();

    TS_ASSERT_EQUALS(names.size(), 5);
    TS_ASSERT_EQUALS(names.at(0), "MuonDataPeriod_1");
    TS_ASSERT_EQUALS(names.at(4), "MuonDataPeriod_5");

    AnalysisDataService::Instance().clear();
  }

  void test_createMultiPeriodWorkspace_detectorIDs() {

    auto wsGroup = createMultiPeriodWorkspaceGroup(5, 2, 10, "muonGroup");

    MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("MuonDataPeriod_1"));
    const std::set<detid_t> detids0 = ws1->getSpectrum(0).getDetectorIDs();
    const std::set<detid_t> detids1 = ws1->getSpectrum(1).getDetectorIDs();

    TS_ASSERT_EQUALS(*detids0.begin(), 1);
    TS_ASSERT_EQUALS(*detids1.begin(), 2);

    TS_ASSERT_EQUALS(detids0.size(), 1);
    TS_ASSERT_EQUALS(detids1.size(), 1);

    AnalysisDataService::Instance().clear();
  }

  void test_createMultiPeriodWorkspace_XYE_values() {

    auto wsGroup = createMultiPeriodWorkspaceGroup(5, 2, 10, "muonGroup");

    MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("MuonDataPeriod_1"));

    Mantid::MantidVec vecX1 = ws1->getSpectrum(0).readX();
    Mantid::MantidVec vecX2 = ws1->getSpectrum(1).readX();
    TS_ASSERT_DELTA(vecX1.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX1.at(9), 0.900, 0.001);
    TS_ASSERT_DELTA(vecX2.at(0), 0.000, 0.001);
    TS_ASSERT_DELTA(vecX2.at(9), 0.900, 0.001);

    Mantid::MantidVec vecY1 = ws1->getSpectrum(0).readY();
    Mantid::MantidVec vecY2 = ws1->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY1.at(0), 1, 0.1);
    TS_ASSERT_DELTA(vecY1.at(9), 10, 0.1);
    TS_ASSERT_DELTA(vecY2.at(0), 11, 0.1);
    TS_ASSERT_DELTA(vecY2.at(9), 20, 0.1);

    MatrixWorkspace_sptr ws5 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem("MuonDataPeriod_5"));

    Mantid::MantidVec vecY3 = ws5->getSpectrum(0).readY();
    Mantid::MantidVec vecY4 = ws5->getSpectrum(1).readY();
    TS_ASSERT_DELTA(vecY3.at(0), 5, 0.1);
    TS_ASSERT_DELTA(vecY3.at(9), 14, 0.1);
    TS_ASSERT_DELTA(vecY4.at(0), 15, 0.1);
    TS_ASSERT_DELTA(vecY4.at(9), 24, 0.1);

    Mantid::MantidVec vecE1 = ws1->getSpectrum(0).readE();
    Mantid::MantidVec vecE2 = ws1->getSpectrum(1).readE();
    TS_ASSERT_DELTA(vecE1.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE1.at(9), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(0), 0.005, 0.0001);
    TS_ASSERT_DELTA(vecE2.at(9), 0.005, 0.0001);

    AnalysisDataService::Instance().clear();
  }

  void test_createDeadTimeTable_empty_for_incorrect_length_input() {

    std::vector<double> deadTimes = {0.001, 0.002};
    ITableWorkspace_sptr deadTimeTable = createDeadTimeTable(3, deadTimes);

    TS_ASSERT_EQUALS(deadTimeTable->columnCount(), 2);
    TS_ASSERT_EQUALS(deadTimeTable->rowCount(), 0);
  }

  void test_createDeadTimeTable_correct_values() {

    std::vector<double> deadTimes = {0.001, 0.002, 0.003, 0.004, 0.005};
    ITableWorkspace_sptr deadTimeTable = createDeadTimeTable(5, deadTimes);

    TS_ASSERT_EQUALS(deadTimeTable->rowCount(), 5);

    TS_ASSERT_DELTA(deadTimeTable->getColumn(0)->toDouble(0), 1, 0.1);
    TS_ASSERT_DELTA(deadTimeTable->getColumn(1)->toDouble(0), 0.001, 0.0001);
    TS_ASSERT_DELTA(deadTimeTable->getColumn(0)->toDouble(4), 5, 0.1);
    TS_ASSERT_DELTA(deadTimeTable->getColumn(1)->toDouble(4), 0.005, 0.0001);
  }

  void
  test_createWorkspaceWithInstrumentandRun_run_number_and_instrument_set_correctly() {
    MatrixWorkspace_sptr ws =
        createWorkspaceWithInstrumentandRun("MUSR", 12345, 10);

    TS_ASSERT_EQUALS(ws->mutableRun().getPropertyAsIntegerValue("run_number"),
                     12345);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(), "MUSR");
  }

  void
  test_createWorkspaceGroupConsecutiveDetectorIDs_correct_workspace_names() {

    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    createWorkspaceGroupConsecutiveDetectorIDs(3, 3, 10, "MuonAnalysis");
    // group
    TS_ASSERT(ads.doesExist("MuonAnalysis"));
    // workspaces in group
    TS_ASSERT(ads.doesExist("MuonDataPeriod_1"));
    TS_ASSERT(ads.doesExist("MuonDataPeriod_2"));
    TS_ASSERT(ads.doesExist("MuonDataPeriod_3"));

    WorkspaceGroup_sptr group = boost::dynamic_pointer_cast<WorkspaceGroup>(
        ads.retrieve("MuonAnalysis"));
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 3);
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        group->getItem("MuonDataPeriod_1"));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 3);

    ads.clear();
  }

  void test_createWorkspaceGroupConsecutiveDetectorIDs_IDs_are_consequtive() {

    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    createWorkspaceGroupConsecutiveDetectorIDs(3, 3, 10, "MuonAnalysis");

    MatrixWorkspace_sptr wsFirst = boost::dynamic_pointer_cast<MatrixWorkspace>(
        ads.retrieve("MuonDataPeriod_1"));
    MatrixWorkspace_sptr wsLast = boost::dynamic_pointer_cast<MatrixWorkspace>(
        ads.retrieve("MuonDataPeriod_3"));

    TS_ASSERT(wsFirst->getSpectrum(0).hasDetectorID(1));
    TS_ASSERT(wsFirst->getSpectrum(1).hasDetectorID(2));
    TS_ASSERT(wsFirst->getSpectrum(2).hasDetectorID(3));

    TS_ASSERT(wsLast->getSpectrum(0).hasDetectorID(7));
    TS_ASSERT(wsLast->getSpectrum(1).hasDetectorID(8));
    TS_ASSERT(wsLast->getSpectrum(2).hasDetectorID(9));

    ads.clear();
  }
};

#endif /* MANTID_ALGORITHMS_MUONWORKSPACECREATIONHELPERTEST_H_ */
