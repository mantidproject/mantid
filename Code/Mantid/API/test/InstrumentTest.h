#ifndef INSTRUMENTTEST_H_
#define INSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class InstrumentTest : public CxxTest::TestSuite
{
public:
  InstrumentTest()
  {
    ObjComponent *source = new ObjComponent("source");
    source->setPos(0.0,0.0,-10.0);
    instrument.markAsSource(source);
    ObjComponent *sample = new ObjComponent("sample");
    instrument.markAsSamplePos(sample);
    det = boost::shared_ptr<Detector>(new Detector("det1",0));
    det->setID(1);
    det->setPos(1.0,0.0,0.0);
    instrument.markAsDetector(det.get());
    det2 = boost::shared_ptr<Detector>(new Detector("det2",0));
    det2->setID(10);
    instrument.markAsDetector(det2.get());
    det3 = boost::shared_ptr<Detector>(new Detector("det3",0));
    det3->setID(11);
    instrument.markAsDetector(det3.get());
  }

  ~InstrumentTest()
  {
    //delete det, det2, det3;
    //delete instrument.getSample();
  }

  void testType()
  {
    TS_ASSERT_EQUALS( instrument.type(), "Instrument" );
  }

  void testConstructor()
  {
    Instrument i;
    TS_ASSERT( ! i.getSource() );
    TS_ASSERT( ! i.getSample() );

    Instrument ii("anInstrument");
    TS_ASSERT( ! ii.getSource() );
    TS_ASSERT( ! ii.getSample() );
    TS_ASSERT_EQUALS( ii.getName(), "anInstrument" );
  }

  void testSource()
  {
    Instrument i;
    TS_ASSERT( ! i.getSource() );
    ObjComponent *s = new ObjComponent("source");
    TS_ASSERT_THROWS_NOTHING( i.markAsSource(s) );
    TS_ASSERT_EQUALS( i.getSource().get(), s );
    ObjComponent *ss = new ObjComponent("source2");
    TS_ASSERT_THROWS_NOTHING( i.markAsSource(ss) );
    TS_ASSERT_EQUALS( i.getSource().get(), s );
    delete s;
    delete ss;
  }

  void testSamplePos()
  {
    Instrument i;
    TS_ASSERT( ! i.getSample() );
    ObjComponent *s = new ObjComponent("sample");
    TS_ASSERT_THROWS_NOTHING( i.markAsSamplePos(s) );
    TS_ASSERT_EQUALS( i.getSample().get(), s );
    ObjComponent *ss = new ObjComponent("sample2");
    TS_ASSERT_THROWS_NOTHING( i.markAsSamplePos(ss) );
    TS_ASSERT_EQUALS( i.getSample().get(), s );
    delete s;
    delete ss;
  }

  void testBeamDirection()
  {
    TS_ASSERT_EQUALS( instrument.getBeamDirection(), V3D(0,0,1) );
  }

  void testDetector()
  {
    TS_ASSERT_THROWS( instrument.getDetector(0), Exception::NotFoundError );
    TS_ASSERT_EQUALS( instrument.getDetector(1), det );
    TS_ASSERT_THROWS( instrument.getDetector(2), Exception::NotFoundError );
    Detector *d = new Detector("det",0);
    d->setID(2);
    TS_ASSERT_THROWS_NOTHING( instrument.markAsDetector(d) );
    TS_ASSERT_EQUALS( instrument.getDetector(2).get(), d );
    delete d;
  }

  void testCasts()
  {
    Instrument *i = new Instrument;
    TS_ASSERT( dynamic_cast<CompAssembly*>(i) );
    TS_ASSERT( dynamic_cast<Component*>(i) );
    delete i;
  }

  void testIDs()
  {
      ComponentID id1 = det->getComponentID();
      TS_ASSERT_EQUALS(det->getName(), instrument.getComponentByID(id1)->getName() );

      ComponentID id2 = det2->getComponentID();
      TS_ASSERT_EQUALS(det2->getName(), instrument.getComponentByID(id2)->getName() );

      ComponentID id3 = det3->getComponentID();
      TS_ASSERT_EQUALS(det3->getName(), instrument.getComponentByID(id3)->getName() );

  }

  void testGetByName()
  {
    Instrument *i = new Instrument;
    i->setName("TestInstrument");
    
    CompAssembly *bank = new CompAssembly("bank");
    bank->setPos(1.,0,1.);
    Quat q(0.9,0,0,0.2);
    q.normalize();
    bank->setRot(q);
    i->add(bank);

    Detector *det = new Detector("det1",0);
    det->setID(1);
    det->setPos(1.0,0.0,0.0);
    bank->add(det);
    i->markAsDetector(det);

    // Instrument name
    TS_ASSERT( i->getComponentByName("TestInstrument").get() );
    // Bank
    TS_ASSERT( i->getComponentByName("bank").get() );
    //Det 1
    TS_ASSERT( i->getComponentByName("det1").get() );

    delete i;
  }

private:
  Instrument instrument;
  boost::shared_ptr<Detector> det, det2, det3;
};

#endif /*INSTRUMENTTEST_H_*/
