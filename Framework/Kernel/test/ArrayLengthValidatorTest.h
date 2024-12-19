// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidKernel/ArrayLengthValidator.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class ArrayLengthValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ArrayLengthValidatorTest *createSuite() { return new ArrayLengthValidatorTest(); }
  static void destroySuite(ArrayLengthValidatorTest *suite) { delete suite; }

  /// test constructors, both empty and with length, also hasLength and
  /// getLength
  void testConstructor() {
    ArrayLengthValidator<int> av1, av2(3), av3(4, 5);
    TS_ASSERT_EQUALS(av1.hasLength(), false);
    TS_ASSERT_EQUALS(av1.hasMinLength(), false);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), false);
    TS_ASSERT_EQUALS(av2.hasLength(), true);
    TS_ASSERT_EQUALS(av2.hasMinLength(), false);
    TS_ASSERT_EQUALS(av2.hasMaxLength(), false);
    TS_ASSERT_EQUALS(av2.getLength(), 3);
    TS_ASSERT_EQUALS(av3.hasLength(), false);
    TS_ASSERT_EQUALS(av3.hasMinLength(), true);
    TS_ASSERT_EQUALS(av3.hasMaxLength(), true);
    TS_ASSERT_EQUALS(av3.getMinLength(), 4);
    TS_ASSERT_EQUALS(av3.getMaxLength(), 5);
  }

  /// test the clone function
  void testClone() {
    std::shared_ptr<ArrayLengthValidator<int>> vi(new ArrayLengthValidator<int>);
    IValidator_sptr vvi = vi->clone();
    TS_ASSERT_DIFFERS(vi, vvi);
  }

  /// test for setLength and clearLength
  void testSetClear() {
    ArrayLengthValidator<int> av1;
    TS_ASSERT_EQUALS(av1.hasLength(), false);
    TS_ASSERT_EQUALS(av1.hasMinLength(), false);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), false);
    av1.setLength(4);
    TS_ASSERT_EQUALS(av1.hasLength(), true);
    TS_ASSERT_EQUALS(av1.getLength(), 4);
    TS_ASSERT_EQUALS(av1.hasMinLength(), false);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), false);
    av1.clearLength();
    TS_ASSERT_EQUALS(av1.hasLength(), false);
    TS_ASSERT_EQUALS(av1.getLength(), 0);
    TS_ASSERT_EQUALS(av1.hasMinLength(), false);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), false);
    av1.setLengthMax(4);
    TS_ASSERT_EQUALS(av1.hasLength(), false);
    TS_ASSERT_EQUALS(av1.getMaxLength(), 4);
    TS_ASSERT_EQUALS(av1.hasMinLength(), false);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), true);
    av1.setLengthMin(2);
    TS_ASSERT_EQUALS(av1.hasLength(), false);
    TS_ASSERT_EQUALS(av1.getMinLength(), 2);
    TS_ASSERT_EQUALS(av1.hasMinLength(), true);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), true);
    av1.clearLengthMax();
    TS_ASSERT_EQUALS(av1.hasLength(), false);
    TS_ASSERT_EQUALS(av1.hasMinLength(), true);
    TS_ASSERT_EQUALS(av1.hasMaxLength(), false);
  }

  /// test validator, both for OK and for different length
  void testValidator() {
    ArrayLengthValidator<int> vi(3);
    std::vector<int> a;
    a.emplace_back(3);
    TS_ASSERT_DIFFERS(vi.isValid(a).length(), 0);
    a.emplace_back(-1);
    a.emplace_back(11);
    TS_ASSERT_EQUALS(vi.isValid(a).length(), 0);
  }

  void testValidatorRange() {
    ArrayLengthValidator<int> vi(2, 3);
    std::vector<int> a;
    a.emplace_back(3);
    TS_ASSERT_DIFFERS(vi.isValid(a).length(), 0);
    a.emplace_back(11);
    TS_ASSERT_EQUALS(vi.isValid(a).length(), 0);
    a.emplace_back(12);
    TS_ASSERT_EQUALS(vi.isValid(a).length(), 0);
    a.emplace_back(21);
    TS_ASSERT_DIFFERS(vi.isValid(a).length(), 0);
  }
};
