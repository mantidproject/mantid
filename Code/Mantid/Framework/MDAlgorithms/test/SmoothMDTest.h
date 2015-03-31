#ifndef MANTID_MDALGORITHMS_SMOOTHMDTEST_H_
#define MANTID_MDALGORITHMS_SMOOTHMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/SmoothMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <vector>
#include <boost/math/special_functions/fpclassify.hpp>

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
    TSM_ASSERT_THROWS("Function can only be of known types for SmoothMD",
                      alg.setProperty("Function", "magic_function"),
                      std::invalid_argument &);
  }

  void test_reject_negative_width_vector_entry() {
    SmoothMD alg;
    alg.initialize();
    TSM_ASSERT_THROWS("N-pixels contains zero",
                      alg.setProperty("WidthVector", std::vector<int>(1, 0)),
                      std::invalid_argument &);
  }

  void test_mandatory_width_vector_entry() {
    SmoothMD alg;
    alg.initialize();
    TSM_ASSERT_THROWS("Empty WidthVector",
                      alg.setProperty("WidthVector", std::vector<int>()),
                      std::invalid_argument &);
  }

  void test_width_entry_must_be_odd() {
    auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 1 /*numDims*/, 4 /*numBins in each dimension*/);

    SmoothMD alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setProperty("InputWorkspace", toSmooth);
    alg.setProperty(
        "WidthVector",
        std::vector<int>(1, 4)); // Width vector contains even number == 4
    TSM_ASSERT_THROWS("One bad entry. Should throw.", alg.execute(),
                      std::runtime_error &);

    std::vector<int> widthVector;
    widthVector.push_back(3); // OK
    widthVector.push_back(5); // OK
    widthVector.push_back(2); // Not OK

    alg.setProperty("WidthVector",
                    widthVector); // Width vector contains even number
    TSM_ASSERT_THROWS("Some good entries, but should still throw",
                      alg.execute(), std::runtime_error &);
  }

  void test_simple_smooth_hat_function() {
    auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);

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
    for (size_t i = 0; i < out->getNPoints(); ++i) {
      TS_ASSERT_EQUALS(1, out->getSignalAt(i));
      TS_ASSERT_EQUALS(1, out->getErrorAt(i));
    }
  }

  void test_smooth_hat_function_3_pix_width() {
    auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 3 /*numBins in each dimension*/);
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

    TS_ASSERT_EQUALS(5.0 / 4, out->getSignalAt(0));
    TS_ASSERT_EQUALS(7.0 / 6, out->getSignalAt(1));
    TS_ASSERT_EQUALS(10.0 / 9, out->getSignalAt(4));
  }

  void test_smooth_hat_function_5_pix_width() {
    auto toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 5 /*numBins in each dimension*/);
    toSmooth->setSignalAt(12, 4.0);

    /*
     2D MDHistoWorkspace Input

     1 - 1 - 1 - 1 - 1
     1 - 1 - 1 - 1 - 1
     1 - 1 - 4 - 1 - 1
     1 - 1 - 1 - 1 - 1
     1 - 1 - 1 - 1 - 1

    */

    SmoothMD alg;
    alg.setChild(true);
    alg.initialize();
    std::vector<int> widthVector(1, 5); // Smooth with width == 5
    alg.setProperty("WidthVector", widthVector);
    alg.setProperty("InputWorkspace", toSmooth);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");

    /*
     2D MDHistoWorkspace Expected

     key:
     x = 12/9
     y = 18/15
     z = 28/25
     ` = ignore

     x - ` - y - ` - x
     ` - ` - ` - ` - `
     y - ` - z - ` - y
     ` - ` - ` - ` - `
     x - ` - y - ` - x
    */

    // Check vertexes
    double x = 12.0 / 9;
    TS_ASSERT_EQUALS(x, out->getSignalAt(0));
    TS_ASSERT_EQUALS(x, out->getSignalAt(4));
    TS_ASSERT_EQUALS(x, out->getSignalAt(20));
    TS_ASSERT_EQUALS(x, out->getSignalAt(24));

    // Check edges
    double y = 18.0 / 15;
    TS_ASSERT_EQUALS(y, out->getSignalAt(2));
    TS_ASSERT_EQUALS(y, out->getSignalAt(10));
    TS_ASSERT_EQUALS(y, out->getSignalAt(14));
    TS_ASSERT_EQUALS(y, out->getSignalAt(22));

    // Check centre
    double z = 28.0 / 25;
    TS_ASSERT_EQUALS(z, out->getSignalAt(12));
  }

  void test_dimensional_check_of_weight_ws() {

      MDHistoWorkspace_sptr a =
          MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal value*/, 1 /*dimensionality*/,
                                                       9);

      MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(
          1.0 /*signal value*/,  2 /*dimensionality*/, 9); // one dimension larger

      SmoothMD alg;
      alg.setChild(true);
      alg.initialize();
      std::vector<int> widthVector(1, 3); // Smooth with width == 3
      alg.setProperty("WidthVector", widthVector);
      alg.setProperty("InputWorkspace", a);
      alg.setProperty("InputNormalizationWorkspace", b);
      alg.setPropertyValue("OutputWorkspace", "dummy");

      TSM_ASSERT_THROWS("Input unsmoothed and input Normalisation workspaces must have the same dimensionality",
                        alg.execute(), std::runtime_error &);

  }

  void test_shape_check_of_weight_ws() {

      const size_t nd = 1;

      MDHistoWorkspace_sptr a =
          MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0 /*signal value*/, nd,
                                                       10);

      MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(
          1.0 /*signal value*/, nd, 10 + 1); // one bin longer

      SmoothMD alg;
      alg.setChild(true);
      alg.initialize();
      std::vector<int> widthVector(1, 3); // Smooth with width == 3
      alg.setProperty("WidthVector", widthVector);
      alg.setProperty("InputWorkspace", a);
      alg.setProperty("InputNormalizationWorkspace", b);
      alg.setPropertyValue("OutputWorkspace", "dummy");

      TSM_ASSERT_THROWS("Input unsmoothed and input Normalisation workspaces must have the same shape",
                        alg.execute(), std::runtime_error &);

  }

  void test_smooth_with_normalization_guidance() {

    const size_t nd = 1;
    MDHistoWorkspace_sptr toSmooth =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0 /*signal value*/, nd,
                                                     10);
    toSmooth->setSignalAt(7, 3);

    MDHistoWorkspace_sptr normWs = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1.0 /*signal value*/, nd, 10);
    normWs->setSignalAt(9, 0);

    /*
     1D MDHistoWorkspace for normalization

     1 - 1 - 1 - 1 - 1 - 1 - 1 - 1 - 1 - 0

     1D MDHistoWorkspace for smoothing

     2 - 2 - 2 - 2 - 2 - 2 - 2 - 3 - 2 - 2
     */

    SmoothMD alg;
    alg.setChild(true);
    alg.initialize();
    std::vector<int> widthVector(1, 3); // Smooth with width == 3
    alg.setProperty("WidthVector", widthVector);
    alg.setProperty("InputWorkspace", toSmooth);
    alg.setProperty("InputNormalizationWorkspace", normWs);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");

    TSM_ASSERT_EQUALS("Second index should have a smoothed using 2 "
                      "neighbours nothing ignored",
                      (toSmooth->getSignalAt(0) + toSmooth->getSignalAt(1) + toSmooth->getSignalAt(2)) / 3,
                      out->getSignalAt(1));


    TSM_ASSERT_EQUALS("Second to last index should have a smoothed using 1 "
                      "neighbour only neighour at 9 should be ignored",
                      (toSmooth->getSignalAt(8) + toSmooth->getSignalAt(7)) / 2,
                      out->getSignalAt(8));

    TSM_ASSERT("Last index should have a smoothed Value of NaN",
               boost::math::isnan(out->getSignalAt(9)));
  }
};

