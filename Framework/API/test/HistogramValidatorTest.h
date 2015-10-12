#ifndef MANTID_API_HISTOGRAMVALIDATORTEST_H_
#define MANTID_API_HISTOGRAMVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/HistogramValidator.h"

using Mantid::API::HistogramValidator;

class HistogramValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramValidatorTest *createSuite() { return new HistogramValidatorTest(); }
  static void destroySuite( HistogramValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_HISTOGRAMVALIDATORTEST_H_ */