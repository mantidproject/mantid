// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_DETECTORINFOTEST_H_
#define MANTID_GEOMETRY_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

#include <algorithm>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  DetectorInfoTest() {
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    m_workspace.initialize(numberOfHistograms, numberOfBins + 1, numberOfBins);
    bool includeMonitors = true;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);

    std::set<int64_t> toMask{0, 3};
    auto &detInfo = m_workspace.mutableDetectorInfo();
    for (size_t i = 0; i < m_workspace.getNumberHistograms(); ++i) {
      if (toMask.find(i) != toMask.end()) {
        detInfo.setMasked(i, true);
      }
    }

    m_workspaceNoInstrument.initialize(numberOfHistograms, numberOfBins + 1,
                                       numberOfBins);
  }

  void test_comparison() {
    TS_ASSERT(
        m_workspace.detectorInfo().isEquivalent(m_workspace.detectorInfo()));
  }

  void test_size() { TS_ASSERT_EQUALS(m_workspace.detectorInfo().size(), 5); }

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
                     const std::runtime_error &);
  }

  void test_l1_no_instrument_call_once_regression() {
    // There is a bug in GCC (5.4): If an exception is thrown from within
    // std::call_once, a subsequent call to that std::call_once will freeze.
    // Previously this happened for DetectorInfo so here we see if a failing
    // call to `l1` can be repeated. If the bug is reintroduced this test will
    // not fail but FREEZE.
    TS_ASSERT_THROWS(m_workspaceNoInstrument.detectorInfo().l1(),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(m_workspaceNoInstrument.detectorInfo().l1(),
                     const std::runtime_error &);
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
    PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
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
    // Monitors
    TS_ASSERT_THROWS(detectorInfo.twoTheta(3), const std::logic_error &);
    TS_ASSERT_THROWS(detectorInfo.twoTheta(4), const std::logic_error &);
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
    // Monitors
    TS_ASSERT_THROWS(detectorInfo.signedTwoTheta(3), const std::logic_error &);
    TS_ASSERT_THROWS(detectorInfo.signedTwoTheta(4), const std::logic_error &);
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

  void test_setMasked() {
    auto &detectorInfo = m_workspace.mutableDetectorInfo();
    TS_ASSERT_EQUALS(detectorInfo.isMasked(0), true);
    detectorInfo.setMasked(0, false);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(0), false);
    detectorInfo.setMasked(0, true);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(0), true);
    // Make sure no other detectors are affected
    TS_ASSERT_EQUALS(detectorInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(3), true);
    TS_ASSERT_EQUALS(detectorInfo.isMasked(4), false);
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

  void test_setPosition_component() {
    auto &detInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldPos = root->getPos();
    const V3D offset(1.0, 0.0, 0.0);

    TS_ASSERT_EQUALS(detInfo.sourcePosition(), V3D(0.0, 0.0, -20.0));
    TS_ASSERT_EQUALS(detInfo.samplePosition(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.position(0), V3D(0.0, -0.1, 5.0));

    auto &compInfo = m_workspace.mutableComponentInfo();
    compInfo.setPosition(compInfo.indexOf(root->getComponentID()),
                         oldPos + offset);

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

    // Reset
    compInfo.setPosition(compInfo.indexOf(root->getComponentID()), oldPos);
  }

  void test_setRotation_component() {
    auto &detInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldRot = root->getRotation();
    V3D e2{0, 1, 0};
    Quat rot(180.0, e2);

    auto &compInfo = m_workspace.mutableComponentInfo();
    compInfo.setRotation(compInfo.indexOf(root->getComponentID()), rot);

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

    // Reset
    compInfo.setRotation(compInfo.indexOf(root->getComponentID()), oldRot);
  }

  void test_setRotation_component_moved_root() {
    auto &detInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldPos = root->getPos();
    const auto oldRot = root->getRotation();
    V3D e2{0, 1, 0};
    Quat rot(180.0, e2);

    auto &compInfo = m_workspace.mutableComponentInfo();
    compInfo.setPosition(compInfo.indexOf(root->getComponentID()),
                         V3D{0.0, 0.0, 1.0});
    compInfo.setRotation(compInfo.indexOf(root->getComponentID()), rot);

    // Rotations *and* positions have changed since *parent* was rotated
    TS_ASSERT_EQUALS(detInfo.rotation(0), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(1), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(2), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(3), rot);
    TS_ASSERT_EQUALS(detInfo.rotation(4), rot);
    TS_ASSERT_EQUALS(detInfo.sourcePosition(), V3D(0.0, 0.0, 21.0));
    TS_ASSERT_EQUALS(detInfo.samplePosition(), V3D(0.0, 0.0, 1.0));
    TS_ASSERT_EQUALS(detInfo.position(0), V3D(0.0, -0.1, -4.0));

    // For additional verification we do *not* use detInfo, but make sure that
    // the changes actually affected the workspace.
    const auto &clone = m_workspace.clone();
    const auto &info = clone->detectorInfo();
    TS_ASSERT_EQUALS(info.sourcePosition(), V3D(0.0, 0.0, 21.0));
    TS_ASSERT_EQUALS(info.samplePosition(), V3D(0.0, 0.0, 1.0));
    TS_ASSERT_EQUALS(info.position(0), V3D(0.0, -0.1, -4.0));

    // Reset
    compInfo.setRotation(compInfo.indexOf(root->getComponentID()), oldRot);
    compInfo.setPosition(compInfo.indexOf(root->getComponentID()), oldPos);
  }

  void test_setRotation_setPosition_commute() {
    auto &detInfo = m_workspace.mutableDetectorInfo();
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");
    const auto oldRot = root->getRotation();
    const auto oldPos = root->getPos();
    V3D axis{0.1, 0.2, 0.7};
    Quat rot(42.0, axis);
    V3D pos{-11.0, 7.0, 42.0};

    // Note the order: We are going in a (figurative) square...
    auto &compInfo = m_workspace.mutableComponentInfo();
    const size_t rootIndex = compInfo.indexOf(root->getComponentID());
    compInfo.setRotation(rootIndex, rot);
    compInfo.setPosition(rootIndex, pos);
    compInfo.setRotation(rootIndex, oldRot);
    compInfo.setPosition(rootIndex, oldPos);

    // ... and check that we come back to where we started.
    TS_ASSERT_EQUALS(detInfo.position(0), V3D(0.0, -0.1, 5.0));
    TS_ASSERT_EQUALS(detInfo.position(1), V3D(0.0, 0.0, 5.0));
    TS_ASSERT_EQUALS(detInfo.position(2), V3D(0.0, 0.1, 5.0));
    TS_ASSERT_EQUALS(detInfo.position(3), V3D(0.0, 0.0, -9.0));
    TS_ASSERT_EQUALS(detInfo.position(4), V3D(0.0, 0.0, -2.0));
    TS_ASSERT_EQUALS(detInfo.rotation(0), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.rotation(1), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.rotation(2), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.rotation(3), Quat(1.0, 0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(detInfo.rotation(4), Quat(1.0, 0.0, 0.0, 0.0));
  }

  void test_positions_rotations_multi_level() {
    WorkspaceTester ws;
    ws.initialize(9, 1, 1);
    ws.setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(1));
    auto &detInfo = ws.mutableDetectorInfo();
    const auto root = ws.getInstrument();
    const auto bank = root->getComponentByName("bank1");
    TS_ASSERT_EQUALS(detInfo.position(0), (V3D{-0.008, -0.0002, 5.0}));
    const auto rootRot = root->getRotation();
    const auto rootPos = root->getPos();
    const auto bankPos = bank->getPos();
    V3D axis{0.1, 0.2, 0.7};
    Quat rot(42.0, axis);
    V3D delta1{-11.0, 7.0, 42.0};
    V3D delta2{1.0, 3.0, 2.0};

    auto &compInfo = ws.mutableComponentInfo();
    const size_t rootIndex = compInfo.indexOf(root->getComponentID());
    const size_t bankIndex = compInfo.indexOf(bank->getComponentID());
    compInfo.setRotation(rootIndex, rot);
    compInfo.setPosition(rootIndex, delta1);
    compInfo.setPosition(bankIndex, delta1 + delta2);
    // Undo, but *not* in reverse order.
    compInfo.setRotation(rootIndex, rootRot);
    compInfo.setPosition(rootIndex, rootPos);
    compInfo.setPosition(bankIndex, bankPos);
    TS_ASSERT_EQUALS(detInfo.position(0), (V3D{-0.008, -0.0002, 5.0}));
  }

  void test_detectorIDs() {
    WorkspaceTester workspace;
    int32_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    workspace.initialize(numberOfHistograms, numberOfBins + 1, numberOfBins);
    for (int32_t i = 0; i < numberOfHistograms; ++i)
      workspace.getSpectrum(i).setSpectrumNo(
          static_cast<int32_t>(numberOfHistograms) - i);
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

  void test_assignment() {
    auto ws1 = makeWorkspace(2);
    auto ws2 = makeWorkspace(2);
    TS_ASSERT_THROWS_NOTHING(ws2->mutableDetectorInfo() = ws1->detectorInfo());
    // TODO Beamline::DetectorInfo is currently not containing data, so there is
    // nothing we can check here. Once the class is getting populated add more
    // checks here.
  }

  void test_assignment_mismatch() {
    auto ws1 = makeWorkspace(1);
    auto ws2 = makeWorkspace(2);
    TS_ASSERT_THROWS(ws2->mutableDetectorInfo() = ws1->detectorInfo(),
                     const std::runtime_error &);
  }

private:
  WorkspaceTester m_workspace;
  WorkspaceTester m_workspaceNoInstrument;

  std::unique_ptr<MatrixWorkspace> makeWorkspace(size_t numSpectra) {
    auto ws = std::make_unique<WorkspaceTester>();
    ws->initialize(numSpectra, 1, 1);
    auto inst = boost::make_shared<Instrument>("TestInstrument");
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      auto det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
      inst->add(det);
      inst->markAsDetector(det);
    }
    ws->setInstrument(inst);
    auto &detInfo = ws->mutableDetectorInfo();
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      ws->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
      if (i % 2 == 0)
        detInfo.setMasked(i, true);
    }
    return std::move(ws);
  }
};

class DetectorInfoTestPerformance : public CxxTest::TestSuite {
public:
  static DetectorInfoTestPerformance *createSuite() {
    return new DetectorInfoTestPerformance();
  }
  static void destroySuite(DetectorInfoTestPerformance *suite) { delete suite; }

  DetectorInfoTestPerformance() : m_workspace() {
    size_t numberOfHistograms = 10000;
    size_t numberOfBins = 1;
    m_workspace.initialize(numberOfHistograms, numberOfBins + 1, numberOfBins);
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
    for (int repeat = 0; repeat < 32; ++repeat) {
      double result = 0.0;
      const auto &detectorInfo = m_workspace.detectorInfo();
      for (size_t i = 0; i < 10000; ++i) {
        result += detectorInfo.l1();
        result += detectorInfo.l2(i);
        result += detectorInfo.twoTheta(i);
      }
      // We are computing and using the result to fool the optimizer.
      TS_ASSERT_DELTA(result, 5214709.740869, 1e-6);
    }
  }

  void test_isMasked() {
    for (int repeat = 0; repeat < 32; ++repeat) {
      bool result = false;
      const auto &detectorInfo = m_workspace.detectorInfo();
      for (size_t i = 0; i < 10000; ++i) {
        result |= detectorInfo.isMasked(i);
      }
      // We are computing and using the result to fool the optimizer.
      TS_ASSERT(!result);
    }
  }

  void test_position() {
    for (int repeat = 0; repeat < 32; ++repeat) {
      Kernel::V3D result;
      const auto &detectorInfo = m_workspace.detectorInfo();
      for (size_t i = 0; i < 10000; ++i) {
        result += detectorInfo.position(i);
      }
      // We are computing and using the result to fool the optimizer.
      TS_ASSERT_DELTA(result[0], 0.0, 1e-6);
    }
  }

  void test_setPosition() {
    for (int repeat = 0; repeat < 32; ++repeat) {
      auto &detectorInfo = m_workspace.mutableDetectorInfo();
      for (size_t i = 0; i < 10000; ++i) {
        detectorInfo.setPosition(i, Kernel::V3D(1.0, 0.0, 0.0));
      }
    }
  }

  void test_position_after_move() {
    for (int repeat = 0; repeat < 32; ++repeat) {
      Kernel::V3D result;
      const auto &detectorInfo = m_workspace.detectorInfo();
      for (size_t i = 0; i < 10000; ++i) {
        result += detectorInfo.position(i);
      }
      // We are computing and using the result to fool the optimizer.
      TS_ASSERT_DELTA(result[0], 10000.0, 1e-6);
    }
  }

  void test_position_after_parent_move() {
    const auto &instrument = m_workspace.getInstrument();
    const auto &root = instrument->getComponentByName("SimpleFakeInstrument");

    auto &compInfo = m_workspace.mutableComponentInfo();
    compInfo.setPosition(compInfo.indexOf(root->getComponentID()),
                         Kernel::V3D(0.1, 0.0, 0.0));
    for (int repeat = 0; repeat < 32; ++repeat) {
      Kernel::V3D result;
      const auto &detectorInfo = m_workspace.detectorInfo();
      for (size_t i = 0; i < 10000; ++i) {
        result += detectorInfo.position(i);
      }
      // We are computing and using the result to fool the optimizer.
      TS_ASSERT_DELTA(result[0], 11000.0, 1e-6);
    }
  }

private:
  WorkspaceTester m_workspace;
};

#endif /* MANTID_GEOMETRY_DETECTORINFOTEST_H_ */
