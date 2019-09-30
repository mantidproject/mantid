// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_MUONPREPROCESSTEST_H_
#define MANTID_MUON_MUONPREPROCESSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMuon/MuonPreProcess.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;
using namespace MuonWorkspaceCreationHelper;

namespace {

// Set only mandatory fields; input and output workspace
IAlgorithm_sptr
algorithmWithoutOptionalPropertiesSet(const std::string &inputWSName) {

  auto alg = boost::make_shared<MuonPreProcess>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

// Simple class to set up the ADS with the configuration required by the
// algorithm (a MatrixWorkspace).
class setUpADSWithWorkspace {
public:
  std::string const inputWSName = "inputData";

  setUpADSWithWorkspace(Workspace_sptr ws) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, ws);
  };
  ~setUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
};

// Set up algorithm with none of the optional properties
IAlgorithm_sptr setUpAlgorithmWithNoOptionalProperties(Workspace_sptr ws) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  return alg;
}

// Set up algorithm with TimeOffset applied
IAlgorithm_sptr setUpAlgorithmWithTimeOffset(MatrixWorkspace_sptr ws,
                                             const double &offset) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("TimeOffset", offset);
  return alg;
}

// Set up algorithm with DeadTimeTable applied
IAlgorithm_sptr
setUpAlgorithmWithDeadTimeTable(MatrixWorkspace_sptr ws,
                                ITableWorkspace_sptr deadTimes) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("DeadTimeTable", deadTimes);
  return alg;
}

// Set up algorithm with TimeMin applied
IAlgorithm_sptr setUpAlgorithmWithTimeMin(MatrixWorkspace_sptr ws,
                                          const double &timeMin) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("TimeMin", timeMin);
  return alg;
}

// Set up algorithm with TimeMax applied
IAlgorithm_sptr setUpAlgorithmWithTimeMax(MatrixWorkspace_sptr ws,
                                          const double &timeMax) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("TimeMax", timeMax);
  return alg;
}

// Get the workspace at a particular index from the output workspace
// group produced by the PreProcess alg
MatrixWorkspace_sptr getOutputWorkspace(IAlgorithm_sptr muonPreProcessAlg,
                                        const int &index) {
  WorkspaceGroup_sptr outputWS;
  outputWS = muonPreProcessAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr wsOut =
      boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(index));
  return wsOut;
}

} // namespace

class MuonPreProcessTest : public CxxTest::TestSuite {
public:
  MuonPreProcessTest() { Mantid::API::FrameworkManager::Instance(); }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonPreProcessTest *createSuite() { return new MuonPreProcessTest(); }
  static void destroySuite(MuonPreProcessTest *suite) { delete suite; }

  void test_algorithm_initializes() {
    MuonPreProcess alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_that_algorithm_executes_with_no_optional_properties_set() {
    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);

    alg->initialize();

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }

  void test_that_output_data_preserves_bin_edges() {}

  void test_that_output_data_preserves_bin_centres() {}

  // --------------------------------------------------------------------------
  // Input property validation : TimeMax and TimeMin
  // --------------------------------------------------------------------------

