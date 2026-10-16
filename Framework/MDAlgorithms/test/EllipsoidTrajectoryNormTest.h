// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeEllipsoid_fwd.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/EllipsoidTrajectoryNorm.h"

#include <array>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <limits>
#include <stdexcept>
#include <vector>

using Mantid::DataObjects::PeakEllipsoidExtent;
using Mantid::DataObjects::PeakEllipsoidFrame;
using Mantid::Kernel::V3D;
using Mantid::MDAlgorithms::EllipsoidNormResult;
using Mantid::MDAlgorithms::EllipsoidTrajectoryNorm;
using Mantid::MDAlgorithms::interpolateIntegratedFlux;

namespace {
const PeakEllipsoidFrame AXES{{V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)}};

PeakEllipsoidExtent sphere(double radius) { return {{radius, radius, radius}}; }

/// orthonormal frame rotated away from the lab axes
PeakEllipsoidFrame rotatedAxes() {
  V3D e0(1, 1, 0), e1(-1, 1, 1);
  e0.normalize();
  e1.normalize();
  V3D e2 = e0.cross_prod(e1);
  return {{e0, e1, e2}};
}

/// flat flux phi from k = 0 to 20, i.e. cumulative flux phi * k
void addFlatFlux(EllipsoidTrajectoryNorm &norm, double phi) {
  std::vector<double> x, y;
  for (int i = 0; i <= 2000; ++i) {
    x.emplace_back(0.01 * i);
    y.emplace_back(phi * 0.01 * i);
  }
  norm.addFluxSpectrum(x, y);
}

bool inside(const V3D &q, const V3D &centre, const PeakEllipsoidFrame &dirs, const PeakEllipsoidExtent &radii) {
  double sum = 0.0;
  for (size_t i = 0; i < 3; ++i) {
    const double comp = (q - centre).scalar_prod(dirs[i]) / radii[i];
    sum += comp * comp;
  }
  return sum <= 1.0;
}
} // namespace

class EllipsoidTrajectoryNormTest : public CxxTest::TestSuite {
public:
  static EllipsoidTrajectoryNormTest *createSuite() { return new EllipsoidTrajectoryNormTest(); }
  static void destroySuite(EllipsoidTrajectoryNormTest *suite) { delete suite; }

