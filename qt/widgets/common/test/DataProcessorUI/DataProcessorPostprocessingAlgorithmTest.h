#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHMTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorPostprocessingAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorPostprocessingAlgorithmTest *createSuite() {
    return new DataProcessorPostprocessingAlgorithmTest();
  }
  static void destroySuite(DataProcessorPostprocessingAlgorithmTest *suite) {
    delete suite;
  }
  DataProcessorPostprocessingAlgorithmTest() { FrameworkManager::Instance(); };

  void test_invalid_algorithms() {
    // Algorithms with no 'str list' property
    TS_ASSERT_THROWS(DataProcessorPostprocessingAlgorithm("StepScan"),
                     std::invalid_argument);
    // Algorithms with more than one 'str list' property
    TS_ASSERT_THROWS(
        DataProcessorPostprocessingAlgorithm("PDDetermineCharacterizations"),
        std::invalid_argument);
    // Algorithms with invalid output ws properties
    TS_ASSERT_THROWS(DataProcessorPostprocessingAlgorithm("GroupWorkspaces"),
                     std::invalid_argument);
  }

  void test_valid_algorithms() {
    // MergeRuns
    TS_ASSERT_THROWS_NOTHING(DataProcessorPostprocessingAlgorithm("MergeRuns"));
  }

  void test_Stitch1DMany() {

    auto stitch = DataProcessorPostprocessingAlgorithm(
        "Stitch1DMany", "IvsQ_",
        std::set<std::string>{"InputWorkspaces", "OutputWorkspace"});
    TS_ASSERT_EQUALS(stitch.name(), "Stitch1DMany");
    TS_ASSERT_EQUALS(stitch.inputProperty(), "InputWorkspaces");
    TS_ASSERT_EQUALS(stitch.outputProperty(), "OutputWorkspace");
    TS_ASSERT_EQUALS(stitch.numberOfOutputProperties(), 1);
    TS_ASSERT_EQUALS(stitch.prefix(), "IvsQ_");
    std::set<std::string> blacklist = {"InputWorkspaces", "OutputWorkspace"};
    TS_ASSERT_EQUALS(stitch.blacklist(), blacklist);
  }

  // Add more algorithms you want to test here
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHMTEST_H */
