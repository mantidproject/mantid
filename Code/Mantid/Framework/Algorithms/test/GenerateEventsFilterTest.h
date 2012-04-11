#ifndef MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_
#define MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/GenerateEventsFilter.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class GenerateEventsFilterTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateEventsFilterTest *createSuite() { return new GenerateEventsFilterTest(); }
  static void destroySuite( GenerateEventsFilterTest *suite ) { delete suite; }


  void test_Init()
  {
    GenerateEventsFilter alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }


};


#endif /* MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_ */
