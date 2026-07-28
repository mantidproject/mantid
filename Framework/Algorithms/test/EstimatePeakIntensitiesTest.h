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
#include "MantidHistogramData/Histogram.h"

#include <cmath>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::Points;

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

/// Ragged window workspace: each spectrum holds its own X list, so spectra may define different
/// numbers of [min, max] pairs.
MatrixWorkspace_sptr makeRaggedWindowWS(const std::vector<std::vector<double>> &winX) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", winX.size(), 2, 1);
  for (size_t sp = 0; sp < winX.size(); ++sp)
    ws->setHistogram(sp, Histogram(Points(winX[sp]), Counts(winX[sp].size(), 0.0)));
  return ws;
}

ITableWorkspace_sptr runAlgWithWindowWS(const MatrixWorkspace_sptr &in, const MatrixWorkspace_sptr &win) {
  EstimatePeakIntensities alg;
  alg.setChild(true);
  alg.setLogging(false);
  alg.initialize();
  alg.setProperty("InputWorkspace", in);
  alg.setProperty("PeakWindowWorkspace", win);
  alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
  alg.execute();
  TS_ASSERT(alg.isExecuted());
  return alg.getProperty("OutputWorkspace");
}

ITableWorkspace_sptr runAlg(const MatrixWorkspace_sptr &in, const std::vector<double> &winX) {
  return runAlgWithWindowWS(in, makeWindowWS(winX));
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

  void test_ragged_windows_with_longer_first_spectrum() {
    // spectrum 0 defines two peaks, spectrum 1 only one; the table is sized by the longest spectrum
    // and (peak 1, spectrum 1) is padded with a zero estimate and a NaN centre
    auto out = runAlgWithWindowWS(makeInputWS(), makeRaggedWindowWS({{0.5, 6.5, 2.5, 6.5}, {0.5, 6.5}}));
    TS_ASSERT_EQUALS(out->rowCount(), 4);
    TS_ASSERT_DELTA(out->cell<double>(0, 2), 8.0, 1e-9);  // (p0, s0) from its own window
    TS_ASSERT_DELTA(out->cell<double>(1, 5), 3.5, 1e-12); // (p0, s1) midpoint of [0.5, 6.5]
    TS_ASSERT_DELTA(out->cell<double>(2, 2), 8.0, 1e-9);  // (p1, s0) second window of spectrum 0
    TS_ASSERT_EQUALS(out->cell<int>(3, 0), 1);            // (p1, s1) has no window
    TS_ASSERT_EQUALS(out->cell<int>(3, 1), 1);
    TS_ASSERT_DELTA(out->cell<double>(3, 2), 0.0, 1e-12);
    TS_ASSERT_DELTA(out->cell<double>(3, 3), 0.0, 1e-12);
    TS_ASSERT_DELTA(out->cell<double>(3, 4), 0.0, 1e-12);
    TS_ASSERT(std::isnan(out->cell<double>(3, 5)));
  }

  void test_ragged_windows_with_longer_later_spectrum() {
    // the reverse case: spectrum 0 is the short one, so the peak count cannot be taken from it and
    // spectrum 1's second window must still be read within its own length
    auto out = runAlgWithWindowWS(makeInputWS(), makeRaggedWindowWS({{0.5, 6.5}, {0.5, 6.5, 2.5, 6.5}}));
    TS_ASSERT_EQUALS(out->rowCount(), 4);
    TS_ASSERT_DELTA(out->cell<double>(0, 2), 8.0, 1e-9);  // (p0, s0)
    TS_ASSERT_DELTA(out->cell<double>(1, 5), 3.5, 1e-12); // (p0, s1) midpoint of [0.5, 6.5]
    TS_ASSERT(std::isnan(out->cell<double>(2, 5)));       // (p1, s0) has no window
    TS_ASSERT_DELTA(out->cell<double>(2, 2), 0.0, 1e-12);
    TS_ASSERT_DELTA(out->cell<double>(3, 5), 4.5, 1e-12); // (p1, s1) midpoint of [2.5, 6.5]
  }

  void test_odd_length_in_a_later_spectrum_is_rejected() {
    // the pair check applies to every spectrum, not just the first
    EstimatePeakIntensities alg;
    alg.setChild(true);
    alg.setLogging(false);
    alg.initialize();
    alg.setProperty("InputWorkspace", makeInputWS());
    alg.setProperty("PeakWindowWorkspace", makeRaggedWindowWS({{0.5, 6.5}, {0.5, 6.5, 2.5}}));
    alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
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
