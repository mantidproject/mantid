#ifndef MANDATORYVALIDATORTEST_H_
#define MANDATORYVALIDATORTEST_H_

#include <string>
#include <cxxtest/TestSuite.h>
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;

class MandatoryValidatorTest : public CxxTest::TestSuite
{
public:

  void testConstructor()
  {
    TS_ASSERT_THROWS_NOTHING(
        MandatoryValidator nsv;
    );
  }

  void testClone()
  {
    IValidator<std::string> *v = new MandatoryValidator;
    IValidator<std::string> *vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv )
    TS_ASSERT( dynamic_cast<MandatoryValidator*>(vv) )
  }

  void testCast()
  {
    MandatoryValidator *v = new MandatoryValidator;
    TS_ASSERT( dynamic_cast<IValidator<std::string>*>(v) )
  }

  void testMandatoryValidator()
  {
    MandatoryValidator p;
    TS_ASSERT_EQUALS(p.isValid("AZ"), true);
    TS_ASSERT_EQUALS(p.isValid("B"), true);
    TS_ASSERT_EQUALS(p.isValid(""), false);
    TS_ASSERT_EQUALS(p.isValid("ta"), true);
  }

};

#endif /*MANDATORYVALIDATORTEST_H_*/
