#ifndef TESTDETECTORGROUP_H_
#define TESTDETECTORGROUP_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::Geometry;

class DetectorGroupTest : public CxxTest::TestSuite
{
public:
  DetectorGroupTest() : m_origin()
  {
    m_detGroup = ComponentCreationHelper::createDetectorGroupWith5CylindricalDetectors();
  }


  void testConstructors()
  {
    TS_ASSERT_EQUALS( m_detGroup->getDetectorIDs().size(), 5);
  }

  void testGetPos()
  {
    V3D pos;
    TS_ASSERT_THROWS_NOTHING( pos = m_detGroup->getPos() );
    TS_ASSERT_DELTA( pos.X(), 3.0, 1e-08 );
    TS_ASSERT_DELTA( pos.Y(), 2.0, 1e-08 );
    TS_ASSERT_DELTA( pos.Z(), 2.0, 1e-08 );
  }

  void testGetDetectorIDs()
  {
    std::vector<int> detIDs = m_detGroup->getDetectorIDs();
    TS_ASSERT_EQUALS( detIDs.size(), 5 );
    for(int i = 0; i < detIDs.size(); ++i )
    {
      TS_ASSERT_EQUALS(detIDs[i], i + 1);
    }
    
  }

  void testGetDetectors()
  {
    std::vector<IDetector_sptr> dets = m_detGroup->getDetectors();
    TS_ASSERT_EQUALS( dets.size(), 5 );
    for(int i = 0; i < dets.size(); ++i )
    {
      TS_ASSERT(dets[i]);
    }
  }


  void testGetID()
  {
    TS_ASSERT_EQUALS( m_detGroup->getID(), 1 );
  }
  
  void testGetDistance()
  {
    TS_ASSERT_DELTA( m_detGroup->getDistance(m_origin), 4.24614987, 1e-08 );
  }

  void testMasked()
  {
    TS_ASSERT( ! m_detGroup->isMasked() );
  }

  void testIsMonitor()
  {
    boost::shared_ptr<DetectorGroup> monitorGroup = ComponentCreationHelper::createGroupOfTwoMonitors();
    TS_ASSERT( !m_detGroup->isMonitor() );
    TS_ASSERT( monitorGroup->isMonitor() );
  }

  void testBoundingBox()
  {
  }

  void testAddDetector()
  {
    boost::shared_ptr<DetectorGroup> detg =  ComponentCreationHelper::createDetectorGroupWith5CylindricalDetectors();
    boost::shared_ptr<Detector> d(new Detector("d",6,0));
    d->setPos(6.0, 3.0, 2.0);
    TS_ASSERT(!detg->isMasked());
    bool warn = true;
    detg->addDetector(d,warn);
    TS_ASSERT_EQUALS( detg->getID(), 1 );
    TS_ASSERT_DELTA( detg->getPos()[0], 3.5, 1e-08 );
    TS_ASSERT_DELTA( detg->getPos()[1], 2.16666667, 1e-08 );
    TS_ASSERT_DELTA( detg->getPos()[2], 2.0, 1e-08 );
  }

  void test_That_The_Bounding_Box_Is_Large_Enough_For_All_Of_The_Detectors()
  {
    BoundingBox bbox;
    m_detGroup->getBoundingBox(bbox);
    V3D min = bbox.minPoint();
    V3D max = bbox.maxPoint();
    TS_ASSERT_DELTA(min.X(), 0.5, 1e-08); 
    TS_ASSERT_DELTA(min.Y(), 2.0, 1e-08);  
    TS_ASSERT_DELTA(min.Z(), 1.5, 1e-08);
    TS_ASSERT_DELTA(max.X(), 5.5, 1e-08); 
    TS_ASSERT_DELTA(max.Y(), 3.5, 1e-08);  
    TS_ASSERT_DELTA(max.Z(), 2.5, 1e-08);
  }

private:
  boost::shared_ptr<DetectorGroup> m_detGroup;
  Component m_origin;
};

#endif /*TESTDETECTORGROUP_H_*/
