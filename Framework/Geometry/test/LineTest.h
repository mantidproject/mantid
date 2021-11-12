// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <vector>

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class LineTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    Line A;
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(0.0, 0.0, 0.0));
  }

  void testParamConstructor() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));
  }

  void testLineConstructor() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    Line B(A);
    TS_ASSERT_EQUALS(B.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(B.getDirect(), V3D(1.0, 0.0, 0.0));
  }

  void testAssignment() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    Line B;
    TS_ASSERT_DIFFERS(B.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_DIFFERS(B.getDirect(), V3D(1.0, 0.0, 0.0));
    B = A;
    TS_ASSERT_EQUALS(B.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(B.getDirect(), V3D(1.0, 0.0, 0.0));
  }

  void testGetPoint() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    // 0 lambda should return origin
    TS_ASSERT_EQUALS(A.getPoint(0.0), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getPoint(-1.0), V3D(0.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getPoint(1.0), V3D(2.0, 1.0, 1.0));
  }

  void testDistance() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.distance(V3D(0.0, 0.0, 0.0)), M_SQRT2);
    TS_ASSERT_EQUALS(A.distance(V3D(1.0, 0.0, 0.0)), M_SQRT2);
    TS_ASSERT_EQUALS(A.distance(V3D(1.0, 1.0, 0.0)), 1.0);
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
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    Matrix<double> rotMat(3, 3);
    makeMatrix(rotMat);
    A.rotate(rotMat);
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(1.0, 1.0, -1.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));
  }

  void testDisplace() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));
    A.displace(V3D(2.0, 1.0, 2.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(3.0, 2.0, 3.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));
  }

  void testIsValid() {
    Line A(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    TS_ASSERT_EQUALS(A.isValid(V3D(1.1, 1.0, 1.0)), 1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0.9, 1.0, 1.0)), 1);
    // test tolerance default 1e-6
    TS_ASSERT_EQUALS(A.isValid(V3D(0.9, 1.0 + 1e-7, 1.0 + 1e-7)), 1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0.9, 1.0 + 2e-6, 1.0 + 2e-6)), 0);
    TS_ASSERT_EQUALS(A.isValid(V3D(0.9, 1.0 - 1e-7, 1.0 - 1e-7)), 1);
    TS_ASSERT_EQUALS(A.isValid(V3D(0.9, 1.0 - 2e-6, 1.0 - 2e-6)), 0);

    TS_ASSERT_EQUALS(A.isValid(V3D(1.0, 0.9, 1.0)), 0);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.0, 1.1, 1.0)), 0);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.0, 1.0, 0.9)), 0);
    TS_ASSERT_EQUALS(A.isValid(V3D(1.0, 1.0, 1.1)), 0);
  }

  void testSetLine() {
    Line A;
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(0.0, 0.0, 0.0));

    A.setLine(V3D(1.0, 1.0, 1.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(1.0, 1.0, 1.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));
  }

  // A Line with equation equivalent to x axis will cut A Cylinder with 1 radius
  // with center at 0,0,0  y axis normal
  // at two points
  void testIntersectCylinder() {
    Line A(V3D(0.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    Cylinder cylinder;
    cylinder.setSurface("c/y 0.0 0.0 1.0");
    TS_ASSERT_EQUALS(cylinder.getCentre(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(cylinder.getRadius(), 1);
    TS_ASSERT_EQUALS(cylinder.getNormal(), V3D(0, 1, 0));

    Line::PType pntOut;
    A.intersect(pntOut, cylinder);

    // forward only solution for cylinders
    TS_ASSERT_EQUALS(pntOut.size(), 1);
    TS_ASSERT_EQUALS(pntOut.front(), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(cylinder.onSurface(pntOut.front()), 1);

    // no intercept case - point outside cylinder pointing away
    Line bad(V3D(2, 0, 0), V3D(1, 0, 0));
    Line::PType badOut;
    bad.intersect(badOut, cylinder);
    TS_ASSERT_EQUALS(badOut.size(), 0);
  }

  void multScattCylinderDetails(Cylinder &cylinder, const V3D &point1, const V3D &point2) {
    int numinside = 0;
    if (sqrt(point1[0] * point1[0] + point1[2] * point1[2]) < cylinder.getRadius())
      numinside += 1;
    if (sqrt(point2[0] * point2[0] + point2[2] * point2[2]) < cylinder.getRadius())
      numinside += 1;
    size_t expNumIntersections = (numinside == 0) ? 2 : 1;

    std::cout << "point1 " << point1 << " radius: " << sqrt(point1[0] * point1[0] + point1[2] * point1[2])
              << " height: " << point1[1] << std::endl;
    std::cout << "point2 " << point2 << " radius: " << sqrt(point2[0] * point2[0] + point2[2] * point2[2])
              << " height: " << point2[1] << std::endl;

    // from point1 to point2
    Line case1(point1, Kernel::normalize(point2 - point1));
    Line::PType case1_out;
    case1.intersect(case1_out, cylinder);
    for (const auto &point : case1_out) {
      TS_ASSERT_EQUALS(cylinder.onSurface(point), 1);
      const double radius = sqrt(point[0] * point[0] + point[2] * point[2]);
      TS_ASSERT_DELTA(radius, cylinder.getRadius(), 0.01 * cylinder.getRadius());
      std::cout << "AA found " << point << std::endl;
    }
    TS_ASSERT_EQUALS(case1_out.size(), expNumIntersections);
    if (case1_out.size() > 1)
      TS_ASSERT_DIFFERS(case1_out[0], case1_out[1]);

    // from point2 to point1 -- duplicated from lines above
    Line case2(point2, Kernel::normalize(point1 - point2));
    Line::PType case2_out;
    case2.intersect(case2_out, cylinder);
    for (const auto &point : case2_out) {
      TS_ASSERT_EQUALS(cylinder.onSurface(point), 1);
      const double radius = sqrt(point[0] * point[0] + point[2] * point[2]);
      TS_ASSERT_DELTA(radius, cylinder.getRadius(), 0.01 * cylinder.getRadius());
      std::cout << "BB found " << point << std::endl;
    }
    TS_ASSERT_EQUALS(case2_out.size(), expNumIntersections);
    if (case2_out.size() > 1)
      TS_ASSERT_DIFFERS(case2_out[0], case2_out[1]);

    // should have found the same points
    // note that order is preserved for opposite directions
    if (expNumIntersections == 1) {
      // TODO should these be the same point?
      TS_ASSERT_EQUALS(case1_out.front(), case2_out.front());
    } else if (expNumIntersections == 2) {
      TS_ASSERT_EQUALS(case1_out.front(), case2_out.back());
      TS_ASSERT_EQUALS(case1_out.back(), case2_out.front());
    } else {
      TS_ASSERT_LESS_THAN(expNumIntersections, 3); // too many interactions
    }

    // pointing away from the cylinder
    Line case3(point2, Kernel::normalize(point2 - point1));
    Line::PType case3_out;
    case3.intersect(case3_out, cylinder);
    if (numinside == 2) {                    // TODO maybe
      TS_ASSERT_EQUALS(case3_out.size(), 1); // should still go through once
    } else {
      TS_ASSERT_EQUALS(case3_out.size(), 0); // should miss
    }
  }

  void testMultScattCylinder() {
    // these tests are a subset of the work done in MultipleScatteringCorrection
    Cylinder cylinder2mm;
    cylinder2mm.setNorm(V3D(0, 1, 0));
    cylinder2mm.setRadius(0.002);

    // verify correct cylinder was created
    TS_ASSERT_EQUALS(cylinder2mm.getCentre(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(cylinder2mm.getRadius(), 0.002);
    TS_ASSERT_EQUALS(cylinder2mm.getNormal(), V3D(0, 1, 0));

    // TODO unclear as to whether there is a point checking against 3mm
    Cylinder cylinder3mm;
    cylinder2mm.setNorm(V3D(0, 1, 0));
    cylinder2mm.setRadius(0.002);

    // both out and parallel to the plane
    std::cout << "CASE 1 !!!!!!!!!!!!!!!" << std::endl;
    multScattCylinderDetails(cylinder2mm, {0.00194856, -0.00475, 0.001125}, {0, -0.00475, 0.00225});

    // one out and one in, wacky angle
    std::cout << "CASE 2 !!!!!!!!!!!!!!!" << std::endl;
    multScattCylinderDetails(cylinder2mm, {-0.00108253, -0.00375, -0.000625}, {0, -0.00475, 0.00225});

    // // both in, wacky angle
    std::cout << "CASE 3 !!!!!!!!!!!!!!!" << std::endl;
    multScattCylinderDetails(cylinder2mm, {-0.00108253, -0.00375, -0.000625}, {-0.000625, -0.00475, -0.00108253});

    // // both in and parallel to the plane
    std::cout << "CASE 4 !!!!!!!!!!!!!!!" << std::endl;
    multScattCylinderDetails(cylinder2mm, {-0.00108253, -0.00375, -0.000625}, {-0.000625, -0.00375, -0.00108253});

    // need some cases where the cylinder is missed altogether
  }

  void testCylinderOnSurface() {
    // 2mm cylinder aligned with the y-axis
    constexpr double RADIUS{0.002};
    Cylinder cylinder2mm;
    cylinder2mm.setNorm(V3D(0, 1, 0));
    cylinder2mm.setRadius(RADIUS);

    // create points on the cylinder to make sure they are all found on the surface
    const size_t numX{10};
    constexpr double STEP{RADIUS / static_cast<double>(numX)};
    const size_t numY{4};
    constexpr double Y_OFFSET{0.5 * STEP * static_cast<double>(numY)};
    for (size_t i = 0; i < numX; ++i) {
      const double x = static_cast<double>(i) * STEP;
      const double z = sqrt(RADIUS * RADIUS - x * x);
      for (size_t j = 0; j < numY; ++j) {
        const V3D position(x, static_cast<double>(j) * STEP - Y_OFFSET, z);
        TS_ASSERT_EQUALS(cylinder2mm.onSurface(position), 1);
      }
    }
  }

  // A Line with equation equivalent to x axis will cut A Cylinder with 1 radius
  // with center at 0,0,0  y axis normal
  // at two points
  void testNotOriginIntersectCylinder() {
    Line A(V3D(-10.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(-10.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    Cylinder B;
    B.setSurface("c/y 0.0 0.0 1.0");
    TS_ASSERT_EQUALS(B.getCentre(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(B.getRadius(), 1);
    TS_ASSERT_EQUALS(B.getNormal(), V3D(0, 1, 0));

    Line::PType pntOut;
    A.intersect(pntOut, B);

    TS_ASSERT_EQUALS(pntOut.size(), 2);
    auto itr = pntOut.begin();
    TS_ASSERT_EQUALS(*(itr++), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(*itr, V3D(-1.0, 0.0, 0.0));
  }

  // A Line with equation equivalent to x axis will cut a plane YZ with equation
  // x=5 will cut at one point 5,0,0
  // Some problem here the negative axis plane is not cut need to investigate
  void testIntersectPlane() {
    Line A(V3D(0.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    Plane B;
    TS_ASSERT_EQUALS(B.setSurface("px 5 0 0"), 0);
    Line::PType pntOut;
    A.intersect(pntOut, B);

    TS_ASSERT_EQUALS(pntOut.size(), 1);
    TS_ASSERT_EQUALS(pntOut.front(), V3D(5.0, 0.0, 0.0));
  }

  // A Line with equation equivalent to x axis will cut A sphere with 2 radius
  // with center at 0,0,0
  // at two points
  void testIntersectSphere() {
    Line A(V3D(0.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(0.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    Sphere B;
    B.setSurface("s 0.0 0.0 0.0 2");
    Line::PType pntOut;
    A.intersect(pntOut, B);
    // forward only solutions
    TS_ASSERT_EQUALS(pntOut.size(), 1);
    TS_ASSERT_EQUALS(pntOut.front(), V3D(2.0, 0.0, 0.0));
  }

  // A Line with equation equivalent to x axis starting at -10 will cut A sphere
  // with 2 radius with center at 0,0,0
  // at two points
  void testNotOriginIntersectSphere() {
    Line A(V3D(-10.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getOrigin(), V3D(-10.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(A.getDirect(), V3D(1.0, 0.0, 0.0));

    Sphere B;
    B.setSurface("s 0.0 0.0 0.0 2");
    Line::PType pntOut;
    A.intersect(pntOut, B);
    TS_ASSERT_EQUALS(pntOut.size(), 2);
    auto itr = pntOut.begin();
    TS_ASSERT_EQUALS(*(itr++), V3D(2.0, 0.0, 0.0));
    TS_ASSERT_EQUALS(*itr, V3D(-2.0, 0.0, 0.0));
  }
};
