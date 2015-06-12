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

    SpaceGroup spaceGroup(167, "R-3c", *(group * centering));

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

  void testIsAllowedReflectionR3m() {
    /* This is just a check that isAllowedReflection behaves correctly,
     * there is a system test in place that checks more examples.
     *
     * This test uses space group 167, R-3c, like above.
     */
    Group_const_sptr group = GroupFactory::create<ProductOfCyclicGroups>(
        "-y,x-y,z; y,x,-z; -x,-y,-z");
    Group_const_sptr centering = GroupFactory::create<CenteringGroup>("R");

    SpaceGroup spaceGroup(167, "R-3c", *(group * centering));

    // Reflections hkl: -h + k + l = 3n
    V3D goodHKL(1, 4, 3); // -1 + 4 + 3 = 6 = 3 * 2
    V3D badHKL(2, 4, 3);  // -2 + 4 + 3 = 5
    TS_ASSERT(spaceGroup.isAllowedReflection(goodHKL));
    TS_ASSERT(!spaceGroup.isAllowedReflection(badHKL));

    // Reflections hk0: -h + k = 3n
    V3D goodHK0(3, 9, 0); // -3 + 9 = 6 = 3 * 2
    V3D badHK0(4, 9, 0);  // -4 + 9 = 5
    TS_ASSERT(spaceGroup.isAllowedReflection(goodHK0));
    TS_ASSERT(!spaceGroup.isAllowedReflection(badHK0));

    // Reflections hhl: l = 3n
    V3D goodHHL(1, 1, 6); // 6 = 3 * 2
    V3D badHHL(1, 1, 7);
    TS_ASSERT(spaceGroup.isAllowedReflection(goodHHL));
    TS_ASSERT(!spaceGroup.isAllowedReflection(badHHL));

    // Reflections h-hl: h + l = 3n
    V3D goodHmHL(3, -3, 6); // 3 + 9 = 9 = 3 * 3
    V3D badHmHL(3, -3, 7); // 3 + 7 = 10
    TS_ASSERT(spaceGroup.isAllowedReflection(goodHmHL));
    TS_ASSERT(!spaceGroup.isAllowedReflection(badHmHL));

    // Reflections 000l: l = 3n
    V3D good00L(0, 0, 6); // 6 = 3 * 2
    V3D bad00L(0, 0, 7);
    TS_ASSERT(spaceGroup.isAllowedReflection(good00L));
    TS_ASSERT(!spaceGroup.isAllowedReflection(bad00L));

    // Reflections h-h0: h = 3n
    V3D goodHH0(3, -3, 0); // 3 = 3 * 1
    V3D badHH0(4, -4, 0);
    TS_ASSERT(spaceGroup.isAllowedReflection(goodHH0));
    TS_ASSERT(!spaceGroup.isAllowedReflection(badHH0));
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
