// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

class ConeTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    Cone A;
    TS_ASSERT_EQUALS(A.getCentre(), V3D(0.0, 0, 0));
    TS_ASSERT_EQUALS(A.getNormal(), V3D(1.0, 0, 0));
    TS_ASSERT_EQUALS(A.getCosAngle(), 1.0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 0\n");
  }

  void testCreateCone() {
    Cone A;
    // Cone can be constructed with a centre, Vector, and Angle.
    TS_ASSERT_EQUALS(A.setSurface("k/x 1.0 1.0 1.0 1.0\n"), 0);
    // A.setAngle(45.0);
    TS_ASSERT_EQUALS(A.getNormal(), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getCentre(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_DELTA(A.getCosAngle(), cos(45.0 * M_PI / 180.0), 0.0000001);
    TS_ASSERT_EQUALS(extractString(A), "-1  k/x 1 1 1 1\n");
  }

  void testClone() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 1.0 1.0 1.0 1.0\n"), 0);
    auto B = A.clone();
    TS_ASSERT_EQUALS(extractString(*B), "-1  k/x 1 1 1 1\n");
    Cone *C = dynamic_cast<Cone *>(B.get());
    TS_ASSERT_EQUALS(C->getNormal(), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(C->getCentre(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(extractString(*C), "-1  k/x 1 1 1 1\n");
  }

  void testSide() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");

    double val = 0.1 * M_SQRT1_2;
    // Point outside Cone
    TS_ASSERT_EQUALS(A.side(V3D(0.1, 0.0, 0.0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val - 0.1, val - 0.1)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val - 0.1, val)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val, val - 0.1)), 1);
    // Point on the Cone
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val, val)), 0);
    // tolerance at default 1e-6
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val + 1e-7, val + 1e-7)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val + 2e-6, val + 2e-6)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val - 1e-7, val - 1e-7)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val - 2e-6, val - 2e-6)), 1);
    // Point inside the cone
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val + 0.001, val + 0.001)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val + 0.001, val)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, val, val + 0.001)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 2)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 1)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 4.9)), -1);
  }

  void testOnSurface() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");

    double val = 0.1 * M_SQRT1_2;
    // Point outside Cone
    TS_ASSERT(!A.onSurface(V3D(0.1, 0.0, 0.0)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val - 0.1, val - 0.1)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val - 0.1, val)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val, val - 0.1)));
    // Point on the Cone
    TS_ASSERT(A.onSurface(V3D(0.1, val, val)));
    // tolerance at default 1e-6
    TS_ASSERT(A.onSurface(V3D(0.1, val + 1e-7, val + 1e-7)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val + 2e-6, val + 2e-6)));
    TS_ASSERT(A.onSurface(V3D(0.1, val - 1e-7, val - 1e-7)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val - 2e-6, val - 2e-6)));
    // Point inside the cone
    TS_ASSERT(!A.onSurface(V3D(0.1, val + 0.001, val + 0.001)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val + 0.001, val)));
    TS_ASSERT(!A.onSurface(V3D(0.1, val, val + 0.001)));
    TS_ASSERT(!A.onSurface(V3D(0, 0, 2)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 1)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 4.9)));
  }

  void testDistance() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");
    double val = 0.1 * M_SQRT1_2;
    TS_ASSERT_DELTA(A.distance(V3D(0.1, val, val)), 0.0, 0.00001);
    // Inside
    TS_ASSERT_DELTA(A.distance(V3D(0.0, val * 10, val * 10)), M_SQRT1_2, 0.00001);
    TS_ASSERT_DELTA(A.distance(V3D(1, 0.0, 0.0)), -1 * M_SQRT1_2, 0.00001);
  }

  void testSetCentre() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");
    A.setCentre(V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(extractString(A), "-1  k/x 1 1 1 1\n");
  }

  void testSetNorm() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");
    A.setNorm(V3D(0.0, 1.0, 0.0));
    TS_ASSERT_EQUALS(extractString(A), "-1  ky 0 1\n");
  }

  void testSetAngle() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");
    A.setAngle(90.0);
    TS_ASSERT_DELTA(A.getCosAngle(), 0.0, 0.00001);
  }

  void setTanAngle() {
    Cone A;
    TS_ASSERT_EQUALS(A.setSurface("k/x 0.0 0.0 0.0 1.0\n"), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");
    A.setTanAngle(90.0);
    TS_ASSERT_DELTA(A.getCosAngle(), 0.0, 0.00001);
  }

private:
  std::string extractString(const Surface &pv) {
    // dump output to sting
    std::ostringstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }
};
