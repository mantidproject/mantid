#ifndef NRCALCULATESLITRESOLUTIONTEST_H_
#define NRCALCULATESLITRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/NRCalculateSlitResolution.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class NRCalculateSlitResolutionTest : public CxxTest::TestSuite {
public:
  void testNRCalculateSlitResolutionX() {
    auto ws =
        createWorkspace("testCalcResWS2", V3D(1, 0, 0), 0.5, V3D(0, 0, 0), 1);

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
        createWorkspace("testCalcResWS", V3D(0, 0, 0), 1, V3D(0, 0, 1), 0.5);

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
    auto ws =
        createWorkspace("testCalcResLogWS", V3D(0, 0, 0), 1, V3D(0, 0, 1), 0.5);

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
        createWorkspace("testCalcTSWS", V3D(0, 0, 0), 1, V3D(0, 0, 1), 0.5);

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

  Workspace2D_sptr createWorkspace(std::string name, V3D s1Pos, double s1VG,
                                   V3D s2Pos, double s2VG) {
    Workspace2D_sptr ws = boost::make_shared<Workspace2D>();
    Instrument_sptr instrument = boost::make_shared<Instrument>();

    ObjComponent *s1 = new ObjComponent("slit1");
    s1->setPos(s1Pos);
    instrument->add(s1);

    ObjComponent *s2 = new ObjComponent("slit2");
    s2->setPos(s2Pos);
    instrument->add(s2);

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 0));
    instrument->add(source);
    instrument->markAsSource(source);

    Detector *monitor = new Detector("Monitor", 1, NULL);
    monitor->setPos(14, 0, 0);
    instrument->add(monitor);
    instrument->markAsMonitor(monitor);

    ObjComponent *sample = new ObjComponent("some-surface-holder");
    source->setPos(V3D(15, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector *det = new Detector("point-detector", 2, NULL);
    det->setPos(20, (20 - sample->getPos().X()), 0);
    instrument->add(det);
    instrument->markAsDetector(det);

    ws->setInstrument(instrument);

    ParameterMap &pmap = ws->instrumentParameters();
    pmap.addDouble(s1, "vertical gap", s1VG);
    pmap.addDouble(s2, "vertical gap", s2VG);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(name, ws));
    return ws;
  }
};

#endif /*NRCALCULATESLITRESOLUTIONTEST_H_*/
