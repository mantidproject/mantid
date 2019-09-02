// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    instrument->add(source);
    instrument->markAsSource(source);
    ObjComponent *sample = new ObjComponent("sample");
    instrument->add(sample);
    instrument->markAsSamplePos(sample);
    det = new Detector("det1", 1, nullptr);
    det->setPos(1.0, 0.0, 0.0);
    instrument->add(det);
    instrument->markAsDetector(det);
    det2 = new Detector("det2", 10, nullptr);
    instrument->add(det2);
    instrument->markAsDetector(det2);
    det3 = new Detector("det3", 11, nullptr);
    instrument->add(det3);
    instrument->markAsMonitor(det3);

    pmap.reset(new ParameterMap);
  }

  void test_Constructor_Throws_With_Invalid_Pointers() {
    TS_ASSERT_THROWS(Instrument(boost::shared_ptr<Instrument>(),
                                boost::shared_ptr<ParameterMap>()),
                     const std::invalid_argument &);
    // boost::shared_ptr<Instrument> instr = boost::make_shared<Instrument>();
    // TS_ASSERT_THROWS(Instrument(instr,boost::shared_ptr<ParameterMap>()),const
    // std::invalid_argument &);
    boost::shared_ptr<ParameterMap> paramMap =
        boost::make_shared<ParameterMap>();
    TS_ASSERT_THROWS(Instrument(boost::shared_ptr<Instrument>(), paramMap),
                     const std::invalid_argument &);
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
    TS_ASSERT_THROWS(pinstrument.getDetector(0),
                     const Exception::NotFoundError &);
    TS_ASSERT_EQUALS(pinstrument.getDetector(1)->getID(), det->getID());
    TS_ASSERT_THROWS(pinstrument.getDetector(2),
                     const Exception::NotFoundError &);

    TS_ASSERT(nullptr == pinstrument.getBaseDetector(0));
    Detector *d = new Detector("det", 2, nullptr);
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
  Detector *det;
  Detector *det2;
  Detector *det3;
};

#endif /*INSTRUMENTTEST_H_*/
