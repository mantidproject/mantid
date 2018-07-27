#ifndef MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/MeshObject2D.h"

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
    alg.setChild(true);
    const std::string inputFile = "SMALLFAKE_example_geometry.hdf5";
    alg.initialize();
    alg.setPropertyValue("FileName", inputFile);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs =
        alg.getProperty("OutputWorkspace");

    auto &componentinfo = outputWs->componentInfo();
    TS_ASSERT_EQUALS(componentinfo.name(componentinfo.root()),
                     "SmallFakeTubeInstrument");
  }
  void test_load_loki() {

    LoadNexusGeometry alg;
    alg.setChild(true);
    const std::string outputWorkspaceName = "LoadNexusGeometryTestWS";
    const std::string inputFile = "LOKI_Definition.hdf5";
    alg.initialize();
    alg.setPropertyValue("FileName", inputFile);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.execute();

    Mantid::API::MatrixWorkspace_sptr outputWs =
        alg.getProperty("OutputWorkspace");

    auto &componentinfo = outputWs->componentInfo();
    TS_ASSERT_EQUALS(componentinfo.name(componentinfo.root()), "LOKI");

    // Add detectors are described by a meshobject 2d
    auto &shape = componentinfo.shape(0);
    auto *match = dynamic_cast<const Mantid::Geometry::MeshObject2D *>(&shape);
    TS_ASSERT(match);
  }
};

#endif /* MANTID_DATAHANDLING_LOADNEXUSGEOMETRYTEST_H_ */
