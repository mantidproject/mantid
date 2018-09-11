#ifndef BOUNDEDVALIDATORTEST_H_
#define BOUNDEDVALIDATORTEST_H_

#include "MantidKernel/BoundedValidator.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class BoundedValidatorTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    BoundedValidator<int> bv(2, 5);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2);
    TS_ASSERT_EQUALS(bv.upper(), 5);
  }

  void testConstructorExclusive() {
    BoundedValidator<int> bv(2, 5, true);
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2);
    TS_ASSERT_EQUALS(bv.upper(), 5);

    TS_ASSERT(bv.isLowerExclusive());
    TS_ASSERT(bv.isUpperExclusive());
  }

  void testClone() {
    boost::shared_ptr<BoundedValidator<int>> bv =
        boost::make_shared<BoundedValidator<int>>(1, 10, true);
    IValidator_sptr vv = bv->clone();
    TS_ASSERT_DIFFERS(bv, vv);
    boost::shared_ptr<BoundedValidator<int>> bvv;
    TS_ASSERT(bvv = boost::dynamic_pointer_cast<BoundedValidator<int>>(vv));
    TS_ASSERT_EQUALS(bv->hasLower(), bvv->hasLower())
    TS_ASSERT_EQUALS(bv->hasUpper(), bvv->hasUpper())
    TS_ASSERT_EQUALS(bv->lower(), bvv->lower())
    TS_ASSERT_EQUALS(bv->upper(), bvv->upper())
    TS_ASSERT_EQUALS(bv->isLowerExclusive(), bvv->isLowerExclusive())
    TS_ASSERT_EQUALS(bv->isUpperExclusive(), bvv->isUpperExclusive())
  }

  void testIntClear() {
    BoundedValidator<int> bv(2, 5, true);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2);
    TS_ASSERT_EQUALS(bv.upper(), 5);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);

    bv.clearLower();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 0);
    TS_ASSERT_EQUALS(bv.upper(), 5);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);

    bv.clearUpper();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), 0);
    TS_ASSERT_EQUALS(bv.upper(), 0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);
  }

  void testDoubleClear() {
    BoundedValidator<double> bv(2.0, 5.0, true);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2.0);
    TS_ASSERT_EQUALS(bv.upper(), 5.0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);

    bv.clearBounds();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), 0);
    TS_ASSERT_EQUALS(bv.upper(), 0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);
  }

  void testSetBounds() {
    BoundedValidator<std::string> bv("A", "B");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "A");
    TS_ASSERT_EQUALS(bv.upper(), "B");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);

    bv.clearBounds();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), "");
    TS_ASSERT_EQUALS(bv.upper(), "");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);

    bv.setBounds("C", "D");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "C");
    TS_ASSERT_EQUALS(bv.upper(), "D");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);
  }

  void testSetValues() {
    BoundedValidator<std::string> bv("A", "B");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "A");
    TS_ASSERT_EQUALS(bv.upper(), "B");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);

    bv.clearBounds();
    TS_ASSERT_EQUALS(bv.hasLower(), false);
    TS_ASSERT_EQUALS(bv.hasUpper(), false);
    TS_ASSERT_EQUALS(bv.lower(), "");
    TS_ASSERT_EQUALS(bv.upper(), "");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);

    bv.setLower("C");
    bv.setLowerExclusive(true);
    bv.setUpper("D");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "C");
    TS_ASSERT_EQUALS(bv.upper(), "D");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);

    bv.setUpper("E");
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), "C");
    TS_ASSERT_EQUALS(bv.upper(), "E");
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);
  }

  void testSetExlcusive() {
    BoundedValidator<double> bv(2.0, 5.0, true);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2.0);
    TS_ASSERT_EQUALS(bv.upper(), 5.0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);

    bv.setLowerExclusive(false);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2.0);
    TS_ASSERT_EQUALS(bv.upper(), 5.0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);

    bv.setUpperExclusive(false);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2.0);
    TS_ASSERT_EQUALS(bv.upper(), 5.0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), false);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), false);

    bv.setExclusive(true);
    TS_ASSERT_EQUALS(bv.hasLower(), true);
    TS_ASSERT_EQUALS(bv.hasUpper(), true);
    TS_ASSERT_EQUALS(bv.lower(), 2.0);
    TS_ASSERT_EQUALS(bv.upper(), 5.0);
    TS_ASSERT_EQUALS(bv.isLowerExclusive(), true);
    TS_ASSERT_EQUALS(bv.isUpperExclusive(), true);
  }

  void testBoundedValidator() {
    std::string start("Selected value "), end(")");
    std::string greaterThan(" is > the upper bound (");
    std::string lessThan(" is < the lower bound (");

    // int tests
    BoundedValidator<int> pi(1, 10);
    TS_ASSERT_EQUALS(pi.isValid(0), start + "0" + lessThan + "1" + end);
    TS_ASSERT_EQUALS(pi.isValid(1), "");
    TS_ASSERT_EQUALS(pi.isValid(10), "");
    TS_ASSERT_EQUALS(pi.isValid(11), start + "11" + greaterThan + "10" + end);

    pi.clearLower();
    TS_ASSERT_EQUALS(pi.isValid(0), "");
    TS_ASSERT_EQUALS(pi.isValid(-1), "");
    TS_ASSERT_EQUALS(pi.isValid(10), "");
    TS_ASSERT_EQUALS(pi.isValid(11), start + "11" + greaterThan + "10" + end);
    pi.clearUpper();
    TS_ASSERT_EQUALS(pi.isValid(11), "");

    // double tests
    BoundedValidator<double> pd(1.0, 10.0);
    TS_ASSERT_EQUALS(pd.isValid(0.9), start + "0.9" + lessThan + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(1.0), "");
    TS_ASSERT_EQUALS(pd.isValid(10.0), "");
    TS_ASSERT_EQUALS(pd.isValid(10.1),
                     start + "10.1" + greaterThan + "10" + end);

    pd.clearUpper();
    TS_ASSERT_EQUALS(pd.isValid(0.9), start + "0.9" + lessThan + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(-1.0), start + "-1" + lessThan + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(10.0), "");
    TS_ASSERT_EQUALS(pd.isValid(10.1), "");
    pd.clearLower();
    TS_ASSERT_EQUALS(pd.isValid(-2.0), "");

    // string tests
    BoundedValidator<std::string> ps("B", "T");
    TS_ASSERT_EQUALS(ps.isValid("AZ"), start + "AZ" + lessThan + "B" + end);
    TS_ASSERT_EQUALS(ps.isValid("B"), "");
    TS_ASSERT_EQUALS(ps.isValid("T"), "");
    TS_ASSERT_EQUALS(ps.isValid("TA"), start + "TA" + greaterThan + "T" + end);

    ps.clearLower();
    TS_ASSERT_EQUALS(ps.isValid("AZ"), "");
    TS_ASSERT_EQUALS(ps.isValid("B"), "");
    TS_ASSERT_EQUALS(ps.isValid("T"), "");
    TS_ASSERT_EQUALS(ps.isValid("TA"), start + "TA" + greaterThan + "T" + end);
    ps.clearUpper();
    TS_ASSERT_EQUALS(ps.isValid("TA"), "");
  }

  void testBoundedValidatorExclusive() {
    std::string start("Selected value "), end(")");
    std::string greaterThanEq(" is >= the upper bound (");
    std::string lessThanEq(" is <= the lower bound (");

    // int tests
    BoundedValidator<int> pi(1, 10, true);

    // lower bounds
    TS_ASSERT_EQUALS(pi.isValid(0), start + "0" + lessThanEq + "1" + end);
    TS_ASSERT_EQUALS(pi.isValid(1), start + "1" + lessThanEq + "1" + end);
    TS_ASSERT_EQUALS(pi.isValid(2), "");

    // upper bounds
    TS_ASSERT_EQUALS(pi.isValid(9), "");
    TS_ASSERT_EQUALS(pi.isValid(10), start + "10" + greaterThanEq + "10" + end);
    TS_ASSERT_EQUALS(pi.isValid(11), start + "11" + greaterThanEq + "10" + end);

    pi.clearLower();
    TS_ASSERT_EQUALS(pi.isValid(0), "");
    TS_ASSERT_EQUALS(pi.isValid(-1), "");
    TS_ASSERT_EQUALS(pi.isValid(10), start + "10" + greaterThanEq + "10" + end);
    TS_ASSERT_EQUALS(pi.isValid(11), start + "11" + greaterThanEq + "10" + end);
    pi.clearUpper();
    TS_ASSERT_EQUALS(pi.isValid(10), "");
    TS_ASSERT_EQUALS(pi.isValid(11), "");

    // double tests
    BoundedValidator<double> pd(1.0, 10.0, true);

    // lower bounds
    TS_ASSERT_EQUALS(pd.isValid(0.9), start + "0.9" + lessThanEq + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(1.0), start + "1" + lessThanEq + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(1.1), "");

    // upper bounds
    TS_ASSERT_EQUALS(pd.isValid(9.9), "");
    TS_ASSERT_EQUALS(pd.isValid(10.0),
                     start + "10" + greaterThanEq + "10" + end);
    TS_ASSERT_EQUALS(pd.isValid(10.1),
                     start + "10.1" + greaterThanEq + "10" + end);

    pd.clearUpper();
    TS_ASSERT_EQUALS(pd.isValid(0.9), start + "0.9" + lessThanEq + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(-1.0), start + "-1" + lessThanEq + "1" + end);
    TS_ASSERT_EQUALS(pd.isValid(9.9), "");
    TS_ASSERT_EQUALS(pd.isValid(10.0), "");
    TS_ASSERT_EQUALS(pd.isValid(10.1), "");
    pd.clearLower();
    TS_ASSERT_EQUALS(pd.isValid(-2.0), "");

    // string tests
    BoundedValidator<std::string> ps("B", "T", true);

    // lower bounds
    TS_ASSERT_EQUALS(ps.isValid("AZ"), start + "AZ" + lessThanEq + "B" + end);
    TS_ASSERT_EQUALS(ps.isValid("B"), start + "B" + lessThanEq + "B" + end);
    TS_ASSERT_EQUALS(ps.isValid("C"), "");

    // upper bounds
    TS_ASSERT_EQUALS(ps.isValid("S"), "");
    TS_ASSERT_EQUALS(ps.isValid("T"), start + "T" + greaterThanEq + "T" + end);
    TS_ASSERT_EQUALS(ps.isValid("TA"),
                     start + "TA" + greaterThanEq + "T" + end);

    ps.clearLower();
    TS_ASSERT_EQUALS(ps.isValid("AZ"), "");
    TS_ASSERT_EQUALS(ps.isValid("B"), "");
    TS_ASSERT_EQUALS(ps.isValid("S"), "");
    TS_ASSERT_EQUALS(ps.isValid("T"), start + "T" + greaterThanEq + "T" + end);
    TS_ASSERT_EQUALS(ps.isValid("TA"),
                     start + "TA" + greaterThanEq + "T" + end);
    ps.clearUpper();
    TS_ASSERT_EQUALS(ps.isValid("T"), "");
    TS_ASSERT_EQUALS(ps.isValid("TA"), "");
  }
};

#endif /*BOUNDEDVALIDATORTEST_H_*/
