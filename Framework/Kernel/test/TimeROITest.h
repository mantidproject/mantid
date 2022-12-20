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
  }
  void test_sortedROI() {
    TimeROI value;
    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION);

    // add New Year's eve
    value.addROI(NEW_YEARS_START, NEW_YEARS_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), HANUKKAH_DURATION + ONE_DAY_DURATION);
  }

  void test_reversesortedROI() {
    TimeROI value;
    // add New Year's eve
    value.addROI(DateAndTime(NEW_YEARS_START), DateAndTime(NEW_YEARS_STOP));
    TS_ASSERT_EQUALS(value.durationInSeconds(), ONE_DAY_DURATION);

    // add Hanukkah
    value.addROI(HANUKKAH_START, HANUKKAH_STOP);
    TS_ASSERT_EQUALS(value.durationInSeconds(), ONE_DAY_DURATION + HANUKKAH_DURATION);
  }
};
