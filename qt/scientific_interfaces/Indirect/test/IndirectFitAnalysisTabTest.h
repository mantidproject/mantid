// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITANALYSISTABTEST_H_
#define MANTIDQT_INDIRECTFITANALYSISTABTEST_H_

#include "IndirectFitAnalysisTab.cpp"
#include "MantidAPI/FunctionFactory.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

class IndirectFitAnalysisTabTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectFitAnalysisTabTest() {}

  static IndirectFitAnalysisTabTest *createSuite() {
    return new IndirectFitAnalysisTabTest();
  }

  static void destroySuite(IndirectFitAnalysisTabTest *suite) { delete suite; }

  void test_that_single_function_correctly_identified() {
    std::string functionName = "ExpDecay";
    auto fitFunction =
        Mantid::API::FunctionFactory::Instance().createFunction(functionName);
    auto occurances =
        getNumberOfSpecificFunctionContained(functionName, fitFunction);
    TS_ASSERT_EQUALS(occurances, 1);
  }

  void test_that_single_layer_composite_function_handled_correctly() {
    std::string functionName = "name=ExpDecay;name=StretchExp";
    auto fitFunction =
        Mantid::API::FunctionFactory::Instance().createInitialized(
            functionName);
    auto occurances =
        getNumberOfSpecificFunctionContained("ExpDecay", fitFunction);
    auto stretchOccurances =
        getNumberOfSpecificFunctionContained("StretchExp", fitFunction);
    TS_ASSERT_EQUALS(occurances, 1);
    TS_ASSERT_EQUALS(stretchOccurances, 1);
  }

  void test_that_multi_layer_composite_function_handled_correctly() {
    std::string functionName = "name=ExpDecay;name=ExpDecay;(composite="
                               "ProductFunction,NumDeriv=false;name=ExpDecay;"
                               "name=ExpDecay)";
    auto fitFunction =
        Mantid::API::FunctionFactory::Instance().createInitialized(
            functionName);
    auto occurances =
        getNumberOfSpecificFunctionContained("ExpDecay", fitFunction);
    TS_ASSERT_EQUALS(occurances, 4);
  }
};
#endif