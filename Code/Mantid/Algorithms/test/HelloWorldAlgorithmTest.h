#ifndef HELLOWORLDALGORITHMTEST_H_
#define HELLOWORLDALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/HelloWorldAlgorithm.h"

using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;

class HelloWorldAlgorithmTest : public CxxTest::TestSuite
{
private:
  HelloWorldAlgorithm* alg;  

public:
  
  HelloWorldAlgorithmTest()
  { 
    alg = new HelloWorldAlgorithm;
  }
  
  ~HelloWorldAlgorithmTest()
  {
     delete alg;
  }

  
  void testExec()
  {
    StatusCode status = alg->exec();
    TS_ASSERT( ! status.isFailure() );
  }

};

#endif /*HELLOWORLDALGORITHMTEST_H_*/
