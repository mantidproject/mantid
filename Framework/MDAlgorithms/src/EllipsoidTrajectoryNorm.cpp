// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/EllipsoidTrajectoryNorm.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ConfigService.h"
#include "MantidMDAlgorithms/MDNormBase.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

namespace Mantid::MDAlgorithms {

using Kernel::V3D;

namespace {
using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;

/// pixels are tested against an ellipsoid in blocks of this size, so the discriminant pass vectorizes
constexpr std::size_t BLOCK_SIZE = 256;
/// pixels with |u|^2 below this are along the forward beam and measure no momentum transfer
constexpr double MIN_U_NORM2 = 1e-20;

/// ellipsoid axes scaled by 1/radius, so the ellipsoid is |E (Q - c)|^2 = 1
struct ScaledEllipsoid {
  std::array<V3D, 3> axes;
  std::array<double, 3> centre; // E c
  double d;                     // |E c|^2 - 1
};

ScaledEllipsoid scaleEllipsoid(const V3D &centre, const DataObjects::PeakEllipsoidFrame &directions,
                               const DataObjects::PeakEllipsoidExtent &radii) {
  ScaledEllipsoid e;
  e.d = -1.0;
  for (size_t i = 0; i < 3; ++i) {
    e.axes[i] = directions[i] / radii[i];
    e.centre[i] = centre.scalar_prod(e.axes[i]);
    e.d += e.centre[i] * e.centre[i];
  }
  return e;
}

/// roots k1 <= k2 of a k^2 - 2 b k + d = 0, given disc = b^2 - a d > 0 and a > 0, avoiding cancellation
inline void stableRoots(double a, double b, double d, double disc, double &k1, double &k2) {
  const double q = b >= 0.0 ? b + std::sqrt(disc) : b - std::sqrt(disc);
  k1 = q / a;
  k2 = d / q;
  if (k1 > k2)
    std::swap(k1, k2);
}

/**
 * Test pixels [begin, end) of (ux, uy, uz) against the ellipsoid and call
 * onHit(p, k1, k2) for each one whose line crosses it.
 */
template <typename OnHit>
void forEachIntersection(const ScaledEllipsoid &e, const std::vector<double> &ux, const std::vector<double> &uy,
                         const std::vector<double> &uz, OnHit &&onHit) {
  const std::size_t n = ux.size();
  const auto &ax = e.axes;
  std::array<double, BLOCK_SIZE> a, b, disc;
  for (std::size_t start = 0; start < n; start += BLOCK_SIZE) {
    const std::size_t len = std::min(BLOCK_SIZE, n - start);
    for (std::size_t j = 0; j < len; ++j) {
      const std::size_t p = start + j;
      const double s0 = ux[p] * ax[0].X() + uy[p] * ax[0].Y() + uz[p] * ax[0].Z();
      const double s1 = ux[p] * ax[1].X() + uy[p] * ax[1].Y() + uz[p] * ax[1].Z();
      const double s2 = ux[p] * ax[2].X() + uy[p] * ax[2].Y() + uz[p] * ax[2].Z();
      a[j] = s0 * s0 + s1 * s1 + s2 * s2;
      b[j] = s0 * e.centre[0] + s1 * e.centre[1] + s2 * e.centre[2];
      disc[j] = b[j] * b[j] - a[j] * e.d;
    }
    for (std::size_t j = 0; j < len; ++j) {
      if (disc[j] <= 0.0 || a[j] <= 0.0)
        continue;
      double k1, k2;
      stableRoots(a[j], b[j], e.d, disc[j], k1, k2);
      onHit(start + j, k1, k2);
    }
  }
}
/// beyond this many standard deviations a Gaussian's contribution is negligible (exp(-32))
constexpr double GAUSSIAN_CUTOFF = 8.0;

/**
 * Integrals of G(k) and k G(k) with respect to Phi(k) over (lo, hi], for
 * G(k) = h exp(-(k - k0)^2 / (2 s^2)) and Phi the cumulative flux (x, y)
 * interpolated as interpolateIntegratedFlux does: linear between points,
 * with jumps at x[0] (from 0), at the end of the last interpolated interval
 * (to y.back()) and at any repeated x.
 */
void fluxWeightedGaussianMoments(const double lo, const double hi, const double h, const double k0, const double s,
                                 const std::vector<double> &x, const std::vector<double> &y, double &m0, double &m1) {
  m0 = 0.0;
  m1 = 0.0;
  const std::size_t spSize = y.size();
  const double erfScale = h * s * std::sqrt(M_PI / 2.0);
  auto gauss = [&](double t) {
    const double z = (t - k0) / s;
    return h * std::exp(-0.5 * z * z);
  };
  // antiderivatives of G and k G
  auto e0 = [&](double t) { return erfScale * std::erf((t - k0) / (M_SQRT2 * s)); };
  auto e1 = [&](double t) { return k0 * e0(t) - s * s * gauss(t); };
  auto addMass = [&](double t, double mass) {
    const double g = gauss(t);
    m0 += mass * g;
    m1 += mass * g * t;
  };

  // Phi(x[0]) = y[0] and Phi = 0 below it
  if (lo < x[0] && x[0] <= hi)
    addMass(x[0], y[0]);
  if (spSize < 2)
    return;

  // linear pieces [x[j-1], x[j]] for j = 1 .. last; Phi jumps to y.back() just above x[last]
  const std::size_t last = spSize - 2;
  const double windowLow = std::max(lo, k0 - GAUSSIAN_CUTOFF * s);
  const double windowHigh = std::min(hi, k0 + GAUSSIAN_CUTOFF * s);
  if (windowHigh > windowLow) {
    const auto begin = x.begin();
    const auto end = x.begin() + static_cast<std::ptrdiff_t>(last + 1);
    const auto jStart = std::max<std::size_t>(
        1, static_cast<std::size_t>(std::distance(begin, std::lower_bound(begin, end, windowLow))));
    const auto jEnd = std::min<std::size_t>(
        last, static_cast<std::size_t>(std::distance(begin, std::lower_bound(begin, end, windowHigh))));
    for (std::size_t j = jStart; j <= jEnd; ++j) {
      if (x[j] == x[j - 1]) {
        // Phi(x[j]) = y[j - 1], and y[j] just above it
        if (lo <= x[j] && x[j] < hi)
          addMass(x[j], y[j] - y[j - 1]);
        continue;
      }
      const double a = std::max(windowLow, x[j - 1]);
      const double b = std::min(windowHigh, x[j]);
      if (b <= a)
        continue;
      const double slope = (y[j] - y[j - 1]) / (x[j] - x[j - 1]);
      m0 += slope * (e0(b) - e0(a));
      m1 += slope * (e1(b) - e1(a));
    }
  }
  if (lo <= x[last] && x[last] < hi)
    addMass(x[last], y.back() - y[last]);
}
} // namespace

double interpolateIntegratedFlux(const double x, const std::vector<double> &xData, const std::vector<double> &yData) {
  const std::size_t spSize = yData.size();
  // all integrals below xStart must be 0
  if (x < xData.front())
    return 0.0;
  const double yMax = yData.back();
  // all integrals above xEnd must be equal to yMax
  if (x > xData.back())
    return yMax;
  // first interpolation point at or above x, or the last one
  const auto last = xData.begin() + static_cast<std::ptrdiff_t>(spSize - 1);
  const auto j = static_cast<std::size_t>(std::distance(xData.begin(), std::lower_bound(xData.begin(), last, x)));
  // if x falls onto an interpolation point return the corresponding y
  if (x == xData[j])
    return yData[j];
  // in the last interval it's yMax
  if (j == spSize - 1)
    return yMax;
  if (j == 0)
    return 0.0;
  // interpolate between the consecutive points
  const double x0 = xData[j - 1];
  const double x1 = xData[j];
  const double y0 = yData[j - 1];
  const double y1 = yData[j];
  return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

std::size_t EllipsoidTrajectoryNorm::addFluxSpectrum(std::vector<double> x, std::vector<double> y) {
  if (y.empty() || x.size() < y.size())
    throw std::invalid_argument("EllipsoidTrajectoryNorm: a flux spectrum needs at least one point, and at least as "
                                "many x values as y values.");
  m_fluxX.emplace_back(std::move(x));
  m_fluxY.emplace_back(std::move(y));
  return m_fluxX.size() - 1;
}

bool EllipsoidTrajectoryNorm::addPixel(const V3D &u, const double weight, const std::size_t fluxIndex,
                                       const double kMin, const double kMax) {
  if (fluxIndex >= m_fluxX.size())
    throw std::out_of_range("EllipsoidTrajectoryNorm: flux spectrum index out of range.");
  if (u.norm2() < MIN_U_NORM2)
    return false;
  m_ux.emplace_back(u.X());
  m_uy.emplace_back(u.Y());
  m_uz.emplace_back(u.Z());
  m_weight.emplace_back(weight);
  m_fluxIndex.emplace_back(fluxIndex);
  m_kMin.emplace_back(kMin);
  m_kMax.emplace_back(kMax);
  return true;
}

bool EllipsoidTrajectoryNorm::addMaskedPixel(const V3D &u, const double kMin, const double kMax) {
  if (u.norm2() < MIN_U_NORM2)
    return false;
  m_maskedUx.emplace_back(u.X());
  m_maskedUy.emplace_back(u.Y());
  m_maskedUz.emplace_back(u.Z());
  m_maskedKMin.emplace_back(kMin);
  m_maskedKMax.emplace_back(kMax);
  return true;
}

bool EllipsoidTrajectoryNorm::intersect(const V3D &u, const V3D &centre,
                                        const DataObjects::PeakEllipsoidFrame &directions,
                                        const DataObjects::PeakEllipsoidExtent &radii, double &k1, double &k2) {
  const auto e = scaleEllipsoid(centre, directions, radii);
  double a = 0.0, b = 0.0;
  for (size_t i = 0; i < 3; ++i) {
    const double s = u.scalar_prod(e.axes[i]);
    a += s * s;
    b += s * e.centre[i];
  }
  const double disc = b * b - a * e.d;
  if (disc <= 0.0 || a <= 0.0)
    return false;
  stableRoots(a, b, e.d, disc, k1, k2);
  return true;
}

/**
 * Only the part of a line at positive k is physical, so a chord wholly at
 * k <= 0 (the ellipsoid lies opposite the pixel) is ignored rather than
 * flagged as clipped.
 */
EllipsoidNormResult EllipsoidTrajectoryNorm::integrate(const V3D &centre,
                                                       const DataObjects::PeakEllipsoidFrame &directions,
                                                       const DataObjects::PeakEllipsoidExtent &radii) const {
  EllipsoidNormResult result;
  const auto e = scaleEllipsoid(centre, directions, radii);

  forEachIntersection(e, m_ux, m_uy, m_uz, [&](std::size_t p, double k1, double k2) {
    if (k2 <= 0.0)
      return;
    if (k1 < m_kMin[p] || k2 > m_kMax[p])
      result.clipped = true;
    const double lo = std::max(k1, m_kMin[p]);
    const double hi = std::min(k2, m_kMax[p]);
    if (hi <= lo)
      return;
    const auto &x = m_fluxX[m_fluxIndex[p]];
    const auto &y = m_fluxY[m_fluxIndex[p]];
    result.norm += m_weight[p] * (interpolateIntegratedFlux(hi, x, y) - interpolateIntegratedFlux(lo, x, y));
    ++result.numPixels;
  });

  forEachIntersection(e, m_maskedUx, m_maskedUy, m_maskedUz, [&](std::size_t p, double k1, double k2) {
    if (k2 > 0.0 && std::min(k2, m_maskedKMax[p]) > std::max(k1, m_maskedKMin[p]))
      result.maskedIntersects = true;
  });

  return result;
}

EllipsoidChords EllipsoidTrajectoryNorm::chords(const V3D &centre, const DataObjects::PeakEllipsoidFrame &directions,
                                                const DataObjects::PeakEllipsoidExtent &radii) const {
  EllipsoidChords chords;
  auto &result = chords.result;
  const auto e = scaleEllipsoid(centre, directions, radii);

  forEachIntersection(e, m_ux, m_uy, m_uz, [&](std::size_t p, double k1, double k2) {
    if (k2 <= 0.0)
      return;
    if (k1 < m_kMin[p] || k2 > m_kMax[p])
      result.clipped = true;
    const double lo = std::max(k1, m_kMin[p]);
    const double hi = std::min(k2, m_kMax[p]);
    if (hi <= lo)
      return;
    const auto &x = m_fluxX[m_fluxIndex[p]];
    const auto &y = m_fluxY[m_fluxIndex[p]];
    result.norm += m_weight[p] * (interpolateIntegratedFlux(hi, x, y) - interpolateIntegratedFlux(lo, x, y));
    ++result.numPixels;
    chords.pixels.emplace_back(p);
    chords.kLow.emplace_back(lo);
    chords.kHigh.emplace_back(hi);
  });

  forEachIntersection(e, m_maskedUx, m_maskedUy, m_maskedUz, [&](std::size_t p, double k1, double k2) {
    if (k2 > 0.0 && std::min(k2, m_maskedKMax[p]) > std::max(k1, m_maskedKMin[p]))
      result.maskedIntersects = true;
  });

  return chords;
}

/**
 * Along a pixel's line the Gaussian is itself a Gaussian in k, with center
 * k0 = b / a, standard deviation 1 / sqrt(a) and height
 * exp(-(c^T S^-1 c - b^2 / a) / 2), where a = u^T S^-1 u and b = u^T S^-1 c.
 * Its gradient with respect to c is S^-1 (u int k g dPhi - c int g dPhi).
 */
GaussianExposure EllipsoidTrajectoryNorm::integrateGaussian(const EllipsoidChords &chords, const V3D &centre,
                                                            const DataObjects::PeakEllipsoidFrame &directions,
                                                            const DataObjects::PeakEllipsoidExtent &sigmas) const {
  GaussianExposure exposure;
  // the Gaussian's axes scaled by 1/sigma, so S^-1 = sum_i e_i e_i^T
  std::array<V3D, 3> axes;
  std::array<double, 3> c;
  double cc = 0.0;
  for (size_t i = 0; i < 3; ++i) {
    axes[i] = directions[i] / sigmas[i];
    c[i] = centre.scalar_prod(axes[i]);
    cc += c[i] * c[i];
  }

  // accumulate u * int k g dPhi - c * int g dPhi in the scaled frame
  std::array<double, 3> gradient{0.0, 0.0, 0.0};
  for (std::size_t n = 0; n < chords.pixels.size(); ++n) {
    const std::size_t p = chords.pixels[n];
    const V3D u(m_ux[p], m_uy[p], m_uz[p]);
    std::array<double, 3> uScaled;
    double a = 0.0, b = 0.0;
    for (size_t i = 0; i < 3; ++i) {
      uScaled[i] = u.scalar_prod(axes[i]);
      a += uScaled[i] * uScaled[i];
      b += uScaled[i] * c[i];
    }
    const double distance2 = std::max(cc - b * b / a, 0.0);
    if (distance2 > GAUSSIAN_CUTOFF * GAUSSIAN_CUTOFF)
      continue;
    double m0, m1;
    fluxWeightedGaussianMoments(chords.kLow[n], chords.kHigh[n], std::exp(-0.5 * distance2), b / a, 1.0 / std::sqrt(a),
                                m_fluxX[m_fluxIndex[p]], m_fluxY[m_fluxIndex[p]], m0, m1);
    exposure.value += m_weight[p] * m0;
    for (size_t i = 0; i < 3; ++i)
      gradient[i] += m_weight[p] * (uScaled[i] * m1 - c[i] * m0);
  }
  for (size_t i = 0; i < 3; ++i)
    exposure.gradient += axes[i] * gradient[i];
  return exposure;
}

EllipsoidTrajectoryNorm EllipsoidTrajectoryNorm::fromWorkspaces(const API::MatrixWorkspace &inputWS,
                                                                const API::MatrixWorkspace &solidAngleWS,
                                                                const API::MatrixWorkspace &fluxWS) {
  const auto lowValues = getLogValues<VectorDoubleProperty>(inputWS, "MDNorm_low");
  const auto highValues = getLogValues<VectorDoubleProperty>(inputWS, "MDNorm_high");
  const auto &spectrumInfo = inputWS.spectrumInfo();
  if (lowValues.size() != spectrumInfo.size() || highValues.size() != spectrumInfo.size())
    throw std::runtime_error("The MDNorm_low and MDNorm_high logs must have one value per spectrum of the input "
                             "workspace. Please use CropWorkspaceForMDNorm on the input workspace.");

  // same sign convention as MDTransfQ3D: Q = k_i - k_f (Inelastic) or k_f - k_i (Crystallography)
  const double sign = Kernel::ConfigService::Instance().getString("Q.convention") == "Crystallography" ? -1.0 : 1.0;
  const double protonCharge = inputWS.run().getProtonCharge();

  const auto fluxDetToIdx = fluxWS.getDetectorIDToWorkspaceIndexMap();
  const auto solidAngDetToIdx = solidAngleWS.getDetectorIDToWorkspaceIndexMap();

  EllipsoidTrajectoryNorm norm;
  // flux workspace index -> index into norm's flux spectra, copying each spectrum only once
  std::unordered_map<std::size_t, std::size_t> fluxSpectra;

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    // direction as computed by PreprocessDetectorsToMD for the events
    const auto &detector = spectrumInfo.detector(i);
    const double polar = spectrumInfo.twoTheta(i);
    const double azim = detector.getPhi();
    const double sPhi = std::sin(polar);
    const V3D u = V3D(-sPhi * std::cos(azim), -sPhi * std::sin(azim), 1.0 - std::cos(polar)) * sign;

    // if the detector is a group, this is the ID of the first detector, as in MDNorm
    const auto detID = detector.getID();
    const auto fluxIt = fluxDetToIdx.find(detID);
    const auto solidIt = solidAngDetToIdx.find(detID);
    if (spectrumInfo.isMasked(i) || fluxIt == fluxDetToIdx.end() || solidIt == solidAngDetToIdx.end()) {
      norm.addMaskedPixel(u, lowValues[i], highValues[i]);
      continue;
    }

    auto [it, inserted] = fluxSpectra.try_emplace(fluxIt->second, norm.m_fluxX.size());
    if (inserted)
      norm.addFluxSpectrum(fluxWS.x(fluxIt->second).rawData(), fluxWS.y(fluxIt->second).rawData());

    const double weight = solidAngleWS.y(solidIt->second)[0] * protonCharge;
    norm.addPixel(u, weight, it->second, lowValues[i], highValues[i]);
  }
  return norm;
}

} // namespace Mantid::MDAlgorithms
