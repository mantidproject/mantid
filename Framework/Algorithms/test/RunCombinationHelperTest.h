#ifndef MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_
#define MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

using Mantid::Algorithms::RunCombinationHelper;

class RunCombinationHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunCombinationHelperTest *createSuite() { return new RunCombinationHelperTest(); }
  static void destroySuite( RunCombinationHelperTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_RUNCOMBINATIONHELPERTEST_H_ */