// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ARRAYBOUNDEDVALIDATORTEST_H_
#define ARRAYBOUNDEDVALIDATORTEST_H_

#include "MantidKernel/ArrayBoundedValidator.h"
#include <array>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace std;

class ArrayBoundedValidatorTest : public CxxTest::TestSuite {
public:
  void testDoubleClone() {
    IValidator_sptr vd(new ArrayBoundedValidator<double>());
    IValidator_sptr vvd = vd->clone();
    TS_ASSERT_DIFFERS(vd, vvd);
  }

  void testIntClone() {
    IValidator_sptr vi(new ArrayBoundedValidator<int>);
    IValidator_sptr vvi = vi->clone();
    TS_ASSERT_DIFFERS(vi, vvi);
  }

  void testDoubleParamConstructor() {
    ArrayBoundedValidator<double> v(2, 5);
    TS_ASSERT_EQUALS(v.hasLower(), true);
    TS_ASSERT_EQUALS(v.hasUpper(), true);
    TS_ASSERT_EQUALS(v.lower(), 2);
    TS_ASSERT_EQUALS(v.upper(), 5);
  }

  void testIntParamConstructor() {
    ArrayBoundedValidator<int> v(1, 8);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(v.hasLower(), true);
    TS_ASSERT_EQUALS(v.hasUpper(), true);
    TS_ASSERT_EQUALS(v.lower(), 1);
    TS_ASSERT_EQUALS(v.upper(), 8);
  }

  void testExclusiveConstructor() {
    const std::array<bool, 2> exclusives{{true, false}};
    for (const auto exclusive : exclusives) {
      ArrayBoundedValidator<int> v(1, 8, exclusive);
      TS_ASSERT_EQUALS(v.hasLower(), true);
      TS_ASSERT_EQUALS(v.hasUpper(), true);
      TS_ASSERT_EQUALS(v.lower(), 1);
      TS_ASSERT_EQUALS(v.upper(), 8);
      TS_ASSERT_EQUALS(v.isLowerExclusive(), exclusive)
      TS_ASSERT_EQUALS(v.isUpperExclusive(), exclusive)
    }
  }

  void testDoubleBoundedValidatorConstructor() {
    BoundedValidator<double> bv(3, 9);
    ArrayBoundedValidator<double> v(bv);
    TS_ASSERT_EQUALS(v.hasLower(), true);
    TS_ASSERT_EQUALS(v.hasUpper(), true);
    TS_ASSERT_EQUALS(v.lower(), 3);
    TS_ASSERT_EQUALS(v.upper(), 9);
  }

  void testSetLowerSetUpper() {
    BoundedValidator<int> v;
    TS_ASSERT(!v.hasLower())
    TS_ASSERT(!v.hasUpper())
    v.setLower(3);
    TS_ASSERT_EQUALS(v.lower(), 3)
    v.setUpper(9);
    TS_ASSERT_EQUALS(v.upper(), 9)
  }

  void testHasLowerHasUpper() {
    BoundedValidator<int> v;
    TS_ASSERT(!v.hasLower())
    TS_ASSERT(!v.hasUpper())
    v.setLower(1);
    TS_ASSERT(v.hasLower())
    TS_ASSERT(!v.hasUpper())
    v.clearLower();
    v.setUpper(9);
    TS_ASSERT(!v.hasLower())
    TS_ASSERT(v.hasUpper())
  }

  void testClearLowerClearUpper() {
    BoundedValidator<int> v(2, 9);
    TS_ASSERT(v.hasLower())
    TS_ASSERT(v.hasUpper())
    v.clearLower();
    TS_ASSERT(!v.hasLower())
    TS_ASSERT(v.hasUpper())
    v.setLower(2);
    v.clearUpper();
    TS_ASSERT(v.hasLower())
    TS_ASSERT(!v.hasUpper())
  }

  void testSetExclusive() {
    BoundedValidator<int> v;
    TS_ASSERT(!v.isLowerExclusive())
    TS_ASSERT(!v.isUpperExclusive())
    v.setLowerExclusive(true);
    TS_ASSERT(v.isLowerExclusive())
    v.setUpperExclusive(true);
    TS_ASSERT(v.isUpperExclusive())
    v.setExclusive(false);
    TS_ASSERT(!v.isLowerExclusive())
    TS_ASSERT(!v.isUpperExclusive())
  }

  void testArrayValidation() {
    const string index_start("At index ");
    const string index_end(": ");
    const string start("Selected value ");
    const string end(")");
    const string greaterThan(" is > the upper bound (");
    const string lessThan(" is < the lower bound (");

    ArrayBoundedValidator<int> vi(0, 10);
    vector<int> ai{10, 3, -1, 2, 11, 0};

    TS_ASSERT_EQUALS(vi.isValid(ai), index_start + "2" + index_end + start +
                                         "-1" + lessThan + "0" + end +
                                         index_start + "4" + index_end + start +
                                         "11" + greaterThan + "10" + end);

    vi.clearLower();
    TS_ASSERT_EQUALS(vi.isValid(ai), index_start + "4" + index_end + start +
                                         "11" + greaterThan + "10" + end);

    vi.clearUpper();
    TS_ASSERT_EQUALS(vi.isValid(ai), "");

    ArrayBoundedValidator<double> vd(0, 10);
    vector<double> ad{10.001, 3., -1., 2., 11., -0.01};

    TS_ASSERT_EQUALS(vd.isValid(ad),
                     index_start + "0" + index_end + start + "10.001" +
                         greaterThan + "10" + end + index_start + "2" +
                         index_end + start + "-1" + lessThan + "0" + end +
                         index_start + "4" + index_end + start + "11" +
                         greaterThan + "10" + end + index_start + "5" +
                         index_end + start + "-0.01" + lessThan + "0" + end);

    vd.clearUpper();
    TS_ASSERT_EQUALS(vd.isValid(ad), index_start + "2" + index_end + start +
                                         "-1" + lessThan + "0" + end +
                                         index_start + "5" + index_end + start +
                                         "-0.01" + lessThan + "0" + end);

    vd.clearLower();
    TS_ASSERT_EQUALS(vd.isValid(ad), "");
  }
};

#endif // ARRAYBOUNDEDVALIDATORTEST_H_
