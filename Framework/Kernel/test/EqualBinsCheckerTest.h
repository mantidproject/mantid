// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/EqualBinsChecker.h"

using Mantid::Kernel::EqualBinsChecker;

/**
 * Derived class used for testing protected methods
 */
class TestEqualBinsChecker : public EqualBinsChecker {
public:
  TestEqualBinsChecker(const Mantid::MantidVec &xData, const double errorLevel)
      : EqualBinsChecker(xData, errorLevel) {};
  double wrapGetReferenceDx() const { return this->getReferenceDx(); }
  double wrapGetDifference(const size_t bin, const double dx) const { return this->getDifference(bin, dx); }
};

class EqualBinsCheckerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EqualBinsCheckerTest *createSuite() { return new EqualBinsCheckerTest(); }
  static void destroySuite(EqualBinsCheckerTest *suite) { delete suite; }

  void test_validate_pass() {
    const auto fivePercentData = generateData(10, 0.05);
    EqualBinsChecker checker(fivePercentData, 0.1, -1);
    const std::string message = checker.validate();
    TS_ASSERT(message.empty());
  }

  void test_validate_fail() {
    const auto tenPercentData = generateData(10, 0.1);
    EqualBinsChecker checker(tenPercentData, 0.05, -1);
    const std::string message = checker.validate();
    TS_ASSERT(!message.empty());
  }

  void test_getReferenceDx_First() {
    const auto data = generateData(10, 1.0);
    TestEqualBinsChecker checker(data, 0.1);
    checker.setReferenceBin(EqualBinsChecker::ReferenceBin::First);
    const double dx = checker.wrapGetReferenceDx();
    TS_ASSERT_DELTA(dx, 1.0, 1e-7);
  }

  void test_getReferenceDx_Average() {
    const auto data = generateData(10, 1.0);
    TestEqualBinsChecker checker(data, 0.1);
    // Average should be the default - no need to set
    const double dx = checker.wrapGetReferenceDx();
    TS_ASSERT_DELTA(dx, 1.1111111, 1e-7);
  }

  void test_getDifference_Individual() {
    auto data = generateData(10, 0.0);
    data[3] = 3.1;
    TestEqualBinsChecker checker(data, 0.1);
    checker.setErrorType(EqualBinsChecker::ErrorType::Individual);
    const double errorLower = checker.wrapGetDifference(2, 1.0);
    const double errorHigher = checker.wrapGetDifference(3, 1.0);
    TS_ASSERT_DELTA(errorLower, 0.1, 1e-7);
    TS_ASSERT_DELTA(errorHigher, 0.1, 1e-7);
  }

  void test_getDifference_Cumulative() {
    auto data = generateData(10, 0.0);
    data[3] = 3.1;
    TestEqualBinsChecker checker(data, 0.1);
    // Cumulative should be default - no need to set
    const double errorLower = checker.wrapGetDifference(2, 1.0);
    const double errorHigher = checker.wrapGetDifference(3, 1.0);
    TS_ASSERT_DELTA(errorLower, 0.1, 1e-7);
    TS_ASSERT_DELTA(errorHigher, 0.0, 1e-7);
  }

private:
  /// Generate a vector of X data with "error" percentage error
  Mantid::MantidVec generateData(const size_t length, const double error) {
    Mantid::MantidVec data;
    for (size_t i = 0; i < length; i++) {
      double val = static_cast<double>(i);
      if (i == length - 1) {
        val += error;
      }
      data.emplace_back(val);
    }
    return data;
  }
};
