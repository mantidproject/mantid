// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_MUONGROUPINGCOUNTSTEST_H_
#define MANTID_MUON_MUONGROUPINGCOUNTSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMuon/MuonGroupingCounts.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;
using namespace MuonWorkspaceCreationHelper;

namespace {

// Simple class to set up the ADS with the configuration required by the
// algorithm (a MatrixWorkspace).
class setUpADSWithWorkspace {
public:
  setUpADSWithWorkspace(Workspace_sptr ws) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, ws);
  };
  ~setUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };

  std::string const inputWSName = "inputData";
};

// Set only mandatory fields; input and output workspace
IAlgorithm_sptr
algorithmWithoutOptionalPropertiesSet(const std::string &inputWSName) {

  auto alg = boost::make_shared<MuonGroupingCounts>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

// Set up algorithm without any optional properties
// i.e. just the input workspace and group name.
IAlgorithm_sptr
setUpAlgorithmWithoutOptionalProperties(WorkspaceGroup_sptr ws,
                                        const std::string &name) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("GroupName", name);
  return alg;
}

// Set up algorithm with GroupName applied
IAlgorithm_sptr setUpAlgorithmWithGroupName(WorkspaceGroup_sptr ws,
                                            const std::string &name) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("GroupName", name);
  return alg;
}

// Set up algorithm with TimeOffset applied
IAlgorithm_sptr
setUpAlgorithmWithGroupNameAndDetectors(WorkspaceGroup_sptr ws,
                                        const std::string &name,
                                        const std::vector<int> &detectors) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg =
      algorithmWithoutOptionalPropertiesSet(setup.inputWSName);
  alg->setProperty("GroupName", name);
  alg->setProperty("Grouping", detectors);
  return alg;
}

// Retrieve the output workspace from an executed algorithm
MatrixWorkspace_sptr getOutputWorkspace(IAlgorithm_sptr alg) {
  Workspace_sptr outputWS = alg->getProperty("OutputWorkspace");
  auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
  return wsOut;
}

} // namespace

class MuonGroupingCountsTest : public CxxTest::TestSuite {
public:
  static MuonGroupingCountsTest *createSuite() {
    return new MuonGroupingCountsTest();
  }
  static void destroySuite(MuonGroupingCountsTest *suite) { delete suite; }

  // --------------------------------------------------------------------------
  // Initialization / Execution
  // --------------------------------------------------------------------------

  void test_that_algorithm_initializes() {
    MuonGroupingCounts alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_that_algorithm_executes_with_no_optional_properties_set() {

    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");
    auto alg = setUpAlgorithmWithoutOptionalProperties(ws, "group1");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
  }

  // --------------------------------------------------------------------------
  // Validation : Group Names and Detector Grouping
  // --------------------------------------------------------------------------

  void test_that_input_workspace_cannot_be_a_Workspace2D() {

    auto ws = createCountsWorkspace(5, 10, 0.0);
    setUpADSWithWorkspace setup(ws);
    auto alg = boost::make_shared<MuonGroupingCounts>();
    alg->initialize();

    TS_ASSERT_THROWS_ANYTHING(
        alg->setProperty("InputWorkspace", setup.inputWSName));
  }

  void test_that_input_workspace_can_be_a_WorkspaceGroup() {

    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");
    setUpADSWithWorkspace setup(ws);
    auto alg = boost::make_shared<MuonGroupingCounts>();
    alg->initialize();

    TSM_ASSERT_THROWS_NOTHING(
        "", alg->setProperty("InputWorkspace", setup.inputWSName))
  }

  void test_that_group_name_must_be_supplied() {

    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");
    setUpADSWithWorkspace setup(ws);
    IAlgorithm_sptr alg =
        algorithmWithoutOptionalPropertiesSet(setup.inputWSName);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_group_names_with_alphanumeric_characters_or_underscores_are_allowed() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");

    std::vector<std::string> validNames = {"fwd", "fwd2", "bwd_2"};
    for (auto &&validName : validNames) {
      auto alg = setUpAlgorithmWithGroupName(ws, validName);
      TSM_ASSERT_THROWS_NOTHING("", alg->execute());
    }
  }

  void
  test_that_exec_throws_if_group_name_is_not_alphanumeric_or_underscored() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 1, 10, "group1");

