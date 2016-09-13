#ifndef MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_
#define MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/WorkflowAlgorithmRunner.h"

using Mantid::Algorithms::WorkflowAlgorithmRunner;

class WorkflowAlgorithmRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkflowAlgorithmRunnerTest *createSuite() { return new WorkflowAlgorithmRunnerTest(); }
  static void destroySuite( WorkflowAlgorithmRunnerTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_ */