#ifndef EQSANSCORRECTFRAMETEST_H_
#define EQSANSCORRECTFRAMETEST_H_

#include "MantidAlgorithms/EQSANSCorrectFrame.h"
#include "MantidKernel/Unit.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Types::Event::TofEvent;
using serveEW = WorkspaceCreationHelper::createEventWorkspace;


class EQSANSCorrectFrameTest : public CxxTest::TestSuite {
public:

// boilerplate methods prevent the suite being created statically
// This means the constructor isn't called when running other tests
static EQSANSCorrectFrameTest *createSuite() {
  return new EQSANSCorrectFrameTest();
}

static void destroySuite(EQSANSCorrectFrameTest *suite) {
  suite->cleanup();
  delete suite;
}

// constructor
EQSANSCorrectFrameTest() : m_pulseWidth(1.E6/60), m_frameWidth(2.E6/60),
m_minTOF(4.1E6/60), m_frameSkipping(true) {

  // Create event workspace with four pixels
  int numPixels = 4;
  int numBins = 1;
  int numEvents = 0;
  double x0 = 0.0;
  double binDelta = 1.0;  // microseconds
  int eventPattern = 1;
  m_ews = serveEW(numPixels, numBins, numEvents, x0, binDelta, eventPattern);
  m_ews->getAxis(0)->setUnit("TOF");

  // insert one event in each pixel
  std::vector<double> tofs = {0.05, 0.15, 1.05, 1.15};
  std::transform(tofs.begin(), tofs.end(), tofs.begin(),
                 [](double tof) { return tof * m_pulseWidth});
  for (int i=0; i < numPixels; i++) {
    EventList &evlist = m_ews->getSpectrum(ihist);
    evlist.addEventQuickly(TofEvent(tofs[i]));
  }
}

void testConstructor() { TS_ASSERT_THROWS_NOTHING(EQSANSCorrectFrameTest); }

void testInit() {
  EQSANSCorrectFrame alg;
  TS_ASSERT_THROWS_NOTHING(alg.initialize());
  TS_ASSERT(alg.isInitialized());
}

void testExec() {
  EQSANSCorrectFrame alg;
  alg.initialize();
  alg.setPropertyValue("InputWorkspace", m_ews->getName());
  alg.setProperty("MinTOF", m_minTOF);
  alg.setProperty("FrameWidth", m_frameWidth);
  alg.setProperty("FrameSkipping", m_frameSkipping);
  alg.execute();

  std::vector<double> tofs = {6.05, 4.15, 5.05, 6.15};
  std::transform(tofs.begin(), tofs.end(), tofs.begin(),
                 [](double tof) { return tof * m_pulseWidth});
  for (int i=0; i < numPixels; i++) {
    std::vector<TofEvent> &events = inputWS->getSpectrum(i).getEvents();
    TS_ASSERT_EQUALS(events.size(), 1);
    TS_ASSERT_DELTA(events[0], tofs[i], 1.0E-03 * m_pulseWidth);
  }
}


private:
  EventWorkspace_sptr m_ews;
  double m_pulseWidth;
  double m_frameWidth;
  double m_minTOF;
  bool m_frameSkipping;
  void cleanup() {
    return;
  }

};

#endif EQSANSCORRECTFRAMETEST_H_
