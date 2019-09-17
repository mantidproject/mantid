// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_INDEX_PEAKS_TEST_H_
#define MANTID_CRYSTAL_INDEX_PEAKS_TEST_H_

#include "MantidAPI/Sample.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

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
PeaksWorkspace_sptr
createPeaksWorkspace(const MinimalPeaksList<NPeaks> &testPeaksInfo,
                     const std::vector<double> &ub, const int maxOrder = -1,
                     const std::vector<V3D> &modVectors = std::vector<V3D>()) {
  auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(NPeaks);
  OrientedLattice lattice;
  lattice.setUB(ub);
  if (maxOrder > 0) {
    lattice.setMaxOrder(maxOrder);
    if (modVectors.empty() || modVectors.size() > 3)
      throw std::runtime_error("Only <= 3 modulation vectors can be provided.");
    auto modVecOrDefault = [&modVectors](const size_t index) {
      if (index < modVectors.size())
        return modVectors[index];
      else
        return V3D();
    };
    lattice.setModVec1(modVecOrDefault(0));
    lattice.setModVec2(modVecOrDefault(1));
    lattice.setModVec3(modVecOrDefault(2));
  }
  peaksWS->mutableSample().setOrientedLattice(&lattice);

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
  const std::vector<double> ub = {-0.0122354, 0.00480056,  -0.0860404,
                                  0.1165450,  0.00178145,  0.0045884,
                                  0.0273738,  -0.08973560, 0.0252595};
  // peaks from TOPAZ_3007.peaks: 0, 1, 2, 10, 42 with sign for Q swapped
  // as we don't use the crystallographic convention
  constexpr std::array<MinimalPeak, npeaks> testPeaksInfo = {
      MinimalPeak{3007, V3D(-3.52961, 3.13589, 1.0899)},
      MinimalPeak{3007, V3D(-2.42456, 2.29581, 1.71147)},
      MinimalPeak{3007, V3D(-3.04393, 3.05739, 2.03727)},
      MinimalPeak{3007, V3D(-4.02271, 2.4073, 1.62228)},
      MinimalPeak{3007, V3D(-4.04552, 1.59916, 3.71776)}};
  return createPeaksWorkspace<npeaks>(testPeaksInfo, ub);
}

PeaksWorkspace_sptr
createTestPeaksWorkspaceWithSatellites(const int maxOrder,
                                       const std::vector<V3D> &modVectors) {
  constexpr int npeaks{5};
  const std::vector<double> ub = {0.269,  -0.01, 0.033, 0.081, -0.191,
                                  -0.039, 0.279, 0.347, -0.02};
  constexpr std::array<MinimalPeak, npeaks> testPeaksInfo = {
      MinimalPeak{1, V3D(-3.691, -0.694, 3.762)},     // main
      MinimalPeak{1, V3D(-1.234, -0.225, 1.25212)},   // satellite
      MinimalPeak{1, V3D(-3.824, 0.728, 1.711)},      // main
      MinimalPeak{1, V3D(0.872, -0.1998, 2.7476)},    // satellite
      MinimalPeak{1, V3D(-1.54093, 0.129343, 1.445)}, // satellite
  };
  return createPeaksWorkspace<npeaks>(testPeaksInfo, ub, maxOrder, modVectors);
}

