// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_
#define MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_

#include "MantidKernel/DateAndTimeHelpers.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <cxxtest/TestSuite.h>

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

  void test_average() {
    std::vector<DateAndTime> times;
    TS_ASSERT_THROWS(averageSorted(times), const std::invalid_argument &);

    times.push_back(
        createFromSanitizedISO8601("1977-05-25T00:00Z")); // Star Wars IV
    times.push_back(
        createFromSanitizedISO8601("1977-09-11T00:00Z")); // ATARI 2600
    times.push_back(
        createFromSanitizedISO8601("1980-05-17T00:00Z")); // Star Wars V
    times.push_back(
        createFromSanitizedISO8601("1983-05-25T00:00Z")); // Star Wars VI
    TS_ASSERT_EQUALS(averageSorted(times),
                     createFromSanitizedISO8601("1979-09-19T00:00Z"));
  }
};

#endif /* MANTID_KERNEL_DATEANDTIMEHELPERSTEST_H_ */
