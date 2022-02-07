// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/Events.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Types::Event::TofEvent;

using std::runtime_error;
using std::size_t;
using std::vector;

class EventsTest : public CxxTest::TestSuite {

public:
  void setUp() override {}

  void test_Compare() {
    // tof event
    TofEvent tofEvent1(20.0, Types::Core::DateAndTime("1990-01-02 00:00:02.000"));
    TofEvent tofEvent2(20.05, Types::Core::DateAndTime("1990-01-02 00:00:02.000000001"));

    TS_ASSERT(tofEvent1 == tofEvent1);
    TS_ASSERT(!(tofEvent1 == tofEvent2));
    TS_ASSERT(tofEvent1.equals(tofEvent2, .1, 1));

    // weighted
    WeightedEvent wghtEvent1(20.0, Types::Core::DateAndTime("1990-01-02 00:00:02.000"), 1., 1.);
    WeightedEvent wghtEvent2(20.05, Types::Core::DateAndTime("1990-01-02 00:00:02.000000001"), 1.05, 1.);

    TS_ASSERT(wghtEvent1 == wghtEvent1);
    TS_ASSERT(!(wghtEvent1 == wghtEvent2));
    TS_ASSERT(wghtEvent1.equals(wghtEvent2, .1, .1, 1));

    WeightedEvent fromTofEvent(tofEvent1);
    TS_ASSERT(wghtEvent1 == fromTofEvent);

    // weighted no time
    WeightedEventNoTime notimeEvent1(20.0, 1., 1.);
    WeightedEventNoTime notimeEvent2(20.05, 1.05, 1.05);

    TS_ASSERT(notimeEvent1 == notimeEvent1);
    TS_ASSERT(!(notimeEvent1 == notimeEvent2));
    TS_ASSERT(notimeEvent1.equals(notimeEvent2, .1, .1));
  }
};
