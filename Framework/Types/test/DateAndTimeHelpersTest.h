#ifndef MANTID_TYPES_CORE_DATEANDTIMEHELPERSTEST_H_
#define MANTID_TYPES_CORE_DATEANDTIMEHELPERSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTypes/Core/DateAndTimeHelpers.h"

using namespace Mantid::Types::Core::DateAndTimeHelpers;

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
};

#endif /* MANTID_TYPES_CORE_DATEANDTIMEHELPERSTEST_H_ */
