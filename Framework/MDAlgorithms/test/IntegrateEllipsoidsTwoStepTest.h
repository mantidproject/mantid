#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;

class IntegrateEllipsoidsTwoStepTest : public CxxTest::TestSuite {

public:
  static void destroySuite(IntegrateEllipsoidsTwoStepTest *suite) { delete suite; }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTwoStepTest *createSuite() {
    return new IntegrateEllipsoidsTwoStepTest();
  }

  IntegrateEllipsoidsTwoStepTest() {
  }

  void test_init() {
    Mantid::MDAlgorithms::IntegrateEllipsoidsTwoStep alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }
};

