#ifndef INSTRUMENTTEST_H_
#define INSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/DetectorGroup.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class InstrumentTest : public CxxTest::TestSuite
{
public:
  InstrumentTest()
  {
    ObjComponent *sample = new ObjComponent;
    instrument.markAsSamplePos(sample);
    det = new Detector;
    det->setID(1);
    det->setPos(1.0,0.0,0.0);
    instrument.markAsDetector(det);
    det2 = new Detector;
    det2->setID(10);
    instrument.markAsDetector(det2);
    det3 = new Detector;
    det3->setID(11);
    instrument.markAsDetector(det3);
  }
  
  ~InstrumentTest()
  {
    delete det, det2, det3;
    delete instrument.getSamplePos();
  }
  
  void testType()
  {
    TS_ASSERT_EQUALS( instrument.type(), "Instrument" )
  }

  void testConstructor()
  {
    Instrument i;
    TS_ASSERT( ! i.getSource() )
    TS_ASSERT( ! i.getSamplePos() )
	  
    Instrument ii("anInstrument");
    TS_ASSERT( ! ii.getSource() )
    TS_ASSERT( ! ii.getSamplePos() )
    TS_ASSERT_EQUALS( ii.getName(), "anInstrument" )
  }

  void testSource()
  {
    Instrument i;
    TS_ASSERT( ! i.getSource() )
    ObjComponent *s = new ObjComponent;
    TS_ASSERT_THROWS_NOTHING( i.markAsSource(s) )
    TS_ASSERT_EQUALS( i.getSource(), s )
    ObjComponent *ss = new ObjComponent;
    TS_ASSERT_THROWS_NOTHING( i.markAsSource(ss) )
    TS_ASSERT_EQUALS( i.getSource(), s )
    delete s;
    delete ss;
  }

  void testSamplePos()
  {
    Instrument i;
    TS_ASSERT( ! i.getSamplePos() )
    ObjComponent *s = new ObjComponent;
    TS_ASSERT_THROWS_NOTHING( i.markAsSamplePos(s) )
    TS_ASSERT_EQUALS( i.getSamplePos(), s )
    ObjComponent *ss = new ObjComponent;
    TS_ASSERT_THROWS_NOTHING( i.markAsSamplePos(ss) )
    TS_ASSERT_EQUALS( i.getSamplePos(), s )
    delete s;
    delete ss;
  }

  void testDetector()
  {
    TS_ASSERT_THROWS( instrument.getDetector(0), Exception::NotFoundError )
    TS_ASSERT_EQUALS( instrument.getDetector(1), det )
    TS_ASSERT_THROWS( instrument.getDetector(2), Exception::NotFoundError )
    Detector *d = new Detector;
    d->setID(2);
    TS_ASSERT_THROWS_NOTHING( instrument.markAsDetector(d) )
    TS_ASSERT_EQUALS( instrument.getDetector(2), d )
    delete d;
  }

  void testDetectorLocation()
  {
    double l2, twoTheta;
    TS_ASSERT_THROWS_NOTHING( instrument.detectorLocation(1,l2,twoTheta) )
    TS_ASSERT_DELTA( l2, 1.0, 0.00001 )
    TS_ASSERT_DELTA( twoTheta, M_PI/2.0, 0.00001 )

    // Should throw because detector ID not in map
    TS_ASSERT_THROWS( instrument.detectorLocation(3,l2,twoTheta), Exception::NotFoundError )
	  
    Instrument i;
    double d,dd;
    // Should throw because sample hasn't been set
    TS_ASSERT_THROWS( i.detectorLocation(0,d,dd), Exception::NotFoundError )
  }

  void testGroupDetectors()
  {
    TS_ASSERT_THROWS_NOTHING( instrument.getDetector(10) )
    TS_ASSERT_THROWS_NOTHING( instrument.getDetector(11) )
	  
    std::vector<int> s;
    s.push_back(10);
    s.push_back(11);
    TS_ASSERT_THROWS_NOTHING( instrument.groupDetectors(s) )
    IDetector *d;
    TS_ASSERT_THROWS_NOTHING( d = instrument.getDetector(10) )
    TS_ASSERT( dynamic_cast<DetectorGroup*>(d) )
    TS_ASSERT_THROWS( instrument.getDetector(11), Exception::NotFoundError )
    TS_ASSERT_THROWS( instrument.groupDetectors(s), Exception::NotFoundError )
  }
	
  void testCasts()
  {
    Instrument *i = new Instrument;
    TS_ASSERT( dynamic_cast<CompAssembly*>(i) )
    TS_ASSERT( dynamic_cast<Component*>(i) )
    delete i;
  }
	
private:
  Instrument instrument;
  Detector *det, *det2, *det3;
};

#endif /*INSTRUMENTTEST_H_*/
