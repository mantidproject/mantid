// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeROI.h"

using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;

constexpr double ONE_DAY_DURATION{24 * 3600};

const std::string HANUKKAH_START("2022-12-19T00:01");
const std::string HANUKKAH_STOP("2022-12-26T00:01");
constexpr double HANUKKAH_DURATION{7. * ONE_DAY_DURATION};

const std::string CHRISTMAS_START("2022-12-25T00:01");
const std::string CHRISTMAS_STOP("2022-12-26T00:01");

const std::string NEW_YEARS_START("2022-12-31T00:01");
const std::string NEW_YEARS_STOP("2023-01-01T00:01");

class TimeROITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeROITest *createSuite() { return new TimeROITest(); }
  static void destroySuite(TimeROITest *suite) { delete suite; }

  void test_emptyROI() {
    TimeROI value;
    TS_ASSERT_EQUALS(value.durationInSeconds(), 0.);
    TS_ASSERT(value.empty());
    TS_ASSERT_EQUALS(value.numBoundaries(), 0);
    value.removeRedundantEntries();
  }
  void test_sortedROI() {
    TimeROI value;
    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    TS_ASSERT(!value.empty());

    // add New Year's eve
    value.addROI(NEW_YEARS_START, NEW_YEARS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);

    // add Christmas - fully contained in existing TimeROI
    value.addROI(CHRISTMAS_START, CHRISTMAS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 6);

    // get rid of entries that have no effect
    value.removeRedundantEntries();
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
  }

  void test_reversesortedROI() {
    TimeROI value;
    // add New Year's eve
    value.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value.durationInSeconds(), ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);

    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), ONE_DAY_DURATION + HANUKKAH_DURATION);
    TS_ASSERT_EQUALS(value.numBoundaries(), 4);
  }

  void test_onlyMask() {
    TimeROI value;
    value.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));

    // since it ends with "on" the duration is infinite
    TS_ASSERT_LESS_THAN(ONE_DAY_DURATION, value.durationInSeconds());
    TS_ASSERT_EQUALS(value.numBoundaries(), 2);
    value.removeRedundantEntries();
    TS_ASSERT_EQUALS(value.numBoundaries(), 1);
  }

  void test_overwrite() {
    // mask first
    TimeROI value1;
    value1.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    // since it ends with "on" the duration is infinite
    TS_ASSERT_LESS_THAN(ONE_DAY_DURATION, value1.durationInSeconds());
    TS_ASSERT_EQUALS(value1.numBoundaries(), 2);

    value1.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value1.durationInSeconds(), ONE_DAY_DURATION);
    value1.removeRedundantEntries();
    TS_ASSERT_EQUALS(value1.numBoundaries(), 2);

    // roi first
    TimeROI value2;
    value2.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value2.durationInSeconds(), ONE_DAY_DURATION);
    TS_ASSERT_EQUALS(value2.numBoundaries(), 2);

    value2.addMask(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_LESS_THAN(ONE_DAY_DURATION, value2.durationInSeconds());
    value2.removeRedundantEntries();
    TS_ASSERT_EQUALS(value2.numBoundaries(), 1);
  }
};
