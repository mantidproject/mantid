#ifndef MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_
#define MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

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

#endif /* MANTID_ALGORITHMS_RAYTRACERTESTERTEST_H_ */