    std::vector<std::string> invalidNames = {"@", "fwd!", "#1", "fwd @", "   "};
    for (auto &&invalidName : invalidNames) {
      auto alg = setUpAlgorithmWithGroupName(ws, invalidName);
      TS_ASSERT_THROWS_ANYTHING(alg->execute());
    }
  }

  void
  test_that_cannot_add_spectra_to_group_which_exceed_those_in_the_workspace() {
    auto ws = createMultiPeriodWorkspaceGroup(1, 5, 10, "group1");

    std::vector<int> detectors = {6, 7, 8, 9, 10};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group1", detectors);
    alg->setRethrows(true);

    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());
  }

  // --------------------------------------------------------------------------
  // Validation : multi period data
  // --------------------------------------------------------------------------

  void test_that_at_least_one_period_must_be_specified() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    std::vector<int> detectors = {1, 2};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group", detectors);

    std::vector<int> summedPeriods = {};
    std::vector<int> subtractedPeriods = {};
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_supplying_too_many_periods_to_SummedPeriods_throws_on_execute() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group", detectors);

    std::vector<int> summedPeriods = {3};
    alg->setProperty("SummedPeriods", summedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void
  test_that_supplying_too_many_periods_to_SubtractedPeriods_throws_on_execute() {
    auto ws = createMultiPeriodWorkspaceGroup(2, 3, 10, "group");
    std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group", detectors);

    std::vector<int> subtractedPeriods = {3};
    alg->setProperty("SubtractedPeriods", subtractedPeriods);

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  // --------------------------------------------------------------------------
  // Correct Output
  // --------------------------------------------------------------------------

  void test_that_single_period_data_combines_detectors_correctly() {
    // Spec 1 y-vals : 1,  2,  3,  4,  5,  6,  7,  8,  9,  10
    // Spec 2 y-vals : 11, 12, 13, 14, 15, 16 ,17, 18, 19, 20
    // Spec 3 y-vals : 21, 22, 23, 24, 25, 26, 27, 28, 29, 30
    auto ws = createMultiPeriodWorkspaceGroup(1, 3, 10, "group");
    std::vector<int> detectors = {1, 2, 3};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group", detectors);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 33.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 45.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 60.000, 0.001);
    // Quadrature errors : Sqrt(3 * 0.005^2 )
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.00866, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.00866, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.00866, 0.0001);
  }

  void test_that_summing_periods_combines_detectors_correctly() {
    // Period 1 :
    // Spec 1 y-vals : 1,  2,  3,  4,  5,  6,  7,  8,  9,  10
    // Spec 2 y-vals : 11, 12, 13, 14, 15, 16 ,17, 18, 19, 20
    // Period 2 :
    // Spec 1 y-vals : 2,  3,  4,  5,  6,  7,  8,  9,  10, 11
    // Spec 2 y-vals : 12, 13, 14, 15, 16, 17 ,18, 19, 20, 21
    auto ws = createMultiPeriodWorkspaceGroup(2, 2, 10, "group");
    std::vector<int> detectors = {1, 2};
    std::vector<int> summedPeriods = {1, 2};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group", detectors);
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 26.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 42.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 62.000, 0.001);
    // Quadrature errors : Sqrt(4 * 0.005^2 )
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0100, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0100, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0100, 0.0001);
  }

  void test_that_subtracting_periods_combines_detectors_correctly() {
    // Period 1 :
    // Spec 1 y-vals : 1,  2,  3,  4,  5,  6,  7,  8,  9,  10
    // Spec 2 y-vals : 11, 12, 13, 14, 15, 16 ,17, 18, 19, 20
    // Period 2 :
    // Spec 1 y-vals : 2,  3,  4,  5,  6,  7,  8,  9,  10, 11
    // Spec 2 y-vals : 12, 13, 14, 15, 16, 17 ,18, 19, 20, 21
    auto ws = createMultiPeriodWorkspaceGroup(2, 2, 10, "group");
    std::vector<int> detectors = {1, 2};
    std::vector<int> summedPeriods = {2};
    std::vector<int> subtractedPeriods = {1};
    auto alg = setUpAlgorithmWithGroupNameAndDetectors(ws, "group", detectors);
    alg->setProperty("SummedPeriods", summedPeriods);
    alg->setProperty("SubtractedPeriods", subtractedPeriods);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    TS_ASSERT_DELTA(wsOut->readX(0)[0], 0.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[4], 0.400, 0.001);
    TS_ASSERT_DELTA(wsOut->readX(0)[9], 0.900, 0.001);

    TS_ASSERT_DELTA(wsOut->readY(0)[0], 2.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[4], 2.000, 0.001);
    TS_ASSERT_DELTA(wsOut->readY(0)[9], 2.000, 0.001);
    // Quadrature errors : Sqrt(4 * 0.005^2 )
    TS_ASSERT_DELTA(wsOut->readE(0)[0], 0.0100, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[4], 0.0100, 0.0001);
    TS_ASSERT_DELTA(wsOut->readE(0)[9], 0.0100, 0.0001);
  }
};

#endif /* MANTID_MUON_MUONGROUPINGCOUNTSTEST_H_ */
