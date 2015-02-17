#ifndef MANTID_API_PROJECTIONTEST_H_
#define MANTID_API_PROJECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Projection.h"

using namespace Mantid;
using namespace Mantid::API;

class ProjectionTest : public CxxTest::TestSuite {
public:
  void test_blank_constructor() {
    Projection p;
    TS_ASSERT_EQUALS(p.getNumDims(), 2);
    TS_ASSERT_EQUALS(p.getOffset(0), 0.0);
    TS_ASSERT_EQUALS(p.getAxis(0).getNumDims(), 2);
    TS_ASSERT_EQUALS(p.getUnit(0), RLU);
  }

  void test_ndim_constructor() {
    Projection p2(2), p3(3), p4(4);
    TS_ASSERT_EQUALS(p2.getNumDims(), 2);
    TS_ASSERT_EQUALS(p3.getNumDims(), 3);
    TS_ASSERT_EQUALS(p4.getNumDims(), 4);

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
    TS_ASSERT_EQUALS(p.getAxis(0), u);
    TS_ASSERT_EQUALS(p.getAxis(1), v);
    TS_ASSERT_EQUALS(p.getAxis(2), w);
    TS_ASSERT_EQUALS(p.getAxis(0), u);
    TS_ASSERT_EQUALS(p.getAxis(1), v);
    TS_ASSERT_EQUALS(p.getAxis(2), w);
  }

  void test_throw_invalid_dimension_constructor() {
    TS_ASSERT_THROWS_ANYTHING(Projection(-1));
    TS_ASSERT_THROWS_ANYTHING(Projection(0));
    TS_ASSERT_THROWS_ANYTHING(Projection(1));
    TS_ASSERT_THROWS_NOTHING(Projection(2));
  }

  void test_throw_out_of_range_access() {
    Projection p(3);
    TS_ASSERT_THROWS_ANYTHING(p.getOffset(-1));
    TS_ASSERT_THROWS_NOTHING(p.getOffset(2));
    TS_ASSERT_THROWS_ANYTHING(p.getOffset(3));

    TS_ASSERT_THROWS_ANYTHING(p.getAxis(-1));
    TS_ASSERT_THROWS_NOTHING(p.getAxis(2));
    TS_ASSERT_THROWS_ANYTHING(p.getAxis(3));

    TS_ASSERT_THROWS_ANYTHING(p.getUnit(-1));
    TS_ASSERT_THROWS_NOTHING(p.getUnit(2));
    TS_ASSERT_THROWS_ANYTHING(p.getUnit(3));
  }

  void test_copy_constructor() {
    VMD u(1, -1, 0);
    VMD v(1, 1, 0);
    VMD w(0, 0, 1);
    Projection p(u, v, w);
    p.setUnit(0, RLU);
    p.setUnit(1, INV_ANG);

    Projection q(p);

    TS_ASSERT_EQUALS(q.getAxis(0), u);
    TS_ASSERT_EQUALS(q.getAxis(1), v);
    TS_ASSERT_EQUALS(q.getAxis(2), w);
    TS_ASSERT_EQUALS(q.getUnit(0), RLU);
    TS_ASSERT_EQUALS(q.getUnit(1), INV_ANG);
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

    TS_ASSERT_EQUALS(q.getAxis(0), u);
    TS_ASSERT_EQUALS(q.getAxis(1), v);
    TS_ASSERT_EQUALS(q.getAxis(2), w);
  }

  void test_setOffset() {
    Projection p(3);
    p.setOffset(0, 1.00);
    p.setOffset(1, 1.50);
    p.setOffset(2, 4.75);
    TS_ASSERT_EQUALS(p.getOffset(0), 1.00);
    TS_ASSERT_EQUALS(p.getOffset(1), 1.50);
    TS_ASSERT_EQUALS(p.getOffset(2), 4.75);
  }

  void test_setAxis() {
    Projection p(3);
    p.setAxis(0, VMD(1,2,3));
    p.setAxis(1, VMD(4,5,6));
    p.setAxis(2, VMD(7,8,8));
    TS_ASSERT_EQUALS(p.getAxis(0), VMD(1,2,3));
    TS_ASSERT_EQUALS(p.getAxis(1), VMD(4,5,6));
    TS_ASSERT_EQUALS(p.getAxis(2), VMD(7,8,8));
  }

  void test_setUnit() {
    Projection p(3);
    p.setUnit(0, INV_ANG);
    p.setUnit(1, RLU);
    p.setUnit(2, INV_ANG);
    TS_ASSERT_EQUALS(p.getUnit(0), INV_ANG);
    TS_ASSERT_EQUALS(p.getUnit(1), RLU);
    TS_ASSERT_EQUALS(p.getUnit(2), INV_ANG);
  }
};

#endif /* MANTID_API_PROJECTIONTEST_H_ */
