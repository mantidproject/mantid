// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimatePeakIntensities.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/Histogram.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;
using namespace DataObjects;
using HistogramData::Histogram;

DECLARE_ALGORITHM(EstimatePeakIntensities)

namespace {

/// Result of a single spectrum/peak estimate.
struct Estimate {
  double intensity{0.0};
  double sigma{0.0};
  double background{0.0};
  double centre{std::numeric_limits<double>::quiet_NaN()};
};

/// A window resolved to half-open bin indices of a spectrum.
struct Window {
  bool ok{false};
  size_t istart{0};
  size_t iend{0};
};

/// Resolve a [lo, hi] X window to half-open bin indices for one spectrum as ws.yIndexOfX
/// An edge outside the spectrum's range, or an empty window, yields false.
Window resolveWindow(const MatrixWorkspace &ws, const size_t wi, const double lo, const double hi) {
  try {
    const size_t istart = ws.yIndexOfX(lo, wi);
    const size_t iend = ws.yIndexOfX(hi, wi);
    if (iend > istart)
      return {true, istart, iend};
  } catch (const std::out_of_range &) {
    // fall through to the invalid result
  }
  return {};
}

/// Third central moment E[(x-mu)^3] of a set summarised by its power sums (see
/// https://mathworld.wolfram.com/SampleCentralMoment.html). Reproduces the value of scipy.stats.moment(x, 3) used by
/// Python fitting estimates.
///
double centralMoment3(const double s1, const double s2, const double s3, const double count) {
  const double mu = s1 / count;
  const double mu2 = s2 / count;
  const double mu3 = s3 / count;
  return 2.0 * (mu * mu * mu) - (3.0 * mu * mu2) + mu3;
}

/// Mean of one window's background points, selected by the "seed_skew" method (PeakData.find_bg_pts_seed_skew):
///
/// sort the points by descending value and peel them off one at a time,
/// continuing while the third central moment (measure of skew in the data) of the remaining points
/// keeps decreasing and stays non-negative; the points left over are the
/// background.
///
/// The idea being that the sorted data will be positively skewed with the data from the peak appearing before the
/// data from the background. As peak pata points are removed the magnitude of the positive skew should decrease as left
/// leading "outliers" are removed from the set. Once only the background points remain the set should still be
/// positively skewed as a result of the sorting but at this point removal of the largest background point is expected
/// to result in a net increase in the magnitude of the positive skew (see example below)
///
///     |                 |
///     |                 |
///     ||                ||                  |
///     ||                ||                  |
///     |||               |||                 ||                    |
///  || ||| |             ||||||              |||||                 ||||                 |||              ||
/// ||||||||||            ||||||||||          |||||||||             ||||||||             |||||||          ||||||
/// 0123456789   sort->   4561280379  peel->  561280379   peel->    61280379    peel->   1280379  peel->  280379
///
///                       mean = 2.5          mean = 2.0           mean = 1.625          mean = 1.42      mean= 1.33
///                       moment = 9.3        moment = 2.67        moment = 0.227        moment = 0.035   moment = 0.074
///
///                                           delta = -ve          delta = -ve           delta = -ve      delta = +ve
///                                                                                                        (REJECT)
double estimateSkewBackground(const std::vector<double> &y) {
  const size_t n = y.size();
  if (n == 0)
    return 0.0;
  if (n == 1)
    return y[0];

  // indices of y sorted by descending value - peeled off largest first
  std::vector<size_t> order(n);
  std::iota(order.begin(), order.end(), size_t{0});
  std::sort(order.begin(), order.end(), [&y](size_t a, size_t b) { return y[a] > y[b]; });

  // running power sums of the remaining (candidate background) set, starting with every point
  double s1 = 0.0, s2 = 0.0, s3 = 0.0;
  for (const double v : y) {
    s1 += v;
    s2 += v * v;
    s3 += v * v * v;
  }

  // best accepted background so far = every point (nothing peeled off yet)
  double prevMoment = centralMoment3(s1, s2, s3, static_cast<double>(n));
  double bgSum = s1;
  double bgCount = static_cast<double>(n);

  for (size_t dropped = 1; dropped < n; ++dropped) {
    const double v = y[order[dropped - 1]]; // next largest point leaves the background set
    s1 -= v;
    s2 -= v * v;
    s3 -= v * v * v;
    const double count = static_cast<double>(n - dropped);
    const double moment = centralMoment3(s1, s2, s3, count);
    if (moment >= prevMoment || moment < 0.0)
      break; // stop peeling - keep the last accepted background
    prevMoment = moment;
    bgSum = s1;
    bgCount = count;
  }
  return bgSum / bgCount;
}

/// Estimate the background and integrated intensity of one spectrum over the bins.
/// centre is the observed maximum when the window has positive data, otherwise it is left at the
/// window midpoint.
Estimate estimateWindow(const Histogram &histo, const Window &window, const double fallbackCentre) {
  Estimate est;
  est.centre = fallbackCentre;
  if (!window.ok)
    return est;

  const auto &X = histo.x();
  const auto &Y = histo.y();
  const auto &E = histo.e();
  const size_t istart = window.istart;
  const size_t m = window.iend - istart;

  std::vector<double> yseg(Y.begin() + istart, Y.begin() + window.iend);
  if (!std::any_of(yseg.cbegin(), yseg.cend(), [](double v) { return v > 0.0; }))
    return est; // no positive data -> zero estimate, keep the fallback centre

  const double bg = estimateSkewBackground(yseg);

  // trapezoidal integral of (y - bg)
  double integral = 0.0;
  for (size_t k = 0; k + 1 < m; ++k)
    integral += 0.5 * ((yseg[k] - bg) + (yseg[k + 1] - bg)) * (X[istart + k + 1] - X[istart + k]);

  // sigma = sqrt(sum((e * bin_width)^2))
  double sig2 = 0.0;
  for (size_t k = 0; k < m; ++k) {
    const double bw = (k + 1 < m) ? (X[istart + k + 1] - X[istart + k])
                      : (m >= 2)  ? (X[istart + m - 1] - X[istart + m - 2])
                                  : 0.0;
    const double t = E[istart + k] * bw;
    sig2 += t * t;
  }

  const auto argmax = static_cast<size_t>(std::distance(yseg.cbegin(), std::max_element(yseg.cbegin(), yseg.cend())));

  est.intensity = integral;
  est.sigma = std::sqrt(sig2);
  est.background = bg;
  est.centre = X[istart + argmax];
  return est;
}

} // namespace

