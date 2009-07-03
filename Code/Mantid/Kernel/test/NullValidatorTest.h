#ifndef NULLVALIDATORTEST_H_
#define NULLVALIDATORTEST_H_

#include <string>
#include <cxxtest/TestSuite.h>
#include "MantidKernel/NullValidator.h"

using namespace Mantid::Kernel;

class NullValidatorTest : public CxxTest::TestSuite
{
public:

  void testConstructor()
  {
    TS_ASSERT_THROWS_NOTHING(
        NullValidator<int> niv;
        NullValidator<double> ndv;
        NullValidator<std::string> nsv;
    );
  }

  void testClone()
  {
    IValidator<int> *v = new NullValidator<int>;
    IValidator<int> *vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv )
    TS_ASSERT( dynamic_cast<NullValidator<int>*>(vv) )
    delete v;
    delete vv;
  }

  void testCast()
  {
    NullValidator<int> *v = new NullValidator<int>;
    TS_ASSERT( dynamic_cast<IValidator<int>*>(v) )
    NullValidator<double> *vv = new NullValidator<double>;
    TS_ASSERT( dynamic_cast<IValidator<double>*>(vv) )
    NullValidator<std::string> *vvv = new NullValidator<std::string>;
    TS_ASSERT( dynamic_cast<IValidator<std::string>*>(vvv) )
    delete v;
    delete vv;
    delete vvv;
  }

  void testIntNullValidator()
  {
    NullValidator<int> p;
    TS_ASSERT_EQUALS(p.isValid(0), "");
    TS_ASSERT_EQUALS(p.isValid(1), "");
    TS_ASSERT_EQUALS(p.isValid(10), "");
    TS_ASSERT_EQUALS(p.isValid(-11), "");
  }

  void testDoubleNullValidator()
  {
    NullValidator<double> p;
    TS_ASSERT_EQUALS(p.isValid(0.0), "");
    TS_ASSERT_EQUALS(p.isValid(1.0), "");
    TS_ASSERT_EQUALS(p.isValid(10.0), "");
    TS_ASSERT_EQUALS(p.isValid(-10.1), "");
  }

  void testStringNullValidator()
  {
    NullValidator<std::string> p;
    TS_ASSERT_EQUALS(p.isValid("AZ"), "");
    TS_ASSERT_EQUALS(p.isValid("B"), "");
    TS_ASSERT_EQUALS(p.isValid(""), "");
    TS_ASSERT_EQUALS(p.isValid("ta"), "");
  }

};

#endif /*NULLVALIDATORTEST_H_*/