class SmoothMDTestPerformance : public CxxTest::TestSuite {
private:
  IMDHistoWorkspace_sptr m_toSmooth;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SmoothMDTestPerformance *createSuite() {
    return new SmoothMDTestPerformance();
  }
  static void destroySuite(SmoothMDTestPerformance *suite) { delete suite; }

  SmoothMDTestPerformance() {
    m_toSmooth = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        1 /*signal*/, 2 /*numDims*/, 500 /*numBins in each dimension*/);
  }

  void test_execute_hat_function() {
    SmoothMD alg;
    alg.setChild(true);
    alg.initialize();
    std::vector<int> widthVector(1, 5); // Smooth with width == 5
    alg.setProperty("WidthVector", widthVector);
    alg.setProperty("InputWorkspace", m_toSmooth);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
  }

  void test_execute_hat_function_with_normalisation() {
    SmoothMD alg;
    alg.setChild(true);
    alg.initialize();
    std::vector<int> widthVector(1, 3); // Smooth with width == 3
    alg.setProperty("WidthVector", widthVector);
    alg.setProperty("InputWorkspace", m_toSmooth);
    alg.setProperty("InputNormalizationWorkspace", m_toSmooth);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();
    IMDHistoWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
  }
};

#endif /* MANTID_MDALGORITHMS_SMOOTHMDTEST_H_ */
