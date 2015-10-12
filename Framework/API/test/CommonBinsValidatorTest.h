#ifndef MANTID_API_COMMONBINSVALIDATORTEST_H_
#define MANTID_API_COMMONBINSVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CommonBinsValidator.h"

using Mantid::API::CommonBinsValidator;

class CommonBinsValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CommonBinsValidatorTest *createSuite() { return new CommonBinsValidatorTest(); }
  static void destroySuite( CommonBinsValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_COMMONBINSVALIDATORTEST_H_ */