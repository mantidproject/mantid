#ifndef MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_
#define MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvolutionFitSequential.h"

using Mantid::Algorithms::ConvolutionFitSequential;
using namespace Mantid::API;

class ConvolutionFitSequentialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvolutionFitSequentialTest *createSuite() {
    return new ConvolutionFitSequentialTest();
  }
  static void destroySuite(ConvolutionFitSequentialTest *suite) {
    delete suite;
  }

  // TODO: Check if this test is required (may only realistically be called by
  // convFitUI
  void test_function_is_in_correct_format() {}

  //-------------------------- Failure cases ----------------------------
  void test_empty_function_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Function", ""),
                     std::invalid_argument);
  }

  void test_empty_startX_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Start X", ""),
                     std::invalid_argument);
  }

  void test_empty_endX_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("End X", ""), std::invalid_argument);
  }

  void test_empty_specMin_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Spec Min", ""),
                     std::invalid_argument);
  }

  void test_empty_specMax_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Spec Max", ""),
                     std::invalid_argument);
  }

  void test_empty_maxIterations_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Max Iterations", ""),
                     std::invalid_argument);
  }

  // Temperature is allowed to be empty as there is not always a correction
  // required
  void test_empty_temperature_is_allowed() {}

  //------------------------- Execution cases ---------------------------
  void test_exec() {}

  void test_Something() {}
};

#endif /* MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_ */