std::unique_ptr<IndexPeaks>
indexPeaks(PeaksWorkspace_sptr peaksWS,
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
    OrientedLattice lattice;
    peaksWS->mutableSample().setOrientedLattice(&lattice);

    auto alg = indexPeaks(peaksWS, {{"Tolerance", "0.1"}});

    TS_ASSERT(alg->isExecuted())
    assertNumberPeaksIndexed(*alg, 0, 0, 0);
  }

  void test_no_commonUB_optimizes_UB_per_run_for_no_satellites() {
    const auto ws = createTestPeaksWorkspaceMainReflOnly();
    // Change some run numbers
    ws->getPeak(0).setRunNumber(3008);
    ws->getPeak(4).setRunNumber(3008);

    auto alg = indexPeaks(
        ws,
        {{"Tolerance", "0.1"}, {"RoundHKLs", "1"}, {"CommonUBForAll", "0"}});

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

  void test_no_roundHKLS_leaves_peaks_as_found() {
    const auto ws = createTestPeaksWorkspaceMainReflOnly();
    auto alg = indexPeaks(ws, {{"Tolerance", "0.1"}, {"RoundHKLs", "0"}});

    // Check the output properties
    assertNumberPeaksIndexed(*alg, 5, 5, 0);
    assertErrorsAsExpected(*alg, 0.00639, 0.00639, 0.);

    // spot check a few peaks for
    // fractional Miller indices
    const V3D peak_0_hkl_d(4.00682, 0.97956, 5.99368); // first peak
    const V3D peak_1_hkl_d(2.99838, -0.99760, 4.00141);
    const V3D peak_2_hkl_d(3.99737, -0.99031, 5.00250);
    const V3D peak_3_hkl_d(2.99419, 0.01736, 7.00538);
    const V3D peak_4_hkl_d(2.00277, -4.00813, 6.99744); // last peak

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
    auto alg =
        indexPeaks(ws, {{"Tolerance", mainToleranceAsStr}, {"RoundHKLs", "1"}});
    // Check that the peaks were all indexed
    const double mainTolerance{std::stod(mainToleranceAsStr)};
    auto &peaks = ws->getPeaks();
    for (const auto &peak : peaks) {
      TS_ASSERT(IndexingUtils::ValidIndex(peak.getHKL(), mainTolerance))
    }

    // Check the output properties
    assertNumberPeaksIndexed(*alg, 5, 5, 0);
    assertErrorsAsExpected(*alg, 0.00639, 0.00639, 0.);

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

  void
  test_zero_satellite_tol_only_indexes_main_refl_with_modvectors_from_ub() {
    const auto peaksWS =
        createTestPeaksWorkspaceWithSatellites(1, {V3D(0.333, -0.667, 0.333)});
    const auto mainTolerance{"0.1"}, sateTolerance{"0."};

    const auto alg =
        indexPeaks(peaksWS, {{"Tolerance", mainTolerance},
                             {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 2, 2, 0);
    std::cerr << "\n";
    for (int i = 0; i < peaksWS->getNumberPeaks(); ++i) {
      std::cerr << peaksWS->getPeak(i).getHKL() << "\n";
    }
    assertIndexesAsExpected(*peaksWS,
                            {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10),
                             V3D(0, 0, 0), V3D(0, 0, 0)});
  }

  void test_zero_maxorder_on_ub_indexes_main_refl_only() {
    const auto peaksWS =
        createTestPeaksWorkspaceWithSatellites(0, std::vector<V3D>());
    const auto mainTolerance{"0.1"}, sateTolerance{"1."};

    const auto alg =
        indexPeaks(peaksWS, {{"Tolerance", mainTolerance},
                             {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 2, 2, 0);
    assertIndexesAsExpected(*peaksWS,
                            {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10),
                             V3D(0, 0, 0), V3D(0, 0, 0)});
  }

  void test_exec_with_common_ub_and_zero_mod_vectors_indexes_main_refl_only() {
    const auto peaksWS =
        createTestPeaksWorkspaceWithSatellites(0, std::vector<V3D>());
    const auto sateTolerance{"1."};

    const auto alg =
        indexPeaks(peaksWS, {{"CommonUBForAll", "1"},
                             {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 2, 2, 0);
    assertIndexesAsExpected(*peaksWS,
                            {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10),
                             V3D(0, 0, 0), V3D(0, 0, 0)});
    assertErrorsAsExpected(*alg, 0.0140447, 0.0140447, 0.);
  }

  void xtest_exec_with_common_ub_and_non_zero_mod_vectors_indexes_both() {
    const auto peaksWS =
        createTestPeaksWorkspaceWithSatellites(1, {V3D(0.333, -0.667, 0.333)});
    const auto sateTolerance{"1."};

    const auto alg =
        indexPeaks(peaksWS, {{"CommonUBForAll", "1"},
                             {"ToleranceForSatellite", sateTolerance}});

    assertNumberPeaksIndexed(*alg, 5, 2, 3);
    assertIndexesAsExpected(*peaksWS,
                            {V3D(-1, 2, -9), V3D(0, 0, 0), V3D(-1, 1, -10),
                             V3D(0, 0, 0), V3D(0, 0, 0)});
    assertErrorsAsExpected(*alg, 0.0140447, 0.0140447, 0.);
  }

  // --------------------------- Failure tests -----------------------------

  void test_workspace_with_no_oriented_lattice_gives_validateInputs_error() {
    const auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(1);
    IndexPeaks alg;
    alg.initialize();
    alg.setProperty("PeaksWorkspace", peaksWS);

    auto helpMsgs = alg.validateInputs();

    const auto valueIter = helpMsgs.find("PeaksWorkspace");
    TS_ASSERT(valueIter != helpMsgs.cend())
    if (valueIter != helpMsgs.cend()) {
      const auto msg = valueIter->second;
      TS_ASSERT(msg.find("OrientedLattice") != std::string::npos)
    }
  }

private:
  void assertIndexesAsExpected(const PeaksWorkspace_sptr::element_type &peaksWS,
                               const std::vector<V3D> &expectedIndexes) {
    assert(static_cast<size_t>(peaksWS.getNumberPeaks()) ==
           expectedIndexes.size());
    for (auto i = 0u; i < expectedIndexes.size(); ++i) {
      TS_ASSERT_EQUALS(expectedIndexes[i], peaksWS.getPeak(i).getHKL())
    }
  }

  void assertNumberPeaksIndexed(const IndexPeaks &alg,
                                const int numIndexedExpected,
                                const int mainNumIndexedExpected,
                                const int sateNumIndexedExpected) {
    TS_ASSERT_EQUALS(numIndexedExpected,
                     static_cast<int>(alg.getProperty("NumIndexed")))
    TS_ASSERT_EQUALS(mainNumIndexedExpected,
                     static_cast<int>(alg.getProperty("MainNumIndexed")))
    TS_ASSERT_EQUALS(sateNumIndexedExpected,
                     static_cast<int>(alg.getProperty("SateNumIndexed")))
  }

  void assertErrorsAsExpected(const IndexPeaks &alg,
                              const double expectedAverageError,
                              const double expectedMainError,
                              const double expectedSatError) {
    TS_ASSERT_DELTA(expectedAverageError,
                    static_cast<double>(alg.getProperty("AverageError")), 1e-5);
    TS_ASSERT_DELTA(expectedMainError,
                    static_cast<double>(alg.getProperty("MainError")), 1e-5);
    TS_ASSERT_DELTA(expectedSatError,
                    static_cast<double>(alg.getProperty("SatelliteError")),
                    1e-5);
  }
};

#endif /* MANTID_CRYSTAL_INDEX_PEAKS_TEST_H_ */
