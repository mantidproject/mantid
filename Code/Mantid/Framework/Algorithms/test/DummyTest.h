#ifndef MANTID_ALGORITHMS_DUMMYTEST_H_
#define MANTID_ALGORITHMS_DUMMYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/Dummy.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class DummyTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    Dummy alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

};


#endif /* MANTID_ALGORITHMS_DUMMYTEST_H_ */

