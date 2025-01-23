// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ReferenceFrame.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ReferenceFrameTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReferenceFrameTest *createSuite() { return new ReferenceFrameTest(); }
  static void destroySuite(ReferenceFrameTest *suite) { delete suite; }

  void testDefaultSettings() {
    ReferenceFrame defaultInstance;
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", Y, defaultInstance.pointingUp());
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", Z, defaultInstance.pointingAlongBeam());
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", Right, defaultInstance.getHandedness());
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", "source", defaultInstance.origin());
  }

  void testGetUp() {
    ReferenceFrame frame1(X, Y, Right, "source");
    ReferenceFrame frame2(Z, Y, Right, "source");
    TS_ASSERT_EQUALS(X, frame1.pointingUp());
    TS_ASSERT_EQUALS(Z, frame2.pointingUp());
  }

  void testGetAlongBeam() {
    ReferenceFrame frame1(X, Y, Right, "source");
    ReferenceFrame frame2(Z, X, Right, "source");
    TS_ASSERT_EQUALS(Y, frame1.pointingAlongBeam());
    TS_ASSERT_EQUALS(X, frame2.pointingAlongBeam());
  }

  void testVectorAlongBeam() {
    V3D testVector = V3D(0, 0, 1);
    ReferenceFrame frame1(Y, Z, Right, "source");
    ReferenceFrame frame2(Z, X, Right, "source");

    TSM_ASSERT("Should be positive because test vector is paralell to along "
               "beam vector",
               frame1.isVectorPointingAlongBeam(testVector));
    TSM_ASSERT("Should be negative because test vector is perpendicular to "
               "along beam vector",
               !frame2.isVectorPointingAlongBeam(testVector));
  }

  void testGetHorizontal() {
    doGetHorizontalTest(Right);
    doGetHorizontalTest(Left);
  }

  void doGetHorizontalTest(Mantid::Geometry::Handedness handed) {
    ReferenceFrame frame1(X, Y, handed, "source"); // X up, Y along beam
    TS_ASSERT_EQUALS(Z, frame1.pointingHorizontal());

    ReferenceFrame frame2(X, Z, handed, "source"); // X up, Z along beam
    TS_ASSERT_EQUALS(Y, frame2.pointingHorizontal());

    ReferenceFrame frame3(Y, Z, handed, "source"); // Y up, Z along beam
    TS_ASSERT_EQUALS(X, frame3.pointingHorizontal());

    ReferenceFrame frame4(Y, X, handed, "source"); // Y up, X along beam
    TS_ASSERT_EQUALS(Z, frame4.pointingHorizontal());

    ReferenceFrame frame5(Z, X, handed, "source"); // Z up, X along beam
    TS_ASSERT_EQUALS(Y, frame5.pointingHorizontal());

    ReferenceFrame frame6(Z, Y, handed, "source"); // Z up, Y along beam
    TS_ASSERT_EQUALS(X, frame6.pointingHorizontal());
  }

  void testGetHandedNess() {
    ReferenceFrame frameRight(X, Y, Right, "source");
    ReferenceFrame frameLeft(X, Y, Left, "source");
    TS_ASSERT_EQUALS(Right, frameRight.getHandedness());
    TS_ASSERT_EQUALS(Left, frameLeft.getHandedness());
  }

  void testGetOrigin() {
    ReferenceFrame frame(X, Y, Right, "source");
    TS_ASSERT_EQUALS("source", frame.origin());
  }

  void testIdenticalUpAndBeamDirectionsThrow() {
    TS_ASSERT_THROWS(ReferenceFrame(X, X, Right, "source"), const std::invalid_argument &);
  }

  void testGetUpDirectionVector() {
    ReferenceFrame x(X, Y, Right, "source");
    V3D x_vec = x.vecPointingUp();
    TS_ASSERT_EQUALS(1, x_vec[0]);
    TS_ASSERT_EQUALS(0, x_vec[1]);
    TS_ASSERT_EQUALS(0, x_vec[2]);

    ReferenceFrame y(Y, X, Right, "source");
    V3D y_vec = y.vecPointingUp();
    TS_ASSERT_EQUALS(0, y_vec[0]);
    TS_ASSERT_EQUALS(1, y_vec[1]);
    TS_ASSERT_EQUALS(0, y_vec[2]);

    ReferenceFrame z(Z, Y, Right, "source");
    V3D z_vec = z.vecPointingUp();
    TS_ASSERT_EQUALS(0, z_vec[0]);
    TS_ASSERT_EQUALS(0, z_vec[1]);
    TS_ASSERT_EQUALS(1, z_vec[2]);
  }

  void testGetAlongBeamDirectionVector() {
    ReferenceFrame x(Y, X, Right, "source");
    V3D x_vec = x.vecPointingAlongBeam();
    TS_ASSERT_EQUALS(1, x_vec[0]);
    TS_ASSERT_EQUALS(0, x_vec[1]);
    TS_ASSERT_EQUALS(0, x_vec[2]);

    ReferenceFrame y(X, Y, Right, "source");
    V3D y_vec = y.vecPointingAlongBeam();
    TS_ASSERT_EQUALS(0, y_vec[0]);
    TS_ASSERT_EQUALS(1, y_vec[1]);
    TS_ASSERT_EQUALS(0, y_vec[2]);

    ReferenceFrame z(X, Z, Right, "source");
    V3D z_vec = z.vecPointingAlongBeam();
    TS_ASSERT_EQUALS(0, z_vec[0]);
    TS_ASSERT_EQUALS(0, z_vec[1]);
    TS_ASSERT_EQUALS(1, z_vec[2]);
  }

  void testGetHorizontalDirectionVector() {
    ReferenceFrame z1(Y, X, Right, "source");
    V3D z_vec = z1.vecPointingHorizontal();
    TS_ASSERT_EQUALS(0, z_vec[0]);
    TS_ASSERT_EQUALS(0, z_vec[1]);
    TS_ASSERT_EQUALS(1, z_vec[2]);

    ReferenceFrame z2(X, Y, Right, "source");
    z_vec = z2.vecPointingHorizontal();
    TS_ASSERT_EQUALS(0, z_vec[0]);
    TS_ASSERT_EQUALS(0, z_vec[1]);
    TS_ASSERT_EQUALS(1, z_vec[2]);

    ReferenceFrame y(X, Z, Right, "source");
    V3D y_vec = y.vecPointingHorizontal();
    TS_ASSERT_EQUALS(0, y_vec[0]);
    TS_ASSERT_EQUALS(1, y_vec[1]);
    TS_ASSERT_EQUALS(0, y_vec[2]);

    ReferenceFrame x(Y, Z, Right, "source");
    V3D x_vec = x.vecPointingHorizontal();
    TS_ASSERT_EQUALS(1, x_vec[0]);
    TS_ASSERT_EQUALS(0, x_vec[1]);
    TS_ASSERT_EQUALS(0, x_vec[2]);
  }

  void testGetThetaSignAxisVector() {
    // Default constructor, theta-sign axis is the same as up
    ReferenceFrame y(Y, X, Right, "source");
    V3D y_vec = y.vecThetaSign();
    TS_ASSERT_EQUALS(0, y_vec[0]);
    TS_ASSERT_EQUALS(1, y_vec[1]);
    TS_ASSERT_EQUALS(0, y_vec[2]);

    // Again up but with explicit constructor
    ReferenceFrame y2(Y, X, Y, Right, "source");
    V3D y_vec2 = y2.vecThetaSign();
    TS_ASSERT_EQUALS(0, y_vec2[0]);
    TS_ASSERT_EQUALS(1, y_vec2[1]);
    TS_ASSERT_EQUALS(0, y_vec2[2]);

    // Theta sign is the third axis
    ReferenceFrame x(Y, Z, X, Right, "source");
    V3D x_vec = x.vecThetaSign();
    TS_ASSERT_EQUALS(1, x_vec[0]);
    TS_ASSERT_EQUALS(0, x_vec[1]);
    TS_ASSERT_EQUALS(0, x_vec[2]);

    // Error if theta sign is along the beam
    TS_ASSERT_THROWS_EQUALS(ReferenceFrame z(Y, Z, Z, Right, "source"), const std::invalid_argument &e,
                            std::string(e.what()),
                            "Scattering angle sign axis cannot be the same as the beam direction");
  }

  void testAxisLabelReturns() {
    ReferenceFrame x(Y, X, Right, "source");
    TS_ASSERT_EQUALS("Y", x.pointingUpAxis());
    TS_ASSERT_EQUALS("X", x.pointingAlongBeamAxis());
    TS_ASSERT_EQUALS("Z", x.pointingHorizontalAxis());

    ReferenceFrame y(X, Y, Right, "source");
    TS_ASSERT_EQUALS("X", y.pointingUpAxis());
    TS_ASSERT_EQUALS("Y", y.pointingAlongBeamAxis());
    TS_ASSERT_EQUALS("Z", y.pointingHorizontalAxis());

    ReferenceFrame z(X, Z, Right, "source");
    TS_ASSERT_EQUALS("X", z.pointingUpAxis());
    TS_ASSERT_EQUALS("Z", z.pointingAlongBeamAxis());
    TS_ASSERT_EQUALS("Y", z.pointingHorizontalAxis());
  }
};
