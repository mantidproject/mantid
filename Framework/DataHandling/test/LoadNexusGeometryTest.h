#ifndef MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument.h"

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

  void test_output_workspace_contains_instrument_with_expected_name() {
    LoadNexusGeometry alg;
    const std::string outputWorkspaceName = "LoadNexusGeometryTestWS";
    const std::string inputFile = "SMALLFAKE_example_geometry.hdf5";
    const std::string instrumentName = "SmallFakeTubeInstrument";
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("FileName", inputFile);
    alg.setPropertyValue("InstrumentName", instrumentName);
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWorkspaceName))
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted())

    Mantid::API::MatrixWorkspace_sptr outputWs =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(outputWorkspaceName);
    Mantid::Geometry::Instrument_const_sptr instrument;
    TS_ASSERT_THROWS_NOTHING(instrument = outputWs->getInstrument())
    TS_ASSERT_EQUALS(instrument->getFullName(), instrumentName)
  }
};

#endif /* MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_ */
