#ifndef CALCULATERESOLUTIONTEST_H_
#define CALCULATERESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateResolution.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CalculateResolutionTest : public CxxTest::TestSuite
{
public:

  void testCalculateResolutionX()
  {
    auto ws = createWorkspace("testCalcResWS2", V3D(1,0,0), 0.5, V3D(0,0,0), 1);

    CalculateResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->name());
    alg.setProperty("TwoTheta", 1.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0429, 0.0001);
  }

  void testCalculateResolutionZ()
  {
    auto ws = createWorkspace("testCalcResWS", V3D(0,0,0), 1, V3D(0,0,1), 0.5);

    CalculateResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->name());
    alg.setProperty("TwoTheta", 1.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.0429, 0.0001);
  }

  Workspace2D_sptr createWorkspace(std::string name, V3D s1Pos, double s1VG, V3D s2Pos, double s2VG)
  {
    Workspace2D_sptr ws = boost::make_shared<Workspace2D>();
    Instrument_sptr instrument = boost::make_shared<Instrument>();

    ObjComponent* s1 = new ObjComponent("slit1");
    s1->setPos(s1Pos);
    instrument->add(s1);

    ObjComponent* s2 = new ObjComponent("slit2");
    s2->setPos(s2Pos);
    instrument->add(s2);

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 0));
    instrument->add(source);
    instrument->markAsSource(source);

    Detector* monitor = new Detector("Monitor", 1, NULL);
    monitor->setPos(14, 0, 0);
    instrument->add(monitor);
    instrument->markAsMonitor(monitor);

    ObjComponent *sample = new ObjComponent("some-surface-holder");
    source->setPos(V3D(15, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector* det = new Detector("point-detector", 2, NULL);
    det->setPos(20, (20 - sample->getPos().X()), 0);
    instrument->add(det);
    instrument->markAsDetector(det);

    ws->setInstrument(instrument);

    ParameterMap& pmap = ws->instrumentParameters();
    pmap.addDouble(s1, "vertical gap", s1VG);
    pmap.addDouble(s2, "vertical gap", s2VG);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(name, ws));
    return ws;
  }
};

#endif /*CALCULATERESOLUTIONTEST_H_*/
