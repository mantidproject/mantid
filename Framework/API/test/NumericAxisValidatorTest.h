#ifndef MANTID_API_NUMERICAXISVALIDATORTEST_H_
#define MANTID_API_NUMERICAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NumericAxisValidator.h"

using Mantid::API::NumericAxisValidator;

class NumericAxisValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NumericAxisValidatorTest *createSuite() { return new NumericAxisValidatorTest(); }
  static void destroySuite( NumericAxisValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_NUMERICAXISVALIDATORTEST_H_ */