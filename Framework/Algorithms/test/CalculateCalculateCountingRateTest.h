#ifndef MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_
#define MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateCountingRate.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::CalculateCountingRateTest;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using namespace Mantid::API;
using namespace Mantid::Geometry;


class CalculateCountingRateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateCountingRateTest *createSuite() { return new CalculateCountingRateTest(); }
  static void destroySuite(CalculateCountingRateTest *suite) { delete suite; }

  void test_Init() {
    CalculateCountingRateTest alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }


};

#endif /* MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_ */
