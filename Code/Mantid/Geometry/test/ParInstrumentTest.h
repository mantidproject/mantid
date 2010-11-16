#ifndef PARINSTRUMENTTEST_H_
#define PARINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid::Kernel;
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
    det = boost::shared_ptr<Detector>(new Detector("det1",0));
    det->setID(1);
    det->setPos(1.0,0.0,0.0);
    instrument->markAsDetector(det.get());
    det2 = boost::shared_ptr<Detector>(new Detector("det2",0));
    det2->setID(10);
    instrument->markAsDetector(det2.get());
    det3 = boost::shared_ptr<Detector>(new Detector("det3",0));
    det3->setID(11);
    instrument->markAsDetector(det3.get());
  }

  void testType()
  {
    Instrument pinstrument(instrument,pmap);
    TS_ASSERT_EQUALS( pinstrument.type(), "Instrument" );
  }

  void testDetector()
  {
    Instrument pinstrument(instrument,pmap);
    TS_ASSERT_THROWS( pinstrument.getDetector(0), Exception::NotFoundError );
    TS_ASSERT_EQUALS( pinstrument.getDetector(1)->getID(), det->getID() );
    TS_ASSERT_THROWS( pinstrument.getDetector(2), Exception::NotFoundError );
    Detector *d = new Detector("det",0);
    d->setID(2);
    TS_ASSERT_THROWS_NOTHING( instrument->markAsDetector(d) );
    TS_ASSERT_EQUALS( pinstrument.getDetector(2)->getID(), d->getID() );
    delete d;
  }

  void testCasts()
  {
    Instrument *pi = new Instrument(instrument,pmap);
    TS_ASSERT( dynamic_cast<ICompAssembly*>(pi) );
    TS_ASSERT( dynamic_cast<Component*>(pi) );
    delete pi;
  }

  void testIDs()
  {
    ComponentID  id1 = det->getComponentID ();
    TS_ASSERT_EQUALS(det->getName(), instrument->getComponentByID(id1)->getName() );

    ComponentID  id2 = det2->getComponentID ();
    TS_ASSERT_EQUALS(det2->getName(), instrument->getComponentByID(id2)->getName() );

    ComponentID  id3 = det3->getComponentID ();
    TS_ASSERT_EQUALS(det3->getName(), instrument->getComponentByID(id3)->getName() );
  }

private:
  boost::shared_ptr<Instrument> instrument;
  Mantid::Kernel::cow_ptr<Mantid::Geometry::ParameterMap> pmap;
  boost::shared_ptr<Detector> det, det2, det3;
};

#endif /*INSTRUMENTTEST_H_*/
