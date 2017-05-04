#ifndef MANTID_PARALLEL_EXECUTIONMODETEST_H_
#define MANTID_PARALLEL_EXECUTIONMODETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/ExecutionMode.h"

using namespace Mantid::Parallel;

class ExecutionModeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExecutionModeTest *createSuite() { return new ExecutionModeTest(); }
  static void destroySuite(ExecutionModeTest *suite) { delete suite; }

  void test_getCorrespondingExecutionMode() {
    TS_ASSERT_EQUALS(getCorrespondingExecutionMode(StorageMode::Cloned),
                     ExecutionMode::Identical);
    TS_ASSERT_EQUALS(getCorrespondingExecutionMode(StorageMode::Distributed),
                     ExecutionMode::Distributed);
    TS_ASSERT_EQUALS(getCorrespondingExecutionMode(StorageMode::MasterOnly),
                     ExecutionMode::MasterOnly);
  }

  void test_toString() {
    TS_ASSERT_EQUALS(toString(ExecutionMode::Invalid),
                     "Parallel::ExecutionMode::Invalid");
    TS_ASSERT_EQUALS(toString(ExecutionMode::Serial),
                     "Parallel::ExecutionMode::Serial");
    TS_ASSERT_EQUALS(toString(ExecutionMode::Identical),
                     "Parallel::ExecutionMode::Identical");
    TS_ASSERT_EQUALS(toString(ExecutionMode::Distributed),
                     "Parallel::ExecutionMode::Distributed");
    TS_ASSERT_EQUALS(toString(ExecutionMode::MasterOnly),
                     "Parallel::ExecutionMode::MasterOnly");
  }
};

#endif /* MANTID_PARALLEL_EXECUTIONMODETEST_H_ */
