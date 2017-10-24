#ifndef MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_
#define MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTypes/Core/DateAndTime.h"
#include "MantidKernel/DateAndTimeHelpers.h"

using namespace Mantid::Kernel::DateAndTimeHelpers;
using Mantid::Types::Core::DateAndTime;

class DateAndTimeHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DateAndTimeHelpersTest *createSuite() {
    return new DateAndTimeHelpersTest();
  }
  static void destroySuite(DateAndTimeHelpersTest *suite) { delete suite; }

  void test_stringIsISO8601() {
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04:02.000"));
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04:02.000"));
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04:02.000+05:30"));
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04"));
    TS_ASSERT(stringIsISO8601("1990-01-02"));
    TS_ASSERT(stringIsISO8601("1822-01-02"));

    TS_ASSERT(!stringIsISO8601("January 1, 2345"));
    TS_ASSERT(!stringIsISO8601("2010-31-56"));
    TS_ASSERT(!stringIsISO8601("1990-01-02 45:92:22"));
  }

  void test_verifyAndSanitizeISO8601() {
    TS_ASSERT_EQUALS(verifyAndSanitizeISO8601("1990- 1- 2T03:04:02.000"),
                     "1990-01-02T03:04:02.000");
    TS_ASSERT_EQUALS(verifyAndSanitizeISO8601("1882-01- 2T03:04:02.000"),
                     "1882-01-02T03:04:02.000");
  }

  void test_createFromSantizedISO8601() {
    DateAndTime date;

    TS_ASSERT_THROWS_NOTHING(
        date = createFromSanitizedISO8601("1882-01- 2T03:04:02"));
    TS_ASSERT_EQUALS(date.toISO8601String(), "1882-01-02T03:04:02");

    date = createFromSanitizedISO8601("1990- 1- 2T03:04:02.001");
    TS_ASSERT_EQUALS(date.toISO8601String(), "1990-01-02T03:04:02.001000000");
  }
};

#endif /* MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_ */