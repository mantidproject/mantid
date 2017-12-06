#ifndef MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNexusGeometry.h"

using Mantid::DataHandling::LoadNexusGeometry;

class LoadNexusGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadNexusGeometryTest *createSuite() {
    return new LoadNexusGeometryTest();
  }
  static void destroySuite(LoadNexusGeometryTest *suite) { delete suite; }

  void test_Init() {
    LoadNexusGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    LoadNexusGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
  }
};

#endif /* MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_ */
