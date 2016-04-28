#ifndef MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHMTEST_H
#define MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataPreprocessorAlgorithm.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataPreprocessorAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataPreprocessorAlgorithmTest *createSuite() {
    return new DataPreprocessorAlgorithmTest();
  }
  static void destroySuite(DataPreprocessorAlgorithmTest *suite) {
    delete suite;
  }
  DataPreprocessorAlgorithmTest() { FrameworkManager::Instance(); };

  void test_invalid_algorithms() {
    // Algorithm with a single input ws property
    TS_ASSERT_THROWS(DataPreprocessorAlgorithm("Rebin"), std::invalid_argument);
    // Algorithm with more than two input ws properties
    TS_ASSERT_THROWS(DataPreprocessorAlgorithm("ReflectometryReductionOneAuto"),
                     std::invalid_argument);
    // Algorithm with two input ws properties but no output ws properties
    TS_ASSERT_THROWS(DataPreprocessorAlgorithm("ConjoinWorkspaces"),
                     std::invalid_argument);
  }

  void test_valid_algorithms() {
		// WeightedMean
		TS_ASSERT_THROWS_NOTHING(DataPreprocessorAlgorithm("WeightedMean"));
    // Minus
    TS_ASSERT_THROWS_NOTHING(DataPreprocessorAlgorithm("Minus"));
    // Multiply
    TS_ASSERT_THROWS_NOTHING(DataPreprocessorAlgorithm("Multiply"));
    // Divide
    TS_ASSERT_THROWS_NOTHING(DataPreprocessorAlgorithm("Divide"));
    // Default: Plus
    auto plus = DataPreprocessorAlgorithm();
    TS_ASSERT_EQUALS(plus.name(), "Plus");
    TS_ASSERT_EQUALS(plus.lhsProperty(), "LHSWorkspace");
    TS_ASSERT_EQUALS(plus.rhsProperty(), "RHSWorkspace");
    TS_ASSERT_EQUALS(plus.outputProperty(), "OutputWorkspace");
    TS_ASSERT_EQUALS(plus.prefix(), "TOF_");
    TS_ASSERT_EQUALS(plus.show(), true);
    std::set<std::string> blacklist = {"LHSWorkspace", "RHSWorkspace",
                                       "OutputWorkspace"};
    TS_ASSERT_EQUALS(plus.blacklist(), blacklist);
  }
};
#endif /* MANTID_CUSTOMINTERFACES_DATAPREPROCESSORALGORITHMTEST_H */
