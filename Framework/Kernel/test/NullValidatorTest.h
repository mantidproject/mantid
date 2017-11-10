#ifndef NULLVALIDATORTEST_H_
#define NULLVALIDATORTEST_H_

#include <string>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include "MantidKernel/NullValidator.h"

using namespace Mantid::Kernel;

class NullValidatorTest : public CxxTest::TestSuite {
public:
  void testConstructor() { TS_ASSERT_THROWS_NOTHING(NullValidator()); }

  void testClone() {
    IValidator_sptr v = boost::make_shared<NullValidator>();
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS(v, vv)
    TS_ASSERT(boost::dynamic_pointer_cast<NullValidator>(vv))
  }

  void testNullValidatorWithInts() {
    NullValidator p;
    TS_ASSERT_EQUALS(p.isValid(0), "");
    TS_ASSERT_EQUALS(p.isValid(1), "");
    TS_ASSERT_EQUALS(p.isValid(10), "");
    TS_ASSERT_EQUALS(p.isValid(-11), "");
  }

  void testDoubleNullValidatorWithDoubles() {
    NullValidator p;
    TS_ASSERT_EQUALS(p.isValid(0.0), "");
    TS_ASSERT_EQUALS(p.isValid(1.0), "");
    TS_ASSERT_EQUALS(p.isValid(10.0), "");
    TS_ASSERT_EQUALS(p.isValid(-10.1), "");
  }

  void testStringNullValidatorWithStrings() {
    NullValidator p;
    TS_ASSERT_EQUALS(p.isValid("AZ"), "");
    TS_ASSERT_EQUALS(p.isValid("B"), "");
    TS_ASSERT_EQUALS(p.isValid(""), "");
    TS_ASSERT_EQUALS(p.isValid("ta"), "");
  }
};

#endif /*NULLVALIDATORTEST_H_*/
