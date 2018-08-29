#ifndef POLDIDETECTORTEST_H
#define POLDIDETECTORTEST_H

#include "MantidAPI/TableRow.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiHeliumDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Poldi;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class TestablePoldiHeliumDetector : public ConfiguredHeliumDetector {
  friend class PoldiDetectorTest;
};

class PoldiDetectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiDetectorTest *createSuite() { return new PoldiDetectorTest(); }
  static void destroySuite(PoldiDetectorTest *suite) { delete suite; }

  void testDetectorInterface() {
    PoldiHeliumDetector *heliumDetector = new PoldiHeliumDetector();
    TS_ASSERT(heliumDetector);

    PoldiAbstractDetector *abstractDetector =
        static_cast<PoldiAbstractDetector *>(heliumDetector);
    TS_ASSERT(abstractDetector);

    PoldiHeliumDetector *reCastHeliumDetector =
        dynamic_cast<PoldiHeliumDetector *>(abstractDetector);
    TS_ASSERT(reCastHeliumDetector);

    delete heliumDetector;
  }

  void testConfiguration() {
    TestablePoldiHeliumDetector heliumDetector;
    heliumDetector.loadConfiguration(Instrument_const_sptr());

    TS_ASSERT_DELTA(heliumDetector.m_angularResolution, 0.0008333333333, 1e-6);
    TS_ASSERT_DELTA(heliumDetector.m_totalOpeningAngle, 0.3333333333333, 1e-6);
    TS_ASSERT_DELTA(heliumDetector.m_phiCenter, 1.260093451, 5e-7);
    TS_ASSERT_DELTA(heliumDetector.m_phiStart, 1.093426824, 5e-7);

    TS_ASSERT_EQUALS(heliumDetector.elementCount(), 400);
    TS_ASSERT_EQUALS(heliumDetector.centralElement(), 199);
  }

  void testPhi() {
    TestablePoldiHeliumDetector heliumDetector;

    TS_ASSERT_DELTA(heliumDetector.phi(199), 1.259676814, 5e-7);
  }

  void testTwoTheta() {
    ConfiguredHeliumDetector heliumDetector;

    TS_ASSERT_DELTA(heliumDetector.twoTheta(199), 1.577357650, 5e-7);
  }

  void testQLimits() {
    ConfiguredHeliumDetector heliumDetector;

    std::pair<double, double> qLimits = heliumDetector.qLimits(1.1, 5.0);

    TS_ASSERT_DELTA(qLimits.first, 1.549564, 1e-6);
    TS_ASSERT_DELTA(qLimits.second, 8.960878, 1e-6);
  }

  void testDistance() {
    ConfiguredHeliumDetector heliumDetector;

    TS_ASSERT_DELTA(heliumDetector.distanceFromSample(199), 1996.017578125,
                    1e-3);
  }

  void testAvailableElements() {
    ConfiguredHeliumDetector heliumDetector;

    std::vector<int> availableElements = heliumDetector.availableElements();

    TS_ASSERT_EQUALS(availableElements.size(), 400);
    TS_ASSERT_EQUALS(availableElements.front(), 0);
    TS_ASSERT_EQUALS(availableElements.back(), 399);
  }
};

#endif // POLDIDETECTORTEST_H
