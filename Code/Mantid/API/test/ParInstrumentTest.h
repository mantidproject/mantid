#ifndef PARINSTRUMENTTEST_H_
#define PARINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Instrument.h"
#include "MantidAPI/ParInstrument.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/DetectorGroup.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class ParInstrumentTest : public CxxTest::TestSuite
{
public:
  ParInstrumentTest()
  {
    instrument.reset(new Instrument);
    ObjComponent *source = new ObjComponent("source");
    source->setPos(0.0,0.0,-10.0);
    instrument->markAsSource(source);
    ObjComponent *sample = new ObjComponent("sample");
    instrument->markAsSamplePos(sample);
    det = boost::shared_ptr<Detector>(new Detector("det",0));
    det->setID(1);
    det->setPos(1.0,0.0,0.0);
    instrument->markAsDetector(det.get());
    det2 = boost::shared_ptr<Detector>(new Detector("det",0));
    det2->setID(10);
    instrument->markAsDetector(det2.get());
    det3 = boost::shared_ptr<Detector>(new Detector("det",0));
    det3->setID(11);
    instrument->markAsDetector(det3.get());
    pmap.reset(new Mantid::Geometry::ParameterMap);
  }

  ~ParInstrumentTest()
  {
    //delete det, det2, det3;
    //delete instrument->getSample();
  }

  void testType()
  {
    ParInstrument pinstrument(instrument,pmap);
    TS_ASSERT_EQUALS( pinstrument.type(), "ParInstrument" )
  }

  void testDetector()
  {
    ParInstrument pinstrument(instrument,pmap);
    TS_ASSERT_THROWS( pinstrument.getDetector(0), Exception::NotFoundError )
        TS_ASSERT_EQUALS( pinstrument.getDetector(1)->getID(), det->getID() )
    TS_ASSERT_THROWS( pinstrument.getDetector(2), Exception::NotFoundError )
    Detector *d = new Detector("det",0);
    d->setID(2);
    TS_ASSERT_THROWS_NOTHING( instrument->markAsDetector(d) )
        TS_ASSERT_EQUALS( pinstrument.getDetector(2)->getID(), d->getID() )
    delete d;
  }

  void testDetectorTwoTheta()
  {
    ParInstrument pinstrument(instrument,pmap);
    TS_ASSERT_EQUALS( pinstrument.detectorTwoTheta(det), M_PI/2.0 )
  }

  void testCasts()
  {
    ParInstrument *pi = new ParInstrument(instrument,pmap);
    TS_ASSERT( dynamic_cast<ICompAssembly*>(pi) )
    TS_ASSERT( dynamic_cast<IComponent*>(pi) )
    delete pi;
  }

private:
  boost::shared_ptr<Instrument> instrument;
  boost::shared_ptr<Mantid::Geometry::ParameterMap> pmap;
  boost::shared_ptr<Detector> det, det2, det3;
};

#endif /*INSTRUMENTTEST_H_*/
