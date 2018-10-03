#ifndef EQSANSUNFOLDFRAMETEST_H_
#define EQSANSUNFOLDFRAMETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/EQSANSUnfoldFrame.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

// Helper function
EQSANSTransWBands getBands(const std::vector<double> &limits) {
  EQSANSTransWBands wg;
  size_t i = 0;
  do {
    wg.m_bands.emplace_back(EQSANSWBand(limits[i], limits[i + 1]));
    i += 2;
  } while (i < limits.size());
  return wg;
}

class EQSANSWBandTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EQSANSWBandTest *createSuite() { return new EQSANSWBandTest(); }
  static void destroySuite(EQSANSWBandTest *suite) { delete suite; }

  void testConstructor() {
    TS_ASSERT_THROWS_NOTHING(EQSANSWBand());
    TS_ASSERT_THROWS_NOTHING(EQSANSWBand(0.0, 0.0));
    TS_ASSERT_THROWS(EQSANSWBand(-1.0, 1.0), std::domain_error);
    TS_ASSERT_THROWS(EQSANSWBand(1.0, -1.0), std::domain_error);
    TS_ASSERT_THROWS(EQSANSWBand(2.0, 1.0), std::range_error);
  }

  void testWidth() { TS_ASSERT_EQUALS(EQSANSWBand(0.0, 1.5).width(), 1.5); }

  void testIntersect() {
    EQSANSWBand b = EQSANSWBand(1.0, 2.0);
    TS_ASSERT_EQUALS(b.intersect(EQSANSWBand(0.0, 0.5)), EQSANSWBand(0.0, 0.0));
    TS_ASSERT_EQUALS(b.intersect(EQSANSWBand(0.0, 1.0)), EQSANSWBand(0.0, 0.0));
    TS_ASSERT_EQUALS(b.intersect(EQSANSWBand(0.0, 1.5)), EQSANSWBand(1.0, 1.5));
    TS_ASSERT_EQUALS(b.intersect(EQSANSWBand(0.0, 2.0)), b);
    TS_ASSERT_EQUALS(b.intersect(EQSANSWBand(2.0, 3.0)), EQSANSWBand(0.0, 0.0));
    TS_ASSERT_EQUALS(b.intersect(EQSANSWBand(2.5, 3.0)), EQSANSWBand(0.0, 0.0));
  }
};

class EQSANSTransWBandsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EQSANSTransWBandsTest *createSuite() {
    return new EQSANSTransWBandsTest();
  }
  static void destroySuite(EQSANSTransWBandsTest *suite) { delete suite; }

  void testConstructor() {
    TS_ASSERT_EQUALS(EQSANSTransWBands().m_bands.size(), 0);
  }

  void testInsersect() {
    std::vector<double> l1{0, 1, 2, 3, 4, 5}; // bands (0,1),(2,3),(4,5)
    EQSANSTransWBands b1 = getBands(l1);
    // intersection with a single band returns a single band
    EQSANSTransWBands b3 = b1.intersect(EQSANSWBand(2.5, 3.5));
    TS_ASSERT_EQUALS(b3.m_bands[0], EQSANSWBand(2.5, 3.0));
    // intersection with a single band returns no bands
    b3 = b1.intersect(EQSANSWBand(1.5, 2));
    TS_ASSERT_EQUALS(b3.size(), 0);
    b3 = b1.intersect(EQSANSWBand(5.1, 6.0));
    TS_ASSERT_EQUALS(b3.size(), 0);
    // intersection with a single band returns two bands
    b3 = b1.intersect(EQSANSWBand(0.5, 3.5));
    std::vector<double> l2{0.5, 1, 2, 3};
    TS_ASSERT_EQUALS(b3, getBands(l2));
    // intersection between two transmission band sets returns three bands
    std::vector<double> l3{0.5, 1.5, 1.6, 1.9, 2.5, 3.5, 4.5, 5.5, 6.0, 7.0};
    b3 = b1.intersect(getBands(l3));
    std::vector<double> l4{0.5, 1, 2.5, 3, 4.5, 5};
    TS_ASSERT_EQUALS(b3, getBands(l4));
  }
};

