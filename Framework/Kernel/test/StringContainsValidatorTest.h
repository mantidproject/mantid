// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/StringContainsValidator.h"

using Mantid::Kernel::StringContainsValidator;
using namespace Mantid::API;

class StringContainsValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StringContainsValidatorTest *createSuite() { return new StringContainsValidatorTest(); }
  static void destroySuite(StringContainsValidatorTest *suite) { delete suite; }

  void test_string_is_accepted_with_out_any_requirements() {
    StringContainsValidator validator;
    const std::string input = "This is a test string";
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  void test_one_word_required() {
    StringContainsValidator validator;
    validator.setRequiredStrings({"test"});
    const std::string input = "This is a test string";
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  void test_multiple_words_required() {
    StringContainsValidator validator;
    validator.setRequiredStrings({"test", "This"});
    const std::string input = "This is a test string";
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  void test_capitalisation_is_enforced_correctly() {
    StringContainsValidator validator;
    validator.setRequiredStrings({"this"});
    const std::string input = "This is a test string";
    const std::string error = "Error not all the required substrings were "
                              "contained within the input '" +
                              input + "'.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }

  void test_error_produced_if_string_does_not_contain_all_the_substrings() {
    StringContainsValidator validator;
    validator.setRequiredStrings({"not", "present"});
    const std::string input = "This is a test string";
    const std::string error = "Error not all the required substrings were "
                              "contained within the input '" +
                              input + "'.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }

  void test_error_produced_if_string_only_contains_some_of_the_substrings() {
    StringContainsValidator validator;
    validator.setRequiredStrings({"not", "This"});
    const std::string input = "This is a test string";
    const std::string error = "Error not all the required substrings were "
                              "contained within the input '" +
                              input + "'.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }

  void test_substring_is_allowed_to_contain_punctuation() {
    StringContainsValidator validator;
    validator.setRequiredStrings({","});
    const std::string input = "This, is a test string";
    TS_ASSERT_EQUALS("", validator.isValid(input));
  }

  void test_an_empty_string_produces_an_error() {
    StringContainsValidator validator;
    validator.setRequiredStrings({","});
    const std::string input = "";
    const std::string error = "A value must be entered for this parameter.";
    TS_ASSERT_EQUALS(error, validator.isValid(input));
  }
};
