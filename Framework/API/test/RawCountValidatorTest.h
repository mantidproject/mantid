#ifndef MANTID_API_RAWCOUNTVALIDATORTEST_H_
#define MANTID_API_RAWCOUNTVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RawCountValidator.h"

using Mantid::API::RawCountValidator;

class RawCountValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RawCountValidatorTest *createSuite() { return new RawCountValidatorTest(); }
  static void destroySuite( RawCountValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_RAWCOUNTVALIDATORTEST_H_ */