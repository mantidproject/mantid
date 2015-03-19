#ifndef MANTID_MDALGORITHMS_SMOOTHMDTEST_H_
#define MANTID_MDALGORITHMS_SMOOTHMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/SmoothMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <vector>


using Mantid::MDAlgorithms::SmoothMD;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class SmoothMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SmoothMDTest *createSuite() { return new SmoothMDTest(); }
  static void destroySuite(SmoothMDTest *suite) { delete suite; }

  void test_Init() {
    SmoothMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
  }

  void test_function_is_of_right_type() {
      SmoothMD alg;
      alg.initialize();
      TSM_ASSERT_THROWS("Function can only be of known types for SmoothMD", alg.setProperty("Function", "magic_function"), std::invalid_argument&);
  }

  void test_reject_negative_width_vector_entry()
  {
      SmoothMD alg;
      alg.initialize();
      TSM_ASSERT_THROWS("N-pixels contains zero", alg.setProperty("WidthVector", std::vector<int>(1, 0)), std::invalid_argument&);
  }

  void test_mandatory_width_vector_entry()
  {
      SmoothMD alg;
      alg.initialize();
      TSM_ASSERT_THROWS("Empty WidthVector", alg.setProperty("WidthVector", std::vector<int>()), std::invalid_argument&);
  }

  void test_smooth_hat_function()
  {
      auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

      SmoothMD alg;
      alg.setChild(true);
      alg.initialize();
      std::vector<int> widthVector(1, 1);
      alg.setProperty("WidthVector", widthVector);
      alg.setProperty("InputWorkspace", toSmooth);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");
  }



};

#endif /* MANTID_MDALGORITHMS_SMOOTHMDTEST_H_ */
