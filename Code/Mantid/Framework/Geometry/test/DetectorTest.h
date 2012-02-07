#ifndef MANTID_TESTDETECTOR__
#define MANTID_TESTDETECTOR__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Component.h"
#include <cmath>
#include <iostream>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Quat;

class DetectorTest : public CxxTest::TestSuite
{
public:
  void testNameConstructor()
  {
    Detector det("det1",0,0);
    TS_ASSERT_EQUALS(det.getName(),"det1");
    TS_ASSERT(!det.getParent());
    TS_ASSERT_EQUALS(det.getID(),0);
    TS_ASSERT(!det.isMasked());
    TS_ASSERT(!det.isMonitor());
    TS_ASSERT(!det.isParametrized());
  }
  void testDetTopology()
  {
      V3D center;
      Detector det("det1",0,0);
      TSM_ASSERT_EQUALS("single detector should have rectangular topology",rect,det.getTopology(center));
  }

  void testNameParentConstructor()
  {
    Component parent("Parent");
    Detector det("det1",0,&parent);
    TS_ASSERT_EQUALS(det.getName(),"det1");
    TS_ASSERT(det.getParent());
    TS_ASSERT_EQUALS(det.getID(),0);
    TS_ASSERT(!det.isMasked());
    TS_ASSERT(!det.isMonitor());
  }

  void testId()
  {
    int id1=41;
    Detector det("det1",id1,0);
    TS_ASSERT_EQUALS(det.getID(),id1);
  }

  void testType()
  {
    Detector det("det",0,0);
    TS_ASSERT_EQUALS(det.type(),"DetectorComponent");
  }

  void testDead()
  {
    Detector det("det",0,0);
    TS_ASSERT( ! det.isMasked() );
  }

  void testMonitor()
  {
    Detector det("det",0,0);
    TS_ASSERT( ! det.isMonitor() );
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor() );
    TS_ASSERT( det.isMonitor() );
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor(false) );
    TS_ASSERT( ! det.isMonitor() );
  }

  void testGetNumberParameter()
  {
    Detector det("det",0,0);
    TS_ASSERT_EQUALS(det.getNumberParameter("testparam").size(), 0);
  }

  void testGetPositionParameter()
  {
    Detector det("det",0,0);
    TS_ASSERT_EQUALS(det.getPositionParameter("testparam").size(), 0);

  }

  void testGetRotationParameter()
  {
    Detector det("det",0,0);
    TS_ASSERT_EQUALS(det.getRotationParameter("testparam").size(), 0);
  }

  void testCalculateSignedTwoTheta()
  {
    Detector det("det",0,0);
    V3D observer(0, 1, 0);
    V3D axis(1, 0, 0);
    double theta = det.getTwoTheta(observer, axis);
    TS_ASSERT_DELTA(1.57, theta, 0.01);
  }

  void testCalculateTwoTheta()
  {
    Detector det("det",0,0);
    V3D observer(0, 0, 0); //sample
    V3D axis(1, 0, 0);
    V3D up(0,0,1);

    det.setPos(1, 0, 1);
    double theta = det.getTwoTheta(observer, axis);
    double signedTheta = det.getSignedTwoTheta(observer, axis, up);
    
    TSM_ASSERT_EQUALS("Absolute theta values should be identical", theta, std::abs(signedTheta));
    TSM_ASSERT_LESS_THAN("Defined to give a positive theta value", 0, signedTheta);

    det.setPos(1, 0, -1); //Move the detector round 180 degrees
    theta = det.getTwoTheta(observer, axis);
    signedTheta = det.getSignedTwoTheta(observer, axis, up);

    TSM_ASSERT_EQUALS("Absolute theta values should be identical", theta, std::abs(signedTheta));
    TSM_ASSERT_LESS_THAN("Defined to give a negative theta value", signedTheta, 0);
  }

  void testCalculateTwoThetaBoundaries()
  {
    Detector det("det",0,0);
    V3D observer(0, 0, 0); //sample
    V3D axis(1, 0, 0);
    V3D up(0,0,1);

    det.setPos(1, 1, 0);
    double twelveOClock = det.getSignedTwoTheta(observer, axis, up);

    det.setPos(1, 0.99, 0.01);
    double justPastHour = det.getSignedTwoTheta(observer, axis, up);

    det.setPos(1, 0.99, -0.01);
    double justBeforeHour = det.getSignedTwoTheta(observer, axis, up);

    det.setPos(1,-0.99, -0.01);
    double justGoneSix = det.getSignedTwoTheta(observer, axis, up);
    
    TS_ASSERT(twelveOClock > 0);
    TS_ASSERT(justPastHour > 0);
    TS_ASSERT(justBeforeHour < 0);
    TS_ASSERT(justGoneSix < 0);
  }

};

#endif
