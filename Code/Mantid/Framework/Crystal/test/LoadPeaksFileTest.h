#ifndef MANTID_CRYSTAL_LOADPEAKSFILETEST_H_
#define MANTID_CRYSTAL_LOADPEAKSFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCrystal/LoadPeaksFile.h"

using namespace Mantid::Crystal;

class LoadPeaksFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    LoadPeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_CRYSTAL_LOADPEAKSFILETEST_H_ */

