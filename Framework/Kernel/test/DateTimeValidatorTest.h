// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DateTimeValidator.h"

using Mantid::Kernel::DateTimeValidator;

class DateTimeValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DateTimeValidatorTest *createSuite() { return new DateTimeValidatorTest(); }
  static void destroySuite(DateTimeValidatorTest *suite) { delete suite; }

  //---------------------------- Success cases
  //--------------------------------------

  void test_full_extended_iso_format_is_accepted() {
    DateTimeValidator validator;
    const std::string input = "2014-03-21T00:01:30.52";
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  void test_time_without_fractional_seconds_in_iso_format_is_accepted() {
    DateTimeValidator validator;
    const std::string input = "2014-03-21T00:01:30";
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  //---------------------------- Failure cases
  //--------------------------------------

  void test_empty_string_is_invalid_when_allowed_is_false() {
    DateTimeValidator validator;
    validator.allowEmpty(false);
    TS_ASSERT_EQUALS("Error interpreting string '' as a date/time.", validator.isValid(""));
  }

  void test_empty_string_is_valid_when_allowed_is_true() {
    DateTimeValidator validator;
    validator.allowEmpty(true);
    TS_ASSERT_EQUALS("", validator.isValid(""));
  }

  void test_text_string_is_invalid() {
    DateTimeValidator validator;
    const std::string input = "not a timestamp";
    const std::string error = "Error interpreting string '" + input + "' as a date/time.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }

  void test_date_alone_is_invalid() {
    DateTimeValidator validator;
    const std::string input = "2014-03-21";
    const std::string error = "Error interpreting string '" + input + "' as a date/time.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }

  void test_time_alone_is_invalid() {
    DateTimeValidator validator;
    const std::string input = "09:03:30";
    const std::string error = "Error interpreting string '" + input + "' as a date/time.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }
};
