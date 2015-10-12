#ifndef MANTID_API_SPECTRAAXISVALIDATORTEST_H_
#define MANTID_API_SPECTRAAXISVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectraAxisValidator.h"

using Mantid::API::SpectraAxisValidator;

class SpectraAxisValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectraAxisValidatorTest *createSuite() { return new SpectraAxisValidatorTest(); }
  static void destroySuite( SpectraAxisValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_SPECTRAAXISVALIDATORTEST_H_ */