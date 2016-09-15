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
  static void destroySuite(WorkflowAlgorithmRunnerTest *suite) { delete suite; }


  void test_EmptyInput() {
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT_THROWS_ANYTHING(algorithm.execute())
    TS_ASSERT(!algorithm.isExecuted())
  }

  void test_Init() {
    WorkflowAlgorithmRunner algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
  }

  void test_Name() {
    WorkflowAlgorithmRunner algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "WorkflowAlgorithmRunner")
  }

  void test_Version() {
    WorkflowAlgorithmRunner algorithm;
    TS_ASSERT_EQUALS(algorithm.version(), 1)
  }
};


#endif /* MANTID_ALGORITHMS_WORKFLOWALGORITHMRUNNERTEST_H_ */
