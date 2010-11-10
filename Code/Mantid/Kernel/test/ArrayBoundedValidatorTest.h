#ifndef ARRAYBOUNDEDVALIDATORTEST_H_
#define ARRAYBOUNDEDVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <string>
#include <vector>

using namespace Mantid::Kernel;
using namespace std;

class ArrayBoundedValidatorTest : public CxxTest::TestSuite
{
public:
  void testDoubleClone()
  {
    IValidator<vector<double> > *vd = new ArrayBoundedValidator<double>();
    IValidator<vector<double> > *vvd = vd->clone();
    TS_ASSERT_DIFFERS( vd, vvd );
    delete vd;
    delete vvd;
  }

  void testIntClone()
  {
    ArrayBoundedValidator<int> *vi = new ArrayBoundedValidator<int>;
    IValidator<vector<int> > *vvi = vi->clone();
    TS_ASSERT_DIFFERS( vi, vvi );
    delete vi;
    delete vvi;
  }

  void testDoubleParamConstructor()
  {
    ArrayBoundedValidator<double> v(2, 5);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(v.getValidator()->hasLower(), true);
    TS_ASSERT_EQUALS(v.getValidator()->hasUpper(), true);
    TS_ASSERT_EQUALS(v.getValidator()->lower(), 2);
    TS_ASSERT_EQUALS(v.getValidator()->upper(), 5);
  }

  void testIntParamConstructor()
  {
    ArrayBoundedValidator<int> v(1, 8);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(v.getValidator()->hasLower(), true);
    TS_ASSERT_EQUALS(v.getValidator()->hasUpper(), true);
    TS_ASSERT_EQUALS(v.getValidator()->lower(), 1);
    TS_ASSERT_EQUALS(v.getValidator()->upper(), 8);
  }

  void testDoubleBoundedValidatorConstructor()
  {
    BoundedValidator<double> bv(3, 9);
    ArrayBoundedValidator<double> v(bv);
    TS_ASSERT_EQUALS(v.getValidator()->hasLower(), true);
    TS_ASSERT_EQUALS(v.getValidator()->hasUpper(), true);
    TS_ASSERT_EQUALS(v.getValidator()->lower(), 3);
    TS_ASSERT_EQUALS(v.getValidator()->upper(), 9);
  }

  void testArrayValidation()
  {
    string index_start("At index ");
    string index_end(": ");
    string start("Selected value ");
    string end(")");
    string greaterThan(" is > the upper bound (");
    string lessThan(" is < the lower bound (");

    ArrayBoundedValidator<int> vi(0, 10);
    vector<int> ai;
    ai.push_back(10);
    ai.push_back(3);
    ai.push_back(-1);
    ai.push_back(2);
    ai.push_back(11);
    ai.push_back(0);

    TS_ASSERT_EQUALS(vi.isValid(ai), index_start + "2" + index_end + start +\
        "-1" + lessThan + "0" + end + index_start + "4" + index_end + start +\
        "11" + greaterThan +"10" + end);

    vi.getValidator()->clearLower();
    TS_ASSERT_EQUALS(vi.isValid(ai), index_start + "4" + index_end + start +\
        "11" + greaterThan +"10" + end);

    vi.getValidator()->clearUpper();
    TS_ASSERT_EQUALS(vi.isValid(ai), "");

    ArrayBoundedValidator<double> vd(0, 10);
    vector<double> ad;
    ad.push_back(10.001);
    ad.push_back(3);
    ad.push_back(-1);
    ad.push_back(2);
    ad.push_back(11);
    ad.push_back(-0.01);

    TS_ASSERT_EQUALS(vd.isValid(ad), index_start + "0" + index_end + start +\
        "10.001" + greaterThan + "10" + end + index_start + "2" + index_end + start +\
        "-1" + lessThan + "0" + end + index_start + "4" + index_end + start +\
        "11" + greaterThan +"10" + end + index_start + "5" + index_end + start +\
        "-0.01" + lessThan +"0" + end);

    vd.getValidator()->clearUpper();
    TS_ASSERT_EQUALS(vd.isValid(ad), index_start + "2" + index_end + start +\
        "-1" + lessThan + "0" + end + index_start + "5" + index_end + start +\
        "-0.01" + lessThan +"0" + end);

    vd.getValidator()->clearLower();
    TS_ASSERT_EQUALS(vd.isValid(ad), "");
  }
};

#endif // ARRAYBOUNDEDVALIDATORTEST_H_
