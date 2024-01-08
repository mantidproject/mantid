// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include <cxxtest/TestSuite.h>

#include <MantidAPI/Result.h>

using namespace Mantid::API;

class ResultTest : public CxxTest::TestSuite {
public:
  void testStringSuccessHasNoErrors() {
    std::string in = "CorrectResult";
    testSuccessHasNoErrors(in);
  }

  void testStringFailureHasErrors() {
    std::string in = "BadResult";
    testFailureHasErrors(in);
  }

  void testDoubleSuccessHasNoErrors() {
    double result = 1.0;
    testSuccessHasNoErrors(result);
  }

  void testDoubleFailureHasErrors() {
    double result = 1.0;
    testFailureHasErrors(result);
  }

  void testIntSuccessHasNoErrors() {
    int result = 5;
    testSuccessHasNoErrors(result);
  }

  void testIntFailureHasErrors() {
    int result = 5;
    testFailureHasErrors(result);
  }

  void testResultInceptionSuccessHasNoErrors() {
    // It's a Result<Result<double>> we're testing.
    auto result = Result<double>(2.5);
    testSuccessHasNoErrors(result);
  }

  void testResultInceptionFailureHasErrors() {
    // It's a Result<Result<double>> we're testing.
    auto result = Result<double>(2.5);
    testFailureHasErrors(result);
  }

private:
  template <typename T> void testSuccessHasNoErrors(const T &in) {
    auto testResult = Result<T>(in);
    TS_ASSERT_EQUALS(true, testResult);
    TS_ASSERT_EQUALS("", testResult.errors());
    TS_ASSERT_EQUALS(in, testResult.result());
  }

  template <typename T> void testFailureHasErrors(const T &in) {
    auto testResult = Result<T>(in, "Error");
    TS_ASSERT_EQUALS(false, testResult);
    TS_ASSERT_EQUALS("Error", testResult.errors());
    TS_ASSERT_EQUALS(in, testResult.result());
  }
};