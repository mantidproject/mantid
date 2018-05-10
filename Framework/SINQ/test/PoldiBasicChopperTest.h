#ifndef MANTID_SINQ_POLDIBASICCHOPPERTEST_H_
#define MANTID_SINQ_POLDIBASICCHOPPERTEST_H_

#include "MantidAPI/TableRow.h"
#include "MantidSINQ/PoldiUtilities/PoldiBasicChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Poldi;
using namespace Mantid::Geometry;

class TestablePoldiBasicChopper : public PoldiBasicChopper {
  friend class PoldiBasicChopperTest;

  void loadConfiguration(Instrument_const_sptr poldiInstrument) override {
    UNUSED_ARG(poldiInstrument);

    double rawSlitPositions[] = {0.000000, 0.162156, 0.250867, 0.3704,
                                 0.439811, 0.588455, 0.761389, 0.895667};
    std::vector<double> slitPositions(
        rawSlitPositions, rawSlitPositions + sizeof(rawSlitPositions) /
                                                 sizeof(rawSlitPositions[0]));

    initializeFixedParameters(slitPositions, 11800.0, 0.0005, -0.60);
    initializeVariableParameters(10000.0);
  }
};

class PoldiBasicChopperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiBasicChopperTest *createSuite() {
    return new PoldiBasicChopperTest();
  }
  static void destroySuite(PoldiBasicChopperTest *suite) { delete suite; }

  void testChopperInterface() {
    Mantid::Poldi::PoldiBasicChopper *basicChopper =
        new Mantid::Poldi::PoldiBasicChopper();
    TS_ASSERT(basicChopper);

    Mantid::Poldi::PoldiAbstractChopper *abstractChopper =
        static_cast<Mantid::Poldi::PoldiAbstractChopper *>(basicChopper);
    TS_ASSERT(abstractChopper);

    Mantid::Poldi::PoldiBasicChopper *reCastBasicChopper =
        dynamic_cast<Mantid::Poldi::PoldiBasicChopper *>(abstractChopper);
    TS_ASSERT(reCastBasicChopper);

    delete basicChopper;
  }

  void testConfigurationCorrectness() {
    TestablePoldiBasicChopper basicChopper;
    basicChopper.loadConfiguration(Instrument_const_sptr());

    std::vector<double> slitPositions = basicChopper.slitPositions();
    TS_ASSERT_EQUALS(slitPositions.size(), 8);
    TS_ASSERT_DELTA(slitPositions[0], 0.0, 1e-7);
    TS_ASSERT_DELTA(slitPositions[1], 0.162156, 1e-7);

    TS_ASSERT_DELTA(basicChopper.cycleTime(), 1500.0, 1e-7);
    TS_ASSERT_DELTA(basicChopper.distanceFromSample(), 11800.0, 1e-7);
    TS_ASSERT_DELTA(basicChopper.zeroOffset(), 0.15, 1e-7);

    std::vector<double> slitTimes = basicChopper.slitTimes();
    TS_ASSERT_EQUALS(slitTimes.size(), 8);
    TS_ASSERT_DELTA(slitTimes[0], 0.0, 1e-7);
    TS_ASSERT_DELTA(slitTimes[1], 243.234, 1e-3)
  }
};

#endif /* MANTID_SINQ_POLDIBASICCHOPPERTEST_H_ */
