// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_POLYGONPOINTITERATORTEST_H_
#define MANTID_GEOMETRY_POLYGONPOINTITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/V2D.h"

using Mantid::Geometry::ConvexPolygon;
using Mantid::Kernel::V2D;

class ConvexPolygonIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvexPolygonIteratorTest *createSuite() {
    return new ConvexPolygonIteratorTest();
  }
  static void destroySuite(ConvexPolygonIteratorTest *suite) { delete suite; }

  // --------------------- Success cases --------------------------------------

  void test_Iterator_Initially_Points_At_Index_Zero() {
    auto poly = makeRectangle();
    ConvexPolygon::Iterator iter(poly);
    TS_ASSERT_EQUALS(V2D(), *iter);
  }

  void test_Increment_Moves_On_One_Point() {
    auto poly = makeRectangle();
    ConvexPolygon::Iterator iter(poly);
    ++iter;
    TS_ASSERT_EQUALS(V2D(0.0, 1.0), *iter);
  }

  void test_Incrementing_By_Number_Of_Points_Produces_First_Point() {
    auto poly = makeRectangle();
    ConvexPolygon::Iterator iter(poly);
    for (size_t i = 0; i < poly.npoints(); ++i) {
      ++iter;
    }
    TS_ASSERT_EQUALS(V2D(), *iter);
  }

  void test_edge_points_from_current_to_next() {
    auto poly = makeRectangle();
    ConvexPolygon::Iterator iter(poly);
    auto p01 = iter.edge();
    TS_ASSERT_EQUALS(V2D(), p01.start());
    TS_ASSERT_EQUALS(V2D(0.0, 1.0), p01.end());

    ++iter;
    auto p12 = iter.edge();
    TS_ASSERT_EQUALS(V2D(0.0, 1.0), p12.start());
    TS_ASSERT_EQUALS(V2D(2.0, 1.0), p12.end());

    // Check final has first as end point
    ++iter;
    ++iter;
    auto p40 = iter.edge();
    TS_ASSERT_EQUALS(V2D(2.0, 0.0), p40.start());
    TS_ASSERT_EQUALS(V2D(), p40.end());
  }

  // --------------------- Failure cases --------------------------------------

  void test_Invalid_Polygon_Theows_Error_On_Construction() {
    ConvexPolygon invalid;
    // TS_ASSERT_THROWS cannot be used if there is no default constructor
    // so use a pointer instead
    ConvexPolygon::Iterator *iter(nullptr);
    TS_ASSERT_THROWS(iter = new ConvexPolygon::Iterator(invalid),
                     const std::invalid_argument &);
    delete iter;
  }

private:
  ConvexPolygon makeRectangle() {
    ConvexPolygon rectangle;
    rectangle.insert(0.0, 0.0);
    rectangle.insert(0.0, 1.0);
    rectangle.insert(2.0, 1.0);
    rectangle.insert(2.0, 0.0);
    return rectangle;
  }
};

#endif /* MANTID_GEOMETRY_POLYGONPOINTITERATORTEST_H_ */
