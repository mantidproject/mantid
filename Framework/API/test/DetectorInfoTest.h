#ifndef MANTID_API_DETECTORINFOTEST_H_
#define MANTID_API_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

#include <algorithm>

using namespace Mantid::Geometry;
using namespace Mantid::API;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  DetectorInfoTest() : m_workspace(nullptr) {
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins + 1, numberOfBins);
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
    const auto &instrument = m_workspace.getInstrument();
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(*instrument));
  }

  void test_sourcePosition() {
    TS_ASSERT_EQUALS(m_workspace.detectorInfo().sourcePosition(),
                     V3D(0.0, 0.0, -20.0));
  }

  void test_samplePosition() {
    TS_ASSERT_EQUALS(m_workspace.detectorInfo().samplePosition(),
                     V3D(0.0, 0.0, 0.0));
  }

  void test_l1() { TS_ASSERT_EQUALS(m_workspace.detectorInfo().l1(), 20.0); }

  void test_l1_no_instrument() {
    TS_ASSERT_THROWS(m_workspaceNoInstrument.detectorInfo().l1(),
                     std::runtime_error);
  }

  void test_isMonitor() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    TS_ASSERT_EQUALS(detectorInfo.isMonitor(0), false);
    TS_ASSERT_EQUALS(detectorInfo.isMonitor(1), false);
    TS_ASSERT_EQUALS(detectorInfo.isMonitor(2), false);
    TS_ASSERT_EQUALS(detectorInfo.isMonitor(3), true);
    TS_ASSERT_EQUALS(detectorInfo.isMonitor(4), true);
  }

  void test_isMasked() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    TS_ASSERT_EQUALS(detectorInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(3), true);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(4), false);
  }

  void test_isMasked_unthreaded() {
    size_t count = 1000;
    auto ws = makeWorkspace(count);
    const auto &info = ws->detectorInfo();
    for (size_t i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(i), i % 2 == 0);
  }

  void test_isMasked_threaded() {
    int count = 1000;
    auto ws = makeWorkspace(count);
    const auto &info = ws->detectorInfo();
    // This attempts to test threading, but probably it is not really exercising
    // much.
    PARALLEL_FOR1(ws)
    for (int i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(static_cast<size_t>(i)), i % 2 == 0);
  }

  void test_l2() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    double x2 = 5.0 * 5.0;
    double y2 = 2.0 * 2.0 * 0.05 * 0.05;
    TS_ASSERT_EQUALS(detectorInfo.l2(0), sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(detectorInfo.l2(1), sqrt(x2 + 0 * 0 * y2));
    TS_ASSERT_EQUALS(detectorInfo.l2(2), sqrt(x2 + 1 * 1 * y2));
    TS_ASSERT_EQUALS(detectorInfo.l2(3), -9.0);
    TS_ASSERT_EQUALS(detectorInfo.l2(4), -2.0);
  }

  void test_twoTheta() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    TS_ASSERT_DELTA(detectorInfo.twoTheta(0), 0.0199973, 1e-6);
    TS_ASSERT_DELTA(detectorInfo.twoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(detectorInfo.twoTheta(2), 0.0199973, 1e-6);
  }

  // Legacy test via the workspace method detectorTwoTheta(), which might be
  // removed at some point.
  void test_twoThetaLegacy() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    auto det = m_workspace.getDetector(2);
    TS_ASSERT_EQUALS(detectorInfo.twoTheta(2),
                     m_workspace.detectorTwoTheta(*det));
  }

  void test_signedTwoTheta() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    TS_ASSERT_DELTA(detectorInfo.signedTwoTheta(0), -0.0199973, 1e-6);
    TS_ASSERT_DELTA(detectorInfo.signedTwoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(detectorInfo.signedTwoTheta(2), 0.0199973, 1e-6);
  }

  void test_position() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    TS_ASSERT_EQUALS(detectorInfo.position(0), V3D(0.0, -0.1, 5.0));
    TS_ASSERT_EQUALS(detectorInfo.position(1), V3D(0.0, 0.0, 5.0));
    TS_ASSERT_EQUALS(detectorInfo.position(2), V3D(0.0, 0.1, 5.0));
    TS_ASSERT_EQUALS(detectorInfo.position(3), V3D(0.0, 0.0, -9.0));
    TS_ASSERT_EQUALS(detectorInfo.position(4), V3D(0.0, 0.0, -2.0));
  }

  void test_setPosition() {
    auto &detectorInfo = m_workspace.mutableDetectorInfo();
    const auto oldPos = detectorInfo.position(0);
    TS_ASSERT_EQUALS(oldPos, V3D(0.0, -0.1, 5.0));
    V3D newPos(1.0, 2.0, 3.0);
    detectorInfo.setPosition(0, newPos);
    TS_ASSERT_EQUALS(detectorInfo.position(0), newPos);
    TS_ASSERT_EQUALS(detectorInfo.position(1), V3D(0.0, 0.0, 5.0));
    TS_ASSERT_EQUALS(detectorInfo.position(2), V3D(0.0, 0.1, 5.0));
    TS_ASSERT_EQUALS(detectorInfo.position(3), V3D(0.0, 0.0, -9.0));
    TS_ASSERT_EQUALS(detectorInfo.position(4), V3D(0.0, 0.0, -2.0));
    // Restore old state
    detectorInfo.setPosition(0, oldPos);
  }

  void test_rotation() {
    const auto &detectorInfo = m_workspace.detectorInfo();
    TS_ASSERT_EQUALS(detectorInfo.rotation(0), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(1), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(2), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(3), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(4), Quat(1.0, 0.0, 0.0, 0.0));
  }

  void test_setRotation() {
    V3D e3{0, 0, 1};
    Quat r3(90.0, e3);
    auto &detectorInfo = m_workspace.mutableDetectorInfo();
    const auto oldPos = detectorInfo.position(0);
    const auto oldRot = detectorInfo.rotation(0);
    TS_ASSERT_EQUALS(detectorInfo.rotation(0), Quat(1.0, 0.0, 0.0, 0.0));
    detectorInfo.setRotation(0, r3);
    // Rotation does *not* rotate the detector in the global coordinate system
    // but simply changes the orientation of the detector, keeping its position.
    TS_ASSERT_EQUALS(detectorInfo.position(0), oldPos);
    TS_ASSERT_EQUALS(detectorInfo.rotation(0), r3);
    TS_ASSERT_EQUALS(detectorInfo.rotation(1), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(2), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(3), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detectorInfo.rotation(4), Quat(1.0, 0.0, 0.0, 0.0));
    detectorInfo.setRotation(0, oldRot);
  }

  void test_setPosition_component_works_without_cached_positions() {
    auto &detectorInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldPos = root->getPos();
    const V3D offset(1.0, 0.0, 0.0);
    // No detector, source, or sample data has been accessed, make sure that
    // uninitialized caches done break the position update.
    TS_ASSERT_THROWS_NOTHING(detectorInfo.setPosition(*root, oldPos + offset));
    TS_ASSERT_THROWS_NOTHING(detectorInfo.setPosition(*root, oldPos));
  }

  void test_setPosition_component() {
    auto &detInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldPos = root->getPos();
    const V3D offset(1.0, 0.0, 0.0);

    TS_ASSERT_EQUALS(detInfo.sourcePosition(), V3D(0.0, 0.0, -20.0));
    TS_ASSERT_EQUALS(detInfo.samplePosition(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.position(0), V3D(0.0, -0.1, 5.0));

    detInfo.setPosition(*root, oldPos + offset);

    TS_ASSERT_EQUALS(detInfo.sourcePosition(), V3D(1.0, 0.0, -20.0));
    TS_ASSERT_EQUALS(detInfo.samplePosition(), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.position(0), V3D(1.0, -0.1, 5.0));

    // For additional verification we do *not* use detInfo, but make sure that
    // the changes actually affected the workspace.
    const auto &clone = m_workspace.clone();
    const auto &info = clone->detectorInfo();
    TS_ASSERT_EQUALS(info.sourcePosition(), V3D(1.0, 0.0, -20.0));
    TS_ASSERT_EQUALS(info.samplePosition(), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(info.position(0), V3D(1.0, -0.1, 5.0));

    detInfo.setPosition(*root, oldPos);
  }

  void test_setRotation_component() {
    auto &detInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldRot = root->getRotation();
    V3D e2{0, 1, 0};
    Quat rot(180.0, e2);

    detInfo.setRotation(*root, rot);

    // Rotations *and* positions have changed since *parent* was rotated
    TS_ASSERT_EQUALS(detInfo.rotation(0), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(1), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(2), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(3), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(4), rot);
    TS_ASSERT_EQUALS(detInfo.sourcePosition(), V3D(0.0, 0.0, 20.0));
    TS_ASSERT_EQUALS(detInfo.samplePosition(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.position(0), V3D(0.0, -0.1, -5.0));

    // For additional verification we do *not* use detInfo, but make sure that
    // the changes actually affected the workspace.
    const auto &clone = m_workspace.clone();
    const auto &info = clone->detectorInfo();
    TS_ASSERT_EQUALS(info.sourcePosition(), V3D(0.0, 0.0, 20.0));
    TS_ASSERT_EQUALS(info.samplePosition(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(info.position(0), V3D(0.0, -0.1, -5.0));

    detInfo.setRotation(*root, oldRot);
  }

  void test_detectorIDs() {
    WorkspaceTester workspace;
    int32_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    for (int32_t i = 0; i < numberOfHistograms; ++i)
      workspace.getSpectrum(i)
          .setSpectrumNo(static_cast<int32_t>(numberOfHistograms) - i);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        workspace, includeMonitors, startYNegative, instrumentName);
    // Check that *workspace* does not have sorted IDs.
    TS_ASSERT_EQUALS(workspace.getDetector(0)->getID(), 5);
    TS_ASSERT_EQUALS(workspace.getDetector(1)->getID(), 4);
    TS_ASSERT_EQUALS(workspace.getDetector(2)->getID(), 3);
    TS_ASSERT_EQUALS(workspace.getDetector(3)->getID(), 2);
    TS_ASSERT_EQUALS(workspace.getDetector(4)->getID(), 1);
    const auto &info = workspace.detectorInfo();
    const auto &ids = info.detectorIDs();
    auto sorted_ids(ids);
    std::sort(sorted_ids.begin(), sorted_ids.end());
    // The ids we get from DetectorInfo should be sorted.
    TS_ASSERT_EQUALS(ids, sorted_ids);
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

#endif /* MANTID_API_DETECTORINFOTEST_H_ */
