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
    QString testConstraint = "";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    std::pair<QString, std::pair<QString, QString>> error;
    TS_ASSERT_EQUALS(splitConstraints, error);
  }

  void test_splitConstraintString_double_constraint() {
    QString testConstraint = "0.1<A<0.2";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(QString("A"), std::make_pair(QString("0.1"), QString("0.2")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }

  void test_splitConstraintString_lower_bound() {
    QString testConstraint = "0.1<A";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(QString("A"), std::make_pair(QString("0.1"), QString("")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }

  void test_splitConstraintString_upper_bound() {
    QString testConstraint = "A<0.2";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(QString("A"), std::make_pair(QString(""), QString("0.2")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }

  void test_splitConstraintString_invalid_double_constraint() {
    QString testConstraint = "a<A<0.2";
    auto splitConstraints = MantidQt::MantidWidgets::splitConstraintString(testConstraint);
    auto expected_value = std::make_pair(QString(""), std::make_pair(QString(""), QString("")));
    TS_ASSERT_EQUALS(splitConstraints, expected_value);
  }
};
