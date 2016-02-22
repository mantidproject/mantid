#ifndef MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_
#define MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxentEntropyNegativeValues.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using Mantid::Algorithms::MaxentEntropyNegativeValues;

class MaxentEntropyNegativeValuesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentEntropyNegativeValuesTest *createSuite() { return new MaxentEntropyNegativeValuesTest(); }
  static void destroySuite( MaxentEntropyNegativeValuesTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_MAXENTENTROPYNEGATIVEVALUESTEST_H_ */