#ifndef MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentEntropyNegativeValues.h"
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

    // Some values
    std::vector<double> values = {0., 1., 10.};
    // Background
    double background = 1.;
    // Derivative
    std::vector<double> result = entropy.derivative(values, background);

    TS_ASSERT_EQUALS(values.size(), result.size());
    // -log(x + std::sqrt(x * x + 1))
    TS_ASSERT_DELTA(result[0], 0, 1E-6);
    TS_ASSERT_DELTA(result[1], -0.881374, 1E-6);
  }
  void test_second_derivative() {
    MaxentEntropyNegativeValues entropy;

    // Some values
    std::vector<double> values = {-1., -2., 10.};
    // Derivative
    std::vector<double> result = entropy.secondDerivative(values, 1);
    TS_ASSERT_EQUALS(values.size(), result.size());
    // fabs(x)
    TS_ASSERT_DELTA(result[0], std::sqrt(2), 1E-6);
    TS_ASSERT_DELTA(result[1], std::sqrt(5), 1E-6);
    TS_ASSERT_DELTA(result[2], std::sqrt(101), 1E-6);
  }
  void test_correct_value() {
    MaxentEntropyNegativeValues entropy;

    // Some values
    std::vector<double> values = {-1., -2., 10.};
    // Background
    double background = 1.;
    // Derivative
    std::vector<double> result = entropy.correctValues(values, background);
    TS_ASSERT_EQUALS(values.size(), result.size());
    TS_ASSERT_DELTA(result[0], -1, 1E-6);
    TS_ASSERT_DELTA(result[1], -2, 1E-6);
    TS_ASSERT_DELTA(result[2], 10, 1E-6);
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_ */
