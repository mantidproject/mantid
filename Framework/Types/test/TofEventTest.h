// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidTypes/Event/TofEvent.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

using std::size_t;
using std::vector;

//==========================================================================================
class TofEventTest : public CxxTest::TestSuite {
private:
  TofEvent e;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TofEventTest *createSuite() { return new TofEventTest(); }
  static void destroySuite(TofEventTest *suite) { delete suite; }

  TofEventTest() { e = TofEvent(123, 456); }

  void testInit() {
    TS_ASSERT_EQUALS(e.tof(), 123);
    TS_ASSERT_EQUALS(e.pulseTime(), 456);
  }

  void testAssign() {
    TofEvent e2;
    e2 = e;
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);
  }

  void testConstructors() {
    TofEvent e2 = TofEvent(e);
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);
    TS_ASSERT_EQUALS(e2.pulseTOFTime(), DateAndTime(0., 123456.));

    TofEvent e3 = TofEvent(890.234, 321);
    TS_ASSERT_EQUALS(e3.tof(), 890.234);
    TS_ASSERT_EQUALS(e3.pulseTime(), 321);
    TS_ASSERT_EQUALS(e3.pulseTOFTime(), DateAndTime(0., 890234. + 321.));
  }
};
