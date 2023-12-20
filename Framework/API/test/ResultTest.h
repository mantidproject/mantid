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
  void testStringErrors() {
    std::string in = "CorrectResult";
    testWithT(in);
  }

  void testDoubleErrors() {
    double result = 1.0;
    testWithT(result);
  }

  void testIntErrors() {
    int result = 5;
    testWithT(result);
  }

  void testResultInception() {
    // It's a Result<Result<double>> we're testing.
    auto result = Result<double>(2.5);
    testWithT(result);
  }

private:
  template <typename T> void testWithT(T &in) {
    auto testResult = Result<T>(in);
    TS_ASSERT_EQUALS(true, testResult);
    TS_ASSERT_EQUALS("", testResult.errors());
    testResult = Result<T>(in, "Error");
    TS_ASSERT_EQUALS(false, testResult);
    TS_ASSERT_EQUALS("Error", testResult.errors());
    TS_ASSERT_EQUALS(in, testResult.result());
  }
};