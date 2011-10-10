#ifndef MANTID_ALGORITHMS_STRIPVANADIUMPEAKS2TEST_H_
#define MANTID_ALGORITHMS_STRIPVANADIUMPEAKS2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/StripVanadiumPeaks2.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class StripVanadiumPeaks2Test : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StripVanadiumPeaks2Test *createSuite() { return new StripVanadiumPeaks2Test(); }
  static void destroySuite( StripVanadiumPeaks2Test *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_STRIPVANADIUMPEAKS2TEST_H_ */