class EQSANSDiskChopperTest : public CxxTest::TestSuite {
private:
  EQSANSDiskChopper m_d;
  MatrixWorkspace_sptr m_ews;
  void setChopper() {
    m_d.m_index = 0;      // chopper index
    m_d.m_speed = 60;     // 60Hz;
    m_d.m_aperture = 36;  // 36 degrees
    m_d.m_phase = 420.0;  // micro-seconds
    m_d.m_location = 4.5; // meters
  }
  void createWorkspace() {
    m_ews = WorkspaceCreationHelper::createEventWorkspace();
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*m_ews, false, false,
                                                           "");
    // Set chopper speed
    auto timeStamp = Types::Core::DateAndTime("2019-09-26T16:40:10");
    auto speedLog = new TimeSeriesProperty<double>("Speed1");
    speedLog->addValue(timeStamp, 30.0);
    m_ews->mutableRun().addLogData(speedLog);
    // Set chopper phase
    auto phaseLog = new TimeSeriesProperty<double>("Phase1");
    phaseLog->addValue(timeStamp, 240.0);
    m_ews->mutableRun().addLogData(phaseLog);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EQSANSDiskChopperTest *createSuite() {
    EQSANSDiskChopperTest *suite = new EQSANSDiskChopperTest();
    suite->setChopper();
    suite->createWorkspace();
    return suite;
  }
  static void destroySuite(EQSANSDiskChopperTest *suite) { delete suite; }

  void testConstructor() { TS_ASSERT_THROWS_NOTHING(EQSANSDiskChopper()); }

  void testPeriod() { TS_ASSERT_DELTA(m_d.period(), 16666.6, 1.0); }

  void testTransmissionDuration() {
    TS_ASSERT_DELTA(m_d.transmissionDuration(), 1666.6, 1);
  }

  void testOpeningPhase() { TS_ASSERT_DELTA(m_d.openingPhase(), -413.3, 1.0); }

  void testClosingPhase() { TS_ASSERT_DELTA(m_d.closingPhase(), 1253.3, 1.0); }

  void testRewind() {
    TS_ASSERT_DELTA(m_d.rewind(), -413.3, 1.0);
    m_d.m_phase += m_d.period(); // chopper "too far ahead"
    TS_ASSERT_DELTA(m_d.rewind(), -413.3, 1.0);
    m_d.m_phase -= 2 * m_d.period(); // chopper "too far back"
    TS_ASSERT_DELTA(m_d.rewind(), -413.3, 1.0);
  }

  void testTofToWavelength() {
    double tof(25000.0);
    TS_ASSERT_DELTA(m_d.tofToWavelength(tof, 0, false), 21.9, 0.1);
    // additional delay
    TS_ASSERT_DELTA(m_d.tofToWavelength(tof, m_d.period(), false), 36.6, 0.1);
    // additional delay and prompt pulse correction
    TS_ASSERT_DELTA(m_d.tofToWavelength(tof, m_d.period(), true), 35.9, 0.1);
  }

  void testSetSpeed() {
    auto oldSpeed = m_d.m_speed;
    m_d.setSpeed(m_ews->run());
    TS_ASSERT_EQUALS(m_d.m_speed, 30.0);
    m_d.m_speed = oldSpeed; // reinstate old speed
  }

  void testSetPhase() {
    auto oldPhase = m_d.m_phase;
    m_d.setPhase(m_ews->run(), 39.0);
    TS_ASSERT_EQUALS(m_d.m_phase, 201.0);
    m_d.m_phase = oldPhase; // reinstate old speed
  }

