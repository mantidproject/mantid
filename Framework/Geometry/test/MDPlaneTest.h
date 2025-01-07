// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VMD.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::VMD;

class MDPlaneTest : public CxxTest::TestSuite {
public:
  void test_constructor_vectors() {
    std::vector<coord_t> normal;
    std::vector<coord_t> point;
    TSM_ASSERT_THROWS_ANYTHING("O-dimensions are not allowed.", MDPlane test(normal, point));
    normal.emplace_back(1.234f);
    normal.emplace_back(4.56f);
    point.emplace_back(0.f);
    TSM_ASSERT_THROWS_ANYTHING("Mismatched dimensions in normal/point are not allowed.", MDPlane test(normal, point));
    point.emplace_back(0.f);
    MDPlane p(normal, point);
    TS_ASSERT_EQUALS(p.getNumDims(), 2);
    TS_ASSERT_DELTA(p.getNormal()[0], 1.234, 1e-5);
    TS_ASSERT_DELTA(p.getNormal()[1], 4.56, 1e-5);
    TS_ASSERT_DELTA(p.getInequality(), 0, 1e-5);
  }

  void test_constructor_bareArrays() {
    coord_t normal[2] = {1.234f, 4.56f};
    coord_t point[2] = {1.0, 0.0};
    TSM_ASSERT_THROWS_ANYTHING("O-dimensions are not allowed.", MDPlane test(0, normal, point));
    MDPlane p(2, normal, point);
    TS_ASSERT_EQUALS(p.getNumDims(), 2);
    TS_ASSERT_DELTA(p.getNormal()[0], 1.234, 1e-5);
    TS_ASSERT_DELTA(p.getNormal()[1], 4.56, 1e-5);
    TS_ASSERT_DELTA(p.getInequality(), 1.234, 1e-5);
  }

  void test_constructor_VMD() {
    VMD normal(1.234, 4.56);
    VMD point(1.0, 0.0);
    MDPlane p(normal, point);
    TS_ASSERT_EQUALS(p.getNumDims(), 2);
    TS_ASSERT_DELTA(p.getNormal()[0], 1.234, 1e-5);
    TS_ASSERT_DELTA(p.getNormal()[1], 4.56, 1e-5);
    TS_ASSERT_DELTA(p.getInequality(), 1.234, 1e-5);
  }

  void test_constructorVectors_badInputs() {
    std::vector<VMD> points;
    VMD insidePoint(1);
    TS_ASSERT_THROWS_ANYTHING(MDPlane p(points, VMD(1, 2, 3), insidePoint));
    points.emplace_back(VMD(1, 2, 3));
    TS_ASSERT_THROWS_ANYTHING(MDPlane p(points, VMD(1, 2, 3), VMD(2, 3, 4)));
  }

  void test_constructorVectors_2D() {
    std::vector<VMD> points;
    points.emplace_back(VMD(1.0, 1.0));
    MDPlane p(points, VMD(0., 0.), VMD(1.5, 0.5));
    TS_ASSERT(p.isPointBounded(VMD(0.2, 0.1)));
  }

  /// Define a plane along x=y axis vertical in Z
  void test_constructorVectors_3D() {
    std::vector<VMD> points;
    points.emplace_back(VMD(1., 1., 0.));
    points.emplace_back(VMD(0., 0., 1.));
    MDPlane p(points, VMD(0., 0., 0.), VMD(0.5, 1.5, 1.0));
    TS_ASSERT(p.isPointBounded(VMD(0.5, 1.5, 1.0)));
  }

  /// Bad vectors = they are collinear
  void test_constructorVectors_3D_collinear() {
    std::vector<VMD> points;
    points.emplace_back(VMD(1., 1., 0.));
    points.emplace_back(VMD(2., 2., 0.));
    TS_ASSERT_THROWS_ANYTHING(MDPlane p(points, VMD(0., 0., 0.), VMD(0.5, 1.5, 1.0)));
  }

  /// Define a plane along x=y axis vertical in Z and t
  void test_constructorVectors_4D() {
    std::vector<VMD> points;
    points.emplace_back(VMD(1., 1., 0., 0.));
    points.emplace_back(VMD(0., 0., 1., 0.));
    points.emplace_back(VMD(0., 0., 0., 1.));
    MDPlane p(points, VMD(0., 0., 0., 0.), VMD(0.5, 1.5, 1.0, 1.0));
    TS_ASSERT(p.isPointBounded(VMD(0.5, 1.5, 1.0, -23.0)));
    TS_ASSERT(!p.isPointBounded(VMD(1.5, 0.5, 1.0, -23.0)));
  }

