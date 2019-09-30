// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHMTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class PreprocessingAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PreprocessingAlgorithmTest *createSuite() {
    return new PreprocessingAlgorithmTest();
  }
  static void destroySuite(PreprocessingAlgorithmTest *suite) { delete suite; }
  PreprocessingAlgorithmTest() { FrameworkManager::Instance(); };

  void test_invalid_algorithms() {
    // Algorithm with a single input ws property
    TS_ASSERT_THROWS(PreprocessingAlgorithm("Rebin"),
                     const std::invalid_argument &);
    // Algorithm with more than two input ws properties
    TS_ASSERT_THROWS(PreprocessingAlgorithm("ReflectometryReductionOneAuto"),
                     const std::invalid_argument &);
    // Algorithm with two input ws properties but no output ws properties
    TS_ASSERT_THROWS(PreprocessingAlgorithm("ConjoinWorkspaces"),
                     const std::invalid_argument &);
  }

  void test_valid_algorithms() {
    // Minus
    TS_ASSERT_THROWS_NOTHING(PreprocessingAlgorithm("Minus"));
    // Multiply
    TS_ASSERT_THROWS_NOTHING(PreprocessingAlgorithm("Multiply"));
    // Divide
    TS_ASSERT_THROWS_NOTHING(PreprocessingAlgorithm("Divide"));
    // Default: Plus
    TS_ASSERT_THROWS_NOTHING(PreprocessingAlgorithm());
    // WeightedMean
    TS_ASSERT_THROWS_NOTHING(PreprocessingAlgorithm("WeightedMean"));
  }

  void test_default() {

    // Default: no algorithm
    auto plus = PreprocessingAlgorithm();
    TS_ASSERT_EQUALS(plus.name(), "");
    TS_ASSERT_EQUALS(plus.lhsProperty(), "");
    TS_ASSERT_EQUALS(plus.rhsProperty(), "");
    TS_ASSERT_EQUALS(plus.outputProperty(), "");
    TS_ASSERT_EQUALS(plus.prefix(), "");
    TS_ASSERT_EQUALS(plus.separator(), "");
    TS_ASSERT_EQUALS(plus.blacklist().size(), 0);
  }

  void test_WeightedMean() {

    // WeightedMean
    std::set<QString> blacklist = {"InputWorkspace1", "InputWorkspace2",
                                   "OutputWorkspace"};
    auto mean = PreprocessingAlgorithm("WeightedMean", "", "+", blacklist);
    TS_ASSERT_EQUALS(mean.lhsProperty(), "InputWorkspace1");
    TS_ASSERT_EQUALS(mean.rhsProperty(), "InputWorkspace2");
    TS_ASSERT_EQUALS(mean.outputProperty(), "OutputWorkspace");
    TS_ASSERT_EQUALS(mean.blacklist().size(), 3);
  }

  // Add specific algorithms you want to test here
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPREPROCESSINGALGORITHMTEST_H */
