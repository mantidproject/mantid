#ifndef MANTID_MUON_MUONPAIRINGASYMMETRYTEST_H_
#define MANTID_MUON_MUONPAIRINGASYMMETRYTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidMuon/MuonPairingAsymmetry.h"
#include "MantidTestHelpers/MuonWorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Muon;

namespace {

// Set sensible default algorithm properties
IAlgorithm_sptr algorithmWithPropertiesSet(const std::string &inputWSName) {

  auto alg = boost::make_shared<MuonPairingAsymmetry>();
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWSName);
 // alg->setProperty("GroupName", "fwd");
 // alg->setProperty("DetectorIndex", 1);
 // alg->setProperty("AsymmetryTimeMin", 0.0);
 // alg->setProperty("AsymmetryTimeMax", 30.0);
 // alg->setProperty("SummedPeriods", std::to_string(1));
 // alg->setProperty("SubtractedPeriods", "");
  alg->setLogging(false);
  return alg;
}

} // namespace

class MuonPairingAsymmetryTest : public CxxTest::TestSuite {
public:
  // WorkflowAlgorithms do not appear in the FrameworkManager without this line
  MuonPairingAsymmetryTest() { Mantid::API::FrameworkManager::Instance(); }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonPairingAsymmetryTest *createSuite() {
    return new MuonPairingAsymmetryTest();
  }
  static void destroySuite(MuonPairingAsymmetryTest *suite) { delete suite; }

  void test_algorithm_initializes() {}
};

#endif /* MANTID_MUON_MUONPAIRINGASYMMETRYTEST_H_ */
