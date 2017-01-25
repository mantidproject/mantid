#ifndef PARINSTRUMENTTEST_H_
#define PARINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/Exception.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ParInstrumentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParInstrumentTest *createSuite() { return new ParInstrumentTest(); }
  static void destroySuite(ParInstrumentTest *suite) { delete suite; }

  ParInstrumentTest() {
    instrument.reset(new Instrument);
    ObjComponent *source = new ObjComponent("source");
    source->setPos(0.0, 0.0, -10.0);
    instrument->markAsSource(source);
    ObjComponent *sample = new ObjComponent("sample");
    instrument->markAsSamplePos(sample);
    det = boost::make_shared<Detector>("det1", 1, nullptr);
    det->setPos(1.0, 0.0, 0.0);
    instrument->markAsDetector(det.get());
    det2 = boost::make_shared<Detector>("det2", 10, nullptr);
    instrument->markAsDetector(det2.get());
    det3 = boost::make_shared<Detector>("det3", 11, nullptr);
    instrument->markAsDetector(det3.get());
    instrument->markAsMonitor(det3.get());

    pmap.reset(new ParameterMap);
    delete source;
    delete sample;
  }

  void test_Constructor_Throws_With_Invalid_Pointers() {
    TS_ASSERT_THROWS(Instrument(boost::shared_ptr<Instrument>(),
                                boost::shared_ptr<ParameterMap>()),
                     std::invalid_argument);
    // boost::shared_ptr<Instrument> instr = boost::make_shared<Instrument>();
    // TS_ASSERT_THROWS(Instrument(instr,boost::shared_ptr<ParameterMap>()),std::invalid_argument);
    boost::shared_ptr<ParameterMap> paramMap =
        boost::make_shared<ParameterMap>();
    TS_ASSERT_THROWS(Instrument(boost::shared_ptr<Instrument>(), paramMap),
                     std::invalid_argument);
  }

  void test_getMonitors() {
    // testDetector injects a pointer into `instrument` that is deleted later.
    // Instrument::getMonitors will then cause a segmentation fault, so this
    // test must run before we have invalid pointers.
    Instrument pinstrument(instrument, pmap);
    TS_ASSERT_EQUALS(pinstrument.getMonitors().size(), 1)
  }

  void testDetector() {
    Instrument pinstrument(instrument, pmap);
    TS_ASSERT_THROWS(pinstrument.getDetector(0), Exception::NotFoundError);
    TS_ASSERT_EQUALS(pinstrument.getDetector(1)->getID(), det->getID());
    TS_ASSERT_THROWS(pinstrument.getDetector(2), Exception::NotFoundError);

    TS_ASSERT(NULL == pinstrument.getBaseDetector(0));
    Detector *d = new Detector("det", 2, 0);
    TS_ASSERT_THROWS_NOTHING(instrument->markAsDetector(d));
    TS_ASSERT_EQUALS(pinstrument.getDetector(2)->getID(), d->getID());

    TS_ASSERT_EQUALS(pinstrument.getBaseDetector(2)->getID(), d->getID());
    delete d;
  }

  void testCasts() {
    Instrument *pi = new Instrument(instrument, pmap);
    TS_ASSERT(dynamic_cast<ICompAssembly *>(pi));
    TS_ASSERT(dynamic_cast<Component *>(pi));
    delete pi;
  }

  void testIDs() {
    ComponentID id1 = det->getComponentID();
    TS_ASSERT_EQUALS(det->getName(),
                     instrument->getComponentByID(id1)->getName());

    ComponentID id2 = det2->getComponentID();
    TS_ASSERT_EQUALS(det2->getName(),
                     instrument->getComponentByID(id2)->getName());

    ComponentID id3 = det3->getComponentID();
    TS_ASSERT_EQUALS(det3->getName(),
                     instrument->getComponentByID(id3)->getName());
  }

private:
  boost::shared_ptr<Instrument> instrument;
  Mantid::Geometry::ParameterMap_sptr pmap;
  boost::shared_ptr<Detector> det, det2, det3;
};

#endif /*INSTRUMENTTEST_H_*/