  void test_that_algorithm_does_not_execute_if_TimeMax_lower_than_TimeMin() {
    auto ws = createCountsWorkspace(2, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    alg->setProperty("TimeMin", 0.6);
    alg->setProperty("TimeMax", 0.4);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_that_negative_TimeMin_is_an_accepted_input() {
    auto ws = createCountsWorkspace(2, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    alg->setProperty("TimeMin", -1.0);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void test_that_TimeMin_and_TimeMax_must_be_different() {
    auto ws = createCountsWorkspace(2, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    alg->setProperty("TimeMin", 0.5);
    alg->setProperty("TimeMax", 0.5);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_that_TimeMin_and_TimeMax_both_in_same_bin_throws_logic_error() {
    // bins : 0.0 , 0.1 , 0.2 , ... , 1.0 (bin edges)
    auto ws = createCountsWorkspace(2, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    alg->setProperty("OutputWorkspace", "__notUsed");
    alg->setAlwaysStoreInADS(false);

    alg->setProperty("TimeMin", 0.55);
    alg->setProperty("TimeMax", 0.58);

    // Expect runtime error as alg is set to rethrow
    TS_ASSERT_THROWS(alg->execute(), const std::logic_error &);
  }

  // --------------------------------------------------------------------------
  // Input property validation : Dead time table
  // --------------------------------------------------------------------------

  void
  test_that_cannot_execute_if_dead_time_has_more_rows_than_workspace_spectra() {
    // workspace has 2 spectra, dead time table has 5 rows
    auto ws = createCountsWorkspace(2, 10, 0.0);
    std::vector<double> deadTimes = {0.05, 0.05, 0.05, 0.05, 0.05};
    ITableWorkspace_sptr deadTimeTable = createDeadTimeTable(5, deadTimes);

    auto alg = setUpAlgorithmWithDeadTimeTable(ws, deadTimeTable);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  // --------------------------------------------------------------------------
  // Correct output : Rebin Args
  // --------------------------------------------------------------------------

  void test_rebinning_with_fixed_bin_widths_produces_correct_x_and_y_values() {
    // x =  0.0 , 0.1 , 0.2 , ... , 1.0 (bin edges)
    // y =  0   , 1   , 2   , ... , 9
    auto ws = createCountsWorkspace(2, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    std::vector<double> rebinArgs = {0.2};
    alg->setProperty("RebinArgs", rebinArgs);
    alg->setProperty("OutputWorkspace", "__notUsed");
    alg->setAlwaysStoreInADS(false);
    alg->execute();

    WorkspaceGroup_sptr outputWS;
    outputWS = alg->getProperty("OutputWorkspace");

    MatrixWorkspace_sptr wsOut =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.200, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.800, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 1.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 5.00, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 17.000, 0.001);
  }

  void
  test_rebinning_with_logarithmic_binning_produces_correct_x_and_y_values() {
    // x =  1.0 , 1.1 , 1.2 , ... , 2.0 (bin edges)
    // y =  0   , 1   , 2   , ... , 9
    auto ws = createCountsWorkspace(1, 10, 0.0, 0, true, 1.0, 2.0);
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    std::vector<double> rebinArgs = {-0.2};
    alg->setProperty("RebinArgs", rebinArgs);
    alg->setProperty("OutputWorkspace", "__notUsed");
    alg->setAlwaysStoreInADS(false);
    alg->execute();

    WorkspaceGroup_sptr outputWS;
    outputWS = alg->getProperty("OutputWorkspace");

    MatrixWorkspace_sptr wsOut =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));

    // Using "FullBinsOnly" as false in Rebin preserves
    // the counts at the expense of an uneven bin
    // at the end of the range, as seen below.
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 1.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 1.200, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[2], 1.440, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[3], 1.728, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 2.000, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 1.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[1], 6.600, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[2], 15.360, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[3], 22.040, 0.001);
  }

  // --------------------------------------------------------------------------
  // Correct output : Time offset
  // --------------------------------------------------------------------------

  void test_that_positive_time_offset_applied_correctly() {
    // x =  0.0 , 0.1 , 0.2 , ... , 1.0 (bin edges)
    // y =  0   , 1   , 2   , ... , 9
    auto ws = createCountsWorkspace(1, 10, 0.0);

    auto alg = setUpAlgorithmWithTimeOffset(ws, 0.5);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg, 0);
    // x-values
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000 + 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.100 + 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[10], 1.000 + 0.500, 0.001);
    // y-values
    TS_ASSERT_DELTA(wsOut->readY(0)[0], 0.0, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 9.0, 0.001);
  }

  void test_that_negative_time_offset_applied_correctly() {
    // x =  0.0 , 0.1 , 0.2 , ... , 1.0 (bin edges)
    // y =  0   , 1   , 2   , ... , 9
    auto ws = createCountsWorkspace(1, 10, 0.0);

    auto alg = setUpAlgorithmWithTimeOffset(ws, -0.5);

    alg->execute();
    auto wsOut = getOutputWorkspace(alg, 0);
    // x-values
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000 - 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.100 - 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[10], 1.000 - 0.500, 0.001);
    // y-values
    TS_ASSERT_DELTA(wsOut->readY(0)[0], 0.0, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 9.0, 0.001);
  }

  // --------------------------------------------------------------------------
  // Correct output : cropping via TimeMax and TimeMin
  // --------------------------------------------------------------------------

  void test_that_cropping_with_TimeMin_crops_correctly() {
    // bins : 0.0 , 0.1 , 0.2 , ... , 1.0 (bin edges)
    auto ws = createCountsWorkspace(2, 10, 0.0);

    auto alg = setUpAlgorithmWithTimeMin(ws, 0.5);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg, 0);
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.600, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[5], 1.000, 0.001);
  }

