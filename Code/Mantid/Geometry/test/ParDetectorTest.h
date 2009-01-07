#ifndef MANTID_TESTPARDETECTOR__
#define MANTID_TESTPARDETECTOR__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Detector.h"
#include "MantidGeometry/ParDetector.h"
#include "MantidGeometry/Component.h"

using namespace Mantid::Geometry;

class testParDetector : public CxxTest::TestSuite
{
public:
  void testNameConstructor()
  {
    Detector det("det1",0);

    ParameterMap pmap;
    ParDetector pdet(&det,&pmap);

    TS_ASSERT_EQUALS(pdet.getName(),"det1");
    TS_ASSERT(!pdet.getParent());
    TS_ASSERT_EQUALS(pdet.getID(),0);
    TS_ASSERT(!pdet.isDead());
    TS_ASSERT(!pdet.isMonitor());
  }

  void testNameParentConstructor()
  {
    Component parent("Parent");
    Detector det("det1", &parent);

    ParameterMap pmap;
    ParDetector pdet(&det,&pmap);

    TS_ASSERT_EQUALS(pdet.getName(),"det1");
    TS_ASSERT(pdet.getParent());
    TS_ASSERT_EQUALS(pdet.getID(),0);
    TS_ASSERT(!pdet.isDead());
    TS_ASSERT(!pdet.isMonitor());
  }

  void testId()
  {
    int id1=41;
    int id2=-43;
    Detector det("det1",0);

    ParameterMap pmap;
    ParDetector pdet(&det,&pmap);

    TS_ASSERT_EQUALS(pdet.getID(),0);
    det.setID(id1);
    TS_ASSERT_EQUALS(pdet.getID(),id1);
    det.setID(id2);
    TS_ASSERT_EQUALS(pdet.getID(),id2);
  }

  void testType()
  {
    Detector det("det",0);

    ParameterMap pmap;
    ParDetector pdet(&det,&pmap);

    TS_ASSERT_EQUALS(pdet.type(),"ParDetectorComponent");
  }

  void testDead()
  {
    Detector det("det",0);

    ParameterMap pmap;
    ParDetector pdet(&det,&pmap);

    TS_ASSERT( ! pdet.isDead() )
    TS_ASSERT_THROWS_NOTHING( det.markDead() )
    TS_ASSERT( pdet.isDead() )
    // Re-flagging as dead doesn't throw, just prints a warning
    TS_ASSERT_THROWS_NOTHING( det.markDead() )
    TS_ASSERT( pdet.isDead() )
  }

  void testMonitor()
  {
    Detector det("det",0);

    ParameterMap pmap;
    ParDetector pdet(&det,&pmap);

    TS_ASSERT( ! pdet.isMonitor() )
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor() )
    TS_ASSERT( pdet.isMonitor() )
    TS_ASSERT_THROWS_NOTHING( det.markAsMonitor(false) )
    TS_ASSERT( ! pdet.isMonitor() )
  }
};

#endif
