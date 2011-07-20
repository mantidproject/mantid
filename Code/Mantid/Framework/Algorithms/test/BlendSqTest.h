#ifndef MANTID_ALGORITHMS_BLENDSQTEST_H_
#define MANTID_ALGORITHMS_BLENDSQTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/BlendSq.h"

using namespace Mantid::Algorithms;

class BlendSqTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    BlendSq alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_BLENDSQTEST_H_ */

