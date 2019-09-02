// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef UNCERTAINVALUETEST_H
#define UNCERTAINVALUETEST_H

#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::Poldi;

class UncertainValueTest : public CxxTest::TestSuite {
public:
  static UncertainValueTest *createSuite() { return new UncertainValueTest(); }
  static void destroySuite(UncertainValueTest *suite) { delete suite; }

  void testConstructor() {
    UncertainValue value(1.0, 3.0);

    TS_ASSERT_EQUALS(value.value(), 1.0);
    TS_ASSERT_EQUALS(value.error(), 3.0);

    UncertainValue other;

    TS_ASSERT_EQUALS(other.value(), 0.0);
    TS_ASSERT_EQUALS(other.error(), 0.0);

    UncertainValue noError(2.0);
    TS_ASSERT_EQUALS(noError.value(), 2.0);
    TS_ASSERT_EQUALS(noError.error(), 0.0);

    TS_ASSERT_THROWS(UncertainValue(0.0, -3.0), const std::domain_error &);
  }

  void testPlainAddition() {
    UncertainValue left(1.0, 1.0);
    UncertainValue right(2.0, 2.0);

    UncertainValue sum = UncertainValue::plainAddition(left, right);

    TS_ASSERT_EQUALS(sum.value(), 3.0);
    TS_ASSERT_EQUALS(sum.error(), 3.0);
  }

  void testlessThanError() {
    UncertainValue first(1.0, 2.0);
    UncertainValue second(1.0, 3.0);

    TS_ASSERT(UncertainValue::lessThanError(first, second));
  }

  void testvalueToErrorRatio() {
    UncertainValue value(2.0, 4.0);

    TS_ASSERT_EQUALS(UncertainValue::valueToErrorRatio(value), 0.5);

    UncertainValue invalid(2.0, 0.0);

    TS_ASSERT_THROWS(UncertainValue::valueToErrorRatio(invalid),
                     const std::domain_error &);
  }

  void testErrorToValueRatio() {
    UncertainValue value(2.0, 4.0);
    TS_ASSERT_EQUALS(UncertainValue::errorToValueRatio(value), 2.0);

    UncertainValue valueWithoutError(2.0, 0.0);
    TS_ASSERT_EQUALS(UncertainValue::errorToValueRatio(valueWithoutError), 0.0);

    UncertainValue invalid(0.0, 2.0);
    TS_ASSERT_THROWS(UncertainValue::errorToValueRatio(invalid),
                     const std::domain_error &);
  }

  void testdoubleOperator() {
    UncertainValue value(2.0, 4.0);

    double doubleValue = value;

    TS_ASSERT_EQUALS(doubleValue, 2.0);
    TS_ASSERT_EQUALS(2.0 * value, 4.0);
  }

  void testdoubleMultiplication() {
    UncertainValue value(10.0, 2.0);

    UncertainValue newValue = 2.0 * value;
    TS_ASSERT_EQUALS(newValue.value(), 20.0);
    TS_ASSERT_EQUALS(newValue.error(), 4.0);

    UncertainValue newerValue = newValue * 2.0;
    TS_ASSERT_EQUALS(newerValue.value(), 40.0);
    TS_ASSERT_EQUALS(newerValue.error(), 8.0);
  }

  void testdoubleDivision() {
    UncertainValue value(40.0, 8.0);

    UncertainValue newValue = value / 2.0;
    TS_ASSERT_EQUALS(newValue.value(), 20.0);
    TS_ASSERT_EQUALS(newValue.error(), 4.0);

    UncertainValue newerValue = 80.0 / newValue;
    TS_ASSERT_EQUALS(newerValue.value(), 4.0);
    TS_ASSERT_EQUALS(newerValue.error(), 0.8);

    TS_ASSERT_THROWS(newValue / 0.0, const std::domain_error &);
    TS_ASSERT_THROWS(2.0 / UncertainValue(0.0), const std::domain_error &);
  }

  void testadditionOperator() {
    UncertainValue value(2.0, 1.0);

    UncertainValue newValue = value + 1.0;
    TS_ASSERT_EQUALS(newValue.value(), 3.0);
    TS_ASSERT_EQUALS(newValue.error(), 1.0);

    UncertainValue newerValue = 3.0 + newValue;
    TS_ASSERT_EQUALS(newerValue.value(), 6.0);
    TS_ASSERT_EQUALS(newerValue.error(), 1.0);
  }

  void testsubtractionOperator() {
    UncertainValue value(2.0, 1.0);

    UncertainValue newValue = value - 1.0;
    TS_ASSERT_EQUALS(newValue.value(), 1.0);
    TS_ASSERT_EQUALS(newValue.error(), 1.0);

    UncertainValue newerValue = 3.0 - newValue;
    TS_ASSERT_EQUALS(newerValue.value(), 2.0);
    TS_ASSERT_EQUALS(newerValue.error(), 1.0);
  }

  void testcombinedOperations() {
    UncertainValue value(2.0, 1.0);

    UncertainValue newValue = 20.0 / (value / 2.0 + 3.0);
    TS_ASSERT_EQUALS(newValue.value(), 5.0);
    TS_ASSERT_EQUALS(newValue.error(), 0.625);

    UncertainValue otherValue(3.0, 0.0);
    UncertainValue newerValue = 2.0 * (otherValue + 2.0) / 8.0;

    TS_ASSERT_EQUALS(newerValue.value(), 1.25);
    TS_ASSERT_EQUALS(newerValue.error(), 0.0);
  }
};

#endif // UNCERTAINVALUETEST_H
