#ifndef MANTID_CRYSTAL_LOADISAWSpectrumTEST_H_
#define MANTID_CRYSTAL_LOADISAWSpectrumTEST_H_

#include "MantidCrystal/LoadIsawSpectrum.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidAPI/FrameworkManager.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentFilename",
                                                  "TOPAZ_Definition_2010.xml"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("SpectraFile", "Spectrum_ISAW.dat"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "LoadIsawSpectrumTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "LoadIsawSpectrumTest_ws"));

    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->readX(0)[9], 413.65, 0.01);
    TS_ASSERT_DELTA(ws->readY(0)[9], -0.0219, 0.01);
    TS_ASSERT_DELTA(ws->readX(12)[5], 407.2, 0.01);
    TS_ASSERT_DELTA(ws->readY(12)[5], 0.0182, 0.01);

    AnalysisDataService::Instance().remove("LoadIsawSpectrumTest_ws");
  }
};

#endif /* MANTID_CRYSTAL_LOADISAWSpectrumTEST_H_ */
