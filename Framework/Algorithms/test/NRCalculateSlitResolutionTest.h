#ifndef NRCALCULATESLITRESOLUTIONTEST_H_
#define NRCALCULATESLITRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/NRCalculateSlitResolution.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class NRCalculateSlitResolutionTest : public CxxTest::TestSuite {
public:
  void testNRCalculateSlitResolutionX() {
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, V3D(1, 0, 0), V3D(0, 0, 0), 0.5, 1.0);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setProperty("Workspace", ws);
    alg.setProperty("TwoTheta", 1.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  void testNRCalculateSlitResolutionZ() {
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, V3D(0, 0, 0), V3D(0, 0, 1), 1.0, 0.5);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setProperty("Workspace", ws);
    alg.setProperty("TwoTheta", 1.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  void testNRCalculateSlitResolutionThetaFromLog() {
    // Test getting theta from a log property with value
    // Test using the default log name
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, V3D(0, 0, 0), V3D(0, 0, 1), 1.0, 0.5);

    PropertyWithValue<double> *p =
        new PropertyWithValue<double>("Theta", 0.5); // default name is Theta
    ws->mutableRun().addLogData(p);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setProperty("Workspace", ws);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }

  void testNRCalculateSlitResolutionThetaFromTimeSeriesLog() {
    // Test getting theta from a time series property
    // Test using a non-default log name
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, V3D(0, 0, 0), V3D(0, 0, 1), 1.0, 0.5);

    TimeSeriesProperty<double> *p = new TimeSeriesProperty<double>("ThetaTSP");
    TS_ASSERT_THROWS_NOTHING(p->addValue("2007-11-30T16:17:00", 0.5));
    ws->mutableRun().addProperty(p, true);

    NRCalculateSlitResolution alg;
    alg.initialize();
    alg.setProperty("Workspace", ws);
    alg.setProperty("ThetaLogName", "ThetaTSP");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0859414, 1e-6);
  }
};

#endif /*NRCALCULATESLITRESOLUTIONTEST_H_*/
