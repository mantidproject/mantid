#ifndef MANTID_API_ADSVALIDATORTEST_H_
#define MANTID_API_ADSVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ADSValidator.h"

using Mantid::API::ADSValidator;

class ADSValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ADSValidatorTest *createSuite() { return new ADSValidatorTest(); }
  static void destroySuite( ADSValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_ADSVALIDATORTEST_H_ */