  void test_copy_ctor() {
    coord_t normal[2] = {1.25, 4.5};
    coord_t point[2] = {1.0, 0.0};
    MDPlane p_orig(2, normal, point);
    MDPlane p(p_orig);
    TS_ASSERT_EQUALS(p.getNumDims(), 2);
    TS_ASSERT_DELTA(p.getNormal()[0], 1.25, 1e-5);
    TS_ASSERT_DELTA(p.getNormal()[1], 4.5, 1e-5);
    TS_ASSERT_DELTA(p.getInequality(), p_orig.getInequality(), 1e-5);
  }

  void test_assignment_operator() {
    coord_t normal[2] = {1.25, 4.5};
    coord_t point[2] = {1.0, 0.0};
    coord_t normal3[3] = {434, 456, 789};
    coord_t point3[3] = {1.0, 0.0, 0.0};
    MDPlane p_orig(2, normal, point);
    MDPlane p(3, normal3, point3);
    p = p_orig;
    TS_ASSERT_EQUALS(p.getNumDims(), 2);
    TS_ASSERT_DELTA(p.getNormal()[0], 1.25, 1e-5);
    TS_ASSERT_DELTA(p.getNormal()[1], 4.5, 1e-5);
    TS_ASSERT_DELTA(p.getInequality(), 1.25, 1e-5);
  }

  /// Helper function for the 2D case
  bool try2Dpoint(MDPlane &p, double x, double y) {
    coord_t centers[2] = {static_cast<coord_t>(x), static_cast<coord_t>(y)};
    return p.isPointBounded(centers);
  }

  /// 2D test with some simple linear inequations
  void test_2D_point() {
    // Plane where x < 5
    coord_t normal1[2] = {-1., 0};
    coord_t point1[2] = {5., 0};
    MDPlane p1(2, normal1, point1);
    TS_ASSERT(try2Dpoint(p1, 4.0, 12.));
    TS_ASSERT(!try2Dpoint(p1, 6.0, -5.));
    TS_ASSERT(!try2Dpoint(p1, 5.001, 1.));

    // Plane where x > 5
    coord_t normal2[2] = {+1., 0};
    MDPlane p2(2, normal2, point1);
    TS_ASSERT(!try2Dpoint(p2, 4.0, 12.));
    TS_ASSERT(try2Dpoint(p2, 6.0, -5.));
    TS_ASSERT(try2Dpoint(p2, 5.001, 1.));

    // Plane where y < 10
    coord_t normal3[2] = {0., -1.};
    coord_t point3[2] = {0., 10.};
    MDPlane p3(2, normal3, point3);
    TS_ASSERT(try2Dpoint(p3, 100., 9.0));
    TS_ASSERT(!try2Dpoint(p3, -99., 11.0));

    // Plane below a 45 degree line passing through (0,0)
    coord_t normal4[2] = {1., -1.};
    coord_t point4[2] = {0., 0.};
    MDPlane p4(2, normal4, point4);
    TS_ASSERT(try2Dpoint(p4, 1., 0.1));
    TS_ASSERT(try2Dpoint(p4, 1., 0.9));
    TS_ASSERT(try2Dpoint(p4, 1., -5.));
    TS_ASSERT(!try2Dpoint(p4, 1., 1.1));
    TS_ASSERT(!try2Dpoint(p4, 0., 0.1));

    // Plane above a 45 degree line passing through (0,2)
    coord_t normal5[2] = {-1., +1.};
    coord_t point5[2] = {0., 2.};
    MDPlane p5(2, normal5, point5);
    TS_ASSERT(!try2Dpoint(p5, 0., 1.99));
    TS_ASSERT(try2Dpoint(p5, 0., 2.01));
    TS_ASSERT(!try2Dpoint(p5, 0.1, 2.01));
  }

  /// Helper function for the 2D case of a line intersecting the plane
  bool try2Dline(MDPlane &p, double x1, double y1, double x2, double y2) {
    coord_t centers1[2] = {static_cast<coord_t>(x1), static_cast<coord_t>(y1)};
    coord_t centers2[2] = {static_cast<coord_t>(x2), static_cast<coord_t>(y2)};
    return p.doesLineIntersect(centers1, centers2);
  }

  void test_2D_line() {
    // Plane where x < 5
    coord_t normal1[2] = {-1., 0};
    coord_t point1[2] = {5., 0};
    MDPlane p1(2, normal1, point1);
    TS_ASSERT(try2Dline(p1, 1, 2, 6, 2));
    TS_ASSERT(try2Dline(p1, 10, 12, 4.99, 8));
    TS_ASSERT(!try2Dline(p1, 5.01, 2, 5.02, 2));
    TS_ASSERT(!try2Dline(p1, 4.99, 2, 4.25, 2));

    // Plane below a 45 degree line passing through (0,0)
    coord_t normal4[2] = {1., -1.};
    coord_t point4[2] = {0., 0.};
    MDPlane p4(2, normal4, point4);
    TS_ASSERT(try2Dline(p4, 0.1, 0.0, 0.1, 0.2));
    TS_ASSERT(!try2Dline(p4, 0.1, 0.0, 0.3, 0.2));
    TS_ASSERT(try2Dline(p4, 0.1, 0.2, 0.3, 0.2));
  }

