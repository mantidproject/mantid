#ifndef NORMALDISTRIBUTIONTEST_H_
#define NORMALDISTRIBUTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/NormalDistribution.h"

using Mantid::Kernel::NormalDistribution;

class NormalDistributionTest : public CxxTest::TestSuite {

public:
  void test_That_Object_Construction_Does_Not_Throw() {
    TS_ASSERT_THROWS_NOTHING(NormalDistribution());
    TS_ASSERT_THROWS_NOTHING(NormalDistribution(2.0, 1.1));
    TS_ASSERT_THROWS_NOTHING(NormalDistribution(1, 2.0, 1.1));
  }

  void test_bad_input() {
    TS_ASSERT_THROWS(NormalDistribution(1.0, 0.0), std::runtime_error);
#ifdef NDEBUG
    // In debug assert in boost code calls abort()
    TS_ASSERT_THROWS(NormalDistribution(1.0, -1.0), std::runtime_error);
#endif // NDEBUG
  }

  void test_standard_normal_distribution() {
    NormalDistribution norm;
    size_t in(0), out(0);
    for(size_t i = 0; i < 100; ++i) {
      auto value = norm.nextValue();
      if (std::fabs(value) < 1.0) {
        ++in;
      } else {
        ++out;
      }
    }
    TS_ASSERT_LESS_THAN(out, in);
  }

  void test_normal_distribution() {
    NormalDistribution norm(30.0, 5.0);
    size_t in(0), out(0);
    for(size_t i = 0; i < 100; ++i) {
      auto value = (norm.nextValue() - 30.0) / 5.0;
      if (std::fabs(value) < 1.0) {
        ++in;
      } else {
        ++out;
      }
    }
    TS_ASSERT_LESS_THAN(out, in);
  }

};

#endif // NORMALDISTRIBUTIONTEST_H_
