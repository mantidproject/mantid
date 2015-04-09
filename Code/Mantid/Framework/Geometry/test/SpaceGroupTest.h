#ifndef MANTID_GEOMETRY_SPACEGROUPTEST_H_
#define MANTID_GEOMETRY_SPACEGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/ProductOfCyclicGroups.h"
#include "MantidGeometry/Crystal/CenteringGroup.h"

#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class SpaceGroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpaceGroupTest *createSuite() { return new SpaceGroupTest(); }
  static void destroySuite(SpaceGroupTest *suite) { delete suite; }

  void testConstruction() {
    Group_const_sptr inversion = GroupFactory::create<CyclicGroup>("-x,-y,-z");
    SpaceGroup p1bar(2, "P-1", *inversion);

    TS_ASSERT_EQUALS(p1bar.number(), 2);
    TS_ASSERT_EQUALS(p1bar.hmSymbol(), "P-1");
    TS_ASSERT_EQUALS(p1bar.order(), 2);
    TS_ASSERT_EQUALS(p1bar.getSymmetryOperations().size(), 2);
  }

  void testNumber() {
    TestableSpaceGroup empty;
    TS_ASSERT_EQUALS(empty.number(), 0);

    empty.m_number = 2;
    TS_ASSERT_EQUALS(empty.number(), 2);
  }

  void testSymbol() {
    TestableSpaceGroup empty;
    TS_ASSERT_EQUALS(empty.hmSymbol(), "");

    empty.m_hmSymbol = "Test";
    TS_ASSERT_EQUALS(empty.hmSymbol(), "Test");
  }

  void testAssignmentOperator() {
    Group_const_sptr inversion = GroupFactory::create<CyclicGroup>("-x,-y,-z");
    SpaceGroup p1bar(2, "P-1", *inversion);

    SpaceGroup other = p1bar;

    TS_ASSERT_EQUALS(other.number(), p1bar.number());
    TS_ASSERT_EQUALS(other.hmSymbol(), p1bar.hmSymbol());
    TS_ASSERT_EQUALS(other.order(), p1bar.order());
  }

  void testGetEquivalentsR3c() {
    Group_const_sptr group = GroupFactory::create<ProductOfCyclicGroups>(
        "-y,x-y,z; y,x,-z+1/2; -x,-y,-z");
    Group_const_sptr centering = GroupFactory::create<CenteringGroup>("R");

    SpaceGroup spaceGroup(167, "R-3c", *(group * centering));

    std::vector<V3D> byOperator = spaceGroup * V3D(0.3, 0.0, 0.25);
    for (size_t i = 0; i < byOperator.size(); ++i) {
      byOperator[i] = getWrappedVector(byOperator[i]);
    }
    std::sort(byOperator.begin(), byOperator.end());

    std::vector<V3D> byEquivalents =
        spaceGroup.getEquivalentPositions(V3D(0.3, 0.0, 0.25));
    std::sort(byEquivalents.begin(), byEquivalents.end());

    TS_ASSERT_EQUALS(byOperator.size(), 18);
    TS_ASSERT_EQUALS(byOperator.size(), byEquivalents.size());
    for (size_t i = 0; i < byEquivalents.size(); ++i) {
      TS_ASSERT_EQUALS(byOperator[i], byEquivalents[i]);
    }
  }

  void testGetEquivalentsR3m9e() {
    Group_const_sptr group = GroupFactory::create<ProductOfCyclicGroups>(
        "-y,x-y,z; y,x,-z; -x,-y,-z");
    Group_const_sptr centering = GroupFactory::create<CenteringGroup>("R");

    SpaceGroup spaceGroup(167, "R-3m", *(group * centering));

    std::vector<V3D> byOperator = spaceGroup * V3D(0.5, 0.0, 0.0);
    for (size_t i = 0; i < byOperator.size(); ++i) {
      byOperator[i] = getWrappedVector(byOperator[i]);
    }
    std::sort(byOperator.begin(), byOperator.end());

    std::vector<V3D> byEquivalents =
        spaceGroup.getEquivalentPositions(V3D(0.5, 0.0, 0.0));
    std::sort(byEquivalents.begin(), byEquivalents.end());

    TS_ASSERT_EQUALS(byOperator.size(), 9);
    TS_ASSERT_EQUALS(byOperator.size(), byEquivalents.size());
    for (size_t i = 0; i < byEquivalents.size(); ++i) {
      TS_ASSERT_EQUALS(byOperator[i], byEquivalents[i]);
    }
  }

private:
  class TestableSpaceGroup : public SpaceGroup {
    friend class SpaceGroupTest;

  public:
    TestableSpaceGroup() : SpaceGroup(0, "", Group()) {}

    ~TestableSpaceGroup() {}
  };
};

#endif /* MANTID_GEOMETRY_SPACEGROUPTEST_H_ */
