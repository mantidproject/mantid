#ifndef MANTID_GEOMETRY_PARCOMPONENTFACTORYTEST_H_
#define MANTID_GEOMETRY_PARCOMPONENTFACTORYTEST_H_

#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;

class ParComponentFactoryTest : public CxxTest::TestSuite {
public:
  void test_createDetector() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);
    IDetector_const_sptr idet = inst->getDetector(1);
    const Detector *det =
        boost::dynamic_pointer_cast<const Detector>(idet).get();
    ParameterMap *map = new ParameterMap();

    boost::shared_ptr<IDetector> pdet;
    TS_ASSERT_THROWS_NOTHING(pdet =
                                 ParComponentFactory::createDetector(det, map));
    TS_ASSERT(pdet);
    delete map;
  }

  void test_createInstrument() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);
    ParameterMap_sptr map(new ParameterMap());

    Instrument_sptr pinst;
    TS_ASSERT_THROWS_NOTHING(
        pinst = ParComponentFactory::createInstrument(
            boost::dynamic_pointer_cast<Instrument>(inst), map));
    TS_ASSERT(pinst);
  }
};

#endif /* MANTID_GEOMETRY_PARCOMPONENTFACTORYTEST_H_ */
