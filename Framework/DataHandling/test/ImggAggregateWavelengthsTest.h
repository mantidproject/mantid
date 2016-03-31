#ifndef MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_
#define MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ImggAggregateWavelengths.h"

using Mantid::DataHandling::ImggAggregateWavelengths;

class ImggAggregateWavelengthsTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImggAggregateWavelengthsTest *createSuite() {
    return new ImggAggregateWavelengthsTest();
  }

  static void destroySuite(ImggAggregateWavelengthsTest *suite) {
    delete suite;
  }

  void test_init() {
    ImggAggregateWavelengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_fail() {
    ImggAggregateWavelengths alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputPath", "."));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHSTEST_H_ */