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

  void test_simple_smooth_hat_function()
  {
      auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

      /*
       2D MDHistoWorkspace Input

       1 - 1 - 1
       1 - 1 - 1
       1 - 1 - 1
      */

      SmoothMD alg;
      alg.setChild(true);
      alg.initialize();
      std::vector<int> widthVector(1, 3);
      alg.setProperty("WidthVector", widthVector);
      alg.setProperty("InputWorkspace", toSmooth);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");

      /*
       2D MDHistoWorkspace Expected

       1 - 1 - 1
       1 - 1 - 1
       1 - 1 - 1
      */
      for(size_t i = 0; i < out->getNPoints(); ++i)
      {
          TS_ASSERT_EQUALS(1, out->getSignalAt(i));
          TS_ASSERT_EQUALS(1, out->getErrorAt(i));
      }

  }

  void test_smooth_hat_function()
  {
      auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);
      toSmooth->setSignalAt(4, 2.0);

      /*
       2D MDHistoWorkspace Input

       1 - 1 - 1
       1 - 2 - 1
       1 - 1 - 1
      */

      SmoothMD alg;
      alg.setChild(true);
      alg.initialize();
      std::vector<int> widthVector(1, 3);
      alg.setProperty("WidthVector", widthVector);
      alg.setProperty("InputWorkspace", toSmooth);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");

      /*
       2D MDHistoWorkspace Expected

       5/4 -  7/6 - 5/4
       7/6 - 10/9 - 7/6
       5/4 -  7/6 - 5/4
      */

      TS_ASSERT_EQUALS(5.0/4, out->getSignalAt(0));
      TS_ASSERT_EQUALS(7.0/6, out->getSignalAt(1));
      TS_ASSERT_EQUALS(10.0/9, out->getSignalAt(4));


  }



};

#endif /* MANTID_MDALGORITHMS_SMOOTHMDTEST_H_ */
