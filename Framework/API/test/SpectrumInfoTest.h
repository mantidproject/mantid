#ifndef MANTID_API_SPECTRUMINFOTEST_H_
#define MANTID_API_SPECTRUMINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/make_unique.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;

class SpectrumInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumInfoTest *createSuite() { return new SpectrumInfoTest(); }
  static void destroySuite(SpectrumInfoTest *suite) { delete suite; }

  SpectrumInfoTest() : m_workspace(nullptr) {
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    bool includeMonitors = true;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);

    std::set<int64_t> toMask{0, 3};
    ParameterMap &pmap = m_workspace.instrumentParameters();
    for (size_t i = 0; i < m_workspace.getNumberHistograms(); ++i) {
      if (toMask.find(i) != toMask.end()) {
        IDetector_const_sptr det = m_workspace.getDetector(i);
        pmap.addBool(det.get(), "masked", true);
      }
    }

    m_workspaceNoInstrument.init(numberOfHistograms, numberOfBins,
                                 numberOfBins - 1);
  }

  void test_constructor() {
    auto ws = makeWorkspace(3);
    TS_ASSERT_THROWS_NOTHING(SpectrumInfo(*ws));
  }

  void test_l1() { TS_ASSERT_EQUALS(m_workspace.spectrumInfo().l1(), 20.0); }

  void test_l1_no_instrument() {
    TS_ASSERT_THROWS(m_workspaceNoInstrument.spectrumInfo().l1(),
                     std::runtime_error);
  }

  void test_isMonitor() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(0), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(4), true);
  }

  void test_isMasked() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(4), false);
  }

  void test_isMasked_unthreaded() {
    size_t count = 1000;
    auto ws = makeWorkspace(count);
    SpectrumInfo info(*ws);
    for (size_t i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(i), i % 2 == 0);
  }

  void test_isMasked_threaded() {
    size_t count = 1000;
    auto ws = makeWorkspace(count);
    SpectrumInfo info(*ws);
    // This attempts to test threading, but probably it is not really exercising
    // much.
    PARALLEL_FOR1(ws)
    for (size_t i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(i), i % 2 == 0);
  }

  void test_l2() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    double x2 = 5.0 * 5.0;
    double y2 = 2.0 * 2.0 * 0.05 * 0.05;
    TS_ASSERT_EQUALS(spectrumInfo.l2(0), sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(spectrumInfo.l2(1), sqrt(x2 + 0 * 0 * y2));
    TS_ASSERT_EQUALS(spectrumInfo.l2(2), sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(spectrumInfo.l2(3), -9.0);
    TS_ASSERT_EQUALS(spectrumInfo.l2(4), -2.0);
  }

  void test_twoTheta() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), 0.0199973, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(2), 0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorTwoTheta(), which might be
  // removed at some point.
  void test_twoThetaLegacy() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto det = m_workspace.getDetector(2);
    TS_ASSERT_EQUALS(spectrumInfo.twoTheta(2),
                     m_workspace.detectorTwoTheta(*det));
  }

  void test_signedTwoTheta() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(0), -0.0199973, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(2), 0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorSignedTwoTheta(), which might
  // be removed at some point.
  void test_signedTwoThetaLegacy() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto det = m_workspace.getDetector(2);
    TS_ASSERT_EQUALS(spectrumInfo.signedTwoTheta(2),
                     m_workspace.detectorSignedTwoTheta(*det));
  }

private:
  WorkspaceTester m_workspace;
  WorkspaceTester m_workspaceNoInstrument;

  std::unique_ptr<MatrixWorkspace> makeWorkspace(size_t numSpectra) {
    auto ws = Kernel::make_unique<WorkspaceTester>();
    ws->initialize(numSpectra, 1, 1);
    auto inst = boost::make_shared<Instrument>("TestInstrument");
    ws->setInstrument(inst);
    auto &pmap = ws->instrumentParameters();
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      auto det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
      inst->add(det);
      inst->markAsDetector(det);
      ws->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
      if (i % 2 == 0)
        pmap.addBool(det->getComponentID(), "masked", true);
    }
    return std::move(ws);
  }
};

class SpectrumInfoTestPerformance : public CxxTest::TestSuite {
public:
  static SpectrumInfoTestPerformance *createSuite() {
    return new SpectrumInfoTestPerformance();
  }
  static void destroySuite(SpectrumInfoTestPerformance *suite) { delete suite; }

  SpectrumInfoTestPerformance() : m_workspace(nullptr) {
    size_t numberOfHistograms = 10000;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);
  }

  void test_typical() {
    // Typically:
    // - workspace with > 10k histograms
    // - need L1, L2, and 2-theta
    // Note that the instrument in this case is extremely simple, with few
    // detectors and no parameters, so the actual performance will be worse.
    double result = 0.0;
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    for (size_t i = 0; i < 10000; ++i) {
      result += spectrumInfo.l1();
      result += spectrumInfo.l2(i);
      result += spectrumInfo.twoTheta(i);
    }
    // We are computing and using the result to fool the optimizer.
    TS_ASSERT_DELTA(result, 5214709.740869, 1e-6);
  }

private:
  WorkspaceTester m_workspace;
};

#endif /* MANTID_API_SPECTRUMINFOTEST_H_ */
