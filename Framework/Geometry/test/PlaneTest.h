// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class PlaneTest : public CxxTest::TestSuite {

public:
  void testConstructor() {
    Plane A;
    TS_ASSERT_EQUALS(A.getNormal(), V3D(1.0, 0, 0));
    TS_ASSERT_EQUALS(A.getDistance(), 0);
    TS_ASSERT_EQUALS(extractString(A), "-1 px 0\n");
  }
  void testSetPlane() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 3.0), V3D(2.0 / 9.0, 2.0 / 9.0, 1.0 / 9.0));
    TS_ASSERT_EQUALS(A.getNormal(), V3D(2.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0));
    TS_ASSERT_EQUALS(A.getDistance(), 5.0);
    // TS_ASSERT_EQUALS(extractString(A),"-1 p 0.6666666667 0.6666666667
    // 0.3333333333 5\n");
  }

  void testClone() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 3.0), V3D(2.0 / 9.0, 2.0 / 9.0, 1.0 / 9.0));
    auto B = A.clone();
    TS_ASSERT_EQUALS(A.getNormal(), V3D(2.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0));
    TS_ASSERT_EQUALS(A.getDistance(), 5.0);
    // TS_ASSERT_EQUALS(extractString(*B),"-1 p 0.6666666667 0.6666666667
    // 0.3333333333 5\n");
  }

  void testSide() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 5.0), V3D(0, 0, 5.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 5\n");

    // Point outside plane on the same side of the normal
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 6)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 8)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 5.1)), 1);
    // Point on the plane
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 5)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 5)), 0);
    // test tolerance default 1e-6
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 5 + 1e-7)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 5 + 2e-6)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 5 - 1e-7)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 5 - 2e-6)), -1);
    // Point on flip side of the plane
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 2)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 1)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(10, 10, 4.9)), -1);
  }

  void testOnSurface() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 5.0), V3D(0, 0, 5.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 5\n");

    // Point outside plane on the same side of the normal
    TS_ASSERT(!A.onSurface(V3D(0, 0, 6)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 8)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 5.1)));
    // Point on the plane
    TS_ASSERT(A.onSurface(V3D(0, 0, 5)));
    TS_ASSERT(A.onSurface(V3D(10, 10, 5)));
    // test tolerance default 1e-6
    TS_ASSERT(A.onSurface(V3D(10, 10, 5 + 1e-7)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 5 + 2e-6)));
    TS_ASSERT(A.onSurface(V3D(10, 10, 5 - 1e-7)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 5 - 2e-6)));
    // Point on flip side of the plane
    TS_ASSERT(!A.onSurface(V3D(0, 0, 2)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 1)));
    TS_ASSERT(!A.onSurface(V3D(10, 10, 4.9)));
  }

  void testDotProduct() {
    Plane A, B;
    A.setPlane(V3D(3.0, 3.0, 6.0), V3D(0, 0, 6.0));
    B.setPlane(V3D(3.0, 0.0, 4.0), V3D(0.0, 0.0, 4.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
    TS_ASSERT_EQUALS(extractString(B), "-1 pz 4\n");
    // planes are parallel to each other
    TS_ASSERT_EQUALS(A.dotProd(B), 1);
    // With zero dot product i.e orthogonal to each other
    A.setPlane(V3D(3.0, 3.0, 6.0), V3D(0, 0, 6.0));
    B.setPlane(V3D(4.0, 1.0, 0.0), V3D(4.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
    TS_ASSERT_EQUALS(extractString(B), "-1 px 4\n");
    TS_ASSERT_EQUALS(A.dotProd(B), 0);
  }

  void testCrossProduct() {
    Plane A, B;
    // Cross product of two xy plane and zy plane is xz plane
    A.setPlane(V3D(3.0, 3.0, 6.0), V3D(0, 0, 6.0));
    B.setPlane(V3D(4.0, 1.0, 0.0), V3D(4.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
    TS_ASSERT_EQUALS(extractString(B), "-1 px 4\n");
    TS_ASSERT_EQUALS(A.crossProd(B), V3D(0.0, 1.0, 0.0));
  }

  void testDistance() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 6.0), V3D(0, 0, 6.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
    TS_ASSERT_EQUALS(A.distance(V3D(0.0, 1.0, 0.0)), -6.0);
  }

  void makeMatrix(Matrix<double> &A) const {
    A.setMem(3, 3);
    A[0][0] = 1.0;
    A[1][0] = 0.0;
    A[0][1] = 0.0;
    A[0][2] = 0.0;
    A[2][0] = 0.0;
    A[1][1] = cos(90.0 * M_PI / 180.0);
    A[2][1] = -sin(90.0 * M_PI / 180.0);
    A[1][2] = sin(90.0 * M_PI / 180.0);
    A[2][2] = cos(90.0 * M_PI / 180.0);
    return;
  }

  void testRotate() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 6.0), V3D(0, 0, 6.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
    Matrix<double> rotMat(3, 3);
    makeMatrix(rotMat);
    // rotating z plane 90 degress w.r.t x axis
    A.rotate(rotMat);
    TS_ASSERT_EQUALS(extractString(A), "-1 py 6\n");
  }

  void testDisplace() {
    Plane A;
    A.setPlane(V3D(3.0, 3.0, 6.0), V3D(0, 0, 6.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
    A.displace(V3D(0.0, 1.0, 7.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 13\n");
    A.displace(V3D(0.0, 1.0, -7.0));
    TS_ASSERT_EQUALS(extractString(A), "-1 pz 6\n");
  }

  void testSetSurface() {
    Plane A;
    A.setSurface("p 0.6666666667 0.6666666667 0.3333333333 5\n");
    V3D result = A.getNormal();
    TS_ASSERT_DELTA(result[0], 2.0 / 3.0, 0.0001);
    TS_ASSERT_DELTA(result[1], 2.0 / 3.0, 0.0001);
    TS_ASSERT_DELTA(result[2], 1.0 / 3.0, 0.0001);
    TS_ASSERT_DELTA(A.getDistance(), 5, 0.0001);
  }

  void testGetBoundingBox() {
    Plane A;
    A.setSurface("px 5");
    double Xmax, Xmin, Ymax, Ymin, Zmax, Zmin;
    Xmax = Ymax = Zmax = 20;
    Xmin = Ymin = Zmin = -20;
    A.getBoundingBox(Xmax, Ymax, Zmax, Xmin, Ymin, Zmin);
    TS_ASSERT_DELTA(Xmax, 5.0, 0.0001);
    TS_ASSERT_DELTA(Ymax, 20.0, 0.0001);
    TS_ASSERT_DELTA(Zmax, 20.0, 0.0001);
    TS_ASSERT_DELTA(Xmin, -20, 0.0001);
    TS_ASSERT_DELTA(Ymin, -20.0, 0.0001);
    TS_ASSERT_DELTA(Zmin, -20.0, 0.0001);
    A.setSurface("p -1 -1 -1 -1"); // removing just one vertex
    Xmax = Ymax = Zmax = 20;
    Xmin = Ymin = Zmin = 0;
    A.getBoundingBox(Xmax, Ymax, Zmax, Xmin, Ymin, Zmin);
    TS_ASSERT_DELTA(Xmax, 20, 0.0001);
    TS_ASSERT_DELTA(Ymax, 20, 0.0001);
    TS_ASSERT_DELTA(Zmax, 20, 0.0001);
    TS_ASSERT_DELTA(Xmin, 0.0, 0.0001);
    TS_ASSERT_DELTA(Ymin, 0.0, 0.0001);
    TS_ASSERT_DELTA(Zmin, 0.0, 0.0001);
    A.setSurface("p 0.57735 0.57735 0.57735 1"); // no reduction test
    Xmax = Ymax = Zmax = 20;
    Xmin = Ymin = Zmin = 0;
    A.getBoundingBox(Xmax, Ymax, Zmax, Xmin, Ymin, Zmin);
    TS_ASSERT_DELTA(Xmax, 1.732, 0.0001);
    TS_ASSERT_DELTA(Ymax, 1.732, 0.0001);
    TS_ASSERT_DELTA(Zmax, 1.732, 0.0001);
    TS_ASSERT_DELTA(Xmin, 0.0, 0.0001);
    TS_ASSERT_DELTA(Ymin, 0.0, 0.0001);
    TS_ASSERT_DELTA(Zmin, 0.0, 0.0001);
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
