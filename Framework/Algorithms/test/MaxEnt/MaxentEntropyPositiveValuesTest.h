#ifndef MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentEntropyPositiveValues.h"

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
    // Some values
    std::vector<double> values = {1., 10., 100., M_E};
    // Background
    double background = 1.;
    // Derivative
    std::vector<double> result = entropy.derivative(values, background);
    TS_ASSERT_EQUALS(values.size(), result.size());
    // -log(x)
    TS_ASSERT_DELTA(result[0], 0, 1E-6);
    TS_ASSERT_DELTA(result[1], -2.3025, 1E-4);
    TS_ASSERT_DELTA(result[2], -4.6051, 1E-4);
    TS_ASSERT_DELTA(result[3], -1.0000, 1E-4);
  }
  void test_second_derivative() {

    MaxentEntropyPositiveValues entropy;
    // Some values
    std::vector<double> values = {1., 10., -100., M_E};
    // Metric
    std::vector<double> result = entropy.secondDerivative(values, 1);
    TS_ASSERT_EQUALS(result, values);
  }
  void test_correct_value() {

    MaxentEntropyPositiveValues entropy;
    // Some values
    std::vector<double> values = {-1., -10., 0.1};
    // Background
    double background = 1.;
    // Metric
    std::vector<double> result = entropy.correctValues(values, background);
    TS_ASSERT_EQUALS(values.size(), result.size());
    TS_ASSERT_DELTA(result[0], background, 1e-6);
    TS_ASSERT_DELTA(result[1], background, 1e-6);
    TS_ASSERT_DELTA(result[2], 0.1, 1e-6);
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_ */
