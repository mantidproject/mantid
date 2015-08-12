#ifndef MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_
#define MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ProcessIndirectFitParameters.h"

using Mantid::Algorithms::ProcessIndirectFitParameters;
using namespace Mantid::API;

class ProcessIndirectFitParametersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessIndirectFitParametersTest *createSuite() {
    return new ProcessIndirectFitParametersTest();
  }
  static void destroySuite(ProcessIndirectFitParametersTest *suite) {
    delete suite;
  }

  void test_empty_input_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("InputWorkspace", ""),
                     std::invalid_argument);
  }

  void test_empty_x_column_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("X Column", ""),
                     std::invalid_argument);
  }

  void test_that_empty_param_names_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Parameter Names", ""),
                     std::invalid_argument);
  }

  void test_empty_output_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("OutputWorkspace", ""),
                     std::invalid_argument);
  }
};

#endif /* MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_ */