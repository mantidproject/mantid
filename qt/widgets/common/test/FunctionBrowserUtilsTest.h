// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include <cxxtest/TestSuite.h>

class FunctionBrowserUtilsTest : public CxxTest::TestSuite {

public:
  void test_splitConstraintString_returns_empty_if_given_empty_string() {
    std::string testConstraint = "";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    std::pair<std::string, std::pair<std::string, std::string>> error;
    TS_ASSERT_EQUALS(splitConstraints, error);
  }

  void test_splitConstraintString_double_constraint() {
    std::string testConstraint = "0.1<A<0.2";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(std::string("A"), std::make_pair(std::string("0.1"), std::string("0.2")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }

  void test_splitConstraintString_lower_bound() {
    std::string testConstraint = "0.1<A";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(std::string("A"), std::make_pair(std::string("0.1"), std::string("")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }

  void test_splitConstraintString_upper_bound() {
    std::string testConstraint = "A<0.2";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(std::string("A"), std::make_pair(std::string(""), std::string("0.2")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }

  void test_splitConstraintString_invalid_double_constraint() {
    std::string testConstraint = "a<A<0.2";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(std::string(""), std::make_pair(std::string(""), std::string("")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }
};