  void test_interpolate_points() {
    const std::vector<double> x{1.0, 2.0, 3.0, 4.0};
    const std::vector<double> y{0.5, 1.0, 3.0, 6.0};
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(0.5, x, y), 0.0);
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(1.0, x, y), 0.5);
    TS_ASSERT_DELTA(interpolateIntegratedFlux(1.5, x, y), 0.75, 1e-12);
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(2.0, x, y), 1.0);
    TS_ASSERT_DELTA(interpolateIntegratedFlux(2.25, x, y), 1.5, 1e-12);
    // as in MDNorm, the last interval is not interpolated
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(3.5, x, y), 6.0);
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(4.0, x, y), 6.0);
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(5.0, x, y), 6.0);
  }

  void test_interpolate_bin_edges() {
    const std::vector<double> x{1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> y{0.5, 1.0, 3.0, 6.0};
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(0.5, x, y), 0.0);
    TS_ASSERT_DELTA(interpolateIntegratedFlux(2.5, x, y), 2.0, 1e-12);
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(4.5, x, y), 6.0);
    TS_ASSERT_EQUALS(interpolateIntegratedFlux(5.5, x, y), 6.0);
  }

  void test_intersect_matches_brute_force() {
    const auto dirs = rotatedAxes();
    const PeakEllipsoidExtent radii{{0.05, 0.12, 0.2}};
    const V3D centre(1.0, -2.0, 3.0);
    const std::vector<V3D> us{V3D(0.25, -0.5, 0.75), V3D(0.26, -0.49, 0.75), V3D(0.5, -1.0, 1.52),
                              V3D(0.1, -0.2, 0.3) * 1.25};
    const double dk = 1e-5;
    for (const auto &u : us) {
      double k1, k2;
      TS_ASSERT(EllipsoidTrajectoryNorm::intersect(u, centre, dirs, radii, k1, k2));
      double kLo = std::numeric_limits<double>::max(), kHi = std::numeric_limits<double>::lowest();
      for (int n = 0; n < 2000000; ++n) {
        const double k = n * dk;
        if (inside(u * k, centre, dirs, radii)) {
          kLo = std::min(kLo, k);
          kHi = std::max(kHi, k);
        }
      }
      TS_ASSERT_LESS_THAN(k1, k2);
      TS_ASSERT_DELTA(k1, kLo, 2 * dk);
      TS_ASSERT_DELTA(k2, kHi, 2 * dk);
    }
  }

  void test_intersect_miss_and_tangent() {
    double k1, k2;
    // passes sqrt(2) from the centre of a unit sphere
    TS_ASSERT(!EllipsoidTrajectoryNorm::intersect(V3D(1, 0, 1), V3D(0, 0, 2), AXES, sphere(1.0), k1, k2));
    // just touches the unit sphere
    TS_ASSERT(!EllipsoidTrajectoryNorm::intersect(V3D(1, 0, 0), V3D(0, 1, 0), AXES, sphere(1.0), k1, k2));
  }

  void test_chord_inside_k_band() {
    EllipsoidTrajectoryNorm norm;
    addFlatFlux(norm, 3.0);
    TS_ASSERT(norm.addPixel(V3D(0, 0, 1), 0.5, 0, 1.0, 10.0));
    // chord k in [4, 6]
    const auto result = norm.integrate(V3D(0, 0, 5), AXES, sphere(1.0));
    TS_ASSERT_DELTA(result.norm, 0.5 * 3.0 * 2.0, 1e-10);
    TS_ASSERT_EQUALS(result.numPixels, 1);
    TS_ASSERT(!result.clipped);
    TS_ASSERT(!result.maskedIntersects);
  }

  void test_chord_clipped_by_k_band() {
    EllipsoidTrajectoryNorm norm;
    addFlatFlux(norm, 3.0);
    norm.addPixel(V3D(0, 0, 1), 0.5, 0, 1.0, 5.5);
    auto result = norm.integrate(V3D(0, 0, 5), AXES, sphere(1.0));
    TS_ASSERT_DELTA(result.norm, 0.5 * 3.0 * 1.5, 1e-10);
    TS_ASSERT_EQUALS(result.numPixels, 1);
    TS_ASSERT(result.clipped);

    // chord wholly outside the band
    result = norm.integrate(V3D(0, 0, 8), AXES, sphere(1.0));
    TS_ASSERT_EQUALS(result.norm, 0.0);
    TS_ASSERT_EQUALS(result.numPixels, 0);
    TS_ASSERT(result.clipped);
  }

  void test_chord_at_negative_k_is_ignored() {
    EllipsoidTrajectoryNorm norm;
    addFlatFlux(norm, 3.0);
    norm.addPixel(V3D(0, 0, -1), 0.5, 0, 1.0, 10.0);
    const auto result = norm.integrate(V3D(0, 0, 5), AXES, sphere(1.0));
    TS_ASSERT_EQUALS(result.norm, 0.0);
    TS_ASSERT_EQUALS(result.numPixels, 0);
    TS_ASSERT(!result.clipped);
  }

  void test_flat_flux_sphere_matches_analytic_chords() {
    const double phi = 2.5, solid = 0.7, radius = 0.2;
    const V3D centre(1.0, 2.0, 3.0);
    V3D cHat = centre;
    cHat.normalize();
    // a fan of pixels around the direction of the centre, more than one block of them
    EllipsoidTrajectoryNorm norm;
    addFlatFlux(norm, phi);
    double expected = 0.0;
    size_t expectedPixels = 0;
    for (int i = -20; i <= 20; ++i) {
      for (int j = -10; j <= 10; ++j) {
        const V3D u = (cHat + V3D(0.004 * i, 0.004 * j, -0.002 * i)) * 1.3;
        norm.addPixel(u, solid, 0, 0.5, 15.0);
        const double dist = centre.cross_prod(u).norm() / u.norm();
        if (dist < radius) {
          expected += solid * phi * 2.0 * std::sqrt(radius * radius - dist * dist) / u.norm();
          ++expectedPixels;
        }
      }
    }
    TS_ASSERT_LESS_THAN(256, norm.numPixels());
    const auto result = norm.integrate(centre, AXES, sphere(radius));
    TS_ASSERT_LESS_THAN(0, expectedPixels);
    TS_ASSERT_EQUALS(result.numPixels, expectedPixels);
    TS_ASSERT_DELTA(result.norm, expected, 1e-9 * expected);
    TS_ASSERT(!result.clipped);
  }

  void test_masking_removes_exactly_one_contribution() {
    const auto dirs = rotatedAxes();
    const PeakEllipsoidExtent radii{{0.1, 0.15, 0.25}};
    const V3D centre(-1.0, 0.5, 2.0);
    const V3D masked = centre * 0.5;
    const std::vector<V3D> others{centre * 0.4 + V3D(0.01, 0, 0), centre * 0.6 + V3D(0, 0.02, 0),
                                  centre * 0.3 + V3D(0, 0, -0.01)};

    EllipsoidTrajectoryNorm all, withMask, alone;
    for (auto *norm : {&all, &withMask, &alone})
      addFlatFlux(*norm, 1.5);
    all.addPixel(masked, 0.9, 0, 1.0, 12.0);
    withMask.addMaskedPixel(masked, 1.0, 12.0);
    alone.addPixel(masked, 0.9, 0, 1.0, 12.0);
    for (const auto &u : others) {
      all.addPixel(u, 0.9, 0, 1.0, 12.0);
      withMask.addPixel(u, 0.9, 0, 1.0, 12.0);
    }

    const auto rAll = all.integrate(centre, dirs, radii);
    const auto rMask = withMask.integrate(centre, dirs, radii);
    const auto rAlone = alone.integrate(centre, dirs, radii);
    TS_ASSERT_LESS_THAN(0.0, rAlone.norm);
    TS_ASSERT_DELTA(rAll.norm - rMask.norm, rAlone.norm, 1e-12);
    TS_ASSERT_EQUALS(rAll.numPixels, rMask.numPixels + 1);
    TS_ASSERT(!rAll.maskedIntersects);
    TS_ASSERT(rMask.maskedIntersects);
  }

  void test_forward_pixel_is_ignored() {
    EllipsoidTrajectoryNorm norm;
    addFlatFlux(norm, 1.0);
    TS_ASSERT(!norm.addPixel(V3D(0, 0, 0), 1.0, 0, 1.0, 10.0));
    TS_ASSERT(!norm.addMaskedPixel(V3D(0, 0, 0), 1.0, 10.0));
    TS_ASSERT_EQUALS(norm.numPixels(), 0);
    TS_ASSERT_EQUALS(norm.numMaskedPixels(), 0);
  }

  void test_bad_flux_index_throws() {
    EllipsoidTrajectoryNorm norm;
    TS_ASSERT_THROWS(norm.addPixel(V3D(0, 0, 1), 1.0, 0, 1.0, 10.0), const std::out_of_range &);
    TS_ASSERT_THROWS(norm.addFluxSpectrum({}, {}), const std::invalid_argument &);
  }

  void test_wide_gaussian_matches_norm_including_flux_jumps() {
    // a flux with a jump at its first point and an uninterpolated last interval
    EllipsoidTrajectoryNorm norm;
    norm.addFluxSpectrum({3.5, 4.2, 5.0, 5.7, 6.4}, {0.3, 1.0, 2.5, 3.0, 4.0});
    const V3D centre(0, 0, 5);
    for (int i = -6; i <= 6; ++i)
      norm.addPixel(V3D(0.02 * i, 0.01 * i, 1.0), 1.0 + 0.1 * i, 0, 2.0, 12.0);
    const auto chords = norm.chords(centre, AXES, sphere(1.5));
    TS_ASSERT_LESS_THAN(0.0, chords.result.norm);
    TS_ASSERT_EQUALS(chords.pixels.size(), chords.result.numPixels);
    TS_ASSERT_DELTA(chords.result.norm, norm.integrate(centre, AXES, sphere(1.5)).norm, 1e-12);

    // a Gaussian so wide it is 1 over the chords
    const auto exposure = norm.integrateGaussian(chords, centre, AXES, sphere(1e5));
    TS_ASSERT_DELTA(exposure.value, chords.result.norm, 1e-8 * chords.result.norm);
  }

  void test_gaussian_matches_numerical_integration() {
    EllipsoidTrajectoryNorm norm;
    std::vector<double> x, y;
    for (int i = 0; i <= 400; ++i) {
      x.emplace_back(0.05 * i);
      y.emplace_back(x.back() * x.back() * (1.0 + 0.1 * std::sin(x.back())));
    }
    norm.addFluxSpectrum(x, y);
    const V3D centre(1.0, -2.0, 3.0);
    V3D cHat = centre;
    cHat.normalize();
    for (int i = -5; i <= 5; ++i)
      for (int j = -5; j <= 5; ++j)
        norm.addPixel((cHat + V3D(0.01 * i, 0.01 * j, 0.005 * i)) * 0.7, 0.5, 0, 1.0, 15.0);
    const auto chords = norm.chords(centre, AXES, sphere(0.4));
    const auto dirs = rotatedAxes();
    const PeakEllipsoidExtent sigmas{{0.08, 0.12, 0.15}};
    const V3D gaussCentre = centre + V3D(0.02, -0.01, 0.03);
    const auto exposure = norm.integrateGaussian(chords, gaussCentre, dirs, sigmas);

    // sum g(k u) dPhi over fine steps along each chord
    double expected = 0.0;
    for (int i = -5; i <= 5; ++i) {
      for (int j = -5; j <= 5; ++j) {
        const V3D u = (cHat + V3D(0.01 * i, 0.01 * j, 0.005 * i)) * 0.7;
        double k1, k2;
        if (!EllipsoidTrajectoryNorm::intersect(u, centre, AXES, sphere(0.4), k1, k2))
          continue;
        k1 = std::max(k1, 1.0);
        k2 = std::min(k2, 15.0);
        const int steps = 20000;
        const double dk = (k2 - k1) / steps;
        for (int n = 0; n < steps; ++n) {
          const double ka = k1 + n * dk;
          const V3D q = u * (ka + 0.5 * dk) - gaussCentre;
          double m = 0.0;
          for (size_t a = 0; a < 3; ++a) {
            const double comp = q.scalar_prod(dirs[a]) / sigmas[a];
            m += comp * comp;
          }
          expected += 0.5 * std::exp(-0.5 * m) *
                      (interpolateIntegratedFlux(ka + dk, x, y) - interpolateIntegratedFlux(ka, x, y));
        }
      }
    }
    TS_ASSERT_LESS_THAN(0.0, expected);
    TS_ASSERT_DELTA(exposure.value, expected, 1e-5 * expected);
  }

  void test_gaussian_gradient_matches_finite_differences() {
    EllipsoidTrajectoryNorm norm;
    std::vector<double> x, y;
    for (int i = 0; i <= 400; ++i) {
      x.emplace_back(0.05 * i);
      y.emplace_back(x.back() * x.back());
    }
    norm.addFluxSpectrum(x, y);
    const V3D centre(1.0, -2.0, 3.0);
    V3D cHat = centre;
    cHat.normalize();
    for (int i = -5; i <= 5; ++i)
      for (int j = -5; j <= 5; ++j)
        norm.addPixel((cHat + V3D(0.01 * i, 0.01 * j, 0.005 * i)) * 0.7, 0.5, 0, 1.0, 15.0);
    const auto chords = norm.chords(centre, AXES, sphere(0.4));
    const auto dirs = rotatedAxes();
    const PeakEllipsoidExtent sigmas{{0.08, 0.12, 0.15}};
    const V3D gaussCentre = centre + V3D(0.02, -0.01, 0.03);
    const auto exposure = norm.integrateGaussian(chords, gaussCentre, dirs, sigmas);

    const double h = 1e-6;
    const std::array<V3D, 3> steps{V3D(h, 0, 0), V3D(0, h, 0), V3D(0, 0, h)};
    for (size_t a = 0; a < 3; ++a) {
      const double plus = norm.integrateGaussian(chords, gaussCentre + steps[a], dirs, sigmas).value;
      const double minus = norm.integrateGaussian(chords, gaussCentre - steps[a], dirs, sigmas).value;
      const double numerical = (plus - minus) / (2 * h);
      TS_ASSERT_DELTA(exposure.gradient[a], numerical, 1e-5 * exposure.value / 0.08);
    }
  }
};
