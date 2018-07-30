#ifndef MANTID_DATAHANDLING_LOADEMPTYNEXUSGEOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADEMPTYNEXUSGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadEmptyNexusGeometry.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

using Mantid::DataHandling::LoadEmptyNexusGeometry;

class LoadEmptyNexusGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadEmptyNexusGeometryTest *createSuite() {
    return new LoadEmptyNexusGeometryTest();
  }
  static void destroySuite(LoadEmptyNexusGeometryTest *suite) { delete suite; }

  void test_Init() {
    LoadEmptyNexusGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }
  void test_output_workspace_contains_instrument_with_expected_name() {
    LoadEmptyNexusGeometry alg;
    alg.setChild(true);
    const std::string inputFile = "SMALLFAKE_example_geometry.hdf5";
    alg.initialize();
    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs =
        alg.getProperty("OutputWorkspace");

    auto &componentInfo = outputWs->componentInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()),
                     "SmallFakeTubeInstrument");
  }
  void test_load_loki() {
    LoadEmptyNexusGeometry alg;
    alg.setChild(true);
    const std::string inputFile = "LOKI_Definition.hdf5";
    alg.initialize();
    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs =
        alg.getProperty("OutputWorkspace");

    auto &componentInfo = outputWs->componentInfo();
    auto &detectorInfo = outputWs->detectorInfo();
    TS_ASSERT_EQUALS(componentInfo.name(componentInfo.root()), "LOKI");
    TS_ASSERT_EQUALS(detectorInfo.size(), 8000);

    TS_ASSERT_EQUALS(0, detectorInfo.detectorIDs()[0])
    TS_ASSERT_EQUALS(1, detectorInfo.detectorIDs()[1])
  }
};

#endif /* MANTID_DATAHANDLING_LOADEMPTYNEXUSGEOMETRYTEST_H_ */
