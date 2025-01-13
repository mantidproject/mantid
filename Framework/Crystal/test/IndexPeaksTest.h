// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/WarningSuppressions.h"

#include <cxxtest/TestSuite.h>

using Mantid::API::IPeaksWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::Crystal::IndexPeaks;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::Geometry::IndexingUtils;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::V3D;

namespace {
// run number, det pos, q_sample
using MinimalPeak = std::tuple<int, V3D>;
template <int NPeaks> using MinimalPeaksList = std::array<MinimalPeak, NPeaks>;

template <int NPeaks>
PeaksWorkspace_sptr createPeaksWorkspace(const MinimalPeaksList<NPeaks> &testPeaksInfo, const std::vector<double> &ub,
                                         const int maxOrder = -1,
                                         const std::vector<V3D> &modVectors = std::vector<V3D>(),
                                         const bool crossTerms = false) {
  auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(NPeaks);
  auto lattice = std::make_unique<OrientedLattice>();
  lattice->setUB(ub);
  if (maxOrder > 0) {
    lattice->setMaxOrder(maxOrder);
    if (modVectors.empty() || modVectors.size() > 3)
      throw std::runtime_error("Only <= 3 modulation vectors can be provided.");
    auto modVecOrDefault = [&modVectors](const size_t index) {
      if (index < modVectors.size())
        return modVectors[index];
      else
        return V3D();
    };
    lattice->setModVec1(modVecOrDefault(0));
    lattice->setModVec2(modVecOrDefault(1));
    lattice->setModVec3(modVecOrDefault(2));
    lattice->setCrossTerm(crossTerms);
  }
  peaksWS->mutableSample().setOrientedLattice(std::move(lattice));

  int peakCount{0};
  for (const auto &testPeakInfo : testPeaksInfo) {
    auto &wsPeak = peaksWS->getPeak(peakCount);
    wsPeak.setRunNumber(std::get<0>(testPeakInfo));
    wsPeak.setQSampleFrame(std::get<1>(testPeakInfo));
    ++peakCount;
  }

  return peaksWS;
}

PeaksWorkspace_sptr createTestPeaksWorkspaceMainReflOnly() {
  constexpr int npeaks{5};
  const std::vector<double> ub = {-0.0122354, 0.00480056, -0.0860404,  0.1165450, 0.00178145,
                                  0.0045884,  0.0273738,  -0.08973560, 0.0252595};
  // peaks from TOPAZ_3007.peaks: 0, 1, 2, 10, 42 with sign for Q swapped
  // as we don't use the crystallographic convention
  GNU_DIAG_OFF("missing-braces");
  constexpr std::array<MinimalPeak, npeaks> testPeaksInfo = {
      MinimalPeak{3008, V3D(-3.52961, 3.13589, 1.0899)}, MinimalPeak{3007, V3D(-2.42456, 2.29581, 1.71147)},
      MinimalPeak{3007, V3D(-3.04393, 3.05739, 2.03727)}, MinimalPeak{3007, V3D(-4.02271, 2.4073, 1.62228)},
      MinimalPeak{3008, V3D(-4.04552, 1.59916, 3.71776)}};
  GNU_DIAG_ON("missing-braces");
  return createPeaksWorkspace<npeaks>(testPeaksInfo, ub);
}

PeaksWorkspace_sptr createTestPeaksWorkspaceWithSatellites(const int maxOrder = -1,
                                                           const std::vector<V3D> &modVectors = std::vector<V3D>(),
                                                           const bool crossTerms = false) {
  constexpr int npeaks{5};
  const std::vector<double> ub = {0.269, -0.01, 0.033, 0.081, -0.191, -0.039, 0.279, 0.347, -0.02};
  GNU_DIAG_OFF("missing-braces");
  constexpr std::array<MinimalPeak, npeaks> testPeaksInfo = {
      MinimalPeak{1, V3D(-3.691, -0.694, 3.762)},     // main
      MinimalPeak{2, V3D(-1.234, -0.225, 1.25212)},   // satellite
      MinimalPeak{1, V3D(-3.824, 0.728, 1.711)},      // main
      MinimalPeak{1, V3D(0.872, -0.1998, 2.7476)},    // satellite
      MinimalPeak{2, V3D(-1.54093, 0.129343, 1.445)}, // satellite
  };
  GNU_DIAG_ON("missing-braces");
  return createPeaksWorkspace<npeaks>(testPeaksInfo, ub, maxOrder, modVectors, crossTerms);
}

std::unique_ptr<IndexPeaks> indexPeaks(const IPeaksWorkspace_sptr &peaksWS,
                                       const std::unordered_map<std::string, std::string> &arguments) {
  auto alg = std::make_unique<IndexPeaks>();
  alg->setChild(true);
  alg->initialize();
  alg->setProperty("PeaksWorkspace", peaksWS);
  for (const auto &argument : arguments) {
    alg->setPropertyValue(argument.first, argument.second);
  }
  alg->execute();
  return alg;
}

} // namespace

class IndexPeaksTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    IndexPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_empty_peaks_workspace_indexes_nothing() {
    auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(0);
    peaksWS->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>());

    auto alg = indexPeaks(peaksWS, {{"Tolerance", "0.1"}});

    TS_ASSERT(alg->isExecuted())
    assertNumberPeaksIndexed(*alg, 0, 0, 0);
  }

  void test_no_commonUB_optimizes_UB_per_run_for_no_satellites() {
    const auto ws = createTestPeaksWorkspaceMainReflOnly();

    auto alg = indexPeaks(ws, {{"Tolerance", "0.1"}, {"RoundHKLs", "1"}, {"CommonUBForAll", "0"}});

    // Check the output properties
    assertNumberPeaksIndexed(*alg, 5, 5, 0);
    assertErrorsAsExpected(*alg, 0.00883, 0.00883, 0.);

    // spot check a few peaks for
    // fractional Miller indices
    V3D peak_0_hkl(4, 1, 6);
    V3D peak_1_hkl(3, -1, 4);
    V3D peak_2_hkl(4, -1, 5);
    V3D peak_3_hkl(3, -0, 7);
    V3D peak_4_hkl(2, -4, 7);

    const auto &peaks = ws->getPeaks();
    V3D error = peak_0_hkl - peaks[0].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 2e-4)

    error = peak_1_hkl - peaks[1].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_2_hkl - peaks[2].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_3_hkl - peaks[3].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_4_hkl - peaks[4].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 2e-4)
  }

  void test_tolerance_main_refl() {
    const auto ws = createTestPeaksWorkspaceMainReflOnly();
    auto alg = indexPeaks(ws, {{"Tolerance", "0.02"}, {"RoundHKLs", "0"}});

    // Check the output properties
    assertNumberPeaksIndexed(*alg, 3, 3, 0);

    // spot check a few peaks for
    // fractional Miller indices
    const V3D peak_0_hkl_d(0.0, 0.0, 0.0); // first peak
    const V3D peak_1_hkl_d(3, -1, 4);
    const V3D peak_2_hkl_d(4, -1, 5);
    const V3D peak_3_hkl_d(3, -0, 7);
    const V3D peak_4_hkl_d(0.0, 0.0, 0.0); // last peak

    const auto &peaks = ws->getPeaks();
    V3D error = peak_0_hkl_d - peaks[0].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 2e-4)

    error = peak_1_hkl_d - peaks[1].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_2_hkl_d - peaks[2].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_3_hkl_d - peaks[3].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_4_hkl_d - peaks[4].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 2e-4)
  }

  void test_no_roundHKLS_leaves_peaks_as_found() {
    const auto ws = createTestPeaksWorkspaceMainReflOnly();
    auto alg = indexPeaks(ws, {{"Tolerance", "0.1"}, {"RoundHKLs", "0"}});

    // Check the output properties
    assertNumberPeaksIndexed(*alg, 5, 5, 0);
    assertErrorsAsExpected(*alg, 0.0088322, 0.0088322, 0.);

    // spot check a few peaks for
    // fractional Miller indices
    const V3D peak_0_hkl_d(4.03064, 0.988507, 6.01094); // first peak
    const V3D peak_1_hkl_d(3, -1, 4);
    const V3D peak_2_hkl_d(4, -1, 5);
    const V3D peak_3_hkl_d(3, -0, 7);
    const V3D peak_4_hkl_d(1.97067, -4.02836, 6.97828); // last peak

    const auto &peaks = ws->getPeaks();
    V3D error = peak_0_hkl_d - peaks[0].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 2e-4)

    error = peak_1_hkl_d - peaks[1].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_2_hkl_d - peaks[2].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_3_hkl_d - peaks[3].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-4)

    error = peak_4_hkl_d - peaks[4].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 2e-4)
  }

  void test_roundHKLS_gives_integer_miller_indices() {
    const auto ws = createTestPeaksWorkspaceMainReflOnly();
    const auto mainToleranceAsStr = "0.1";
    auto alg = indexPeaks(ws, {{"Tolerance", mainToleranceAsStr}, {"RoundHKLs", "1"}});
    // Check that the peaks were all indexed
    const double mainTolerance{std::stod(mainToleranceAsStr)};
    auto &peaks = ws->getPeaks();
    for (const auto &peak : peaks) {
      TS_ASSERT(IndexingUtils::ValidIndex(peak.getHKL(), mainTolerance))
    }

    // Check the output properties
    assertNumberPeaksIndexed(*alg, 5, 5, 0);
    assertErrorsAsExpected(*alg, 0.0088322, 0.0088322, 0.);

    // spot check a few peaks for
    // integer Miller indices
    V3D peak_0_hkl(4, 1, 6);
    V3D peak_1_hkl(3, -1, 4);
    V3D peak_2_hkl(4, -1, 5);
    V3D peak_3_hkl(3, 0, 7);
    V3D peak_4_hkl(2, -4, 7); // last peak

    auto error = peak_0_hkl - peaks[0].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-10)

    error = peak_1_hkl - peaks[1].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-10)

    error = peak_2_hkl - peaks[2].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-10)

    error = peak_3_hkl - peaks[3].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-10)

    error = peak_4_hkl - peaks[4].getHKL();
    TS_ASSERT_DELTA(error.norm(), 0.0, 1e-10)
  }

  void test_zero_satellite_tol_only_indexes_main_refl_with_modvectors_from_ub() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(1, {V3D(0.333, -0.667, 0.333)});
    const auto mainTolerance{"0.1"}, sateTolerance{"0."};

    const auto alg = indexPeaks(peaksWS, {{"Tolerance", mainTolerance}, {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 2, 2, 0);
    assertIndexesAsExpected(*peaksWS, {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10), V3D(0, 0, 0), V3D(0, 0, 0)});
  }

  void test_zero_maxorder_on_ub_indexes_main_refl_only() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(0, std::vector<V3D>());
    const auto mainTolerance{"0.1"}, sateTolerance{"1."};

    const auto alg = indexPeaks(peaksWS, {{"Tolerance", mainTolerance}, {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 2, 2, 0);
    assertIndexesAsExpected(*peaksWS, {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10), V3D(0, 0, 0), V3D(0, 0, 0)});
  }

  void test_exec_with_common_ub_and_zero_mod_vectors_indexes_main_refl_only() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(0, std::vector<V3D>());
    const auto sateTolerance{"1."};

    const auto alg = indexPeaks(peaksWS, {{"CommonUBForAll", "1"}, {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 2, 2, 0);
    assertIndexesAsExpected(*peaksWS, {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10), V3D(0, 0, 0), V3D(0, 0, 0)});
    assertErrorsAsExpected(*alg, 0.0140447, 0.0140447, 0.);
  }

  void test_exec_with_common_ub_and_non_zero_mod_vectors_indexes_both_with_roundhkl() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(1, {V3D(0.333, -0.667, 0.333)});
    const auto sateTolerance{"0.4"};

    const auto alg = indexPeaks(peaksWS, {{"CommonUBForAll", "1"}, {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 5, 2, 3);
    const std::vector<V3D> expectedHKL{V3D(-1, 2, -9), V3D(-0.667, 0.333, -2.667), V3D(-1, 1, -10),
                                       V3D(0.333, 0.333, -0.667), V3D(-0.333, 0.667, -4.667)};
    const std::vector<V3D> expectedIntHKL{V3D(-1, 2, -9), V3D(-1, 1, -3), V3D(-1, 1, -10), V3D(0, 1, -1),
                                          V3D(-1, 1, -5)};

    assertIndexesAsExpected(*peaksWS, expectedHKL, expectedIntHKL);
    assertErrorsAsExpected(*alg, 0.1995736054, 0.0140447, 0.32325956);
  }

  void test_exec_with_common_ub_and_non_zero_mod_vectors_indexes_both_no_roundhkl() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(1, {V3D(0.333, -0.667, 0.333)});
    const auto sateTolerance{"0.4"};

    const auto alg =
        indexPeaks(peaksWS, {{"CommonUBForAll", "1"}, {"ToleranceForSatellite", sateTolerance}, {"RoundHKLs", "0"}});

    assertNumberPeaksIndexed(*alg, 5, 2, 3);
    const std::vector<V3D> expectedHKL{V3D(-0.997659, 2.00538, -9.06112), V3D(-0.332659, 0.666681, -3.03773),
                                       V3D(-0.998436, 1.01132, -9.99745), V3D(0.668401, 0.662732, -1.04211),
                                       V3D(-0.333695, 0.671228, -4.50819)};
    const std::vector<V3D> expectedIntHKL{V3D(-1, 2, -9), V3D(-1, 1, -3), V3D(-1, 1, -10), V3D(0, 1, -1),
                                          V3D(-1, 1, -5)};
    assertIndexesAsExpected(*peaksWS, expectedHKL, expectedIntHKL);
    assertErrorsAsExpected(*alg, 0.1995736054, 0.0140447, 0.3232596);
  }

  void test_exec_no_common_ub_and_non_zero_mod_vectors_indexes_both_no_roundhkl() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(1, {V3D(0.333, -0.667, 0.333)});
    const auto sateTolerance{"0.4"};

    const auto alg = indexPeaks(
        peaksWS,
        {{"CommonUBForAll", "0"}, {"Tolerance", "0.15"}, {"ToleranceForSatellite", sateTolerance}, {"RoundHKLs", "0"}});

    assertNumberPeaksIndexed(*alg, 5, 2, 3);
    const std::vector<V3D> expectedHKL{V3D(-0.997659, 2.00538, -9.06112), V3D(-0.332659, 0.666681, -3.03773),
                                       V3D(-0.998436, 1.01132, -9.99745), V3D(0.668401, 0.662732, -1.04211),
                                       V3D(-0.333695, 0.671228, -4.50819)};
    const std::vector<V3D> expectedIntHKL{V3D(-1, 2, -9), V3D(-1, 1, -3), V3D(-1, 1, -10), V3D(0, 1, -1),
                                          V3D(-1, 1, -5)};
    assertIndexesAsExpected(*peaksWS, expectedHKL, expectedIntHKL);
    assertErrorsAsExpected(*alg, 0.1995736, 0.0140447, 0.323256);
  }

  void test_exec_with_three_mod_vectors_no_cross_terms() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(
        1, {V3D(-0.1, 0.1, 0.1), V3D(0.1, 0.2, -0.3), V3D(0.333, 0.667, -0.333)});

    const auto alg =
        indexPeaks(peaksWS, {{"CommonUBForAll", "0"}, {"ToleranceForSatellite", "0.4"}, {"RoundHKLs", "0"}});

    assertNumberPeaksIndexed(*alg, 5, 2, 3);
    const std::vector<V3D> expectedHKL{V3D(-0.997659, 2.00538, -9.06112), V3D(-0.332659, 0.666681, -3.03773),
                                       V3D(-0.998436, 1.01132, -9.99745), V3D(0.668401, 0.662732, -1.04211),
                                       V3D(-0.333695, 0.671228, -4.50819)};
    const std::vector<V3D> expectedIntHKL{V3D(-1, 2, -9), V3D(-1, -0, -3), V3D(-1, 1, -10), V3D(0, -0, -1),
                                          V3D(-1, 0, -4)};
    assertIndexesAsExpected(*peaksWS, expectedHKL, expectedIntHKL);
    assertErrorsAsExpected(*alg, 0.12383213164, 0.0140447, 0.1970237718);
  }

  void test_exec_with_three_mod_vectors_and_cross_terms_from_lattice() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites(
        1, {V3D(0.333, 0.667, -0.333), V3D(-0.333, 0.667, 0.333), V3D(0.333, -0.667, 0.333)}, true);

    const auto alg =
        indexPeaks(peaksWS, {{"CommonUBForAll", "0"}, {"ToleranceForSatellite", "0.2"}, {"RoundHKLs", "0"}});

    assertThreeModVectorWithCross(*alg, *peaksWS);
  }

  void test_exec_with_three_mod_vectors_and_cross_terms_from_alg_input_no_lattice_update() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites();

    const auto alg = indexPeaks(peaksWS, {{"CommonUBForAll", "0"},
                                          {"ToleranceForSatellite", "0.2"},
                                          {"RoundHKLs", "0"},
                                          {"MaxOrder", "1"},
                                          {"ModVector1", "0.333, 0.667, -0.333"},
                                          {"ModVector2", "-0.333, 0.667, 0.333"},
                                          {"ModVector3", "0.333, -0.667, 0.333"},
                                          {"CrossTerms", "1"}});

    assertThreeModVectorWithCross(*alg, *peaksWS);
    const auto &lattice = peaksWS->sample().getOrientedLattice();
    TS_ASSERT_EQUALS(0, lattice.getMaxOrder())
    TS_ASSERT_EQUALS(false, lattice.getCrossTerm())
    TS_ASSERT_DELTA(0.0, lattice.getModUB()[0][0], 1e-6)
  }

  void test_exec_with_three_mod_vectors_and_cross_terms_from_alg_input_with_lattice_update() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites();

    const auto alg = indexPeaks(peaksWS, {{"CommonUBForAll", "0"},
                                          {"ToleranceForSatellite", "0.2"},
                                          {"RoundHKLs", "0"},
                                          {"MaxOrder", "1"},
                                          {"ModVector1", "0.333, 0.667, -0.333"},
                                          {"ModVector2", "-0.333, 0.667, 0.333"},
                                          {"ModVector3", "0.333, -0.667, 0.333"},
                                          {"CrossTerms", "1"},
                                          {"SaveModulationInfo", "1"}});

    assertThreeModVectorWithCross(*alg, *peaksWS);
    const auto &lattice = peaksWS->sample().getOrientedLattice();
    TS_ASSERT_EQUALS(1, lattice.getMaxOrder())
    TS_ASSERT_DELTA(V3D(0.333, 0.667, -0.333).norm(), lattice.getModVec(0).norm(), 1e-8)
    TS_ASSERT_DELTA(V3D(-0.333, 0.667, 0.333).norm(), lattice.getModVec(1).norm(), 1e-8)
    TS_ASSERT_DELTA(V3D(0.333, -0.667, 0.333).norm(), lattice.getModVec(2).norm(), 1e-8)
    TS_ASSERT_EQUALS(true, lattice.getCrossTerm())
    TS_ASSERT_DELTA(0.071918, lattice.getModUB()[0][0], 1e-6)
  }

  void test_exec_mod_vectors_save_cleared_across_runs() {
    const auto peakWS = createTestPeaksWorkspaceWithSatellites();

    const auto alg_three_mod = indexPeaks(peakWS, {{"RoundHKLs", "0"},
                                                   {"MaxOrder", "1"},
                                                   {"ModVector1", "0.333, 0.667, -0.333"},
                                                   {"ModVector2", "-0.333, 0.667, 0.333"},
                                                   {"ModVector3", "0.333, -0.667, 0.333"},
                                                   {"SaveModulationInfo", "1"}});

    // Repeat indexPeaks with 1 mod vector, verify lattice only has one set
    const auto alg_one_mod = indexPeaks(
        peakWS,
        {{"RoundHKLs", "0"}, {"MaxOrder", "1"}, {"ModVector1", "0.333, 0.667, -0.333"}, {"SaveModulationInfo", "1"}});

    const auto &lattice = peakWS->sample().getOrientedLattice();
    TS_ASSERT_EQUALS(1, lattice.getMaxOrder())
    TS_ASSERT_DELTA(V3D(0.333, 0.667, -0.333).norm(), lattice.getModVec(0).norm(), 1e-8)
    TS_ASSERT_DELTA(V3D(0.0, 0.0, 0.0).norm(), lattice.getModVec(1).norm(), 1e-8)
    TS_ASSERT_DELTA(V3D(0.0, 0.0, 0.0).norm(), lattice.getModVec(2).norm(), 1e-8)
  }

  void test_exec_compare_after_modvec_changes() {
    const std::vector<double> ub = {0.0971, 0.1179, 0.0433, 0.1056, -0.0305, -0.0190, 0.0311, 0.0820, -0.0698};

    constexpr int npeaks{5};
    constexpr int run{39056};
    // peak numbers 7, 25, 66, 72, and 76 from TOPAZ_39056
    constexpr std::array<MinimalPeak, npeaks> testPeaksInfo = {
        MinimalPeak{run, V3D(-1.44683, 1.07978, 2.06781)}, MinimalPeak{run, V3D(-1.70476, 1.12522, 2.46955)},
        MinimalPeak{run, V3D(1.22626, 1.20112, 4.88867)}, MinimalPeak{run, V3D(0.934964, 1.19063, 4.15398)},
        MinimalPeak{run, V3D(1.19216, 1.75244, 4.69207)}};

    const auto peakWS_onemod = createPeaksWorkspace<npeaks>(testPeaksInfo, ub);
    const auto peakWS = createPeaksWorkspace<npeaks>(testPeaksInfo, ub);

    // Index with one modulation vector on the first peaks workspace
    const auto alg_one_mod =
        indexPeaks(peakWS_onemod,
                   {{"RoundHKLs", "0"}, {"MaxOrder", "1"}, {"ModVector1", "0.5, 0.5, 0"}, {"SaveModulationInfo", "1"}});

    // Re-index using two modulation vectors
    const auto alg_two_mod = indexPeaks(peakWS, {{"RoundHKLs", "0"},
                                                 {"MaxOrder", "1"},
                                                 {"ModVector1", "0.5, 0, 0"},
                                                 {"ModVector2", "0, 0.5, 0"},
                                                 {"SaveModulationInfo", "1"}});

    // Re-index with one modulation vector, and compare to before
    const auto alg = indexPeaks(
        peakWS, {{"RoundHKLs", "0"}, {"MaxOrder", "1"}, {"ModVector1", "0.5, 0.5, 0"}, {"SaveModulationInfo", "1"}});

    for (int i = 0; i < peakWS->getNumberPeaks(); ++i) {
      const auto &peak_onemod = peakWS_onemod->getPeak(i);
      const auto &peak = peakWS->getPeak(i);
      // Verify the HKL and MNP of each peak since these would change if the
      // last indexPeaks call was using a second modulation vector
      TS_ASSERT(peak_onemod.getHKL() == peak.getHKL());
      TS_ASSERT(peak_onemod.getIntHKL() == peak.getIntHKL());
      TS_ASSERT(peak_onemod.getIntMNP() == peak.getIntMNP());
    }
  }

  // --------------------------- Failure tests -----------------------------

  std::shared_ptr<IndexPeaks> setup_validate_inputs_test_alg() {
    const auto peaksWS = createTestPeaksWorkspaceWithSatellites();
    auto alg = std::make_shared<IndexPeaks>();
    alg->initialize();
    alg->setProperty("PeaksWorkspace", peaksWS);
    return alg;
  }

  void assert_helpmsgs_error_from_validate_inputs(std::shared_ptr<IndexPeaks> alg, std::string prop,
                                                  std::string err_substring) {
    auto helpMsgs = alg->validateInputs();

    const auto valueIter = helpMsgs.find(prop);
    TS_ASSERT(valueIter != helpMsgs.cend())
    if (valueIter != helpMsgs.cend()) {
      const auto msg = valueIter->second;
      TS_ASSERT(msg.find(err_substring) != std::string::npos)
    }
  }

  void test_inputs_with_save_mod_and_zero_max_order_gives_validateInputs_error() {
    auto alg = setup_validate_inputs_test_alg();
    alg->setProperty("SaveModulationInfo", true);

    assert_helpmsgs_error_from_validate_inputs(alg, "MaxOrder", "Modulation info");
  }

  void test_inputs_with_save_mod_and_empty_mod_vectors_gives_validateInputs_error() {
    auto alg = setup_validate_inputs_test_alg();
    alg->setProperty("SaveModulationInfo", true);

    assert_helpmsgs_error_from_validate_inputs(alg, "SaveModulationInfo", "no valid Modulation");
  }

  void test_inputs_with_max_order_set_and_empty_mod_vectors_gives_validateInputs_error() {
    auto alg = setup_validate_inputs_test_alg();
    alg->setProperty("MaxOrder", 3); // arbitary non-zero max order

    assert_helpmsgs_error_from_validate_inputs(alg, "ModVector1", "At least one Modulation");
  }

  void test_inputs_with_zero_max_order_and_valid_mod_vector_gives_validateInputs_error() {
    auto alg = setup_validate_inputs_test_alg();
    alg->setProperty("ModVector1", std::vector<double>{1.0, 2.0, 3.0}); // arbitary non-zero mod vector

    assert_helpmsgs_error_from_validate_inputs(alg, "MaxOrder", "cannot be zero");
  }

  void test_workspace_with_no_oriented_lattice_gives_validateInputs_error() {
    const auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(1);
    auto alg = std::make_shared<IndexPeaks>();
    alg->initialize();
    alg->setProperty("PeaksWorkspace", peaksWS);

    assert_helpmsgs_error_from_validate_inputs(alg, "PeaksWorkspace", "No UB Matrix");
  }

  void test_negative_max_order_throws_error() {
    IndexPeaks alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.setProperty("MaxOrder", -1), const std::invalid_argument &)
  }

  void test_modvector_with_list_length_not_three_throws() {
    IndexPeaks alg;
    alg.initialize();

    for (const auto &propName : {"ModVector1", "ModVector2", "ModVector3"}) {
      TS_ASSERT_THROWS(alg.setProperty(propName, "0"), std::invalid_argument &)
      TS_ASSERT_THROWS(alg.setProperty(propName, "0,0"), std::invalid_argument &)
      TS_ASSERT_THROWS(alg.setProperty(propName, "0,0,0,0"), std::invalid_argument &)
    }
  }

  void test_LeanElasticPeak() {
    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();
    auto ws = std::make_shared<Mantid::DataObjects::LeanElasticPeaksWorkspace>();
    ws->mutableSample().setOrientedLattice(std::move(lattice));
    ws->addPeak(Mantid::DataObjects::LeanElasticPeak(Mantid::Kernel::V3D(2 * M_PI, 0, 0), 1.));
    ws->addPeak(Mantid::DataObjects::LeanElasticPeak(Mantid::Kernel::V3D(0, 4 * M_PI, 0), 1.));

    auto alg = indexPeaks(ws, {});
    TS_ASSERT(alg->isExecuted())
    int numberIndexed = alg->getProperty("NumIndexed");
    TS_ASSERT_EQUALS(numberIndexed, 2)

    TS_ASSERT_DELTA(ws->getPeak(0).getH(), 1, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(0).getK(), 0, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(0).getL(), 0, 1e-9)

    TS_ASSERT_DELTA(ws->getPeak(1).getH(), 0, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(1).getK(), 2, 1e-9)
    TS_ASSERT_DELTA(ws->getPeak(1).getL(), 0, 1e-9)
  }