void EstimatePeakIntensities::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Workspace whose spectra should be integrated.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("PeakWindowWorkspace", "", Direction::Input),
                  "Per-spectrum integration windows, one spectrum per InputWorkspace spectrum, following the "
                  "FitPeaks FitPeakWindowWorkspace convention: each spectrum holds 2*nPeaks X values arranged as "
                  "[min0, max0, min1, max1, ...] (in InputWorkspace X units), so a peak's window can differ per "
                  "spectrum. A ragged workspace is accepted: every spectrum must hold a non-zero even number of "
                  "X values, but spectra may hold different numbers of [min, max] pairs, and a peak index "
                  "beyond a spectrum's pairs is reported with a NaN PeakCentre.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Table with one row per (peak, spectrum): PeakIndex, WorkspaceIndex, Intensity, Sigma, "
                  "Background, PeakCentre. The number of peaks is the largest number of windows held by any "
                  "spectrum of PeakWindowWorkspace. PeakIndex is only the position of the window in that "
                  "spectrum's list, so rows sharing a PeakIndex refer to the same physical peak only if "
                  "PeakWindowWorkspace lists the windows in the same peak order for every spectrum.");
}

std::map<std::string, std::string> EstimatePeakIntensities::validateInputs() {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr windowWS = getProperty("PeakWindowWorkspace");
  if (!inputWS || !windowWS)
    return issues;
  if (windowWS->getNumberHistograms() != inputWS->getNumberHistograms()) {
    issues["PeakWindowWorkspace"] = "must have the same number of spectra as InputWorkspace.";
    return issues;
  }
  // spectra may hold different numbers of windows, so every one has to be checked
  for (size_t wi = 0; wi < windowWS->getNumberHistograms(); ++wi) {
    const size_t nx = windowWS->x(wi).size();
    if (nx == 0 || nx % 2 != 0) {
      issues["PeakWindowWorkspace"] = "each spectrum must hold a non-zero even number of X values ([min, max] "
                                      "pairs, one per peak); spectrum " +
                                      std::to_string(wi) + " holds " + std::to_string(nx) + ".";
      break;
    }
  }
  return issues;
}

