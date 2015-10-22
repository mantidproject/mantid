#ifndef MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_
#define MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/AccumulateMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/IMDHistoWorkspace.h"

using Mantid::MDAlgorithms::AccumulateMD;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class AccumulateMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AccumulateMDTest *createSuite() { return new AccumulateMDTest(); }

  static void destroySuite(AccumulateMDTest *suite) { delete suite; }

  void test_Init() {
    AccumulateMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_something() {
    AccumulateMD alg;
    alg.initialize();
  }
};

#endif /* MANTID_MDALGORITHMS_ACCUMULATEMDTEST_H_ */
