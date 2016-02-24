#ifndef MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxentEntropyPositiveValues.h"

using Mantid::Algorithms::MaxentEntropyPositiveValues;

class MaxentEntropyPositiveValuesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentEntropyPositiveValuesTest *createSuite() { return new MaxentEntropyPositiveValuesTest(); }
  static void destroySuite( MaxentEntropyPositiveValuesTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_MAXENTENTROPYPOSITIVEVALUESTEST_H_ */