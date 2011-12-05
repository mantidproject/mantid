#ifndef MANTID_ALGORITHMS_VULCANFILTEREVENTSTEST_H_
#define MANTID_ALGORITHMS_VULCANFILTEREVENTSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/FilterEventsHighFrequency.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class FilterEventsHighFrequencyTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterEventsHighFrequencyTest *createSuite() { return new FilterEventsHighFrequencyTest(); }
  static void destroySuite( FilterEventsHighFrequencyTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_VULCANFILTEREVENTSTEST_H_ */
