// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/PeakShapeEllipsoid_fwd.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/DllConfig.h"

#include <array>
#include <cstddef>
#include <vector>

namespace Mantid::MDAlgorithms {

/**
 * Evaluate a cumulative (integrated) flux spectrum at momentum x, by linear
 * interpolation between the points (xData[j], yData[j]), exactly as MDNorm
 * does: zero below xData.front(), yData.back() above xData.back() and in the
 * last interval. xData may be bin edges (one longer than yData) or points.
 */
MANTID_MDALGORITHMS_DLL double interpolateIntegratedFlux(double x, const std::vector<double> &xData,
                                                         const std::vector<double> &yData);

/// Normalization of one ellipsoid, with coverage diagnostics
struct EllipsoidNormResult {
  /// sum over pixels of weight * [Phi(k2) - Phi(k1)]
  double norm{0.0};
  /// number of unmasked pixels whose trajectory crosses the ellipsoid within their k range
  std::size_t numPixels{0};
  /// true if any unmasked pixel's chord through the ellipsoid lies partly or wholly outside its k range
  bool clipped{false};
  /// true if any masked pixel's trajectory would have crossed the ellipsoid within its k range
  bool maskedIntersects{false};
};

/// The chords of the unmasked pixels' lines through an ellipsoid, clipped to each pixel's k range
struct EllipsoidChords {
  std::vector<std::size_t> pixels;
  std::vector<double> kLow, kHigh;
  /// normalization and coverage of the ellipsoid
  EllipsoidNormResult result;
};

/// Exposure-weighted integral of a Gaussian over a set of chords, and its gradient with respect to the Gaussian's
/// center
struct GaussianExposure {
  double value{0.0};
  Kernel::V3D gradient;
  /// exposure-weighted second moment about the center, the sum of (Q - c)(Q - c)^T g dPhi, as xx, yy, zz, xy, xz,
  /// yz; the gradient of value with respect to the inverse covariance is minus half of it
  std::array<double, 6> secondMoment{};
};

/** @class EllipsoidTrajectoryNorm

  Flux/solid-angle normalization of a fixed ellipsoidal region in Q, the
  equivalent of summing an MDNorm normalization workspace over the ellipsoid
  without binning.

  Each detector pixel p measures along the line Q(k) = k * u_p for k in
  [kMin_p, kMax_p]. The line crosses the ellipsoid (Q - c)^T M (Q - c) = 1 for
  the k satisfying a k^2 - 2 b k + d = 0, with a = u^T M u, b = u^T M c and
  d = c^T M c - 1, and contributes weight_p * [Phi_p(k2) - Phi_p(k1)] over the
  part of that chord inside its k range, with Phi_p its cumulative flux.
*/
class MANTID_MDALGORITHMS_DLL EllipsoidTrajectoryNorm {
public:
  /// Add a cumulative flux spectrum (momentum, integrated flux); returns the index used by addPixel
  std::size_t addFluxSpectrum(std::vector<double> x, std::vector<double> y);
  /// Add an unmasked pixel; returns false (and ignores it) if u is (nearly) zero, i.e. the forward direction
  bool addPixel(const Kernel::V3D &u, double weight, std::size_t fluxIndex, double kMin, double kMax);
  /// Add a masked pixel, used only to flag ellipsoids with missing coverage
  bool addMaskedPixel(const Kernel::V3D &u, double kMin, double kMax);

  /// Index the pixels by the direction of their lines, so that an ellipsoid is tested only against the pixels whose
  /// lines can reach it; adding pixels afterwards removes the index
  void buildDirectionIndex();

  std::size_t numPixels() const { return m_ux.size(); }
  std::size_t numMaskedPixels() const { return m_maskedUx.size(); }

  /// Normalization of the ellipsoid centred at centre, with principal axes directions and semi-axes radii
  EllipsoidNormResult integrate(const Kernel::V3D &centre, const DataObjects::PeakEllipsoidFrame &directions,
                                const DataObjects::PeakEllipsoidExtent &radii) const;

  /// The chords through the ellipsoid, as used by integrate, kept for integrateGaussian
  EllipsoidChords chords(const Kernel::V3D &centre, const DataObjects::PeakEllipsoidFrame &directions,
                         const DataObjects::PeakEllipsoidExtent &radii) const;

  /**
   * Sum over chords of weight * integral of g(k u) dPhi(k), for the Gaussian
   * g(Q) = exp(-(Q - centre)^T S^-1 (Q - centre) / 2) with principal axes
   * directions and standard deviations sigmas (peak height 1), i.e. the
   * expected counts per unit peak height over the chords' region, and its
   * gradient with respect to centre. Exact for Phi interpolated as
   * interpolateIntegratedFlux does.
   */
  GaussianExposure integrateGaussian(const EllipsoidChords &chords, const Kernel::V3D &centre,
                                     const DataObjects::PeakEllipsoidFrame &directions,
                                     const DataObjects::PeakEllipsoidExtent &sigmas) const;

  /// The range [k1, k2] over which the line Q = k * u is inside the ellipsoid; false if it misses or is tangent
  static bool intersect(const Kernel::V3D &u, const Kernel::V3D &centre,
                        const DataObjects::PeakEllipsoidFrame &directions,
                        const DataObjects::PeakEllipsoidExtent &radii, double &k1, double &k2);

  /**
   * Build from an instrument workspace whose spectra were cropped with
   * CropWorkspaceForMDNorm (its MDNorm_low/MDNorm_high logs give each
   * spectrum's k range), as MDNorm does: monitors and spectra without
   * detectors are skipped; masked spectra, and detectors missing from the flux
   * or solid angle workspaces, are added as masked pixels. Pixel directions
   * and the Q sign follow the same conventions as the event conversion
   * (MDTransfQ3D, Q_lab frame), and each pixel's weight is its solid angle
   * times the run's proton charge.
   */
  static EllipsoidTrajectoryNorm fromWorkspaces(const API::MatrixWorkspace &inputWS,
                                                const API::MatrixWorkspace &solidAngleWS,
                                                const API::MatrixWorkspace &fluxWS);

private:
  // unmasked pixels, stored as structure-of-arrays
  std::vector<double> m_ux, m_uy, m_uz, m_weight, m_kMin, m_kMax;
  std::vector<std::size_t> m_fluxIndex;
  // masked pixels
  std::vector<double> m_maskedUx, m_maskedUy, m_maskedUz, m_maskedKMin, m_maskedKMax;
  // cumulative flux spectra
  std::vector<std::vector<double>> m_fluxX, m_fluxY;
  // pixel indices grouped by the direction of their lines, in bins of polar and azimuthal angle: bin b holds
  // order[start[b]] .. order[start[b + 1] - 1]; start is empty if there is no index
  std::vector<std::size_t> m_indexOrder, m_indexStart, m_maskedIndexOrder, m_maskedIndexStart;
};

} // namespace Mantid::MDAlgorithms
