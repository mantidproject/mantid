#ifndef MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_
#define MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include <string>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMuon/MuonGroupingAsymmetry.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;
using namespace MuonWorkspaceCreationHelper;

namespace {

class setUpADSWithWorkspace {
public:
  setUpADSWithWorkspace(Workspace_sptr ws) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, ws);
  };
  ~setUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
  std::string const inputWSName = "inputData";
};

// Set sensible default algorithm properties
IAlgorithm_sptr algorithmWithPropertiesSet(const std::string &inputWSName) {

  auto alg = boost::make_shared<MuonGroupingAsymmetry>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setProperty("GroupName", "fwd");
  alg->setProperty("DetectorIndex", 1);
  alg->setProperty("AsymmetryTimeMin", 0.0);
  alg->setProperty("AsymmetryTimeMax", 30.0);
  alg->setProperty("SummedPeriods", std::to_string(1));
  alg->setProperty("SubtractedPeriods", "");
  alg->setLogging(false);
  return alg;
}

IAlgorithm_sptr
algorithmWithoutOptionalPropertiesSet(const std::string &inputWSName) {
  auto alg = boost::make_shared<MuonGroupingAsymmetry>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("OutputWorkspace", "__notUsed");
  alg->setAlwaysStoreInADS(false);
  alg->setLogging(false);
  return alg;
}

IAlgorithm_sptr setUpAlgorithmWithGroupNameAndDetector(WorkspaceGroup_sptr ws,
                                                       const std::string &name,
                                                       int &detector) {
  setUpADSWithWorkspace setup(ws);
  IAlgorithm_sptr alg = algorithmWithPropertiesSet(setup.inputWSName);
  alg->setProperty("GroupName", name);
  alg->setProperty("DetectorIndex", detector);
  return alg;
}

MatrixWorkspace_sptr getOutputWorkspace(IAlgorithm_sptr alg) {
  Workspace_sptr outputWS = alg->getProperty("OutputWorkspace");
  auto wsOut = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
  return wsOut;
}

} // namespace

class MuonGroupingAsymmetryTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  MuonGroupingAsymmetryTest() { Mantid::API::FrameworkManager::Instance(); }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonGroupingAsymmetryTest *createSuite() {
    return new MuonGroupingAsymmetryTest();
  }
  static void destroySuite(MuonGroupingAsymmetryTest *suite) { delete suite; }

  void test_algorithm_initializes() {
    MuonGroupingAsymmetry alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_algorithm_executes_with_default_arguments() {
    auto ws = createMultiPeriodWorkspaceGroup(1, 3, 10, "group");
    auto alg = algorithmWithPropertiesSet("group");

    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void
  test_grouping_with_single_detector_and_asymmetry_analysis_gives_correct_values() {
    auto ws = createMultiPeriodWorkspaceGroup(1, 3, 10, "group");
    int detector = 1;
    auto alg = setUpAlgorithmWithGroupNameAndDetector(ws, "group", detector);
    alg->execute();

    auto wsOut = getOutputWorkspace(alg);

    // auto spec = wsOut->getNumberHistograms();

    // TS_ASSERT_DELTA(wsOut->readX(1)[0], 0.000, 0.001);
  }

  void
  test_grouping_with_multiple_detectors_and_asymmetry_analysis_gives_correct_values() {
  }

  void
  test_grouping_asymmetry_with_subtracted_multiple_periods_gives_correct_values() {
  }

  void
  test_grouping_asymmetry_with_summed_multiple_periods_gives_correct_values() {}

  void test_algorithm_fails_if_summed_periods_has_negative_entry() {}

  void test_algorithm_fails_if_subtracted_periods_has_negative_entry() {}

  void test_algorithm_fails_if_summed_periods_exceed_periods_in_workspace() {}

  void
  test_algorithm_fails_if_subtracted_periods_exceed_periods_in_workspace() {}

  void test_algorithm_fails_if_no_periods_given() {}
};

#endif /* MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_ */
