#ifndef MANTID_API_SAMPLEVALIDATORTEST_H_
#define MANTID_API_SAMPLEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SampleValidator.h"

using Mantid::API::SampleValidator;

class SampleValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleValidatorTest *createSuite() { return new SampleValidatorTest(); }
  static void destroySuite( SampleValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_SAMPLEVALIDATORTEST_H_ */