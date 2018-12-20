#ifndef MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_
#define MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_

#include "MantidAlgorithms/RayTracerTester.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

// There are only performance tests here as this is not a real algorithm.
// Functional tests disabled ec34e64616f34f1cf476b65f934272fdfda1212f
// Unfortunately CxxTest/CTest gets confused if no functional test is present!
// The following class does precicely nothing.
class RayTracerTesterTest : public CxxTest::TestSuite {
public:
  static RayTracerTesterTest *createSuite() {
    return new RayTracerTesterTest();
  }
  static void destroySuite(RayTracerTesterTest *suite) { delete suite; }
  void test_dummy() {
    // No tests. See comments above.
  }
};

class RayTracerTesterTestPerformance : public CxxTest::TestSuite {
public:
  static RayTracerTesterTestPerformance *createSuite() {
    return new RayTracerTesterTestPerformance();
  }
  static void destroySuite(RayTracerTesterTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Filename", "CNCS_Definition.xml");
    alg.setPropertyValue("OutputWorkspace", "cncs");
  }

  void test_performance() {
    int iterations = 3;
    doTestExec(iterations);
  }

  void doTestExec(int iterations) {
    for (int i = 0; i < iterations; i++) {
      alg.exec();
    }
  }

private:
  RayTracerTester alg;
};
#endif /* MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_ */
