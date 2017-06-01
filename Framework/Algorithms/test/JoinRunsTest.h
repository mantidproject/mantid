#ifndef MANTID_ALGORITHMS_JOINRUNSTEST_H_
#define MANTID_ALGORITHMS_JOINRUNSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/JoinRuns.h"

using Mantid::Algorithms::JoinRuns;

class JoinRunsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JoinRunsTest *createSuite() { return new JoinRunsTest(); }
  static void destroySuite( JoinRunsTest *suite ) { delete suite; }

  void test_Init()
  {
    JoinRuns alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

};


#endif /* MANTID_ALGORITHMS_JOINRUNSTEST_H_ */
