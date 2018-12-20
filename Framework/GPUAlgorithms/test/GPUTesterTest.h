#ifndef MANTID_GPUALGORITHMS_GPUTESTERTEST_H_
#define MANTID_GPUALGORITHMS_GPUTESTERTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>

#include "MantidGPUAlgorithms/GPUTester.h"

using namespace Mantid;
using namespace Mantid::GPUAlgorithms;

class GPUTesterTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    GPUTester alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("GPUTesterTest_OutputWS");

    GPUTester alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XSize", 256));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("YSize", 256));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check the results
    bool result = alg.getProperty("Result");
    TS_ASSERT(result);

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
  }
};

#endif /* MANTID_GPUALGORITHMS_GPUTESTERTEST_H_ */
