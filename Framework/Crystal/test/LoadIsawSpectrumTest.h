// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/LoadIsawSpectrum.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class LoadIsawSpectrumTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    LoadIsawSpectrum alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    LoadIsawSpectrum alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentFilename", "TOPAZ_Definition_2010.xml"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SpectraFile", "Spectrum_ISAW.dat"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "LoadIsawSpectrumTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("LoadIsawSpectrumTest_ws"));

    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->x(0)[9], 413.65, 0.01);
    TS_ASSERT_DELTA(ws->y(0)[9], -0.0219, 0.01);
    TS_ASSERT_DELTA(ws->x(12)[5], 407.2, 0.01);
    TS_ASSERT_DELTA(ws->y(12)[5], 0.0182, 0.01);

    AnalysisDataService::Instance().remove("LoadIsawSpectrumTest_ws");
  }
};
