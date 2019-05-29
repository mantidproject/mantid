// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SPECTRUMINFOTEST_H_
#define MANTID_API_SPECTRUMINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/SpectrumInfoIterator.h"
#include "MantidBeamline/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
constexpr size_t GroupOfDets2And3 = 0;
constexpr size_t GroupOfDets1And2 = 1;
constexpr size_t GroupOfDets1And4 = 2;
constexpr size_t GroupOfDets4And5 = 3;
constexpr size_t GroupOfAllDets = 4;
} // namespace

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
    m_workspaceNoInstrument.initialize(numberOfHistograms, numberOfBins + 1,
                                       numberOfBins);

    // Workspace has 5 detectors, 1 and 4 are masked, 4 and 5 are monitors.
    m_grouped.getSpectrum(GroupOfDets2And3).setDetectorIDs({2, 3}); // no mask
    m_grouped.getSpectrum(GroupOfDets1And2)
        .setDetectorIDs({1, 2}); // partial mask
    m_grouped.getSpectrum(GroupOfDets1And4)
        .setDetectorIDs({1, 4}); // masked, partial monitor
    m_grouped.getSpectrum(GroupOfDets4And5)
        .setDetectorIDs({4, 5}); // full monitor
    m_grouped.getSpectrum(GroupOfAllDets)
        .setDetectorIDs({1, 2, 3, 4, 5}); // everything
  }

  void test_constructor() {
    Beamline::SpectrumInfo specInfo(3);
    auto ws = makeWorkspace(3);
    TS_ASSERT_THROWS_NOTHING(
        SpectrumInfo(specInfo, *ws, ws->mutableDetectorInfo()));
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
                     const std::runtime_error &);
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
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(GroupOfDets2And3), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(GroupOfDets1And2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(GroupOfDets1And4), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(GroupOfDets4And5), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMonitor(GroupOfAllDets), false);
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
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets2And3), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And4), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets4And5), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfAllDets), false);
  }

  void test_isMasked_unthreaded() {
    size_t count = 1000;
    auto ws = makeWorkspace(count);
    const auto &info = ws->spectrumInfo();
    for (size_t i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(i), i % 2 == 0);
  }

  void test_isMasked_threaded() {
    int count = 1000;
    auto ws = makeWorkspace(count);
    const auto &info = ws->spectrumInfo();
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
    TS_ASSERT_EQUALS(spectrumInfo.l2(GroupOfDets2And3),
                     (sqrt(x2 + 0 * 0 * y2) + sqrt(x2 + 1 * 1 * y2)) / 2.0);
    TS_ASSERT_EQUALS(spectrumInfo.l2(GroupOfDets1And2),
                     (sqrt(x2 + 0 * 0 * y2) + sqrt(x2 + 1 * 1 * y2)) / 2.0);
    // Other lengths are not sensible since the detectors include monitors
  }

  void test_twoTheta() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), 0.0199973, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(2), 0.0199973, 1e-6);
    // Monitors
    TS_ASSERT_THROWS(spectrumInfo.twoTheta(3), const std::logic_error &);
    TS_ASSERT_THROWS(spectrumInfo.twoTheta(4), const std::logic_error &);
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
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(GroupOfDets2And3), 0.0199973 / 2.0,
                    1e-6);
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(GroupOfDets1And2), 0.0199973 / 2.0,
                    1e-6);
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
    auto det = m_grouped.getDetector(GroupOfDets1And2);
    TS_ASSERT_EQUALS(spectrumInfo.twoTheta(GroupOfDets1And2),
                     m_grouped.detectorTwoTheta(*det));
  }

  void test_signedTwoTheta() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(0), -0.0199973, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(1), 0.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(2), 0.0199973, 1e-6);
    // Monitors
    TS_ASSERT_THROWS(spectrumInfo.signedTwoTheta(3), const std::logic_error &);
    TS_ASSERT_THROWS(spectrumInfo.signedTwoTheta(4), const std::logic_error &);
  }

  void test_grouped_signedTwoTheta() {
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(GroupOfDets2And3),
                    0.0199973 / 2.0, 1e-6);
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(GroupOfDets1And2),
                    -0.0199973 / 2.0, 1e-6);
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
    auto det = m_grouped.getDetector(GroupOfDets1And2);
    TS_ASSERT_EQUALS(spectrumInfo.signedTwoTheta(GroupOfDets1And2),
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
    TS_ASSERT_EQUALS(spectrumInfo.position(GroupOfDets2And3),
                     V3D(0.0, 0.1 / 2.0, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(GroupOfDets1And2),
                     V3D(0.0, -0.1 / 2.0, 5.0));
    // Other positions are not sensible since the detectors include monitors
  }

  void test_grouped_position_tracks_changes() {
    auto &detectorInfo = m_grouped.mutableDetectorInfo();
    const auto &spectrumInfo = m_grouped.spectrumInfo();
    const auto oldPos = detectorInfo.position(1);
    // Change Y pos from 0.0 to -0.1
    detectorInfo.setPosition(1, V3D(0.0, -0.1, 5.0));
    TS_ASSERT_EQUALS(spectrumInfo.position(GroupOfDets2And3),
                     V3D(0.0, 0.0, 5.0));
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
    TS_ASSERT(spectrumInfo.hasDetectors(GroupOfDets2And3));
    TS_ASSERT(spectrumInfo.hasDetectors(GroupOfDets1And2));
    TS_ASSERT(spectrumInfo.hasDetectors(GroupOfDets1And4));
    TS_ASSERT(spectrumInfo.hasDetectors(GroupOfDets4And5));
    TS_ASSERT(spectrumInfo.hasDetectors(GroupOfAllDets));
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
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(GroupOfDets2And3));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(GroupOfDets1And2));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(GroupOfDets1And4));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(GroupOfDets4And5));
    TS_ASSERT(!spectrumInfo.hasUniqueDetector(GroupOfAllDets));
  }

  void test_hasUniqueDetector_ignores_bad_IDs() {
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    // Add second *bad* ID, should still be unique.
    m_workspace.getSpectrum(1).addDetectorID(0);
    TS_ASSERT(spectrumInfo.hasUniqueDetector(1));
    // Restore old value
    m_workspace.getSpectrum(1).setDetectorID(2);
  }

  void test_setMasked() {
    auto &spectrumInfo = m_workspace.mutableSpectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), true);
    spectrumInfo.setMasked(0, false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), false);
    spectrumInfo.setMasked(0, true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), true);
    // Make sure no other detectors are affected
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(4), false);
  }

  void test_grouped_setMasked() {
    auto &spectrumInfo = m_grouped.mutableSpectrumInfo();
    spectrumInfo.setMasked(GroupOfAllDets, false);
    // 4 includes all detectors so all other spectra are affected
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets2And3), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And4), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets4And5), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfAllDets), false);
    spectrumInfo.setMasked(GroupOfDets2And3, true);
    // Partial masking => false
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets2And3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And4), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets4And5), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfAllDets), false);
    // Restore initial state
    spectrumInfo.setMasked(GroupOfAllDets, false);
    spectrumInfo.setMasked(GroupOfDets1And4, true);
  }

  void test_grouped_setMasked_reverse_case() {
    auto &spectrumInfo = m_grouped.mutableSpectrumInfo();
    spectrumInfo.setMasked(GroupOfAllDets, true);
    // 4 includes all detectors so all other spectra are affected
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets2And3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And2), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And4), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets4And5), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfAllDets), true);
    spectrumInfo.setMasked(GroupOfDets2And3, false);
    // Partial masking => false
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets2And3), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And4), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets4And5), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfAllDets), false);
    // Restore initial state
    spectrumInfo.setMasked(GroupOfAllDets, false);
    spectrumInfo.setMasked(GroupOfDets1And4, true);
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
    TS_ASSERT_THROWS(spectrumInfo.detector(0),
                     const Kernel::Exception::NotFoundError &);
  }

  void test_no_detector_twice() {
    // Regression test: Make sure that *repeated* access also fails.
    const auto &spectrumInfo = m_workspaceNoInstrument.spectrumInfo();
    TS_ASSERT_THROWS(spectrumInfo.detector(0),
                     const Kernel::Exception::NotFoundError &);
    TS_ASSERT_THROWS(spectrumInfo.detector(0),
                     const Kernel::Exception::NotFoundError &);
  }

  void test_ExperimentInfo_basics() {
    const ExperimentInfo expInfo(m_workspace);
    const auto &spectrumInfo = expInfo.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(3), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(4), false);
  }

  void test_ExperimentInfo_from_grouped() {
    const ExperimentInfo expInfo(m_grouped);
    const auto &spectrumInfo = expInfo.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.size(), 5);
    // We construct from a grouped workspace (via ISpectrum), but grouping is
    // now stored in Beamline::SpectrumInfo as part of ExperimentInfo, so we
    // should also see the grouping here.
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets2And3), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets1And4), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfDets4And5), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(GroupOfAllDets), false);
  }

  void test_ExperimentInfo_grouped() {
    ExperimentInfo expInfo(m_workspace);

    // We cannot really test anything but a single group, since the grouping
    // mechanism in ExperimentInfo is currently based on std::unordered_map, so
    // we have no control over the order and thus cannot write asserts.
    det2group_map mapping{{1, {1, 2}}};
    expInfo.cacheDetectorGroupings(mapping);
    const auto &spectrumInfo = expInfo.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.size(), 1);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), false);

    mapping = {{1, {1, 4}}};
    expInfo.cacheDetectorGroupings(mapping);
    const auto &spectrumInfo2 = expInfo.spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo2.size(), 1);
    TS_ASSERT_EQUALS(spectrumInfo2.isMasked(0), true);
  }

  void test_cacheDetectorGroupings_fails_for_MatrixWorkspace() {
    // This is actually testing a method of MatrixWorkspace but SpectrumInfo
    // needs to be able to rely on this.
    det2group_map mapping{{1, {1, 2}}};
    TS_ASSERT_THROWS(m_workspace.cacheDetectorGroupings(mapping),
                     const std::runtime_error &);
  }

  /**
   * Tests for Iterator Functionality
   **/

  void test_iterator_begin() {
    // Get the SpectrumInfo object
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto iter = spectrumInfo.cbegin();

    // Check we start at the correct place
    TS_ASSERT(iter != spectrumInfo.cend());
  }

  void test_iterator_end() {
    // Get the SpectrumInfo object
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto iter = spectrumInfo.cend();

    // Check we start at the correct place
    TS_ASSERT(iter != spectrumInfo.cbegin());
  }

  void test_iterator_increment_and_hasUniqueDetector() {
    // Get the SpectrumInfo object
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto iter = spectrumInfo.cbegin();

    // Check that we start at the beginning
    TS_ASSERT(iter == spectrumInfo.cbegin());

    // Increment iterator and check hasUniqueDetector
    for (size_t i = 0; i < m_workspace.spectrumInfo().size(); ++i) {
      TS_ASSERT_EQUALS(iter->hasUniqueDetector(), true);
      ++iter;
    }

    // Check we've reached the end
    TS_ASSERT(iter == spectrumInfo.cend());
  }

  void test_iterator_decrement_and_hasUniqueDetector() {
    // Get the SpectrumInfo object
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto iter = spectrumInfo.cend();

    // Check that we start at the end
    TS_ASSERT(iter == spectrumInfo.cend());

    // Decrement iterator and check hasUniqueDetector
    for (size_t i = m_workspace.spectrumInfo().size(); i > 0; --i) {
      --iter;
      TS_ASSERT_EQUALS(iter->hasUniqueDetector(), true);
    }

    // Check we've reached the beginning
    TS_ASSERT(iter == spectrumInfo.cbegin());
  }

  void test_iterator_advance_and_hasUniqueDetector() {
    // Get the SpectrumInfo object
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto iter = spectrumInfo.cbegin();

    // Advance 3 places
    std::advance(iter, 3);
    TS_ASSERT_EQUALS(iter->hasUniqueDetector(), true);

    // Go backwards
    std::advance(iter, -2);
    TS_ASSERT_EQUALS(iter->hasUniqueDetector(), true);

    // Go to the start
    std::advance(iter, -1);
    TS_ASSERT(iter == spectrumInfo.cbegin());
  }

  void test_copy_iterator_and_hasUniqueDetector() {
    // Get the SpectrumInfo object
    const auto &spectrumInfo = m_workspace.spectrumInfo();
    auto iter = spectrumInfo.cbegin();

    // Create a copy
    auto iterCopy = SpectrumInfoConstIt(iter);

    // Check
    TS_ASSERT_EQUALS(iter->hasUniqueDetector(), true);
    TS_ASSERT_EQUALS(iterCopy->hasUniqueDetector(), true);

    // Increment
    ++iter;
    ++iterCopy;

    // Check again
    TS_ASSERT_EQUALS(iter->hasUniqueDetector(), true);
    TS_ASSERT_EQUALS(iterCopy->hasUniqueDetector(), true);
  }

  void test_mutating_via_writable_iterator() {
    auto &spectrumInfo = m_workspace.mutableSpectrumInfo();
    auto it = spectrumInfo.begin();

    it->setMasked(true);
    TS_ASSERT(spectrumInfo.cbegin()->isMasked() == true);
  }

private:
  WorkspaceTester m_workspace;
  WorkspaceTester m_workspaceNoInstrument;
  WorkspaceTester m_grouped;

  std::unique_ptr<MatrixWorkspace> makeWorkspace(size_t numSpectra) {
    auto ws = Kernel::make_unique<WorkspaceTester>();
    ws->initialize(numSpectra, 1, 1);
    auto inst = boost::make_shared<Instrument>("TestInstrument");
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      auto det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
      inst->add(det);
      inst->markAsDetector(det);
      ws->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
    }
    ws->setInstrument(inst);
    auto &detectorInfo = ws->mutableDetectorInfo();
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i)
      if (i % 2 == 0)
        detectorInfo.setMasked(i, true);
    return std::move(ws);
  }

  WorkspaceTester makeDefaultWorkspace() {
    WorkspaceTester ws;
    size_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    ws.initialize(numberOfHistograms, numberOfBins + 1, numberOfBins);
    bool includeMonitors = true;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        ws, includeMonitors, startYNegative, instrumentName);

    std::set<int64_t> toMask{0, 3};
    auto &detectorInfo = ws.mutableDetectorInfo();
    for (const auto &i : toMask)
      detectorInfo.setMasked(i, true);
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
