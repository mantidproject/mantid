#ifndef MANTID_ALGORITHMS_VULCANREBINTEST_H_
#define MANTID_ALGORITHMS_VULCANREBINTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/IDLRebin.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class IDLRebinTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IDLRebinTest *createSuite() { return new IDLRebinTest(); }
  static void destroySuite( IDLRebinTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_VULCANREBINTEST_H_ */

