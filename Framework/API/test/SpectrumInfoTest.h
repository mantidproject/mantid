#ifndef MANTID_API_SPECTRUMINFOTEST_H_
#define MANTID_API_SPECTRUMINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/DetectorInfo.h"
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

  SpectrumInfoTest()
      : m_workspace(makeDefaultWorkspace()), m_grouped(makeDefaultWorkspace()) {
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    m_workspaceNoInstrument.init(numberOfHistograms, numberOfBins + 1,
                                 numberOfBins);

    // Workspace has 5 detectors, 1 and 4 are masked, 4 and 5 are monitors.
    m_grouped.getSpectrum(0).setDetectorIDs({2, 3}); // no mask
    m_grouped.getSpectrum(1).setDetectorIDs({1, 2}); // partial mask
    m_grouped.getSpectrum(2).setDetectorIDs({1, 4}); // masked, partial monitor
    m_grouped.getSpectrum(3).setDetectorIDs({4, 5}); // full monitor
    m_grouped.getSpectrum(4).setDetectorIDs({1, 2, 3, 4, 5}); // everything
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(SpectrumInfo(*makeWorkspace(3)));
  }

  void test_sourcePosition() {
    TS_ASSERT_EQUALS(m_workspace.spectrumInfo().sourcePosition(),
                     V3D(0.0, 0.0, -20.0));
  }

  void test_samplePosition() {
    TS_ASSERT_EQUALS(m_workspace.spectrumInfo().samplePosition(),
                     V3D(0.0, 0.0, 0.0));
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

  void test_grouped_isMonitor() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    // This is adopting the old definition from DetectorGroup: Spectra with at
    // least one non-monitor detector are not monitors. Actually it might make
    // more sense to forbid such a grouping.
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(0), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(4), false);
  }

  void test_isMasked() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(4), false);
  }

  void test_grouped_isMasked() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(2), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(3), false);
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
    int count = 1000;
    auto ws = makeWorkspace(count);
    SpectrumInfo info(*ws);
    // This attempts to test threading, but probably it is not really exercising
    // much.
    PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
    for (int i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(static_cast<size_t>(i)), i % 2 == 0);
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

  void test_grouped_l2() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    double x2 = 5.0 * 5.0;
    double y2 = 2.0 * 2.0 * 0.05 * 0.05;
    TS_ASSERT_EQUALS(spectrumInfo.l2(0),
                     (sqrt(x2 + 0 * 0 * y2) + sqrt(x2 + 1 * 1 * y2)) / 2.0);
    TS_ASSERT_EQUALS(spectrumInfo.l2(1),
                     (sqrt(x2 + 0 * 0 * y2) + sqrt(x2 + 1 * 1 * y2)) / 2.0);
    // Other lengths are not sensible since the detectors include monitors
  }

  void test_twoTheta() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), 0.0199973, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(2), 0.0199973, 1e-6);
    // Monitors
    TS_ASSERT_THROWS(spectrumInfo.twoTheta(3), std::logic_error);
    TS_ASSERT_THROWS(spectrumInfo.twoTheta(4), std::logic_error);
  }

  void test_twoTheta_grouped() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    // Note that updating detector IDs like this is a trick that should not be
    // used in actual code. The correct update happens only because the detector
    // at index 0 is currently not buffered (the previous test last used the
    // detector at index 2).
    m_workspace.getSpectrum(0).setDetectorIDs({1, 3});
    // det 1 at V3D(0.0, -0.1, 5.0)
    // det 3 at V3D(0.0,  0.1, 5.0)
    // Average *scattering* angle is *not* 0.0!
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), 0.0199973, 1e-6);
    m_workspace.getSpectrum(0).setDetectorIDs({1});
  }

  void test_grouped_twoTheta() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), 0.0199973 / 2.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(1), 0.0199973 / 2.0, 1e-6);
    // Other theta values are not sensible since the detectors include monitors
  }

  // Legacy test via the workspace method detectorTwoTheta(), which might be
  // removed at some point.
  void test_twoThetaLegacy() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto det = m_workspace.getDetector(2);
    TS_ASSERT_EQUALS(spectrumInfo.twoTheta(2),
                     m_workspace.detectorTwoTheta(*det));
  }

  // Legacy test via the workspace method detectorTwoTheta(), which might be
  // removed at some point.
  void test_grouped_twoThetaLegacy() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    auto det = m_grouped.getDetector(1);
    TS_ASSERT_EQUALS(spectrumInfo.twoTheta(1),
                     m_grouped.detectorTwoTheta(*det));
  }

  void test_signedTwoTheta() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(0), -0.0199973, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(2), 0.0199973, 1e-6);
    // Monitors
    TS_ASSERT_THROWS(spectrumInfo.signedTwoTheta(3), std::logic_error);
    TS_ASSERT_THROWS(spectrumInfo.signedTwoTheta(4), std::logic_error);
  }

  void test_grouped_signedTwoTheta() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(0), 0.0199973 / 2.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(1), -0.0199973 / 2.0, 1e-6);
    // Other theta values are not sensible since the detectors include monitors
  }

  // Legacy test via the workspace method detectorSignedTwoTheta(), which might
  // be removed at some point.
  void test_signedTwoThetaLegacy() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto det = m_workspace.getDetector(2);
    TS_ASSERT_EQUALS(spectrumInfo.signedTwoTheta(2),
                     m_workspace.detectorSignedTwoTheta(*det));
  }

  // Legacy test via the workspace method detectorSignedTwoTheta(), which might
  // be removed at some point.
  void test_grouped_signedTwoThetaLegacy() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    auto det = m_grouped.getDetector(1);
    TS_ASSERT_EQUALS(spectrumInfo.signedTwoTheta(1),
                     m_grouped.detectorSignedTwoTheta(*det));
  }

  void test_position() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.position(0), V3D(0.0, -0.1, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(1), V3D(0.0, 0.0, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(2), V3D(0.0, 0.1, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(3), V3D(0.0, 0.0, -9.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(4), V3D(0.0, 0.0, -2.0));
  }

  void test_grouped_position() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.position(0), V3D(0.0, 0.1 / 2.0, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(1), V3D(0.0, -0.1 / 2.0, 5.0));
    // Other positions are not sensible since the detectors include monitors
  }

  void test_grouped_position_tracks_changes() {
    auto &detectorInfo = m_grouped.mutableDetectorInfo();
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    const auto oldPos = spectrumInfo.position(0);
    // Change Y pos from 0.0 to -0.1
    detectorInfo.setPosition(1, V3D(0.0, -0.1, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(0), V3D(0.0, 0.0, 5.0));
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), 0.0199973, 1e-6);
    // Restore old position
    detectorInfo.setPosition(1, oldPos);
  }

  void test_hasDetectors() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT(spectrumInfo.hasDetectors(0));
    TS_ASSERT(spectrumInfo.hasDetectors(1));
    TS_ASSERT(spectrumInfo.hasDetectors(2));
    TS_ASSERT(spectrumInfo.hasDetectors(3));
    TS_ASSERT(spectrumInfo.hasDetectors(4));

    // Add second ID, we still have detectors.
    m_workspace.getSpectrum(1).addDetectorID(1);
    TS_ASSERT(spectrumInfo.hasDetectors(1));

    // Clear all IDs, no detectors
    m_workspace.getSpectrum(1).clearDetectorIDs();
    TS_ASSERT(!spectrumInfo.hasDetectors(1));

    // Restore old value
    m_workspace.getSpectrum(1).setDetectorID(2);
  }

  void test_grouped_hasDetectors() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT(spectrumInfo.hasDetectors(0));
    TS_ASSERT(spectrumInfo.hasDetectors(1));
    TS_ASSERT(spectrumInfo.hasDetectors(2));
    TS_ASSERT(spectrumInfo.hasDetectors(3));
    TS_ASSERT(spectrumInfo.hasDetectors(4));
  }

  void test_hasDetectors_ignores_bad_IDs() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    // Set bad value - Ids in instrument start at 1, 0 is out of range.
    m_workspace.getSpectrum(1).setDetectorID(0);
    TS_ASSERT(!spectrumInfo.hasDetectors(1));
    // Restore old value
    m_workspace.getSpectrum(1).setDetectorID(2);
  }

  void test_hasUniqueDetector() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT(spectrumInfo.hasUniqueDetector(0));
    TS_ASSERT(spectrumInfo.hasUniqueDetector(1));
    TS_ASSERT(spectrumInfo.hasUniqueDetector(2));
    TS_ASSERT(spectrumInfo.hasUniqueDetector(3));
    TS_ASSERT(spectrumInfo.hasUniqueDetector(4));

    // Add second ID, should not be unique anymore.
    m_workspace.getSpectrum(1).addDetectorID(1);
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(1));

    // Clear all IDs, also not unique.
    m_workspace.getSpectrum(1).clearDetectorIDs();
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(1));

    // Restore old value
    m_workspace.getSpectrum(1).setDetectorID(2);
  }

  void test_grouped_hasUniqueDetector() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(0));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(1));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(2));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(3));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(4));
  }

  void test_hasUniqueDetector_ignores_bad_IDs() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    // Add second *bad* ID, should still be unique.
    m_workspace.getSpectrum(1).addDetectorID(0);
    TS_ASSERT(spectrumInfo.hasUniqueDetector(1));
    // Restore old value
    m_workspace.getSpectrum(1).setDetectorID(2);
  }

  void test_detector() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_THROWS_NOTHING(spectrumInfo.detector(0));
    TS_ASSERT_EQUALS(spectrumInfo.detector(0).getID(), 1);
    TS_ASSERT_EQUALS(spectrumInfo.detector(1).getID(), 2);
    TS_ASSERT_EQUALS(spectrumInfo.detector(2).getID(), 3);
    TS_ASSERT_EQUALS(spectrumInfo.detector(3).getID(), 4);
    TS_ASSERT_EQUALS(spectrumInfo.detector(4).getID(), 5);
  }

  void test_no_detector() {
    const auto &spectrumInfo = m_workspaceNoInstrument.spectrumInfo();
    TS_ASSERT_THROWS(spectrumInfo.detector(0), std::out_of_range);
  }

private:
  WorkspaceTester m_workspace;
  WorkspaceTester m_workspaceNoInstrument;
  WorkspaceTester m_grouped;

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

  WorkspaceTester makeDefaultWorkspace() {
    WorkspaceTester ws;
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    ws.init(numberOfHistograms, numberOfBins + 1, numberOfBins);
    bool includeMonitors = true;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        ws, includeMonitors, startYNegative, instrumentName);

    std::set<int64_t> toMask{0, 3};
    ParameterMap &pmap = ws.instrumentParameters();
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      if (toMask.find(i) != toMask.end()) {
        IDetector_const_sptr det = ws.getDetector(i);
        pmap.addBool(det.get(), "masked", true);
      }
    }
    return ws;
  }
};

class SpectrumInfoTestPerformance : public CxxTest::TestSuite {
public:
  static SpectrumInfoTestPerformance *createSuite() {
    return new SpectrumInfoTestPerformance();
  }
  static void destroySuite(SpectrumInfoTestPerformance *suite) { delete suite; }

  SpectrumInfoTestPerformance() : m_workspace() {
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
