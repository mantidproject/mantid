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
    d1 = boost::shared_ptr<Detector>(new Detector("d1",0));
    d1->setID(99);
    d1->setPos(2.0,2.0,2.0);
    d1->markAsMonitor();
    detvec.push_back(d1);
    group = new DetectorGroup(detvec);
    d2 = boost::shared_ptr<Detector>(new Detector("d2",0));
    d2->setID(11);
    d2->setPos(3.0,4.0,5.0);
    d2->markAsMonitor();
    group->addDetector(d2);
    std::vector<IDetector_sptr> vec(1,boost::shared_ptr<IDetector>(group));
    dg = new DetectorGroup( vec );
    d3 = boost::shared_ptr<Detector>(new Detector("d3",0));
    d3->setID(10);
    d3->setPos(5.0,5.0,5.0);
    dg->addDetector(d3);
  }

  ~DetectorGroupTest()
  {
    delete dg, group;
  }

  void testConstructors()
  {
    std::vector<boost::shared_ptr<IDetector> > vec;
    vec.push_back(d3);
    vec.push_back(d1);
    DetectorGroup detg(vec);
    TS_ASSERT_EQUALS( detg.getID(), 10 )
    TS_ASSERT( ! detg.isMasked() )
    TS_ASSERT_DELTA( detg.getDistance(comp), 6.0622, 0.0001 )
  }

  void testAddDetector()
  {
    DetectorGroup detg(detvec);
    TS_ASSERT_EQUALS( detg.getID(), 99 )
    TS_ASSERT( ! detg.isMasked() )
    TS_ASSERT_EQUALS( detg.getPos()[0], 2.0 )
    TS_ASSERT_EQUALS( detg.getPos()[1], 2.0 )
    TS_ASSERT_EQUALS( detg.getPos()[2], 2.0 )
    boost::shared_ptr<Detector> d(new Detector("d",0));
    d->setID(5);
    d->setPos(6.0, 3.0, 2.0);
    TS_ASSERT( ! detg.isMasked() )

    detg.addDetector(d);
    TS_ASSERT_EQUALS( detg.getID(), 99 )
    TS_ASSERT_EQUALS( detg.getPos()[0], 4.0 )
    TS_ASSERT_EQUALS( detg.getPos()[1], 2.5 )
    TS_ASSERT_EQUALS( detg.getPos()[2], 2.0 )
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
    TS_ASSERT_DELTA( dg->getDistance(comp), 6.9639, 0.0001 )
  }

  void testMasked()
  {
    TS_ASSERT( ! dg->isMasked() )
  }

  void testIsMonitor()
  {
    TS_ASSERT( group->isMonitor() )
    TS_ASSERT( ! dg->isMonitor() )
  }

private:
  std::vector<boost::shared_ptr<IDetector> > detvec;
  DetectorGroup *dg, *group;
  boost::shared_ptr<Detector> d1, d2, d3;
  Component comp;
};

#endif /*TESTDETECTORGROUP_H_*/
