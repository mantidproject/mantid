#ifndef MANTID_GEOMETRY_PROJECTIONTEST_H_
#define MANTID_GEOMETRY_PROJECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/Projection.h"

using namespace Mantid;
using namespace Mantid::Geometry;

class ProjectionTest : public CxxTest::TestSuite {
public:
  void test_blank_constructor() {
    Projection p;
    TS_ASSERT_EQUALS(p.getNumDims(), 1);
    TS_ASSERT_EQUALS(p.getOffset(0), 0.0);
    TS_ASSERT_EQUALS(p.getAxis(0).getNumDims(), 1);
  }

  void test_ndim_constructor() {
    Projection p1(1), p2(2), p3(3), p4(4);
    TS_ASSERT_EQUALS(p1.getNumDims(), 1);
    TS_ASSERT_EQUALS(p2.getNumDims(), 2);
    TS_ASSERT_EQUALS(p3.getNumDims(), 3);
    TS_ASSERT_EQUALS(p4.getNumDims(), 4);

    TS_ASSERT_EQUALS(p1.getAxis(0).getNumDims(), 1);
    TS_ASSERT_EQUALS(p2.getAxis(0).getNumDims(), 2);
    TS_ASSERT_EQUALS(p3.getAxis(0).getNumDims(), 3);
    TS_ASSERT_EQUALS(p4.getAxis(0).getNumDims(), 4);

    TS_ASSERT_EQUALS(p4.getOffset(0), 0.0);
    TS_ASSERT_EQUALS(p4.getOffset(1), 0.0);
    TS_ASSERT_EQUALS(p4.getOffset(2), 0.0);
    TS_ASSERT_EQUALS(p4.getOffset(3), 0.0);
  }

  void test_uvw_constructors() {
    VMD u(1, -1, 0);
    VMD v(1, 1, 0);
    VMD w(0, 0, 1);
    Projection p(u, v, w);

    TS_ASSERT_EQUALS(p.getNumDims(), 3);
    TS_ASSERT_EQUALS(p.U(), u);
    TS_ASSERT_EQUALS(p.V(), v);
    TS_ASSERT_EQUALS(p.W(), w);
    TS_ASSERT_EQUALS(p.getAxis(0), u);
    TS_ASSERT_EQUALS(p.getAxis(1), v);
    TS_ASSERT_EQUALS(p.getAxis(2), w);
  }

  void test_throw_invalid_dimension_constructor() {
    TS_ASSERT_THROWS_ANYTHING(Projection(0));
    TS_ASSERT_THROWS_ANYTHING(Projection(-1));
  }

  void test_throw_out_of_range_access() {
    Projection p(3);
    TS_ASSERT_THROWS_ANYTHING(p.getOffset(-1));
    TS_ASSERT_THROWS_NOTHING(p.getOffset(2));
    TS_ASSERT_THROWS_ANYTHING(p.getOffset(3));

    TS_ASSERT_THROWS_ANYTHING(p.getAxis(-1));
    TS_ASSERT_THROWS_NOTHING(p.getAxis(2));
    TS_ASSERT_THROWS_ANYTHING(p.getAxis(3));
  }

  void test_copy_constructor() {
    VMD u(1, -1, 0);
    VMD v(1, 1, 0);
    VMD w(0, 0, 1);
    Projection p(u, v, w);

    Projection q(p);

    TS_ASSERT_EQUALS(q.U(), u);
    TS_ASSERT_EQUALS(q.V(), v);
    TS_ASSERT_EQUALS(q.W(), w);
  }

  void test_assign() {
    VMD u(1, -1, 0);
    VMD v(1, 1, 0);
    VMD w(0, 0, 1);
    Projection p(u, v, w);

    Projection q(5);

    TS_ASSERT_EQUALS(q.getNumDims(), 5);
    q = p;
    TS_ASSERT_EQUALS(q.getNumDims(), 3);

    TS_ASSERT_EQUALS(q.U(), u);
    TS_ASSERT_EQUALS(q.V(), v);
    TS_ASSERT_EQUALS(q.W(), w);
  }
};

#endif /* MANTID_GEOMETRY_PROJECTIONTEST_H_ */
