#ifndef MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxentEntropyPositiveValues.h"

using Mantid::Algorithms::MaxentEntropyPositiveValues;

class MaxentEntropyPositiveValuesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentEntropyPositiveValuesTest *createSuite() {
    return new MaxentEntropyPositiveValuesTest();
  }
  static void destroySuite(MaxentEntropyPositiveValuesTest *suite) {
    delete suite;
  }

  void test_derivative() {
    MaxentEntropyPositiveValues entropy;

    // -log(x)
    TS_ASSERT_DELTA(entropy.getDerivative(1), 0, 1E-6);
    TS_ASSERT_DELTA(entropy.getDerivative(M_E), -1, 1E-6);
  }
  void test_second_derivative() {
    MaxentEntropyPositiveValues entropy;

    // x
    TS_ASSERT_DELTA(entropy.getSecondDerivative(1), 1, 1E-6);
    TS_ASSERT_DELTA(entropy.getSecondDerivative(M_E), M_E, 1E-6);
  }
  void test_correct_value() {
    MaxentEntropyPositiveValues entropy;

    TS_ASSERT_DELTA(entropy.correctValue(1, 0), 1, 1E-6);
    TS_ASSERT_DELTA(entropy.correctValue(-1, 0), 0, 1E-6);
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_ */