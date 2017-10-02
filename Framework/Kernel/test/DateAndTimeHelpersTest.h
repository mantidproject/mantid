#ifndef MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_
#define MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DateAndTimeHelpers.h"

using namespace Mantid::Kernel;

class DateAndTimeHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DateAndTimeHelpersTest *createSuite() {
    return new DateAndTimeHelpersTest();
  }
  static void destroySuite(DateAndTimeHelpersTest *suite) { delete suite; }

  void test_stringIsISO8601() {
    TS_ASSERT(DateAndTimeHelpers::stringIsISO8601("1990-01-02 03:04:02.000"));
    TS_ASSERT(DateAndTimeHelpers::stringIsISO8601("1990-01-02T03:04:02.000"));
    TS_ASSERT(
        DateAndTimeHelpers::stringIsISO8601("1990-01-02T03:04:02.000+05:30"));
    TS_ASSERT(DateAndTimeHelpers::stringIsISO8601("1990-01-02 03:04"));
    TS_ASSERT(DateAndTimeHelpers::stringIsISO8601("1990-01-02"));
    TS_ASSERT(DateAndTimeHelpers::stringIsISO8601("1822-01-02"));

    TS_ASSERT(!DateAndTimeHelpers::stringIsISO8601("January 1, 2345"));
    TS_ASSERT(!DateAndTimeHelpers::stringIsISO8601("2010-31-56"));
    TS_ASSERT(!DateAndTimeHelpers::stringIsISO8601("1990-01-02 45:92:22"));
  }
};

#endif /* MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_ */