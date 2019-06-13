// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef POLDICONVERSIONSTEST_H
#define POLDICONVERSIONSTEST_H

#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::Poldi;

class PoldiConversionsTest : public CxxTest::TestSuite {
public:
  static PoldiConversionsTest *createSuite() {
    return new PoldiConversionsTest();
  }
  static void destroySuite(PoldiConversionsTest *suite) { delete suite; }

  void testTOFandDConversions() {
    double distance = 11800.0 + 1996.017578125;
    double sinTheta = sin(1.577357650 / 2.0);
    double tof = 3.0;

    double d = Conversions::TOFtoD(tof, distance, sinTheta);

    TS_ASSERT_DELTA(d, 0.000606307, 1e-9);
    TS_ASSERT_EQUALS(Conversions::dtoTOF(d, distance, sinTheta), tof);

    TS_ASSERT_THROWS(Conversions::TOFtoD(1.0, 0.0, 2.0),
                     const std::domain_error &);
    TS_ASSERT_THROWS(Conversions::TOFtoD(1.0, -2.0, 2.0),
                     const std::domain_error &);
    TS_ASSERT_THROWS(Conversions::TOFtoD(1.0, 2.0, 0.0),
                     const std::domain_error &);
    TS_ASSERT_THROWS(Conversions::TOFtoD(1.0, 2.0, -2.0),
                     const std::domain_error &);
    TS_ASSERT_THROWS(Conversions::TOFtoD(1.0, 0.0, 0.0),
                     const std::domain_error &);
  }

  void testDandQConversions() {
    double d = 0.75;

    TS_ASSERT_DELTA(Conversions::dToQ(d), 8.37758040957278196923, 1e-15);
    TS_ASSERT_EQUALS(Conversions::qToD(Conversions::dToQ(0.75)), 0.75);

    TS_ASSERT_THROWS(Conversions::dToQ(0.0), const std::domain_error &);
    TS_ASSERT_THROWS(Conversions::qToD(0.0), const std::domain_error &);
  }

  void testDegAndRadConversions() {
    double degree = 30.0;

    TS_ASSERT_DELTA(Conversions::degToRad(degree), 0.52359877559829887308,
                    1e-15);
    TS_ASSERT_EQUALS(Conversions::radToDeg(Conversions::degToRad(degree)),
                     degree);
  }
};

#endif // POLDICONVERSIONSTEST_H
