// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNXSPE.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadNXSPETest : public CxxTest::TestSuite {
public:
  void test_Init() {
    LoadNXSPE alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("LoadNXSPETest_OutputWS");

    LoadNXSPE alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "NXSPEData.nxspe"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Checks that the instrument name and deltaE-mode is correct
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(), "IRIS");
    TS_ASSERT_EQUALS(ws->getEMode(), Kernel::DeltaEMode::Indirect);

    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_identifier_confidence() {
    const int high_confidence = LoadNXSPE::identiferConfidence("NXSPE");
    const int good_confidence = LoadNXSPE::identiferConfidence("NXSP");
    const int no_confidence = LoadNXSPE::identiferConfidence("NXS");

    TS_ASSERT(high_confidence > good_confidence);
    TS_ASSERT(good_confidence > no_confidence);
  }
};
