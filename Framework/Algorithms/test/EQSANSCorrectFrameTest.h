#ifndef MANTID_ALGORITHMS_EQSANSCORRECTFRAMETEST_H_
#define MANTID_ALGORITHMS_EQSANSCORRECTFRAMETEST_H_

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/EQSANSCorrectFrame.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Types::Event::TofEvent;

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
  EQSANSCorrectFrameTest()
      : m_pulseWidth(1.E6 / 60), m_frameWidth(2.E6 / 60), m_minTOF(4.1E6 / 60),
        m_frameSkipping(true), m_bankSize(2) {

    // bank contains m_bankSize^2 pixels
    const int numBanks = 1;
    m_ews = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
        numBanks, m_bankSize);
    m_ews->getAxis(0)->setUnit("TOF");

    // insert one event in each pixel
    std::vector<double> tofs = {0.05, 0.15, 1.05, 1.15};
    const double pulseWidth(m_pulseWidth);
    std::transform(tofs.begin(), tofs.end(), tofs.begin(),
                   [&pulseWidth](double tof) { return tof * pulseWidth; });
    const int numPixels(m_bankSize * m_bankSize);
    for (int i = 0; i < numPixels; i++) {
      EventList &evlist = m_ews->getSpectrum(i);
      evlist.addEventQuickly(TofEvent(tofs[i]));
    }
  }

  void testInit() {
    Mantid::Algorithms::EQSANSCorrectFrame alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    Mantid::Algorithms::EQSANSCorrectFrame alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", m_ews);
    alg.setProperty("MinTOF", m_minTOF);
    alg.setProperty("FrameWidth", m_frameWidth);
    alg.setProperty("FrameSkipping", m_frameSkipping);
    alg.execute();

    std::vector<double> tofs = {7.05, 4.15, 5.05, 6.15};
    const double pulseWidth(m_pulseWidth);
    std::transform(tofs.begin(), tofs.end(), tofs.begin(),
                   [&pulseWidth](double tof) { return tof * pulseWidth; });
    const int numPixels(m_bankSize * m_bankSize);
    for (int i = 0; i < numPixels; i++) {
      std::vector<TofEvent> &events = m_ews->getSpectrum(i).getEvents();
      TS_ASSERT_EQUALS(events.size(), 1);
      TS_ASSERT_DELTA(events[0].tof(), tofs[i], 1.0E-03 * m_pulseWidth);
    }
  }

private:
  EventWorkspace_sptr m_ews;
  double m_pulseWidth;
  double m_frameWidth;
  double m_minTOF;
  bool m_frameSkipping;
  int m_bankSize;
  void cleanup() { return; }
};

#endif // MANTID_ALGORITHMS_EQSANSCORRECTFRAMETEST_H_