  void testTransmissionBands() {
    double maxWl = 35.0;
    // No delay, no prompt pulse correction
    auto wg = m_d.transmissionBands(maxWl, 0.0, false);
    std::vector<double> l{0.0, 1.10183, 14.2886, 15.7538, 28.9406, 30.4058};
    auto vg = getBands(l);
    for (size_t i = 0; i < wg.size(); i++) {
      TS_ASSERT_DELTA(wg.m_bands[i].m_min, vg.m_bands[i].m_min, 0.0001);
      TS_ASSERT_DELTA(wg.m_bands[i].m_max, vg.m_bands[i].m_max, 0.0001);
    }
    // No delay, prompt pulse correction
    wg = m_d.transmissionBands(maxWl, 0.0, true);
    std::vector<double> l2{0.0, 1.10183, 14.0417, 15.7538, 28.4405, 30.4058};
    vg = getBands(l2);
    for (size_t i = 0; i < wg.size(); i++) {
      TS_ASSERT_DELTA(wg.m_bands[i].m_min, vg.m_bands[i].m_min, 0.0001);
      TS_ASSERT_DELTA(wg.m_bands[i].m_max, vg.m_bands[i].m_max, 0.0001);
    }
    // delay and prompt pulse correction
    wg = m_d.transmissionBands(maxWl, m_d.period(), true);
    std::vector<double> l3{14.0417, 15.7538, 28.4405, 30.4058};
    vg = getBands(l3);
    for (size_t i = 0; i < wg.size(); i++) {
      TS_ASSERT_DELTA(wg.m_bands[i].m_min, vg.m_bands[i].m_min, 0.0001);
      TS_ASSERT_DELTA(wg.m_bands[i].m_max, vg.m_bands[i].m_max, 0.0001);
    }
  }
};

class EQSANSUnfoldFrameTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EQSANSUnfoldFrameTest *createSuite() {
    return new EQSANSUnfoldFrameTest();
  }
  static void destroySuite(EQSANSUnfoldFrameTest *suite) {
    suite->cleanup();
    delete suite;
  }

  // constructor
  EQSANSUnfoldFrameTest() { createInputWorkspace(); }

  void testConstructor() { TS_ASSERT_THROWS_NOTHING(EQSANSUnfoldFrame()); }

  void testInit() {
    EQSANSUnfoldFrame alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testGetPulseFrequency() {
    EQSANSUnfoldFrame alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", m_ews->getName());
    TS_ASSERT_DELTA(60.0, alg.getPulseFrequency(), 0.1);
  }

  void testSetPulsePeriod() {
    EQSANSUnfoldFrame alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", m_ews->getName());
    alg.setPulsePeriod();
    TS_ASSERT_DELTA(16666.7, alg.getPulsePeriod(), 1.0);
  }

  void testSetFrameSkippingMode() {
    EQSANSUnfoldFrame alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", m_ews->getName());
    TS_ASSERT_EQUALS(alg.getFrameSkippingMode(), false);
    alg.setFrameSkippingMode();
    TS_ASSERT_EQUALS(alg.getFrameSkippingMode(), true);
  }

  void testSetFrameWidth() {
    EQSANSUnfoldFrame alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", m_ews->getName());
    alg.setPulsePeriod();
    alg.setFrameSkippingMode();
    TS_ASSERT_EQUALS(alg.getFrameWidth(), 0.0);
    alg.setFrameWidth();
    TS_ASSERT_DELTA(alg.getFrameWidth(), 33333.3, 1.0);
  }

  void testInitializeChoppers() {
    EQSANSUnfoldFrame alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", m_ews->getName());
    alg.setPulsePeriod();
    alg.setFrameSkippingMode();
    alg.setFrameWidth();
    TS_ASSERT_EQUALS(alg.getFrameSkippingMode(), true);
  }

private:
  EventWorkspace_sptr m_ews;
  void createInputWorkspace() {
    m_ews = WorkspaceCreationHelper::createEventWorkspace();
    m_ews->getAxis(0)->setUnit("TOF");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*m_ews, false, false,
                                                           "");
    auto timeStamp = Types::Core::DateAndTime("2019-09-26T16:40:10");
    // Set frequency
    auto frequencyLog = new TimeSeriesProperty<double>("frequency");
    frequencyLog->addValue(timeStamp, 60.0);
    m_ews->mutableRun().addLogData(frequencyLog);
    // set chopper speed
    auto speedLog = new TimeSeriesProperty<double>("Speed1");
    speedLog->addValue(timeStamp, 30.0);
    m_ews->mutableRun().addLogData(speedLog);
    // add workspace to the analysis data service
    API::AnalysisDataService::Instance().addOrReplace("iws", m_ews);
  }

  void cleanup() {
    // Remove workspace from the data service
    API::AnalysisDataService::Instance().remove(m_ews->getName());
  }
};

#endif /*EQSANSUNFOLDFRAMETEST_H_*/