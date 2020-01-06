// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PREDICTFRACTIONALPEAKSTEST_H_
#define PREDICTFRACTIONALPEAKSTEST_H_

#include "MantidAPI/Sample.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::Workspace_sptr;
using Mantid::Crystal::PredictFractionalPeaks;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::V3D;

using namespace Mantid::DataHandling;
using namespace Mantid::Crystal;

namespace {

PeaksWorkspace_sptr createIndexedPeaksWorkspace() {
  LoadNexusProcessed loader;
  loader.setChild(true);
  loader.initialize();
  loader.isInitialized();
  loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
  loader.setPropertyValue("OutputWorkspace", "__unused__");
  loader.execute();
  Workspace_sptr loadedWS = loader.getProperty("OutputWorkspace");

  LoadIsawUB ubLoader;
  ubLoader.setChild(true);
  ubLoader.initialize();
  ubLoader.setProperty("InputWorkspace", loadedWS);
  ubLoader.setProperty("FileName", "TOPAZ_3007.mat");
  ubLoader.execute();

  IndexPeaks indexer;
  indexer.setChild(true);
  indexer.setLogging(false);
  indexer.initialize();
  indexer.setProperty("PeaksWorkspace", loadedWS);
  indexer.execute();

  return boost::dynamic_pointer_cast<PeaksWorkspace_sptr::element_type>(
      loadedWS);
}

PeaksWorkspace_sptr runPredictFractionalPeaks(
    const PeaksWorkspace_sptr &inputPeaks,
    const std::unordered_map<const char *, const char *> &args) {
  PredictFractionalPeaks alg;
  alg.setChild(true);
  alg.initialize();
  alg.setProperty("Peaks", inputPeaks);
  alg.setPropertyValue("FracPeaks", "__unused__");
  for (const auto &nameValue : args) {
    alg.setProperty(nameValue.first, nameValue.second);
  }
  alg.execute();
  TS_ASSERT(alg.isExecuted())

  return alg.getProperty("FracPeaks");
}

} // namespace

class PredictFractionalPeaksTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    PredictFractionalPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void setUp() override {
    if (!m_indexedPeaks) {
      m_indexedPeaks = createIndexedPeaksWorkspace();
    }
  }

  void test_exec_without_hkl_range_uses_input_peaks() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks,
        {{"HOffset", "-0.5,0,0.5"}, {"KOffset", "0.0"}, {"LOffset", "0.2"}});

    auto nPeaks = fracPeaks->getNumberPeaks();
    TS_ASSERT_EQUALS(117, nPeaks)
    if (nPeaks > 0) {
      const auto &peak0 = fracPeaks->getPeak(0);
      TS_ASSERT_DELTA(peak0.getH(), -5.5, .0001)
      TS_ASSERT_DELTA(peak0.getK(), 7.0, .0001)
      TS_ASSERT_DELTA(peak0.getL(), -3.8, .0001)
      TS_ASSERT_EQUALS(peak0.getDetectorID(), 1146353)
    }

    if (nPeaks > 3) {
      const auto &peak3 = fracPeaks->getPeak(3);
      TS_ASSERT_DELTA(peak3.getH(), -5.5, .0001)
      TS_ASSERT_DELTA(peak3.getK(), 3.0, .0001)
      TS_ASSERT_DELTA(peak3.getL(), -2.8, .0001)
      TS_ASSERT_EQUALS(peak3.getDetectorID(), 1747163)
    }

    if (nPeaks > 6) {
      const auto &peak6 = fracPeaks->getPeak(6);
      TS_ASSERT_DELTA(peak6.getH(), -6.5, .0001)
      TS_ASSERT_DELTA(peak6.getK(), 4.0, .0001)
      TS_ASSERT_DELTA(peak6.getL(), -3.8, .0001)
      TS_ASSERT_EQUALS(peak6.getDetectorID(), 1737894)
    }
  }

  void test_exec_with_include_in_range_and_hit_detector() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks, {{"HOffset", "-0.5,0,0.5"},
                         {"KOffset", "0.0"},
                         {"LOffset", "0.2"},
                         {"IncludeAllPeaksInRange", "1"},
                         {"Hmin", "-1"},
                         {"Hmax", "1"},
                         {"Kmin", "-2"},
                         {"Kmax", "2"},
                         {"Lmin", "-3"},
                         {"Lmax", "3"}

                        });
    TS_ASSERT_EQUALS(9, fracPeaks->getNumberPeaks())
    const auto &peak0 = fracPeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -0.5, .0001)
    TS_ASSERT_DELTA(peak0.getK(), -2.0, .0001)
    TS_ASSERT_DELTA(peak0.getL(), -1.8, .0001)
    TS_ASSERT_EQUALS(peak0.getDetectorID(), 1517488)

    const auto &peak2 = fracPeaks->getPeak(2);
    TS_ASSERT_DELTA(peak2.getH(), -0.5, .0001)
    TS_ASSERT_DELTA(peak2.getK(), 0.0, .0001)
    TS_ASSERT_DELTA(peak2.getL(), 0.2, .0001)
    TS_ASSERT_EQUALS(peak2.getDetectorID(), 3219638)

    const auto &peak4 = fracPeaks->getPeak(4);
    TS_ASSERT_DELTA(peak4.getH(), -1.0, .0001)
    TS_ASSERT_DELTA(peak4.getK(), 1.0, .0001)
    TS_ASSERT_DELTA(peak4.getL(), -0.8, .0001)
    TS_ASSERT_EQUALS(peak4.getDetectorID(), 1176666)

    const auto &peak8 = fracPeaks->getPeak(8);
    TS_ASSERT_DELTA(peak8.getH(), -1.5, .0001)
    TS_ASSERT_DELTA(peak8.getK(), 2.0, .0001)
    TS_ASSERT_DELTA(peak8.getL(), 0.2, .0001)
    TS_ASSERT_EQUALS(peak8.getDetectorID(), 2577373)
  }

  void test_exec_with_reflection_condition_and_hit_detector() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks, {{"HOffset", "-0.5,0,0.5"},
                         {"KOffset", "0.0"},
                         {"LOffset", "0.2"},
                         {"ReflectionCondition", "C-face centred"},
                         {"Hmin", "-1"},
                         {"Hmax", "1"},
                         {"Kmin", "-2"},
                         {"Kmax", "2"},
                         {"Lmin", "-3"},
                         {"Lmax", "3"}});
    TS_ASSERT_EQUALS(5, fracPeaks->getNumberPeaks())
    const auto &peak0 = fracPeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -1.5, .0001)
    TS_ASSERT_DELTA(peak0.getK(), 1.0, .0001)
    TS_ASSERT_DELTA(peak0.getL(), -0.8, .0001)
    TS_ASSERT_EQUALS(peak0.getDetectorID(), 1730014)

    const auto &peak2 = fracPeaks->getPeak(2);
    TS_ASSERT_DELTA(peak2.getH(), -1.5, .0001)
    TS_ASSERT_DELTA(peak2.getK(), 1.0, .0001)
    TS_ASSERT_DELTA(peak2.getL(), 0.2, .0001)
    TS_ASSERT_EQUALS(peak2.getDetectorID(), 3157981)

    const auto &peak4 = fracPeaks->getPeak(4);
    TS_ASSERT_DELTA(peak4.getH(), -0.5, .0001)
    TS_ASSERT_DELTA(peak4.getK(), 0.0, .0001)
    TS_ASSERT_DELTA(peak4.getL(), 0.2, .0001)
    TS_ASSERT_EQUALS(peak4.getDetectorID(), 3219638)
  }

  void test_exec_with_reflection_condition_and_not_required_on_detector() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks, {{"HOffset", "-0.5,0,0.5"},
                         {"KOffset", "0.0"},
                         {"LOffset", "0.2"},
                         {"ReflectionCondition", "C-face centred"},
                         {"RequirePeaksOnDetector", "0"},
                         {"Hmin", "-1"},
                         {"Hmax", "1"},
                         {"Kmin", "-2"},
                         {"Kmax", "2"},
                         {"Lmin", "-3"},
                         {"Lmax", "3"}});
    TS_ASSERT_EQUALS(72, fracPeaks->getNumberPeaks())

    const auto &peak0 = fracPeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -1.5, .0001)
    TS_ASSERT_DELTA(peak0.getK(), -2.0, .0001)
    TS_ASSERT_DELTA(peak0.getL(), -2.8, .0001)
    TS_ASSERT_EQUALS(peak0.getDetectorID(), -1)

    const auto &peak32 = fracPeaks->getPeak(32);
    TS_ASSERT_DELTA(peak32.getH(), 0.0, .0001)
    TS_ASSERT_DELTA(peak32.getK(), -2.0, .0001)
    TS_ASSERT_DELTA(peak32.getL(), -0.8, .0001)
    TS_ASSERT_EQUALS(peak32.getDetectorID(), -1)

    const auto &peak71 = fracPeaks->getPeak(71);
    TS_ASSERT_DELTA(peak71.getH(), 1.0, .0001)
    TS_ASSERT_DELTA(peak71.getK(), 1.0, .0001)
    TS_ASSERT_DELTA(peak71.getL(), -0.8, .0001)
    TS_ASSERT_EQUALS(peak71.getDetectorID(), -1)

    // check a couple of peaks had detectors
    TS_ASSERT_EQUALS(fracPeaks->getPeak(21).getDetectorID(), 1730014)
    TS_ASSERT_EQUALS(fracPeaks->getPeak(24).getDetectorID(), 3157981)
  }

  void test_providing_modulation_vector_saves_properties_to_lattice() {
    const auto fracPeaks = runPredictFractionalPeaks(
        m_indexedPeaks, {{"ModVector1", "-0.5,0,0.5"},
                         {"ModVector2", "0.0,0.5,0.5"},
                         {"MaxOrder", "1"},
                         {"CrossTerms", "0"}});

    TS_ASSERT_EQUALS(124, fracPeaks->getNumberPeaks())

    // check lattice
    const auto &lattice = fracPeaks->sample().getOrientedLattice();
    TS_ASSERT_EQUALS(1, lattice.getMaxOrder())
    TS_ASSERT_EQUALS(false, lattice.getCrossTerm())
    const auto mod1 = lattice.getModVec(0);
    TS_ASSERT_EQUALS(-0.5, mod1.X())
    TS_ASSERT_EQUALS(0.0, mod1.Y())
    TS_ASSERT_EQUALS(0.5, mod1.Z())
    const auto mod2 = lattice.getModVec(1);
    TS_ASSERT_EQUALS(0.0, mod2.X())
    TS_ASSERT_EQUALS(0.5, mod2.Y())
    TS_ASSERT_EQUALS(0.5, mod2.Z())

    // check a couple of peaks
    const auto &peak0 = fracPeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -4.5, .0001)
    TS_ASSERT_DELTA(peak0.getK(), 7.0, .0001)
    TS_ASSERT_DELTA(peak0.getL(), -4.5, .0001)
    const auto mainHKL0 = peak0.getIntHKL();
    TS_ASSERT_DELTA(mainHKL0[0], -5, .0001)
    TS_ASSERT_DELTA(mainHKL0[1], 7.0, .0001)
    TS_ASSERT_DELTA(mainHKL0[2], -4, .0001)
    TS_ASSERT_EQUALS(peak0.getDetectorID(), 1129591)
    const auto mnp0 = peak0.getIntMNP();
    TS_ASSERT_DELTA(mnp0.X(), -1., 1e-08)
    TS_ASSERT_DELTA(mnp0.Y(), 0., 1e-08)
    TS_ASSERT_DELTA(mnp0.Z(), 0., 1e-08)

    const auto &peak34 = fracPeaks->getPeak(34);
    TS_ASSERT_DELTA(peak34.getH(), -7, .0001)
    TS_ASSERT_DELTA(peak34.getK(), 7.5, .0001)
    TS_ASSERT_DELTA(peak34.getL(), -2.5, .0001)
    const auto mainHKL34 = peak34.getIntHKL();
    TS_ASSERT_DELTA(mainHKL34[0], -7, .0001)
    TS_ASSERT_DELTA(mainHKL34[1], 7.0, .0001)
    TS_ASSERT_DELTA(mainHKL34[2], -3, .0001)
    TS_ASSERT_EQUALS(peak34.getDetectorID(), 1812163)
    const auto mnp34 = peak34.getIntMNP();
    TS_ASSERT_DELTA(mnp34.X(), 0., 1e-08)
    TS_ASSERT_DELTA(mnp34.Y(), 1., 1e-08)
    TS_ASSERT_DELTA(mnp34.Z(), 0., 1e-08)
  }

  // ---------------- Failure tests -----------------------------
  void test_empty_peaks_workspace_is_not_allowed() {
    PredictFractionalPeaks alg;
    alg.initialize();
    alg.setProperty("Peaks", WorkspaceCreationHelper::createPeaksWorkspace(0));

    auto helpMsgs = alg.validateInputs();

    const auto valueIter = helpMsgs.find("Peaks");
    TS_ASSERT(valueIter != helpMsgs.cend())
  }

  void test_inconsistent_H_range_gives_validation_error() {
    doInvalidRangeTest("H");
  }

  void test_inconsistent_K_range_gives_validation_error() {
    doInvalidRangeTest("K");
  }

  void test_inconsistent_L_range_gives_validation_error() {
    doInvalidRangeTest("L");
  }

  void test_modulation_vector_requires_maxOrder_gt_0() {
    PredictFractionalPeaks alg;
    alg.initialize();
    alg.setProperty("Peaks", WorkspaceCreationHelper::createPeaksWorkspace(0));
    alg.setProperty("ModVector1", "0.5,0,0.5");

    auto helpMsgs = alg.validateInputs();

    TS_ASSERT(helpMsgs.find("MaxOrder") != helpMsgs.cend())
  }

private:
  void doInvalidRangeTest(const std::string &dimension) {
    PredictFractionalPeaks alg;
    alg.initialize();
    const std::string minName{dimension + "min"}, maxName{dimension + "max"};
    alg.setProperty(minName, 8.0);
    alg.setProperty(maxName, -8.0);

    auto helpMsgs = alg.validateInputs();

    const auto minError = helpMsgs.find(minName);
    TS_ASSERT(minError != helpMsgs.cend())
    const auto maxError = helpMsgs.find(maxName);
    TS_ASSERT(maxError != helpMsgs.cend())
  }

  PeaksWorkspace_sptr m_indexedPeaks;
};

#endif /* PredictFRACTIONALPEAKSTEST_H_ */
