#ifndef MANTID_KERNEL_EQUALBINSCHECKERTEST_H_
#define MANTID_KERNEL_EQUALBINSCHECKERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/EqualBinsChecker.h"

using Mantid::Kernel::EqualBinsChecker;

class EqualBinsCheckerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EqualBinsCheckerTest *createSuite() {
    return new EqualBinsCheckerTest();
  }
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

private:
  /// Generate a vector of X data with "error" percentage error
  Mantid::MantidVec generateData(const size_t length, const double error) {
    Mantid::MantidVec data;
    for (size_t i = 0; i < length; i++) {
      double val = static_cast<double>(i);
      if (i == length - 1) {
        val += error;
      }
      data.push_back(val);
    }
    return data;
  }
};

#endif /* MANTID_KERNEL_EQUALBINSCHECKERTEST_H_ */