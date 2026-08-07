// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/DivergentSlitBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MonteCarloTesting.h"

#include <cmath>

using Mantid::Algorithms::DivergentSlitBeamProfile;
using Mantid::Algorithms::RectangularBeamProfile;
using Mantid::Kernel::V3D;

class DivergentSlitBeamProfileTest : public CxxTest::TestSuite {
public:
  static DivergentSlitBeamProfileTest *createSuite() { return new DivergentSlitBeamProfileTest(); }
  static void destroySuite(DivergentSlitBeamProfileTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Sampling must be identical to the base profile - this is what guarantees that
  // MonteCarloAbsorption results are unchanged by the presence of divergence.
  //----------------------------------------------------------------------------
  void test_GeneratePoint_Matches_RectangularProfile() {
    using namespace MonteCarloTesting;
    using namespace ::testing;

    RectangularBeamProfile rectangular(createTestFrame(), CENTRE, WIDTH, HEIGHT);
    DivergentSlitBeamProfile divergent(createTestFrame(), CENTRE, WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);

    MockRNG rectRng;
    EXPECT_CALL(rectRng, nextValue()).Times(Exactly(2)).WillRepeatedly(Return(0.75));
    const auto expected = rectangular.generatePoint(rectRng);

    MockRNG divRng;
    EXPECT_CALL(divRng, nextValue()).Times(Exactly(2)).WillRepeatedly(Return(0.75));
    const auto actual = divergent.generatePoint(divRng);

    TS_ASSERT_EQUALS(expected.startPos, actual.startPos);
    TS_ASSERT_EQUALS(expected.unitDir, actual.unitDir);
  }

  void test_DefineActiveRegion_Matches_RectangularProfile() {
    auto testSample = ComponentCreationHelper::createCappedCylinder(0.5, 1.0, V3D(), V3D(0, 0, 1), "cyl");
    const auto bounds = testSample->getBoundingBox();

    RectangularBeamProfile rectangular(createTestFrame(), CENTRE, WIDTH, HEIGHT);
    DivergentSlitBeamProfile divergent(createTestFrame(), CENTRE, WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);

    const auto expected = rectangular.defineActiveRegion(bounds);
    const auto actual = divergent.defineActiveRegion(bounds);
    TS_ASSERT_EQUALS(expected.minPoint(), actual.minPoint());
    TS_ASSERT_EQUALS(expected.maxPoint(), actual.maxPoint());
  }

  //----------------------------------------------------------------------------
  // Intensity - eq. 13 and 14 of Creek, Santisteban & Edwards (2005)
  //----------------------------------------------------------------------------
  void test_Zero_Divergence_Reproduces_TopHat() {
    // beam = X, up = Z, so the horizontal transverse direction is Y.
    DivergentSlitBeamProfile profile(createTestFrame(), V3D(), WIDTH, HEIGHT, 0.0, 0.0, SLIT_DISTANCE);

    TS_ASSERT_DELTA(profile.intensityAt(V3D()), 1.0, 1e-12);
    // Just inside each aperture edge
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.49 * WIDTH, 0.0)), 1.0, 1e-12);
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.0, 0.49 * HEIGHT)), 1.0, 1e-12);
    // Just outside
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.51 * WIDTH, 0.0)), 0.0, 1e-12);
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.0, 0.51 * HEIGHT)), 0.0, 1e-12);
  }

  void test_Divergence_Smears_The_Edge_To_Half_Intensity() {
    DivergentSlitBeamProfile profile(createTestFrame(), V3D(), WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);
    // Exactly on the aperture edge, half the smeared distribution falls inside.
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.5 * WIDTH, 0.0)), 0.5, 1e-9);
    TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, 0.0, 0.5 * HEIGHT)), 0.5, 1e-9);
    // And the centre is still fully illuminated for a slit much wider than the smearing.
    TS_ASSERT_DELTA(profile.intensityAt(V3D()), 1.0, 1e-9);
  }

  void test_Matches_Analytic_Erf_Profile() {
    DivergentSlitBeamProfile profile(createTestFrame(), V3D(), WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);
    const double sigma = SLIT_DISTANCE * std::tan(DIVERGENCE);
    for (const double offset : {0.0, 0.2 * WIDTH, 0.45 * WIDTH, 0.5 * WIDTH, 0.6 * WIDTH}) {
      TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, offset, 0.0)),
                      apertureTransmission(offset, WIDTH, sigma) * apertureTransmission(0.0, HEIGHT, sigma), 1e-9);
    }
  }

  void test_Profile_Is_Symmetric_About_The_Beam_Axis() {
    DivergentSlitBeamProfile profile(createTestFrame(), V3D(), WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);
    for (const double offset : {0.2 * WIDTH, 0.5 * WIDTH, 0.8 * WIDTH}) {
      TS_ASSERT_DELTA(profile.intensityAt(V3D(0.0, offset, 0.0)), profile.intensityAt(V3D(0.0, -offset, 0.0)), 1e-12);
    }
  }

  void test_Smearing_Grows_With_Slit_Distance() {
    // Further from the slit, the beam has diverged more, so the edge is softer - the intensity just
    // outside the nominal aperture is higher.
    DivergentSlitBeamProfile near(createTestFrame(), V3D(), WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);
    DivergentSlitBeamProfile far(createTestFrame(), V3D(), WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, 4.0 * SLIT_DISTANCE);
    const V3D justOutside(0.0, 0.6 * WIDTH, 0.0);
    TS_ASSERT_LESS_THAN(near.intensityAt(justOutside), far.intensityAt(justOutside));
  }

  void test_Upstream_Of_The_Slit_Is_Not_Illuminated() {
    DivergentSlitBeamProfile profile(createTestFrame(), V3D(), WIDTH, HEIGHT, DIVERGENCE, DIVERGENCE, SLIT_DISTANCE);
    // beam = X, so a point further upstream than the slit itself receives nothing.
    TS_ASSERT_DELTA(profile.intensityAt(V3D(-2.0 * SLIT_DISTANCE, 0.0, 0.0)), 0.0, 1e-12);
  }

private:
  /// Eq. 14, normalised so a fully illuminated point returns 1.
  double apertureTransmission(const double offset, const double aperture, const double sigma) {
    const double half = 0.5 * aperture;
    const double invNorm = 1.0 / (sigma * std::sqrt(2.0));
    return 0.5 * (std::erf((offset + half) * invNorm) - std::erf((offset - half) * invNorm));
  }

  Mantid::Geometry::ReferenceFrame createTestFrame() {
    using Mantid::Geometry::Handedness;
    using Mantid::Geometry::PointingAlong;
    using Mantid::Geometry::ReferenceFrame;
    // up = Z, beam = X
    return ReferenceFrame(PointingAlong::Z, PointingAlong::X, Handedness::Right, "source");
  }

  const double WIDTH{0.004};
  const double HEIGHT{0.004};
  const double DIVERGENCE{0.0146};  // rad, ~0.84 degrees
  const double SLIT_DISTANCE{0.05}; // m
  const V3D CENTRE{1.0, 2.0, -3.0};
};
