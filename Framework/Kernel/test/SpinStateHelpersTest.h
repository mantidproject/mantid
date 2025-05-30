// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidKernel/SpinStateHelpers.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;

class SpinStateHelpersTest : public CxxTest::TestSuite {
public:
  void testSplitSpinStateStringSuccessful() { runTestSplitSpinStateString("01,11,10,00", {"01", "11", "10", "00"}); }

  void testSplitSpinStateStringWithSpaces() {
    runTestSplitSpinStateString(" 01 ,  11 , 10 ,  00 ", {"01", "11", "10", "00"});
  }

  void testSplitSpinStateStringEmptyString() { runTestSplitSpinStateString("", {}); }

  void testSplitSpinStateStringSingleItem() { runTestSplitSpinStateString("01", {"01"}); }

  void testIndexOfWorkspaceForSpinState_TargetStateExists() {
    runTestIndexOfWorkspaceForSpinState({"00", "11", "10", "01"}, "10", 2);
  }

  void testIndexOfWorkspaceForSpinState_TargetStateDoesNotExist() {
    runTestIndexOfWorkspaceForSpinState({"00", "11", "10", "01"}, "invalid_state");
  }

  void testIndexOfWorkspaceForSpinState_EmptySpinStateOrder() { runTestIndexOfWorkspaceForSpinState({}, "10"); }

  void testIndexOfWorkspaceForSpinState_DuplicateEntries() {
    runTestIndexOfWorkspaceForSpinState({"10", "10", "11"}, "10", 0);
  }

  void testIndexOfWorkspaceForSpinState_TrimWhitespace() {
    runTestIndexOfWorkspaceForSpinState({"00", "11", "10", "01"}, " 10 ", 2);
  }

private:
  void runTestSplitSpinStateString(const std::string &spinStates, const std::vector<std::string> &expectedResult) {
    std::vector<std::string> result = SpinStateHelpers::splitSpinStateString(spinStates);
    TS_ASSERT_EQUALS(result.size(), expectedResult.size());

    for (size_t i = 0; i < result.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], expectedResult[i]);
    }
  }

  void runTestIndexOfWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder,
                                           const std::string &targetSpinState,
                                           const std::optional<size_t> expectedIndex = std::nullopt) {
    auto index = SpinStateHelpers::indexOfWorkspaceForSpinState(spinStateOrder, targetSpinState);
    TS_ASSERT_EQUALS(index, expectedIndex);
  }
};
