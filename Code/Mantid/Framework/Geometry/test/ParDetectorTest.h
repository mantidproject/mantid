#ifndef MANTID_TESTPARDETECTOR__
#define MANTID_TESTPARDETECTOR__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"

using namespace Mantid::Geometry;

class ParDetectorTest : public CxxTest::TestSuite
{
public:
  void testNameConstructor()
  {
    Detector det("det1",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    Detector pdet(&det,pmap.get());

    TS_ASSERT_EQUALS(pdet.getName(),"det1");
    TS_ASSERT(!pdet.getParent());
    TS_ASSERT_EQUALS(pdet.getID(),0);
    TS_ASSERT(!pdet.isMasked());
    TS_ASSERT(!pdet.isMonitor());
  }

  void testNameParentConstructor()
  {
    Component parent("Parent");
    Detector det("det1", 0, &parent);

    ParameterMap_sptr pmap( new ParameterMap() );
    Detector pdet(&det,pmap.get());

    TS_ASSERT_EQUALS(pdet.getName(),"det1");
    TS_ASSERT(pdet.getParent());
    TS_ASSERT_EQUALS(pdet.getID(),0);
    TS_ASSERT(!pdet.isMasked());
    TS_ASSERT(!pdet.isMonitor());
  }

  void testId()
  {
    int id1=41;
    Detector det("det1",id1,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    Detector pdet(&det,pmap.get());

    TS_ASSERT_EQUALS(pdet.getID(),id1);
  }

  void testType()
  {
    Detector det("det",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    Detector pdet(&det,pmap.get());

    TS_ASSERT_EQUALS(pdet.type(),"DetectorComponent");
  }

  void testMasked()
  {
    Detector det("det",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    Detector pdet(&det,pmap.get());

    TS_ASSERT( ! pdet.isMasked() );
    pmap->addBool(&det,"masked",true);
    TS_ASSERT( pdet.isMasked() );
  }

  void testMonitor()
  {
    Detector det("det",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    Detector pdet(&det,pmap.get());

    TS_ASSERT( ! pdet.isMonitor() );
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor() );
    TS_ASSERT( pdet.isMonitor() );
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor(false) );
    TS_ASSERT( ! pdet.isMonitor() );
  }

  void testGetNumberParameter()
  {
    Detector det("det",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    pmap->add("double", &det, "testparam", 5.0);
    Detector pdet(&det,pmap.get());
    IDetector *idet = static_cast<IDetector*>(&pdet);

    TS_ASSERT_EQUALS(idet->getNumberParameter("testparam").size(), 1);
    TS_ASSERT_DELTA(idet->getNumberParameter("testparam")[0], 5.0, 1e-08);

  }

  void testGetPositionParameter()
  {
    Detector det("det",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    pmap->add("V3D", &det, "testparam", Mantid::Geometry::V3D(0.5, 1.0, 1.5));
    Detector pdet(&det,pmap.get());
    IDetector *idet = static_cast<IDetector*>(&pdet);

    std::vector<Mantid::Geometry::V3D> pos = idet->getPositionParameter("testparam");

    TS_ASSERT_EQUALS(pos.size(), 1);
    TS_ASSERT_DELTA(pos[0].X(), 0.5, 1e-08);
    TS_ASSERT_DELTA(pos[0].Y(), 1.0, 1e-08);
    TS_ASSERT_DELTA(pos[0].Z(), 1.5, 1e-08);

  }

  void testGetRotationParameter()
  {
    Detector det("det",0,0);

    ParameterMap_sptr pmap( new ParameterMap() );
    pmap->add("Quat", &det, "testparam", Mantid::Geometry::Quat(1.0, 0.25, 0.5, 0.75));
    Detector pdet(&det,pmap.get());
    IDetector *idet = static_cast<IDetector*>(&pdet);

    std::vector<Mantid::Geometry::Quat> rot = idet->getRotationParameter("testparam");

    TS_ASSERT_EQUALS(rot.size(), 1);
    TS_ASSERT_DELTA(rot[0].real(), 1.0, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagI(), 0.25, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagJ(), 0.5, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagK(), 0.75, 1e-08);
  }
 
};

#endif
