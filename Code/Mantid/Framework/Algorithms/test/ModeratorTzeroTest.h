#ifndef MANTID_ALGORITHMS_MODERATORTZEROTEST_H_
#define MANTID_ALGORITHMS_MODERATORTZEROTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/ModeratorTzero.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class ModeratorTzeroTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModeratorTzeroTest *createSuite() { return new ModeratorTzeroTest(); }
  static void destroySuite( ModeratorTzeroTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_MODERATORTZEROTEST_H_ */