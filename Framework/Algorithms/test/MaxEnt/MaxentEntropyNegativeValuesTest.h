#ifndef MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Maxent/MaxentEntropyNegativeValues.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using Mantid::Algorithms::MaxentEntropyNegativeValues;

class MaxentEntropyNegativeValuesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentEntropyNegativeValuesTest *createSuite() {
    return new MaxentEntropyNegativeValuesTest();
  }
  static void destroySuite(MaxentEntropyNegativeValuesTest *suite) {
    delete suite;
  }

  void test_derivative() {
    MaxentEntropyNegativeValues entropy;

    // -log(x + std::sqrt(x * x + 1))
    TS_ASSERT_DELTA(entropy.getDerivative(0), 0, 1E-6);
    TS_ASSERT_DELTA(entropy.getDerivative(1), -0.881374, 1E-6);
  }
  void test_second_derivative() {
    MaxentEntropyNegativeValues entropy;

    // fabs(x)
    TS_ASSERT_DELTA(entropy.getSecondDerivative(-1), 1, 1E-6);
    TS_ASSERT_DELTA(entropy.getSecondDerivative(-2), 2, 1E-6);
  }
  void test_correct_value() {
    MaxentEntropyNegativeValues entropy;

    TS_ASSERT_DELTA(entropy.correctValue(1, 0), 1, 1E-6);
    TS_ASSERT_DELTA(entropy.correctValue(-1, 0), -1, 1E-6);
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_ */