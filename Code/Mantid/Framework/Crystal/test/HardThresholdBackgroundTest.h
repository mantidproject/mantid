#ifndef MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUNDTEST_H_
#define MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/HardThresholdBackground.h"

using Mantid::Crystal::HardThresholdBackground;

class HardThresholdBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HardThresholdBackgroundTest *createSuite() { return new HardThresholdBackgroundTest(); }
  static void destroySuite( HardThresholdBackgroundTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CRYSTAL_HARDTHRESHOLDBACKGROUNDTEST_H_ */