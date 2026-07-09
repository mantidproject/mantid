// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/EstimatePeakIntensities.h"

#include <cmath>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

namespace {

/// 7-bin histogram (edges 0..7) shared by both spectra; spectrum 0 is a flat background of 2 with a
/// single peak point of 10 at x=3, spectrum 1 is all zeros (no positive data).
MatrixWorkspace_sptr makeInputWS() {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 2, 8, 7);
  const std::vector<double> edges{0., 1., 2., 3., 4., 5., 6., 7.};
  const std::vector<double> peakY{2., 2., 2., 10., 2., 2., 2.};
  for (size_t sp = 0; sp < 2; ++sp) {
    ws->mutableX(sp) = edges;
    for (size_t b = 0; b < 7; ++b) {
      ws->mutableY(sp)[b] = (sp == 0) ? peakY[b] : 0.0;
      ws->mutableE(sp)[b] = 1.0;
    }
  }
  return ws;
}

/// Window workspace (2 spectra) in the FitPeaks convention: each spectrum holds the same X list
/// [min0, max0, min1, max1, ...] (2 per peak).
MatrixWorkspace_sptr makeWindowWS(const std::vector<double> &winX) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 2, winX.size(), winX.size() - 1);
  for (size_t sp = 0; sp < 2; ++sp)
    ws->mutableX(sp) = winX;
  return ws;
}

ITableWorkspace_sptr runAlg(const MatrixWorkspace_sptr &in, const std::vector<double> &winX) {
  EstimatePeakIntensities alg;
  alg.setChild(true);
  alg.setLogging(false);
  alg.initialize();
  alg.setProperty("InputWorkspace", in);
  alg.setProperty("PeakWindowWorkspace", makeWindowWS(winX));
  alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
  alg.execute();
  TS_ASSERT(alg.isExecuted());
  return alg.getProperty("OutputWorkspace");
}
} // namespace

class EstimatePeakIntensitiesTest : public CxxTest::TestSuite {
public:
  static EstimatePeakIntensitiesTest *createSuite() { return new EstimatePeakIntensitiesTest(); }
  static void destroySuite(EstimatePeakIntensitiesTest *suite) { delete suite; }

  EstimatePeakIntensitiesTest() { FrameworkManager::Instance(); }

  void test_init() {
    EstimatePeakIntensities alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_table_shape_and_columns() {
    // one peak x two spectra -> two rows
    auto out = runAlg(makeInputWS(), {0.5, 6.5});
    TS_ASSERT_EQUALS(out->rowCount(), 2);
    TS_ASSERT_EQUALS(out->columnCount(), 6);
    const std::vector<std::string> expected{"PeakIndex", "WorkspaceIndex", "Intensity",
                                            "Sigma",     "Background",     "PeakCentre"};
    TS_ASSERT_EQUALS(out->getColumnNames(), expected);
  }

  void test_peak_spectrum_matches_hand_calculation() {
    // window bins [0,6): y = {2,2,2,10,2,2}, skew-seed background = mean of the five 2s = 2,
    // intensity = trapz(y - bg, x) over unit bins = 0.5*(0+8) + 0.5*(8+0) = 8, centre at the max (x=3),
    // sigma = sqrt(sum((e*bin_width)^2)) = sqrt(6) with e=1 and unit widths
    auto out = runAlg(makeInputWS(), {0.5, 6.5});
    TS_ASSERT_EQUALS(out->cell<int>(0, 0), 0);                      // PeakIndex
    TS_ASSERT_EQUALS(out->cell<int>(0, 1), 0);                      // WorkspaceIndex
    TS_ASSERT_DELTA(out->cell<double>(0, 2), 8.0, 1e-9);            // Intensity
    TS_ASSERT_DELTA(out->cell<double>(0, 3), std::sqrt(6.0), 1e-9); // Sigma
    TS_ASSERT_DELTA(out->cell<double>(0, 4), 2.0, 1e-9);            // Background
    TS_ASSERT_DELTA(out->cell<double>(0, 5), 3.0, 1e-9);            // PeakCentre (observed max)
  }

  void test_empty_spectrum_reports_defaults_and_midpoint_centre() {
    // spectrum 1 has no positive data -> zero intensity/sigma/background and the window midpoint 3.5
    auto out = runAlg(makeInputWS(), {0.5, 6.5});
    TS_ASSERT_EQUALS(out->cell<int>(1, 1), 1);
    TS_ASSERT_DELTA(out->cell<double>(1, 2), 0.0, 1e-12);
    TS_ASSERT_DELTA(out->cell<double>(1, 3), 0.0, 1e-12);
    TS_ASSERT_DELTA(out->cell<double>(1, 4), 0.0, 1e-12);
    TS_ASSERT_DELTA(out->cell<double>(1, 5), 3.5, 1e-12); // midpoint of [0.5, 6.5]
  }

  void test_multiple_windows_are_laid_out_peak_major() {
    // two peaks, both containing the x=3 spike -> each integrates to 8 on spectrum 0; rows are
    // ordered peak-major: (p0,s0), (p0,s1), (p1,s0), (p1,s1)
    auto out = runAlg(makeInputWS(), {0.5, 6.5, 2.5, 6.5});
    TS_ASSERT_EQUALS(out->rowCount(), 4);
    TS_ASSERT_EQUALS(out->cell<int>(2, 0), 1);            // PeakIndex of third row
    TS_ASSERT_EQUALS(out->cell<int>(2, 1), 0);            // WorkspaceIndex of third row
    TS_ASSERT_DELTA(out->cell<double>(2, 2), 8.0, 1e-9);  // Intensity of peak 1 on spectrum 0
    TS_ASSERT_DELTA(out->cell<double>(3, 5), 4.5, 1e-12); // empty spectrum -> midpoint of [2.5, 6.5]
  }

  void test_odd_length_window_is_rejected() {
    // a spectrum with an odd number of X values cannot be split into [min, max] pairs
    EstimatePeakIntensities alg;
    alg.setChild(true);
    alg.setLogging(false);
    alg.initialize();
    alg.setProperty("InputWorkspace", makeInputWS());
    alg.setProperty("PeakWindowWorkspace", makeWindowWS({0.5, 6.5, 2.5}));
    alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_mismatched_spectrum_count_is_rejected() {
    MatrixWorkspace_sptr badWin = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);
    badWin->mutableX(0) = std::vector<double>{0.5, 6.5};
    EstimatePeakIntensities alg;
    alg.setChild(true);
    alg.setLogging(false);
    alg.initialize();
    alg.setProperty("InputWorkspace", makeInputWS());
    alg.setProperty("PeakWindowWorkspace", badWin);
    alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
};
