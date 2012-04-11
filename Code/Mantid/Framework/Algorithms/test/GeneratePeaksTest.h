#ifndef MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_
#define MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/GeneratePeaks.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class GeneratePeaksTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneratePeaksTest *createSuite() { return new GeneratePeaksTest(); }
  static void destroySuite( GeneratePeaksTest *suite ) { delete suite; }


  void test_Init()
  {
    GeneratePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }


};


#endif /* MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_ */
