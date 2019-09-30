// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_NUMERICAXISVALIDATORTEST_H_
#define MANTID_API_NUMERICAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NumericAxisValidator.h"
#include "MantidTestHelpers/FakeObjects.h"

using Mantid::API::NumericAxisValidator;

class NumericAxisValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NumericAxisValidatorTest *createSuite() {
    return new NumericAxisValidatorTest();
  }
  static void destroySuite(NumericAxisValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    auto newAxis = std::make_unique<NumericAxis>(2);
    ws->replaceAxis(1, std::move(newAxis));
    NumericAxisValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "");
  }

  void test_fail() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    NumericAxisValidator validator;
    TS_ASSERT_EQUALS(
        validator.isValid(ws),
        "A workspace with axis being a Numeric Axis is required here.");
  }

  void test_axesless_workspace() {
    auto ws = boost::make_shared<AxeslessWorkspaceTester>();
    ws->initialize(2, 11, 10);
    NumericAxisValidator validator;
    std::string s;
    TS_ASSERT_THROWS_NOTHING(s = validator.isValid(ws))
    TS_ASSERT_DIFFERS(s, "")
  }
};

#endif /* MANTID_API_NUMERICAXISVALIDATORTEST_H_ */
