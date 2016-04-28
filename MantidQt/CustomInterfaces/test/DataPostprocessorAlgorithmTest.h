#ifndef MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHMTEST_H
#define MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataPostprocessorAlgorithm.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataPostprocessorAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataPostprocessorAlgorithmTest *createSuite() {
    return new DataPostprocessorAlgorithmTest();
  }
  static void destroySuite(DataPostprocessorAlgorithmTest *suite) {
    delete suite;
  }
  DataPostprocessorAlgorithmTest() { FrameworkManager::Instance(); };

  void test_invalid_algorithms() {
    // Algorithms with no 'str list' property
    TS_ASSERT_THROWS(DataPostprocessorAlgorithm("StepScan"),
                     std::invalid_argument);
    // Algorithms with more than one 'str list' property
    TS_ASSERT_THROWS(DataPostprocessorAlgorithm("PDDetermineCharacterizations"),
                     std::invalid_argument);
    // Algorithms with invalid output ws properties
    TS_ASSERT_THROWS(DataPostprocessorAlgorithm("GroupWorkspaces"),
                     std::invalid_argument);
  }

  void test_valid_algorithms() {
    TS_ASSERT_THROWS_NOTHING(DataPostprocessorAlgorithm("MergeRuns"));
    // The default algorithm is valid
    auto stitch = DataPostprocessorAlgorithm();
    TS_ASSERT_EQUALS(stitch.name(), "Stitch1DMany");
    TS_ASSERT_EQUALS(stitch.inputProperty(), "InputWorkspaces");
    TS_ASSERT_EQUALS(stitch.outputProperty(), "OutputWorkspace");
    TS_ASSERT_EQUALS(stitch.numberOfOutputProperties(), 1);
    TS_ASSERT_EQUALS(stitch.prefix(), "IvsQ_");
    std::set<std::string> blacklist = {"InputWorkspaces", "OutputWorkspace"};
    TS_ASSERT_EQUALS(stitch.blacklist(), blacklist);
  }
};
#endif /* MANTID_CUSTOMINTERFACES_DATAPOSTPROCESSORALGORITHMTEST_H */
