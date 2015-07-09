#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"

using Mantid::Algorithms::TimeAtSampleStrategyIndirect;

class TimeAtSampleStrategyIndirectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyIndirectTest *createSuite() { return new TimeAtSampleStrategyIndirectTest(); }
  static void destroySuite( TimeAtSampleStrategyIndirectTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECTTEST_H_ */