private:
  void assertThreeModVectorWithCross(const IndexPeaks &alg, const PeaksWorkspace_sptr::element_type &peaksWS) {
    assertNumberPeaksIndexed(alg, 5, 2, 3);
    const std::vector<V3D> expectedHKL{V3D(-0.997659, 2.00538, -9.06112), V3D(-0.332659, 0.666681, -3.03773),
                                       V3D(-0.998436, 1.01132, -9.99745), V3D(0.668401, 0.662732, -1.04211),
                                       V3D(-0.333695, 0.671228, -4.50819)};
    const std::vector<V3D> expectedIntHKL{V3D(-1, 2, -9), V3D(-1, 2, -3), V3D(-1, 1, -10), V3D(0, 2, -1),
                                          V3D(-0, 0, -5)};
    assertIndexesAsExpected(peaksWS, expectedHKL, expectedIntHKL);
    assertErrorsAsExpected(alg, 0.02236871, 0.0140447, 0.027918076);
  }

  // Check that all main indexed peaks match as expected
  void assertIndexesAsExpected(const PeaksWorkspace_sptr::element_type &peaksWS, const std::vector<V3D> &expectedHKL) {
    assert(static_cast<size_t>(peaksWS.getNumberPeaks()) == expectedHKL.size());
    for (auto i = 0u; i < expectedHKL.size(); ++i) {
      const auto &expectedIndex = expectedHKL[i];
      TSM_ASSERT_DELTA("Unexpected index for HKL at index " + std::to_string(i), expectedIndex.norm(),
                       peaksWS.getPeak(i).getHKL().norm(), 1e-5)
      const V3D expectedIntHKL{std::round(expectedIndex[0]), std::round(expectedIndex[1]),
                               std::round(expectedIndex[2])};
      TSM_ASSERT_EQUALS("Unexpected index "
                        "for IntHKL at "
                        "index " +
                            std::to_string(i),
                        expectedIntHKL, peaksWS.getPeak(i).getIntHKL())
    }
  }

  // Check that all main/satellite indexed peaks match as expected
  void assertIndexesAsExpected(const PeaksWorkspace_sptr::element_type &peaksWS, const std::vector<V3D> &expectedHKL,
                               const std::vector<V3D> &expectedIntHKL) {
    assert(static_cast<size_t>(peaksWS.getNumberPeaks()) == expectedHKL.size());
    assert(expectedIntHKL.size() == expectedHKL.size());

    for (auto i = 0u; i < expectedHKL.size(); ++i) {
      TSM_ASSERT_DELTA("Unexpected index for HKL at index " + std::to_string(i), expectedHKL[i].norm(),
                       peaksWS.getPeak(i).getHKL().norm(), 1e-5)
      TSM_ASSERT_DELTA("Unexpected index "
                       "for IntHKL at "
                       "index " +
                           std::to_string(i),
                       expectedIntHKL[i].norm(), peaksWS.getPeak(i).getIntHKL().norm(), 1e-8)
    }
  }

  void assertNumberPeaksIndexed(const IndexPeaks &alg, const int numIndexedExpected, const int mainNumIndexedExpected,
                                const int sateNumIndexedExpected) {
    TS_ASSERT_EQUALS(numIndexedExpected, static_cast<int>(alg.getProperty("NumIndexed")))
    TS_ASSERT_EQUALS(mainNumIndexedExpected, static_cast<int>(alg.getProperty("MainNumIndexed")))
    TS_ASSERT_EQUALS(sateNumIndexedExpected, static_cast<int>(alg.getProperty("SateNumIndexed")))
  }

  void assertErrorsAsExpected(const IndexPeaks &alg, const double expectedAverageError, const double expectedMainError,
                              const double expectedSatError) {
    TS_ASSERT_DELTA(expectedAverageError, static_cast<double>(alg.getProperty("AverageError")), 1e-5);
    TS_ASSERT_DELTA(expectedMainError, static_cast<double>(alg.getProperty("MainError")), 1e-5);
    TS_ASSERT_DELTA(expectedSatError, static_cast<double>(alg.getProperty("SatelliteError")), 1e-5);
  }
};
