// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/RemoveInstrumentGeometry.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include <cxxtest/TestSuite.h>

using Mantid::Algorithms::RemoveInstrumentGeometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class RemoveInstrumentGeometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveInstrumentGeometryTest *createSuite() { return new RemoveInstrumentGeometryTest(); }
  static void destroySuite(RemoveInstrumentGeometryTest *suite) { delete suite; }

  void test_Init() {
    RemoveInstrumentGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_matrix_ws_no_inst() {
    // Create test input
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspace(5, 5);

    RemoveInstrumentGeometry alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "matrix_no_instrument"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    MatrixWorkspace_sptr out = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    TS_ASSERT(out);
    Instrument_sptr instr = std::const_pointer_cast<Instrument>(out->getInstrument());
    TS_ASSERT(instr->isEmptyInstrument());
  }

  void test_matrix_ws() {
    // Create test input
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(5, 5, true);

    RemoveInstrumentGeometry alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "matrix_removed_instrument"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    MatrixWorkspace_sptr out = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    TS_ASSERT(out);
    Instrument_sptr instr = std::const_pointer_cast<Instrument>(out->getInstrument());
    TS_ASSERT(instr->isEmptyInstrument());
  }

  // Helper method to create a MDHW
  std::string createMDHistoWorkspace(const uint16_t nExperimentInfosToAdd = 3) {

    Workspace2D_sptr ws2D = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3, true);
    const std::string wsName = "TestRemoveInstrumentMDWorkspace";

    auto instrument = ws2D->getInstrument();

    auto ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 10, 10, 1, wsName);
    ws->getExperimentInfo(0)->setInstrument(instrument);

    for (uint16_t i = 1; i < nExperimentInfosToAdd; ++i) {
      ExperimentInfo_sptr experimentInfo = std::make_shared<ExperimentInfo>();
      ws->addExperimentInfo(experimentInfo);
      ws->getExperimentInfo(i)->setInstrument(instrument);
    }

    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return wsName;
  }

  void test_md_ws_remove_all() {
    std::string inputWS = createMDHistoWorkspace(5);
    std::string wsName("TestRemoveInstrumentMDWorkspaceOutput");

    RemoveInstrumentGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MDHistoWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(wsName));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumExperimentInfo(), 5);
    for (uint16_t i = 0; i < 5; ++i) {
      TS_ASSERT(ws->getExperimentInfo(i)->getInstrument()->isEmptyInstrument());
    }
  }

  void test_md_ws_remove_partial() {
    std::string inputWS = createMDHistoWorkspace(5);
    std::string wsName("TestRemoveInstrumentMDWorkspaceOutput");

    RemoveInstrumentGeometry alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MDExperimentInfoNumbers", "1,3"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MDHistoWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(wsName));
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumExperimentInfo(), 5);
    for (uint16_t i = 0; i < 5; ++i) {
      if (i % 2 == 0) {
        TS_ASSERT(!(ws->getExperimentInfo(i)->getInstrument()->isEmptyInstrument()));
      } else {
        TS_ASSERT(ws->getExperimentInfo(i)->getInstrument()->isEmptyInstrument());
      }
    }
  }
};
