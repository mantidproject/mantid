// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/RadialCollimatorProfile.h"
#include "MantidKernel/V3D.h"

#include <cmath>

using Mantid::Algorithms::RadialCollimatorProfile;
using Mantid::Kernel::V3D;

class RadialCollimatorProfileTest : public CxxTest::TestSuite {
public:
  static RadialCollimatorProfileTest *createSuite() { return new RadialCollimatorProfileTest(); }
  static void destroySuite(RadialCollimatorProfileTest *suite) { delete suite; }

  void test_rejects_non_positive_gauge_width() {
    TS_ASSERT_THROWS(RadialCollimatorProfile(0.0, UP), const std::invalid_argument &);
    TS_ASSERT_THROWS(RadialCollimatorProfile(-0.004, UP), const std::invalid_argument &);
  }

  void test_rejects_zero_up_direction() {
    TS_ASSERT_THROWS(RadialCollimatorProfile(0.004, V3D(0.0, 0.0, 0.0)), const std::invalid_argument &);
  }

  void test_peaks_on_the_viewing_axis() {
    const RadialCollimatorProfile profile(FWHM, UP);
    TS_ASSERT_DELTA(profile.intensityAt(ORIGIN, ORIGIN, EAST_DETECTOR), 1.0, 1e-12);
  }

  void test_falls_to_half_at_half_the_fwhm() {
    const RadialCollimatorProfile profile(FWHM, UP);
    // A detector due east is viewed along x, so the collimator restricts z - the beam direction.
    const V3D offset(0.0, 0.0, 0.5 * FWHM);
    TS_ASSERT_DELTA(profile.intensityAt(offset, ORIGIN, EAST_DETECTOR), 0.5, 1e-9);
    TS_ASSERT_DELTA(profile.intensityAt(-offset, ORIGIN, EAST_DETECTOR), 0.5, 1e-9);
  }

  void test_is_flat_along_the_viewing_axis_and_vertically() {
    const RadialCollimatorProfile profile(FWHM, UP);
    // Moving toward or away from the detector, or up and down, is unrestricted by the collimator.
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.05, 0.0, 0.0), ORIGIN, EAST_DETECTOR), 1.0, 1e-12);
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.05, 0.0), ORIGIN, EAST_DETECTOR), 1.0, 1e-12);
  }

  void test_restricted_direction_rotates_with_the_detector() {
    const RadialCollimatorProfile profile(FWHM, UP);
    const V3D alongBeam(0.0, 0.0, 0.5 * FWHM);
    // A detector downstream views along z, so it restricts x rather than z. The same offset that
    // halved the acceptance for an east-facing detector is now fully accepted.
    const V3D downstreamDetector(0.0, 0.0, 1.5);
    TS_ASSERT_DELTA(profile.intensityAt(alongBeam, ORIGIN, downstreamDetector), 1.0, 1e-12);
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.5 * FWHM, 0.0, 0.0), ORIGIN, downstreamDetector), 0.5, 1e-9);
  }

  void test_opposite_banks_share_the_same_restricted_direction() {
    // The North and South banks sit at +/-90 degrees but a radial collimator is built so both see
    // the same volume, so an offset along the beam must be attenuated identically for each.
    const RadialCollimatorProfile profile(FWHM, UP);
    const V3D offset(0.0, 0.0, 0.3 * FWHM);
    const double east = profile.intensityAt(offset, ORIGIN, EAST_DETECTOR);
    const double west = profile.intensityAt(offset, ORIGIN, V3D(-1.5, 0.0, 0.0));
    TS_ASSERT_DELTA(east, west, 1e-12);
    TS_ASSERT_LESS_THAN(east, 1.0);
  }

  void test_wider_collimator_is_less_restrictive() {
    const RadialCollimatorProfile narrow(0.002, UP);
    const RadialCollimatorProfile wide(0.004, UP);
    const V3D offset(0.0, 0.0, 0.001);
    TS_ASSERT_LESS_THAN(narrow.intensityAt(offset, ORIGIN, EAST_DETECTOR),
                        wide.intensityAt(offset, ORIGIN, EAST_DETECTOR));
  }

  void test_decreases_monotonically_away_from_the_axis() {
    const RadialCollimatorProfile profile(FWHM, UP);
    double previous = 2.0;
    for (int i = 0; i <= 10; ++i) {
      const double acceptance = profile.intensityAt(V3D(0.0, 0.0, 0.0005 * i), ORIGIN, EAST_DETECTOR);
      TS_ASSERT_LESS_THAN(acceptance, previous);
      TS_ASSERT_LESS_THAN_EQUALS(0.0, acceptance);
      previous = acceptance;
    }
  }

  void test_degenerate_geometry_imposes_no_restriction() {
    const RadialCollimatorProfile profile(FWHM, UP);
    // Detector coincident with the sample, and a detector directly overhead, both leave no
    // horizontal direction to restrict.
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.0, 0.01), ORIGIN, ORIGIN), 1.0, 1e-12);
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.0, 0.01), ORIGIN, V3D(0.0, 1.5, 0.0)), 1.0, 1e-12);
  }

private:
  const double FWHM{0.004};
  const V3D UP{0.0, 1.0, 0.0};
  const V3D ORIGIN{0.0, 0.0, 0.0};
  const V3D EAST_DETECTOR{1.5, 0.0, 0.0};
};
