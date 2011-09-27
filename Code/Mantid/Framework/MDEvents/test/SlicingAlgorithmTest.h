#ifndef MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_
#define MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/SlicingAlgorithm.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class SlicingAlgorithmTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SlicingAlgorithmTest *createSuite() { return new SlicingAlgorithmTest(); }
  static void destroySuite( SlicingAlgorithmTest *suite ) { delete suite; }


  void test_nothing()
  {
    /* Class is abstract, will not test directly for now */
  }
  

};


#endif /* MANTID_MDEVENTS_SLICINGALGORITHMTEST_H_ */

