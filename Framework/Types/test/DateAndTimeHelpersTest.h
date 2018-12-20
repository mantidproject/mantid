#ifndef MANTID_TYPES_CORE_DATEANDTIMEHELPERSTEST_H_
#define MANTID_TYPES_CORE_DATEANDTIMEHELPERSTEST_H_

#include "MantidTypes/Core/DateAndTimeHelpers.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Types::Core::DateAndTimeHelpers;

class DateAndTimeHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DateAndTimeHelpersTest *createSuite() {
    return new DateAndTimeHelpersTest();
  }
  static void destroySuite(DateAndTimeHelpersTest *suite) { delete suite; }

  void test_stringIsISO8601_basic_format() {
    TS_ASSERT(stringIsISO8601("19900102 030402.000"));
    TS_ASSERT(stringIsISO8601("19900102T030402.000"));
    TS_ASSERT(stringIsISO8601("19900102T030402.000+05:30"));
    TS_ASSERT(stringIsISO8601("19900102T030402.000+0530"));
    TS_ASSERT(stringIsISO8601("19900102T030402.000+05"));
    TS_ASSERT(stringIsISO8601("19900102 030402.000Z"))
    TS_ASSERT(stringIsISO8601("19900102 030402Z"))
    TS_ASSERT(stringIsISO8601("19900102 0304Z"))
    TS_ASSERT(stringIsISO8601("19900102T0304Z"))
    TS_ASSERT(stringIsISO8601("19900102 0304"));
    TS_ASSERT(stringIsISO8601("19900102"));
    TS_ASSERT(stringIsISO8601("18220102"));

    TS_ASSERT(!stringIsISO8601("January 1, 2345"));
    TS_ASSERT(!stringIsISO8601("20103156"));
    TS_ASSERT(!stringIsISO8601("19900102 459222"));
    TS_ASSERT(!stringIsISO8601("19900102 030402.000Z00:00"))
  }

  void test_stringIsISO8601_extended_format() {
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04:02.000"));
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04:02.000"));
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04:02.000+05:30"));
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04:02.000+0530"));
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04:02.000+05"));
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04:02.000Z"))
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04:02Z"))
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04Z"))
    TS_ASSERT(stringIsISO8601("1990-01-02T03:04Z"))
    TS_ASSERT(stringIsISO8601("1990-01-02 03:04"));
    TS_ASSERT(stringIsISO8601("1990-01-02"));
    TS_ASSERT(stringIsISO8601("1822-01-02"));

    TS_ASSERT(!stringIsISO8601("January 1, 2345"));
    TS_ASSERT(!stringIsISO8601("2010-31-56"));
    TS_ASSERT(!stringIsISO8601("1990-01-02 45:92:22"));
    TS_ASSERT(!stringIsISO8601("1990-01-02 03:04:02.000Z00:00"))
  }

  void test_stringIsPosix() {
    TS_ASSERT(stringIsPosix("1990-Jan-02 03:04:02.000"));
    TS_ASSERT(stringIsPosix("1990-Jan-02 03:04:02"))

    TS_ASSERT(!stringIsPosix("January 1, 2345"));
    TS_ASSERT(!stringIsPosix("1990-01-02 03:04:02"))
    TS_ASSERT(!stringIsPosix("1990-jan-01 02:04:02"))
    TS_ASSERT(!stringIsPosix("2010-Jan-56"));
    TS_ASSERT(!stringIsPosix("1990-Jan-02 45:92:22"));
    TS_ASSERT(!stringIsPosix("1990-Jan-40 03:04:02"))
    TS_ASSERT(!stringIsPosix("1990-Jan-01 30:04:02"))
    TS_ASSERT(!stringIsPosix("1990-Jan-02 03:04:02.000Z"))
    TS_ASSERT(!stringIsPosix("1990-Jan-40 03:04:02"))
  }
};

#endif /* MANTID_TYPES_CORE_DATEANDTIMEHELPERSTEST_H_ */
