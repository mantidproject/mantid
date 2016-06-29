#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHMTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorPreprocessingAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorPreprocessingAlgorithmTest *createSuite() {
    return new DataProcessorPreprocessingAlgorithmTest();
  }
  static void destroySuite(DataProcessorPreprocessingAlgorithmTest *suite) {
    delete suite;
  }
  DataProcessorPreprocessingAlgorithmTest() { FrameworkManager::Instance(); };

  void test_invalid_algorithms() {
    // Algorithm with a single input ws property
    TS_ASSERT_THROWS(DataProcessorPreprocessingAlgorithm("Rebin"),
                     std::invalid_argument);
    // Algorithm with more than two input ws properties
    TS_ASSERT_THROWS(
        DataProcessorPreprocessingAlgorithm("ReflectometryReductionOneAuto"),
        std::invalid_argument);
    // Algorithm with two input ws properties but no output ws properties
    TS_ASSERT_THROWS(DataProcessorPreprocessingAlgorithm("ConjoinWorkspaces"),
                     std::invalid_argument);
  }

  void test_valid_algorithms() {
    // Minus
    TS_ASSERT_THROWS_NOTHING(DataProcessorPreprocessingAlgorithm("Minus"));
    // Multiply
    TS_ASSERT_THROWS_NOTHING(DataProcessorPreprocessingAlgorithm("Multiply"));
    // Divide
    TS_ASSERT_THROWS_NOTHING(DataProcessorPreprocessingAlgorithm("Divide"));
    // Default: Plus
    TS_ASSERT_THROWS_NOTHING(DataProcessorPreprocessingAlgorithm());
    // WeightedMean
    TS_ASSERT_THROWS_NOTHING(
        DataProcessorPreprocessingAlgorithm("WeightedMean"));
  }

  void test_default() {

    // Default: no algorithm
    auto plus = DataProcessorPreprocessingAlgorithm();
    TS_ASSERT_EQUALS(plus.name(), "");
    TS_ASSERT_EQUALS(plus.lhsProperty(), "");
    TS_ASSERT_EQUALS(plus.rhsProperty(), "");
    TS_ASSERT_EQUALS(plus.outputProperty(), "");
    TS_ASSERT_EQUALS(plus.prefix(), "");
    TS_ASSERT_EQUALS(plus.blacklist().size(), 0);
  }

  void test_WeightedMean() {

    // WeightedMean
    std::set<std::string> blacklist = {"InputWorkspace1", "InputWorkspace2",
                                       "OutputWorkspace"};
    auto mean =
        DataProcessorPreprocessingAlgorithm("WeightedMean", "", blacklist);
    TS_ASSERT_EQUALS(mean.lhsProperty(), "InputWorkspace1");
    TS_ASSERT_EQUALS(mean.rhsProperty(), "InputWorkspace2");
    TS_ASSERT_EQUALS(mean.outputProperty(), "OutputWorkspace");
    TS_ASSERT_EQUALS(mean.blacklist().size(), 3);
  }

  // Add specific algorithms you want to test here
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHMTEST_H */
