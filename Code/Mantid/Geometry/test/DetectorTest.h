#ifndef MANTID_TESTDETECTOR__
#define MANTID_TESTDETECTOR__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Detector.h"
#include "MantidGeometry/Component.h"

using namespace Mantid::Geometry;

class testDetector : public CxxTest::TestSuite
{
public:
  void testNameConstructor()
  {
    Detector det("det1",0);
    TS_ASSERT_EQUALS(det.getName(),"det1");
    TS_ASSERT(!det.getParent());
    TS_ASSERT_EQUALS(det.getID(),0);
  }

  void testNameParentConstructor()
  {
    Component parent("Parent");
    Detector det("det1", &parent);
    TS_ASSERT_EQUALS(det.getName(),"det1");
    TS_ASSERT(det.getParent());
    TS_ASSERT_EQUALS(det.getID(),0);
  }

  void testId()
  {
    int id1=41;
    int id2=-43;
    Detector det("det1",0);
    TS_ASSERT_EQUALS(det.getID(),0);
    det.setID(id1);
    TS_ASSERT_EQUALS(det.getID(),id1);
    det.setID(id2);
    TS_ASSERT_EQUALS(det.getID(),id2);
  }

  void testType()
  {
    Detector det("det",0);
    TS_ASSERT_EQUALS(det.type(),"DetectorComponent");
  }

  void testDead()
  {
    Detector det("det",0);
    TS_ASSERT( ! det.isDead() )
    TS_ASSERT_THROWS_NOTHING( det.markDead() )
    TS_ASSERT( det.isDead() )
    // Re-flagging as dead doesn't throw, just prints a warning
    TS_ASSERT_THROWS_NOTHING( det.markDead() )
    TS_ASSERT( det.isDead() )
  }
};

#endif
