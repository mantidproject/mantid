// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::Poldi;

class UncertainValueIOTest : public CxxTest::TestSuite {
public:
  static UncertainValueIOTest *createSuite() { return new UncertainValueIOTest(); }
  static void destroySuite(UncertainValueIOTest *suite) { delete suite; }

  void testToString() {
    UncertainValue value(4.0);
    TS_ASSERT_EQUALS(UncertainValueIO::toString(value), std::string("4.000000"));

    UncertainValue otherValue(4.0, 4.0);
    TS_ASSERT_EQUALS(UncertainValueIO::toString(otherValue), std::string("4.000000 +/- 4.000000"));
  }

  void testFromString() {
    std::string uncertainEmpty("");
    UncertainValue empty = UncertainValueIO::fromString(uncertainEmpty);
    TS_ASSERT_EQUALS(empty.value(), 0.0);
    TS_ASSERT_EQUALS(empty.error(), 0.0);

    std::string uncertainOne("4.0");
    UncertainValue one = UncertainValueIO::fromString(uncertainOne);
    TS_ASSERT_EQUALS(one.value(), 4.0);
    TS_ASSERT_EQUALS(one.error(), 0.0);

    std::string uncertainTwo("4.0 +/- 1.0");
    UncertainValue two = UncertainValueIO::fromString(uncertainTwo);
    TS_ASSERT_EQUALS(two.value(), 4.0);
    TS_ASSERT_EQUALS(two.error(), 1.0);

    std::string invalidOne("asdf");
    TS_ASSERT_THROWS(UncertainValueIO::fromString(invalidOne), const boost::bad_lexical_cast &);

    std::string invalidTwo("4.0 +/- 3.0 +/- 1.0");
    TS_ASSERT_THROWS(UncertainValueIO::fromString(invalidTwo), const std::runtime_error &);
  }

  void testComplementarity() {
    std::string uncertainString("4.000000 +/- 1.000000");
    TS_ASSERT_EQUALS(UncertainValueIO::toString(UncertainValueIO::fromString(uncertainString)), uncertainString);

    UncertainValue uncertainValue(4.0, 1.0);
    UncertainValue convertedValue = UncertainValueIO::fromString(UncertainValueIO::toString(uncertainValue));
    TS_ASSERT_EQUALS(convertedValue.value(), uncertainValue.value());
    TS_ASSERT_EQUALS(convertedValue.error(), uncertainValue.error());
  }
};
