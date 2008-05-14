#ifndef TESTDETECTORGROUP_H_
#define TESTDETECTORGROUP_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/DetectorGroup.h"
#include "MantidGeometry/Detector.h"

using namespace Mantid::Geometry;

class DetectorGroupTest : public CxxTest::TestSuite
{
public:
  DetectorGroupTest()
  {
//    group = new DetectorGroup(detvec);
    d1 = new Detector;
    d1->setID(99);
    d1->setPos(2.0,2.0,2.0);
    detvec.push_back(d1);
    group = new DetectorGroup(detvec);
//    group->addDetector(d1);
    d2 = new Detector;
    d2->setID(11);
    d2->setPos(3.0,4.0,5.0);
    group->addDetector(d2);
    dg = new DetectorGroup( *(new std::vector<IDetector*>(1,group)) );
//    dg->addDetector(group);
    d3 = new Detector;
    d3->setID(10);
    d3->setPos(5.0,5.0,5.0);
    dg->addDetector(d3);
  }

  ~DetectorGroupTest()
  {
    delete d1, d2, d3;
    delete dg, group;
  }
  
  void testConstructors()
  {
    std::vector<IDetector*> vec;
    vec.push_back(d3);
    vec.push_back(d1);
    DetectorGroup detg(vec);
    TS_ASSERT_EQUALS( detg.getID(), 10 )
    TS_ASSERT( ! detg.isDead() )
    TS_ASSERT_DELTA( detg.getDistance(comp), 6.0622, 0.0001 )
  }

  void testAddDetector()
  {
    DetectorGroup detg(detvec);
    TS_ASSERT_EQUALS( detg.getID(), 99 )
    TS_ASSERT( ! detg.isDead() )
    TS_ASSERT_EQUALS( detg.getPos()[0], 2.0 )
    TS_ASSERT_EQUALS( detg.getPos()[1], 2.0 )
    TS_ASSERT_EQUALS( detg.getPos()[2], 2.0 )
    Detector *d = new Detector;
    d->setID(5);
    d->setPos(6.0, 3.0, 2.0);

    d->markDead();
    TS_ASSERT( ! detg.isDead() )
    d1->markDead();
    TS_ASSERT( detg.isDead() )

    detg.addDetector(d);
    TS_ASSERT_EQUALS( detg.getID(), 99 )
    TS_ASSERT_EQUALS( detg.getPos()[0], 4.0 )
    TS_ASSERT_EQUALS( detg.getPos()[1], 2.5 )
    TS_ASSERT_EQUALS( detg.getPos()[2], 2.0 )
    delete d;
  }

  void testGetID()
  {
    TS_ASSERT_EQUALS( dg->getID(), 99 )
  }

  void testGetPos()
  {
    V3D pos;
    TS_ASSERT_THROWS_NOTHING( pos = dg->getPos() )
    TS_ASSERT_DELTA( pos.X(), 3.75, 0.00001 )
    TS_ASSERT_DELTA( pos.Y(), 4.0, 0.00001 )
    TS_ASSERT_DELTA( pos.Z(), 4.25, 0.00001 )
  }

  void testGetDistance()
  {
    TS_ASSERT_DELTA( dg->getDistance(comp), 6.9372, 0.0001 )
  }

  void testDead()
  {
    TS_ASSERT( ! dg->isDead() )
    TS_ASSERT_THROWS_NOTHING( dg->markDead() )
    TS_ASSERT( dg->isDead() )
    // Re-flagging as dead doesn't throw, just prints a warning
    TS_ASSERT_THROWS_NOTHING( dg->markDead() )  
    TS_ASSERT( dg->isDead() )
  }

private:
  std::vector<IDetector*> detvec;
  DetectorGroup *dg, *group;
  Detector *d1, *d2, *d3;
  Component comp;
};

#endif /*TESTDETECTORGROUP_H_*/
