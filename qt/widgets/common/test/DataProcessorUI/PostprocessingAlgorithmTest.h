// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHMTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class PostprocessingAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PostprocessingAlgorithmTest *createSuite() {
    return new PostprocessingAlgorithmTest();
  }
  static void destroySuite(PostprocessingAlgorithmTest *suite) { delete suite; }
  PostprocessingAlgorithmTest() { FrameworkManager::Instance(); };

  void test_invalid_algorithms() {
    // Algorithms with no 'str list' property
    TS_ASSERT_THROWS(PostprocessingAlgorithm("StepScan"),
                     const std::invalid_argument &);
    // Algorithms with more than one 'str list' property
    TS_ASSERT_THROWS(PostprocessingAlgorithm("PDDetermineCharacterizations"),
                     const std::invalid_argument &);
    // Algorithms with invalid output ws properties
    TS_ASSERT_THROWS(PostprocessingAlgorithm("GroupWorkspaces"),
                     const std::invalid_argument &);
  }

  void test_valid_algorithms() {
    // MergeRuns
    TS_ASSERT_THROWS_NOTHING(PostprocessingAlgorithm("MergeRuns"));
  }

  void test_Stitch1DMany() {

    auto stitch = PostprocessingAlgorithm(
        "Stitch1DMany", "IvsQ_",
        std::set<QString>{"InputWorkspaces", "OutputWorkspace"});
    TS_ASSERT_EQUALS(stitch.name(), "Stitch1DMany");
    TS_ASSERT_EQUALS(stitch.inputProperty(), "InputWorkspaces");
    TS_ASSERT_EQUALS(stitch.outputProperty(), "OutputWorkspace");
    TS_ASSERT_EQUALS(stitch.numberOfOutputProperties(), 1);
    TS_ASSERT_EQUALS(stitch.prefix(), "IvsQ_");
    std::set<QString> blacklist = {"InputWorkspaces", "OutputWorkspace"};
    TS_ASSERT_EQUALS(stitch.blacklist(), blacklist);
  }

  // Add more algorithms you want to test here
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPOSTPROCESSINGALGORITHMTEST_H */
