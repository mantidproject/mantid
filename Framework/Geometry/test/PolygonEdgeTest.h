#ifndef MANTID_GEOMETRY_POLYGONEDGETEST_H_
#define MANTID_GEOMETRY_POLYGONEDGETEST_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/V2D.h"
#include <cxxtest/TestSuite.h>

using Mantid::Geometry::PointClassification;
using Mantid::Geometry::PolygonEdge;
using Mantid::Kernel::V2D;

class PolygonEdgeTest : public CxxTest::TestSuite {
public:
  void test_Constructor_Does_Not_Throw() {
    TS_ASSERT_THROWS_NOTHING(PolygonEdge(V2D(), V2D()));
    TS_ASSERT_THROWS_NOTHING(PolygonEdge(V2D(), V2D(0, 1)));
    TS_ASSERT_THROWS_NOTHING(PolygonEdge(V2D(-0.1, 2.5), V2D(-0.5, -2.5)));
    TS_ASSERT_THROWS_NOTHING(PolygonEdge(V2D(1.5, 2.6), V2D(-0.6, 3.7)));
    TS_ASSERT_THROWS_NOTHING(PolygonEdge(V2D(), V2D(-0.6, 3.7)));
  }

  void test_Point_Accessors_Give_Back_Correct_Value() {
    const V2D start;
    const V2D end(0.5, 0.5);
    PolygonEdge side(V2D(), V2D(0.5, 0.5));
    TS_ASSERT_EQUALS(side.start(), start);
    TS_ASSERT_EQUALS(side.end(), end);
  }

  void test_Point_Fraction_Along_Edge() {
    const PolygonEdge edge(V2D(0.1, 0.1), V2D(2.0, 2.0));
    TS_ASSERT_EQUALS(edge.point(0.5), V2D(1.05, 1.05));
  }

  void test_Intersection_Type_With_Second_Edge() {
    const PolygonEdge edge(V2D(0.1, 0.1), V2D(2.0, 2.0));
    double t(0.0);
    TS_ASSERT_EQUALS(
        orientation(edge, PolygonEdge(V2D(0.0, 1.0), V2D(2.0, 1.0)), t),
        PolygonEdge::Skew);
    TS_ASSERT_EQUALS(
        orientation(edge, PolygonEdge(V2D(0.2, 0.2), V2D(2.0, 2.0)), t),
        PolygonEdge::Collinear);
    TS_ASSERT_EQUALS(
        orientation(edge, PolygonEdge(V2D(0.2, 0.3), V2D(2.0, 2.1)), t),
        PolygonEdge::Parallel);
  }

  void test_A_Valid_Intersection_Gives_A_Valid_Crossing_Pt() {
    const PolygonEdge edge(V2D(0.1, 0.1), V2D(2.0, 2.0));
    V2D crossPt;
    PolygonEdge::Orientation orient =
        crossingPoint(edge, PolygonEdge(V2D(0.0, 1.0), V2D(2.0, 1.0)), crossPt);
    TS_ASSERT_EQUALS(orient, PolygonEdge::SkewCross);
    TS_ASSERT_EQUALS(crossPt.X(), 1.0);
    TS_ASSERT_EQUALS(crossPt.Y(), 1.0);

    // Skewed but no cross
    TS_ASSERT_EQUALS(
        crossingPoint(edge, PolygonEdge(V2D(), V2D(1.5, 0.75)), crossPt),
        PolygonEdge::SkewNoCross);
    TS_ASSERT_EQUALS(
        crossingPoint(edge, PolygonEdge(V2D(0.2, 0.2), V2D(2.0, 2.0)), crossPt),
        PolygonEdge::Collinear);
    TS_ASSERT_EQUALS(
        crossingPoint(edge, PolygonEdge(V2D(0.2, 0.3), V2D(2.0, 2.1)), crossPt),
        PolygonEdge::Parallel);
  }

  void test_Classification_Of_Points() {
    const PolygonEdge edge(V2D(0.1, 0.1), V2D(2.0, 2.0));

    TS_ASSERT_EQUALS(classify(V2D(0.05, 0.1), edge), Mantid::Geometry::OnLeft);
    TS_ASSERT_EQUALS(classify(V2D(0.3, 0.1), edge), Mantid::Geometry::OnRight);
    TS_ASSERT_EQUALS(classify(V2D(-0.05, -0.05), edge),
                     Mantid::Geometry::Behind);
    TS_ASSERT_EQUALS(classify(V2D(2.5, 2.5), edge), Mantid::Geometry::Beyond);
    TS_ASSERT_EQUALS(classify(V2D(1.4, 1.4), edge), Mantid::Geometry::Between);
    TS_ASSERT_EQUALS(classify(edge.start(), edge), Mantid::Geometry::Origin);
    TS_ASSERT_EQUALS(classify(edge.end(), edge), Mantid::Geometry::Destination);
  }
};

#endif /* MANTID_GEOMETRY_POLYGONEDGETEST_H_ */