  void test_isPointBounded_vectorversion() {
    // Plane where x < 5
    coord_t normal1[2] = {-1., 0};
    coord_t point1[2] = {5., 0};
    MDPlane p1(2, normal1, point1);
    std::vector<coord_t> point;
    point.clear();
    point.emplace_back(4.0f);
    point.emplace_back(12.0f);
    TS_ASSERT(p1.isPointBounded(point));

    point.clear();
    point.emplace_back(5.0f);
    point.emplace_back(-5.0f);
    TSM_ASSERT("Point should be found to be bounded by "
               "plane, it lies exactly on the plane",
               p1.isPointBounded(point));

    point.clear();
    point.emplace_back(6.0f);
    point.emplace_back(-5.0f);
    TS_ASSERT(!p1.isPointBounded(point));
  }

  void test_isPointInside_vectorversion() {
    // Plane where x < 5
    coord_t normal1[2] = {-1., 0};
    coord_t point1[2] = {5., 0};
    MDPlane p1(2, normal1, point1);
    std::vector<coord_t> point;
    point.clear();
    point.emplace_back(4.0f);
    point.emplace_back(12.0f);
    TSM_ASSERT("Point should be found to be inside region bounded by plane", p1.isPointInside(point));

    // Point lies on the plane, not inside it
    point.clear();
    point.emplace_back(5.0f);
    point.emplace_back(-5.0f);
    TSM_ASSERT("Point should not be found to be inside region bounded by "
               "plane, it lies exactly on the plane",
               !p1.isPointInside(point));
  }

  void test_isPointInside_arrayversion() {
    // Plane where x < 5
    coord_t normal1[2] = {-1.0, 0.0};
    coord_t point1[2] = {5.0, 0.0};
    MDPlane plane1(2, normal1, point1);
    coord_t test_point1[2] = {4.5, 0.0};
    TSM_ASSERT("Point should be found to be inside region bounded by plane", plane1.isPointInside(test_point1));

    // Point lies on the plane, not inside it
    coord_t test_point2[2] = {5.0, 0.0};
    TSM_ASSERT("Point should not be found to be inside region bounded by "
               "plane, it lies exactly on the plane",
               !plane1.isPointInside(test_point2));
  }
};

//=========================================================================================
class MDPlaneTestPerformance : public CxxTest::TestSuite {
public:
  void test_3D_point() {
    coord_t normal[3] = {1.25, 2.5, 3.5};
    coord_t point[3] = {1, 0, 0};

    coord_t pointA[3] = {0.111f, 0.222f, 0.333f};

    MDPlane p(3, normal, point);
    bool res = false;
    for (size_t i = 0; i < 5 * 1000000 /*5 million*/; i++) {
      res = p.isPointBounded(pointA);
      (void)res;
    }
    TS_ASSERT(res);
  }

  void test_4D_point() {
    coord_t normal[4] = {1.25, 2.5, 3.5, 4.75};
    coord_t point[4] = {1};

    coord_t pointA[4] = {0.111f, 0.222f, 0.333f, 0.444f};

    MDPlane p(4, normal, point);
    bool res = false;
    for (size_t i = 0; i < 5 * 1000000 /*5 million*/; i++) {
      res = p.isPointBounded(pointA);
      (void)res;
    }
    TS_ASSERT(res);
  }

  /** Looks to be about 50% slower on linux in debug! */
  void test_4D_point_vectorVersion() {
    coord_t normal[4] = {1.25f, 2.5f, 3.5f, 4.75f};
    coord_t point[4] = {1};

    std::vector<coord_t> pointA;
    pointA.emplace_back(0.111f);
    pointA.emplace_back(0.222f);
    pointA.emplace_back(0.333f);
    pointA.emplace_back(0.444f);

    MDPlane p(4, normal, point);
    bool res = false;
    for (size_t i = 0; i < 5 * 1000000 /*5 million*/; i++) {
      res = p.isPointBounded(pointA);
      (void)res;
    }
    TS_ASSERT(res);
  }

  void test_3D_line() {
    coord_t normal[3] = {1.23f, 2.34f, 3.45f};
    coord_t origin[3] = {3, 0, 0};
    coord_t pointA[3] = {0.111f, 0.222f, 0.333f};
    coord_t pointB[3] = {9.111f, 9.222f, 9.333f};

    MDPlane p(3, normal, origin);
    bool res = false;
    for (size_t i = 0; i < 5 * 1000000 /*5 million*/; i++) {
      res = p.doesLineIntersect(pointA, pointB);
      (void)res;
    }
    TS_ASSERT(res);
  }
};
