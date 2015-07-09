#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECTTEST_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"

using Mantid::Algorithms::TimeAtSampleStrategyDirect;

class TimeAtSampleStrategyDirectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeAtSampleStrategyDirectTest *createSuite() { return new TimeAtSampleStrategyDirectTest(); }
  static void destroySuite( TimeAtSampleStrategyDirectTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECTTEST_H_ */
