#ifndef MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_
#define MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidAlgorithms/RayTracerTester.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class RayTracerTesterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RayTracerTesterTest *createSuite() {
    return new RayTracerTesterTest();
  }
  static void destroySuite(RayTracerTesterTest *suite) { delete suite; }

  void test_Init() {
    RayTracerTester alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /** Disabled because this isn't a real algorithm, just a testing one */
  void xtest_exec() {
    RayTracerTester alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Filename", "CNCS_Definition.xml");
    alg.setPropertyValue("OutputWorkspace", "cncs");
    alg.exec();
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
