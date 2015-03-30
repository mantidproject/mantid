#ifndef MANTID_MDALGORITHMS_CUTMDTEST_H_
#define MANTID_MDALGORITHMS_CUTMDTEST_H_

#include "MantidMDAlgorithms/CutMD.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
const std::string sharedWSName = "__CutMDTest_dataWS";
}

class CutMDTest : public CxxTest::TestSuite {
private:
  IMDWorkspace_sptr m_inWS;

public:
  CutMDTest() {
    FrameworkManager::Instance().exec("CreateMDWorkspace", 10,
        "OutputWorkspace", sharedWSName.c_str(),
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "A,B,C",
        "Units", "U,U,U");

    FrameworkManager::Instance().exec("SetSpecialCoordinates", 4,
        "InputWorkspace", sharedWSName.c_str(),
        "SpecialCoordinates", "HKL");

    FrameworkManager::Instance().exec("SetUB", 14,
        "Workspace", sharedWSName.c_str(),
        "a", "1",
        "b", "1",
        "c", "1",
        "alpha", "90",
        "beta", "90",
        "gamma", "90");

    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", sharedWSName.c_str(),
        "PeakParams", "10000,0,0,0,1");

    m_inWS =
      AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(sharedWSName);
  }

  virtual ~CutMDTest() { AnalysisDataService::Instance().remove(sharedWSName); }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CutMDTest *createSuite() { return new CutMDTest(); }
  static void destroySuite(CutMDTest *suite) { delete suite; }

  void test_init() {
    CutMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }
};

#endif /* MANTID_MDALGORITHMS_CUTMDTEST_H_ */
