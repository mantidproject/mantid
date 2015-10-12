#ifndef MANTID_API_INSTRUMENTVALIDATORTEST_H_
#define MANTID_API_INSTRUMENTVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/InstrumentValidator.h"

using Mantid::API::InstrumentValidator;

class InstrumentValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentValidatorTest *createSuite() { return new InstrumentValidatorTest(); }
  static void destroySuite( InstrumentValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_INSTRUMENTVALIDATORTEST_H_ */