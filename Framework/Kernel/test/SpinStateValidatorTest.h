// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidKernel/SpinStateValidator.h"

#include <boost/algorithm/string/join.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class SpinStateValidatorTest : public CxxTest::TestSuite {
public:
  void testSinglePairCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1});
    auto correctInputs = std::vector<std::string>{"01", "00", "10", "11", " 01", " 00 ", "11 "};
    checkAllInputs(validator, correctInputs, true);
  }

  void testSingleDigitCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1}, true);
    auto correctInputs = std::vector<std::string>{"01", "00", "10", "11", " 01", " 00 ", "11 ", "0", "1"};
    checkAllInputs(validator, correctInputs, true);
  }

  void testSpinStateSinglePairCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1}, false, '+', '-');
    auto correctInputs = std::vector<std::string>{"-+", "--", "+-", "++", " -+", " -- ", "++ "};
    checkAllInputs(validator, correctInputs, true);
  }

  void testSpinStateSingleDigitCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1}, true, '+', '-');
    auto correctInputs = std::vector<std::string>{"-+", "--", "+-", "++", " -+", " -- ", "++ ", "-", "+"};
    checkAllInputs(validator, correctInputs, true);
  }

  void testSingleIncorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1});
    auto incorrectInputs = std::vector<std::string>{"0 1", "2", "01,10", "!", "001", "", " "};
    checkAllInputs(validator, incorrectInputs, false);
  }

  void testSinglePairAndDigitIncorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1}, true);
    auto incorrectInputs = std::vector<std::string>{"0 1", "2", "01,10", "!", "001", "", " ", "01,1", "0,00"};
    checkAllInputs(validator, incorrectInputs, false);
  }

  void testDuplicateEntry() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3});
    auto duplicates = std::vector<std::string>{"01, 01", "11,10,11", "00,00"};
    checkAllInputs(validator, duplicates, false);
  }

  void testSpinStateDuplicateEntry() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3}, false, false);
    auto duplicates = std::vector<std::string>{"-+, -+", "++,+-,++", "--,--"};
    checkAllInputs(validator, duplicates, false);
  }

  void testDuplicateEntryWithSingleDigit() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3}, true);
    auto duplicates = std::vector<std::string>{"01, 01", "11,10,11", "00,00", "1,1,0", "0,1,0", "1,1"};
    checkAllInputs(validator, duplicates, false);
  }

  void testMultipleStatesCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3, 4});
    auto correctInputs = std::vector<std::string>{"01, 11", "00,10,11", "11,10, 00,01", "00, 10 "};
    checkAllInputs(validator, correctInputs, true);
  }

  void testSpinStateMultipleStatesCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3, 4}, false, '+', '-');
    auto correctInputs = std::vector<std::string>{"-+, ++", "--,+-,++", "++,+-, --,-+", "--, +- "};
    checkAllInputs(validator, correctInputs, true);
  }

  void testTwoSingleDigitCorrectInputs() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2}, true);
    auto correctInputs = std::vector<std::string>{"0, 1", "1,0"};
    checkAllInputs(validator, correctInputs, true);
  }

  void testSpinStateMixedWithFlipperConfig() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3, 4}, false, '+', '-');
    auto invalidInputs = std::vector<std::string>{"-+, 0+", "--,+-,11", "++,01, --,-+", "00, 1- "};
    checkAllInputs(validator, invalidInputs, false);
  }

  void testAllFourSpinStateCombos() {
    auto validator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
    auto correctInputs = std::vector<std::string>();
    std::vector<std::string> initialSpinConfig{{"01", "11", "10", "00"}};
    std::sort(initialSpinConfig.begin(), initialSpinConfig.end());
    correctInputs.push_back(boost::algorithm::join(initialSpinConfig, ","));
    while (std::next_permutation(initialSpinConfig.begin(), initialSpinConfig.end())) {
      correctInputs.push_back(boost::algorithm::join(initialSpinConfig, ","));
    }
    checkAllInputs(validator, correctInputs, true);
  }

private:
  void checkAllInputs(const std::shared_ptr<SpinStateValidator> validator, const std::vector<std::string> &inputsToTest,
                      const bool shouldBeValid) {
    std::string result = "";
    for (const auto &input : inputsToTest) {
      result = validator->isValid(input);
      ASSERT_TRUE(result.empty() == shouldBeValid);
    }
  }
};
