#ifndef MANTID_ALGORITHMS_MAXENTDATATEST_H_
#define MANTID_ALGORITHMS_MAXENTDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxentData.h"

using Mantid::Algorithms::MaxentData;

class MaxentDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentDataTest *createSuite() { return new MaxentDataTest(); }
  static void destroySuite( MaxentDataTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_MAXENTDATATEST_H_ */