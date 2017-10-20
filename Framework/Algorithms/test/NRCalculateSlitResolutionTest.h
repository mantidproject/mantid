#ifndef NRCALCULATESLITRESOLUTIONTEST_H_
#define NRCALCULATESLITRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/NRCalculateSlitResolution.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class NRCalculateSlitResolutionTest : public CxxTest::TestSuite {
public:
  void testNRCalculateSlitResolutionX() {
    auto ws =
        createWorkspace("testCalcResWS2", V3D(1, 0, 0), 0.5, V3D(0, 0, 0), 1.0);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->getName());
    alg.setProperty("TwoTheta", 1.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  void testNRCalculateSlitResolutionZ() {
    auto ws =
        createWorkspace("testCalcResWS", V3D(0, 0, 0), 1.0, V3D(0, 0, 1), 0.5);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->getName());
    alg.setProperty("TwoTheta", 1.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  void testNRCalculateSlitResolutionThetaFromLog() {
    // Test getting theta from a log property with value
    // Test using the default log name
    auto ws = createWorkspace("testCalcResLogWS", V3D(0, 0, 0), 1.0,
                              V3D(0, 0, 1), 0.5);

    PropertyWithValue<double> *p =
        new PropertyWithValue<double>("Theta", 0.5); // default name is Theta
    ws->mutableRun().addLogData(p);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->getName());
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  void testNRCalculateSlitResolutionThetaFromTimeSeriesLog() {
    // Test getting theta from a time series property
    // Test using a non-default log name
    auto ws =
        createWorkspace("testCalcTSWS", V3D(0, 0, 0), 1.0, V3D(0, 0, 1), 0.5);

    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("ThetaTSP");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 0.5));
    ws->mutableRun().addProperty(p, true);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->getName());
    alg.setProperty("ThetaLogName", "ThetaTSP");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  Workspace2D_sptr createWorkspace(const std::string &name, const V3D &s1Pos,
                                   double s1VG, const V3D &s2Pos, double s2VG) {

    Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, 2, 100, 2000, s1Pos, s2Pos, s1VG, s2VG);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(name, ws));
    return ws;
  }
};

#endif /*NRCALCULATESLITRESOLUTIONTEST_H_*/