  void test_that_cropping_with_TimeMax_crops_correctly() {
    // bins : 0.0 , 0.1 , 0.2 , ... , 1.0  (bin edges)
    auto ws = createCountsWorkspace(2, 10, 0.0);

    auto alg = setUpAlgorithmWithTimeMax(ws, 0.5);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg, 0);
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[1], 0.100, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[5], 0.500, 0.001);
  }

  void
  test_that_if_TimeMin_below_lowest_time_then_crop_has_no_effect_on_lower_range() {
    // bins : 0.0 , 0.1 , 0.2 , ... , 1.0  (bin edges)
    auto ws = createCountsWorkspace(2, 10, 0.0);

    auto alg = setUpAlgorithmWithTimeMin(ws, -0.1);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg, 0);
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[5], 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[10], 1.000, 0.001);
  }

  void
  test_that_if_TimeMax_above_highest_time_then_crop_has_no_effect_on_upper_range() {
    // bins : 0.0 , 0.1 , 0.2 , ... , 1.0  (bin edges)
    auto ws = createCountsWorkspace(2, 10, 0.0);

    auto alg = setUpAlgorithmWithTimeMax(ws, 2.0);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg, 0);
    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[5], 0.500, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[10], 1.000, 0.001);
  }

  // --------------------------------------------------------------------------
  // Correct output : Supplying a dead-time table
  // --------------------------------------------------------------------------

  void test_that_y_values_are_corrected_for_dead_time_correctly() {

    auto ws = createCountsWorkspace(2, 10, 0.0);
    std::vector<double> deadTimes = {0.05, 0.05};
    auto deadTimeTable = createDeadTimeTable(2, deadTimes);

    auto alg = setUpAlgorithmWithDeadTimeTable(ws, deadTimeTable);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg, 0);
    TS_ASSERT_DELTA(wsOut->readY(0)[0], 0.0, 0.01);
    TS_ASSERT_DELTA(wsOut->readY(0)[3], 3.53, 0.01);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 16.36, 0.01);
  }

  // --------------------------------------------------------------------------
  // Handling multi-period data
  // --------------------------------------------------------------------------

  void test_that_output_group_workspace_contains_all_the_periods_from_input() {
    WorkspaceGroup_sptr ws =
        createMultiPeriodWorkspaceGroup(3, 1, 10, "MuonAnalysis");

    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
    alg->execute();

    WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 3);
  }

  void test_that_execption_thrown_if_input_workspace_group_is_empty() {

    WorkspaceGroup_sptr wsGroup = boost::make_shared<WorkspaceGroup>();
    auto alg = setUpAlgorithmWithNoOptionalProperties(wsGroup);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_workspaces_in_input_group_must_all_have_the_same_number_of_spectra() {

    auto wsOneSpectra = createCountsWorkspace(1, 10, 0.0);
    auto wsTwoSpectra = createCountsWorkspace(2, 10, 0.0);
    WorkspaceGroup_sptr wsGroup = boost::make_shared<WorkspaceGroup>();
    wsGroup->addWorkspace(wsOneSpectra);
    wsGroup->addWorkspace(wsTwoSpectra);
    auto alg = setUpAlgorithmWithNoOptionalProperties(wsGroup);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }
};

#endif /* MANTID_MUON_MUONPREPROCESSTEST_H_ */
