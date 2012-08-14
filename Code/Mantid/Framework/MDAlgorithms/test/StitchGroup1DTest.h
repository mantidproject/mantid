#ifndef MANTID_MDALGORITHMS_STITCHGROUP1DTEST_H_
#define MANTID_MDALGORITHMS_STITCHGROUP1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/StitchGroup1D.h"

using Mantid::MDAlgorithms::StitchGroup1D;

class StitchGroup1DTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StitchGroup1DTest *createSuite() { return new StitchGroup1DTest(); }
  static void destroySuite( StitchGroup1DTest *suite ) { delete suite; }

  void test_Init()
  {
    StitchGroup1D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


};


#endif /* MANTID_MDALGORITHMS_STITCHGROUP1DTEST_H_ */