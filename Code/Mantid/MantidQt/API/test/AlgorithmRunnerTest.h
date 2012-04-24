#ifndef MANTID_API_ALGORITHMRUNNERTEST_H_
#define MANTID_API_ALGORITHMRUNNERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/AlgorithmRunner.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;

class AlgorithmRunnerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmRunnerTest *createSuite() { return new AlgorithmRunnerTest(); }
  static void destroySuite( AlgorithmRunnerTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_API_ALGORITHMRUNNERTEST_H_ */