void EstimatePeakIntensities::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr windowWS = getProperty("PeakWindowWorkspace");

  const auto nHist = static_cast<int64_t>(inputWS->getNumberHistograms());

  // the window workspace may be ragged, so the table is sized by the spectrum holding the most windows and
  // spectra defining fewer are padded with the default (NaN centre) estimate
  size_t nPeaks = 0;
  size_t minPeaks = std::numeric_limits<size_t>::max();
  for (int64_t i = 0; i < nHist; ++i) {
    const size_t n = windowWS->x(static_cast<size_t>(i)).size() / 2;
    nPeaks = std::max(nPeaks, n);
    minPeaks = std::min(minPeaks, n);
  }
  if (nHist > 0 && minPeaks != nPeaks)
    g_log.warning() << "PeakWindowWorkspace is ragged: spectra hold between " << minPeaks << " and " << nPeaks
                    << " windows. The table has " << nPeaks
                    << " peaks and rows for undefined windows are reported with a NaN PeakCentre. PeakIndex is "
                       "the position of the window in each spectrum's list, so it only identifies the same "
                       "physical peak across spectra if the windows are listed in the same peak order.\n";

  // per (peak, spectrum) results, laid out peak-major (row = peak * nHist + wsIndex) so each peak's
  // rows are contiguous; filled in the parallel loop, then copied into the table serially
  const size_t nRows = nPeaks * static_cast<size_t>(nHist);
  std::vector<Estimate> results(nRows);

  Progress prog(this, 0.0, 1.0, nHist);

  PARALLEL_FOR_IF(threadSafe(*inputWS, *windowWS))
  for (int64_t i = 0; i < nHist; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    const auto wi = static_cast<size_t>(i);
    const auto histo = inputWS->histogram(wi);
    const auto &winX = windowWS->x(wi);
    const size_t nPeaksHere = winX.size() / 2; // <= nPeaks; the rest keep the default estimate
    for (size_t j = 0; j < nPeaksHere; ++j) {
      const double lo = winX[2 * j];
      const double hi = winX[2 * j + 1];
      const Window window = resolveWindow(*inputWS, wi, lo, hi);
      results[j * static_cast<size_t>(nHist) + wi] = estimateWindow(histo, window, 0.5 * (lo + hi));
    }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  ITableWorkspace_sptr out = std::make_shared<TableWorkspace>();
  out->addColumn("int", "PeakIndex");
  out->addColumn("int", "WorkspaceIndex");
  out->addColumn("double", "Intensity");
  out->addColumn("double", "Sigma");
  out->addColumn("double", "Background");
  out->addColumn("double", "PeakCentre");
  for (size_t j = 0; j < nPeaks; ++j) {
    for (int64_t i = 0; i < nHist; ++i) {
      const Estimate &e = results[j * static_cast<size_t>(nHist) + static_cast<size_t>(i)];
      TableRow row = out->appendRow();
      row << static_cast<int>(j) << static_cast<int>(i) << e.intensity << e.sigma << e.background << e.centre;
    }
  }

  setProperty("OutputWorkspace", out);
}

} // namespace Mantid::Algorithms
