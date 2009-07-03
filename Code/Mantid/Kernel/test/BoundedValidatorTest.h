#ifndef BOUNDEDVALIDATORTEST_H_
#define BOUNDEDVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::Kernel;

class BoundedValidatorTest : public CxxTest::TestSuite
{
public:

  void testConstructor()
  {
    BoundedValidator<int> bv(2, 5);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2);
    TS_ASSERT_EQUALS(bv.upper(), 5);
  }

  void testClone()
  {
    IValidator<int> *v = new BoundedValidator<int>;
    IValidator<int> *vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv )
    BoundedValidator<int> &bv = dynamic_cast<BoundedValidator<int>&>(*v);
    BoundedValidator<int> *bvv;
    TS_ASSERT( bvv = dynamic_cast<BoundedValidator<int>*>(vv) )
    TS_ASSERT_EQUALS( bv.hasLower(), bvv->hasLower() )
    TS_ASSERT_EQUALS( bv.hasUpper(), bvv->hasUpper() )
    TS_ASSERT_EQUALS( bv.lower(), bvv->lower() )
    TS_ASSERT_EQUALS( bv.upper(), bvv->upper() )
    delete v;
    delete vv;
  }

  void testCast()
  {
    BoundedValidator<int> *v = new BoundedValidator<int>;
    TS_ASSERT( dynamic_cast<IValidator<int>*>(v) )
    BoundedValidator<double> *vv = new BoundedValidator<double>;
    TS_ASSERT( dynamic_cast<IValidator<double>*>(vv) )
    BoundedValidator<std::string> *vvv = new BoundedValidator<std::string>;
    TS_ASSERT( dynamic_cast<IValidator<std::string>*>(vvv) )
    delete v,
    delete vv;
    delete vvv;
  }

  void testIntClear()
  {
    BoundedValidator<int> bv(2, 5);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2);
    TS_ASSERT_EQUALS(bv.upper(), 5);

    bv.clearLower();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 0);
    TS_ASSERT_EQUALS(bv.upper(), 5);

    bv.clearUpper();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), 0);
    TS_ASSERT_EQUALS(bv.upper(), 0);
  }

  void testDoubleClear()
  {
    BoundedValidator<double> bv(2.0, 5.0);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2.0);
    TS_ASSERT_EQUALS(bv.upper(), 5.0);

    bv.clearBounds();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), 0);
    TS_ASSERT_EQUALS(bv.upper(), 0);
  }

  void testSetBounds()
  {
    BoundedValidator<std::string> bv("A", "B");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "A");
    TS_ASSERT_EQUALS(bv.upper(), "B");

    bv.clearBounds();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), "");
    TS_ASSERT_EQUALS(bv.upper(), "");

    bv.setBounds("C", "D");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "C");
    TS_ASSERT_EQUALS(bv.upper(), "D");
  }

  void testSetValues()
  {
    BoundedValidator<std::string> bv("A", "B");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "A");
    TS_ASSERT_EQUALS(bv.upper(), "B");

    bv.clearBounds();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), "");
    TS_ASSERT_EQUALS(bv.upper(), "");

    bv.setLower("C");
    bv.setUpper("D");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "C");
    TS_ASSERT_EQUALS(bv.upper(), "D");

    bv.setUpper("E");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "C");
    TS_ASSERT_EQUALS(bv.upper(), "E");

  }

  void testBoundedValidator()
  {
	  std::string start("Selected value "), end(")");
    std::string greaterThan(" is > the upper bound (");
    std::string lessThan(" is < the lower bound (");
	  
	  //int tests
	  BoundedValidator<int> pi(1, 10);
	  TS_ASSERT_EQUALS(pi.isValid(0),
		  start + "0" + lessThan + "1" + end);
	  TS_ASSERT_EQUALS(pi.isValid(1), "");
	  TS_ASSERT_EQUALS(pi.isValid(10), "");
	  TS_ASSERT_EQUALS(pi.isValid(11),
		  start + "11" + greaterThan + "10" + end);
	  
	  pi.clearLower();
	  TS_ASSERT_EQUALS(pi.isValid(0), "");
	  TS_ASSERT_EQUALS(pi.isValid(-1), "");
	  TS_ASSERT_EQUALS(pi.isValid(10), "");
	  TS_ASSERT_EQUALS(pi.isValid(11),
		  start + "11" + greaterThan + "10" + end);
	  pi.clearUpper();
	  TS_ASSERT_EQUALS(pi.isValid(11), "");
	  
	  //double tests
	  BoundedValidator<double> pd(1.0, 10.0);
	  TS_ASSERT_EQUALS(pd.isValid(0.9),
		  start + "0.9" + lessThan + "1" + end);
	  TS_ASSERT_EQUALS(pd.isValid(1.0), "");
	  TS_ASSERT_EQUALS(pd.isValid(10.0), "");
	  TS_ASSERT_EQUALS(pd.isValid(10.1),
		  start + "10.1" + greaterThan + "10" + end);
	  
	  pd.clearUpper();
	  TS_ASSERT_EQUALS(pd.isValid(0.9),
		  start + "0.9" + lessThan + "1" + end);
	  TS_ASSERT_EQUALS(pd.isValid(-1.0),
		  start + "-1" + lessThan + "1" + end);
	  TS_ASSERT_EQUALS(pd.isValid(10), "");
	  TS_ASSERT_EQUALS(pd.isValid(10.1), "");
	  pd.clearLower();
	  TS_ASSERT_EQUALS(pd.isValid(-2.0), "");

	  //string tests
	  BoundedValidator<std::string> ps("B", "T");
	  TS_ASSERT_EQUALS(ps.isValid("AZ"),
		  start + "AZ" + lessThan + "B" + end);
	  TS_ASSERT_EQUALS(ps.isValid("B"), "");
	  TS_ASSERT_EQUALS(ps.isValid("T"), "");
	  TS_ASSERT_EQUALS(ps.isValid("TA"),
		  start + "TA" + greaterThan + "T" + end);

    ps.clearLower();
    TS_ASSERT_EQUALS(ps.isValid("AZ"), "");
    TS_ASSERT_EQUALS(ps.isValid("B"), "");
    TS_ASSERT_EQUALS(ps.isValid("T"), "");
    TS_ASSERT_EQUALS(ps.isValid("TA"),
		start + "TA" + greaterThan + "T" + end);
    ps.clearUpper();
    TS_ASSERT_EQUALS(ps.isValid("TA"), "");
  }

};

#endif /*BOUNDEDVALIDATORTEST_H_*/
