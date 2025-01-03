// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <ostream>
#include <sstream>
#include <vector>

#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

class SphereTest : public CxxTest::TestSuite {

public:
  void testDefaultConstructor() {
    Sphere A;
    // both centre and radius = 0
    TS_ASSERT_EQUALS(extractString(A), "-1 so 0\n");
    TS_ASSERT_EQUALS(A.getCentre(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(A.getRadius(), 0);
  }

  void testConstructor() {
    const V3D centre{0, 0, 1};
    const double radius{0.5};
    Sphere A{centre, radius};

    TS_ASSERT_EQUALS(extractString(A), "-1 s [0,0,1] 0.5\n");
    TS_ASSERT_EQUALS(A.getCentre(), centre);
    TS_ASSERT_EQUALS(A.getRadius(), radius);
  }

  void testsetSurface() {
    Sphere A;
    A.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_EQUALS(A.getCentre(), V3D(1.1, -2.1, 1.1));
    TS_ASSERT_EQUALS(A.getRadius(), 2);
    TS_ASSERT_EQUALS(extractString(A), "-1 s [1.1,-2.1,1.1] 2\n");
  }

  void testClone() {
    Sphere A;
    A.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_EQUALS(extractString(A), "-1 s [1.1,-2.1,1.1] 2\n");
    auto B = A.clone();
    TS_ASSERT_EQUALS(extractString(*B), "-1 s [1.1,-2.1,1.1] 2\n");
  }

  /// is a point inside outside or on the side!
  void testSide() {
    Sphere A;
    // radius 2 at the origin
    A.setSurface("so 2");
    TS_ASSERT_EQUALS(extractString(A), "-1 so 2\n");

    // Origin should be inside
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 0)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(1.9, 0, 0)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 1.9, 0)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 1.9)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, -1.9)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(-1.9, 0, 0)), -1);
    TS_ASSERT_EQUALS(A.side(V3D(0, -1.9, 0)), -1);

    // should be on the side
    TS_ASSERT_EQUALS(A.side(V3D(2, 0, 0)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0, 2, 0)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 2)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, -2)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(-2, 0, 0)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0, -2, 0)), 0);
    // test tolerance at default 1e-6
    TS_ASSERT_EQUALS(A.side(V3D(0, -2 + 1e-7, 0)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0, -2 - 1e-7, 0)), 0);
    TS_ASSERT_EQUALS(A.side(V3D(0, -2 - 2e-6, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0, -2 + 2e-6, 0)), -1);
    // should be outside
    TS_ASSERT_EQUALS(A.side(V3D(2.1, 0, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 2.1, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, 2.1)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(-2.1, 0, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0, -2.1, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0, -2.1)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(2, 0.1, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0.1, 2, 0)), 1);
    TS_ASSERT_EQUALS(A.side(V3D(0, 0.1, 2)), 1);
  }

  /// is a point inside outside or on the side!
  void testOnSurface() {
    Sphere A;
    // radius 2 at the origin
    A.setSurface("so 2");
    TS_ASSERT_EQUALS(extractString(A), "-1 so 2\n");

    // Origin should not be on the surface
    TS_ASSERT(!A.onSurface(V3D(0, 0, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 0, 0)));
    TS_ASSERT(!A.onSurface(V3D(1.9, 0, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 1.9, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 0, 1.9)));
    TS_ASSERT(!A.onSurface(V3D(0, 0, -1.9)));
    TS_ASSERT(!A.onSurface(V3D(-1.9, 0, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, -1.9, 0)));

    // should be on the surface
    TS_ASSERT(A.onSurface(V3D(2, 0, 0)));
    TS_ASSERT(A.onSurface(V3D(0, 2, 0)));
    TS_ASSERT(A.onSurface(V3D(0, 0, 2)));
    TS_ASSERT(A.onSurface(V3D(0, 0, -2)));
    TS_ASSERT(A.onSurface(V3D(-2, 0, 0)));
    TS_ASSERT(A.onSurface(V3D(0, -2, 0)));
    // test tolerance at default 1e-6
    TS_ASSERT(A.onSurface(V3D(0, -2 + 1e-7, 0)));
    TS_ASSERT(A.onSurface(V3D(0, -2 - 1e-7, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, -2 - 2e-6, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, -2 + 2e-6, 0)));
    // should not be on the surface
    TS_ASSERT(!A.onSurface(V3D(2.1, 0, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 2.1, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 0, 2.1)));
    TS_ASSERT(!A.onSurface(V3D(-2.1, 0, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, -2.1, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 0, -2.1)));
    TS_ASSERT(!A.onSurface(V3D(2, 0.1, 0)));
    TS_ASSERT(!A.onSurface(V3D(0.1, 2, 0)));
    TS_ASSERT(!A.onSurface(V3D(0, 0.1, 2)));
  }

  void testSphereDistance() {
    Sphere A;
    A.setSurface("so 5"); // sphere at origin radius 5

    // just outside
    TS_ASSERT_DELTA(A.distance(V3D(5.1, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 5.1, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, 5.1)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-5.1, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, -5.1, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, -5.1)), 0.1, 1e-5);

    // just inside
    TS_ASSERT_DELTA(A.distance(V3D(4.9, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 4.9, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, 4.9)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-4.9, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, -4.9, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, -4.9)), 0.1, 1e-5);

    // distant
    TS_ASSERT_DELTA(A.distance(V3D(100, 0, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 100, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, 100)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-100, 0, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, -100, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, -100)), 95, 1e-5);
  }

  void testSphereDistanceTrue() {
    Sphere A;
    A.setSurface("so 5"); // sphere at origin radius 5

    // just outside
    TS_ASSERT_DELTA(A.distance(V3D(5.1, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 5.1, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, 5.1)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-5.1, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, -5.1, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, -5.1)), 0.1, 1e-5);

    // just inside
    TS_ASSERT_DELTA(A.distance(V3D(4.9, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 4.9, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, 4.9)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-4.9, 0, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, -4.9, 0)), 0.1, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, -4.9)), 0.1, 1e-5);

    // distant
    TS_ASSERT_DELTA(A.distance(V3D(100, 0, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 100, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, 100)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-100, 0, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, -100, 0)), 95, 1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0, 0, -100)), 95, 1e-5);
  }

  void testSphereDistanceComplex()
  /**
  Test the distance of a point from the cone
  @retval -1 :: failed build a cone
  @retval -2 :: Failed on the distance calculation
  @retval 0 :: All passed
  */
  {
    std::vector<std::string> SphStr{"so 1", "s 1.5 -2.5 1.8 1"}; // sphere origin, sphere
    Kernel::V3D P(3, 7, 4);
    Kernel::V3D Q(0, 0, 4);
    std::vector<std::string>::const_iterator vc;
    Sphere A;

    for (vc = SphStr.begin(); vc != SphStr.end(); vc++) {
      const int retVal = A.setSurface(*vc);
      TS_ASSERT(retVal == 0);
      // Surface distance and Sphere distance are the same:
      if (fabs(A.distance(P) - A.distance(P)) > 1e-6) {
        TS_ASSERT_DELTA(A.distance(P), A.distance(P), 1e-6);
        std::cout << "Sphere == ";
        A.Surface::write(std::cout);
        std::cout << "TestPoint == " << P << '\n';
        std::cout << "Distance == " << A.distance(P) << " === " << A.distance(P) << '\n';
        std::cout << "--------------\n";
        std::cout << "Distance == " << A.distance(Q) << " === " << A.distance(Q) << '\n';
      }
    }
  }

  void testSurfaceNormal() {
    Sphere A;
    A.setSurface("so 5");

    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(10, 0, 0)), V3D(1, 0, 0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0, 10, 0)), V3D(0, 1, 0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0, 0, 10)), V3D(0, 0, 1));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(-10, 0, 0)), V3D(-1, 0, 0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0, -10, 0)), V3D(0, -1, 0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0, 0, -10)), V3D(0, 0, -1));

    V3D result = A.surfaceNormal(V3D(10, 10, 0));
    TS_ASSERT_DELTA(result.X(), 0.7071, 1e-5);
    TS_ASSERT_DELTA(result.Y(), 0.7071, 1e-5);
    TS_ASSERT_DELTA(result.Z(), 0.0, 1e-5);
  }

  void testSetCentre() {
    Sphere A;
    // centre at origin and radius = 2
    TS_ASSERT_EQUALS(extractString(A), "-1 so 0\n");
    TS_ASSERT_EQUALS(A.getCentre(), V3D(0, 0, 0));
    TS_ASSERT_EQUALS(A.getRadius(), 0);

    V3D point(1, 1, 1);
    A.setCentre(point);
    TS_ASSERT_EQUALS(extractString(A), "-1 s [1,1,1] 0\n");
    TS_ASSERT_EQUALS(A.getCentre(), point);
    TS_ASSERT_EQUALS(A.getRadius(), 0);

    V3D point2(-12.1, 51.6, -563.1);
    A.setCentre(point2);
    TS_ASSERT_EQUALS(extractString(A), "-1 s [-12.1,51.6,-563.1] 0\n");
    TS_ASSERT_EQUALS(A.getCentre(), point2);
    TS_ASSERT_EQUALS(A.getRadius(), 0);
  }

  void testGetBoundingBox() {
    Sphere A;
    A.setSurface("so 1");
    TS_ASSERT_EQUALS(extractString(A), "-1 so 1\n");

    double xmax, ymax, zmax, xmin, ymin, zmin;
    xmax = ymax = zmax = 20;
    xmin = ymin = zmin = -20;
    A.getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
    TS_ASSERT_DELTA(xmax, 1.0, 0.00001);
    TS_ASSERT_DELTA(ymax, 1.0, 0.00001);
    TS_ASSERT_DELTA(zmax, 1.0, 0.00001);
    TS_ASSERT_DELTA(xmin, -1.0, 0.00001);
    TS_ASSERT_DELTA(ymin, -1.0, 0.00001);
    TS_ASSERT_DELTA(zmin, -1.0, 0.00001);
    xmax = ymax = zmax = 0.5;
    xmin = ymin = zmin = -20;
    A.getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
    TS_ASSERT_DELTA(xmax, 1.0, 0.00001);
    TS_ASSERT_DELTA(ymax, 1.0, 0.00001);
    TS_ASSERT_DELTA(zmax, 1.0, 0.00001);
    TS_ASSERT_DELTA(xmin, -1.0, 0.00001);
    TS_ASSERT_DELTA(ymin, -1.0, 0.00001);
    TS_ASSERT_DELTA(zmin, -1.0, 0.00001);
  }

  void testEvalValue() {
    Sphere A;
    A.setCentre(V3D(0.0, 0.0, 0.0));
    A.setRadius(1.0);
    TS_ASSERT_DELTA(A.eqnValue(V3D(0.0, 0.0, 0.0)), -1.0, 0.0001);
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
