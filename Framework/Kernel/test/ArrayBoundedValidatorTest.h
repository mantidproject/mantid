#ifndef ARRAYBOUNDEDVALIDATORTEST_H_
#define ARRAYBOUNDEDVALIDATORTEST_H_

#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <cxxtest/TestSuite.h>
#include <string>
#include <vector>

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
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(v.getValidator()->hasLower(), true);
    TS_ASSERT_EQUALS(v.getValidator()->hasUpper(), true);
    TS_ASSERT_EQUALS(v.getValidator()->lower(), 2);
    TS_ASSERT_EQUALS(v.getValidator()->upper(), 5);
  }

  void testIntParamConstructor() {
    ArrayBoundedValidator<int> v(1, 8);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(v.getValidator()->hasLower(), true);
    TS_ASSERT_EQUALS(v.getValidator()->hasUpper(), true);
    TS_ASSERT_EQUALS(v.getValidator()->lower(), 1);
    TS_ASSERT_EQUALS(v.getValidator()->upper(), 8);
  }

  void testDoubleBoundedValidatorConstructor() {
    BoundedValidator<double> bv(3, 9);
    ArrayBoundedValidator<double> v(bv);
    TS_ASSERT_EQUALS(v.getValidator()->hasLower(), true);
    TS_ASSERT_EQUALS(v.getValidator()->hasUpper(), true);
    TS_ASSERT_EQUALS(v.getValidator()->lower(), 3);
    TS_ASSERT_EQUALS(v.getValidator()->upper(), 9);
  }

  void testArrayValidation() {
    string index_start("At index ");
    string index_end(": ");
    string start("Selected value ");
    string end(")");
    string greaterThan(" is > the upper bound (");
    string lessThan(" is < the lower bound (");

    ArrayBoundedValidator<int> vi(0, 10);
    vector<int> ai{10, 3, -1, 2, 11, 0};

    TS_ASSERT_EQUALS(vi.isValid(ai), index_start + "2" + index_end + start +
                                         "-1" + lessThan + "0" + end +
                                         index_start + "4" + index_end + start +
                                         "11" + greaterThan + "10" + end);

    vi.getValidator()->clearLower();
    TS_ASSERT_EQUALS(vi.isValid(ai), index_start + "4" + index_end + start +
                                         "11" + greaterThan + "10" + end);

    vi.getValidator()->clearUpper();
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

    vd.getValidator()->clearUpper();
    TS_ASSERT_EQUALS(vd.isValid(ad), index_start + "2" + index_end + start +
                                         "-1" + lessThan + "0" + end +
                                         index_start + "5" + index_end + start +
                                         "-0.01" + lessThan + "0" + end);

    vd.getValidator()->clearLower();
    TS_ASSERT_EQUALS(vd.isValid(ad), "");
  }
};

#endif // ARRAYBOUNDEDVALIDATORTEST_H_
