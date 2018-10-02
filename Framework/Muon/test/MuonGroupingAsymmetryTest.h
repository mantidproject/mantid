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

namespace {

// Set sensible default algorithm properties
IAlgorithm_sptr algorithmWithPropertiesSet(const std::string &inputWSName) {

  auto alg = boost::make_shared<MuonGroupingAsymmetry>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
  alg->setProperty("GroupName", "fwd");
  alg->setProperty("DetectorIndex", 1);
  alg->setProperty("AsymmetryTimeMin", 0.0);
  alg->setProperty("AsymmetryTimeMax", 30.0);
  alg->setProperty("SummedPeriods", std::to_string(1));
  alg->setProperty("SubtractedPeriods", "");
  alg->setLogging(false);
  return alg;
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

  void test_algorithm_initializes() {}

  void test_algorithm_executes_with_default_arguments() {}

  void
  test_grouping_with_single_detector_and_asymmetry_analysis_gives_correct_values() {
  }

  void
  test_grouping_with_multiple_detectors_and_asymmetry_analysis_gives_correct_values() {
  }

  void
  test_grouping_asymmetry_with_subtracted_multiple_periods_gives_correct_values() {
  }

  void
  test_grouping_asymmetry_with_summed_multiple_periods_gives_correct_values() {}

  // void test_algorithm_fails_if_summed_periods_has_negative_entry() {}

  // void test_algorithm_fails_if_subtracted_periods_has_negative_entry() {}

  // void test_algorithm_fails_if_summed_periods_exceed_periods_in_workspace()
  // {}

  // void
  // test_algorithm_fails_if_suubtracted_periods_exceed_periods_in_workspace()
  // {}

  // void test_algorithm_fails_if_no_periods_given() {}
};

#endif /* MANTID_MUON_MUONGROUPINGASYMMETRYTEST_H_ */
