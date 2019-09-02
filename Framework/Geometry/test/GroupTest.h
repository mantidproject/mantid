// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_GROUPTEST_H_
#define MANTID_GEOMETRY_GROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  void testDefaultConstructor() {
    Group group;
    TS_ASSERT_EQUALS(group.order(), 1);
    TS_ASSERT(group.getSymmetryOperations().front().isIdentity());
  }

  void testStringConstructor() {
    Group group("x,y,z; -x,-y,-z");

    TS_ASSERT_EQUALS(group.order(), 2);
  }

  void testConstructor() {
    std::vector<SymmetryOperation> symOps;
    symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    TS_ASSERT_THROWS_NOTHING(Group group(symOps));

    Group group(symOps);

    std::vector<SymmetryOperation> groupOps = group.getSymmetryOperations();
    TS_ASSERT_EQUALS(groupOps.size(), 2);

    std::vector<SymmetryOperation> empty;
    TS_ASSERT_THROWS(Group group(empty), const std::invalid_argument &);
  }

  void testCopyConstructor() {
    std::vector<SymmetryOperation> symOps;
    symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    Group group(symOps);
    Group otherGroup(group);

    TS_ASSERT_EQUALS(group.order(), otherGroup.order());
    TS_ASSERT_EQUALS(group.getSymmetryOperations(),
                     otherGroup.getSymmetryOperations());
  }

  void testAssignmentOperator() {
    std::vector<SymmetryOperation> symOps;
    symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    Group otherGroup(symOps);

    Group group;
    TS_ASSERT_DIFFERS(group.order(), otherGroup.order());
    TS_ASSERT_DIFFERS(group.getSymmetryOperations(),
                      otherGroup.getSymmetryOperations());

    group = otherGroup;
    TS_ASSERT_EQUALS(group.order(), otherGroup.order());
    TS_ASSERT_EQUALS(group.getSymmetryOperations(),
                     otherGroup.getSymmetryOperations());
  }

  void testOrder() {
    Group defaultGroup;
    TS_ASSERT_EQUALS(defaultGroup.order(), 1);

    // Making a group of two operations gives order 2
    std::vector<SymmetryOperation> symOps;
    symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    Group biggerGroup(symOps);
    TS_ASSERT_EQUALS(biggerGroup.order(), 2);

    // Adding another one results in 3
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,y,z"));
    Group evenBiggerGroup(symOps);
    TS_ASSERT_EQUALS(evenBiggerGroup.order(), 3);

    // Multiple occurences of the same operation do not count
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));
    Group sameAsBefore(symOps);
    TS_ASSERT_EQUALS(sameAsBefore.order(), 3);
  }

  void testComparison() {
    std::vector<SymmetryOperation> symOps;
    symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    symOps.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    Group groupOne(symOps);
    Group groupTwo(symOps);

    TS_ASSERT(groupOne == groupTwo);
    TS_ASSERT(groupTwo == groupOne);

    Group defaultGroup;
    TS_ASSERT(!(groupOne == defaultGroup));
    TS_ASSERT(!(defaultGroup == groupOne));
    TS_ASSERT(groupOne != defaultGroup);
    TS_ASSERT(defaultGroup != groupOne);
  }

  void testMultiplicationOperator() {
    // We take pointgroup -1
    std::vector<SymmetryOperation> inversion;
    inversion.push_back(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    inversion.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    // And 2 (b-axis unique)
    std::vector<SymmetryOperation> twoFoldY;
    twoFoldY.push_back(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    twoFoldY.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,y,-z"));

    Group one(inversion);
    Group two(twoFoldY);

    // Multiplication results in 2/m
    Group three = one * two;
    TS_ASSERT_EQUALS(three.order(), 4);

    // The multiplication created m perpendicular to b (x,-y,-z)
    SymmetryOperation mirrorY =
        SymmetryOperationFactory::Instance().createSymOp("x,-y,z");
    std::vector<SymmetryOperation> opsOfThree = three.getSymmetryOperations();

    // Check that it is found in the list of symmetry operations of the new
    // group
    TS_ASSERT_DIFFERS(std::find(opsOfThree.begin(), opsOfThree.end(), mirrorY),
                      opsOfThree.end());

    Group four = two * one;
    TS_ASSERT(three == four);
  }

  void testAxisSystemOrthogonal() {
    std::vector<SymmetryOperation> orthogonal;
    orthogonal.push_back(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    orthogonal.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,y,-z"));

    Group two(orthogonal);

    TS_ASSERT_EQUALS(two.getCoordinateSystem(), Group::Orthogonal);
  }

  void testAxisSystemHexagonal() {
    std::vector<SymmetryOperation> hexagonal;
    hexagonal.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-y,x-y,z"));
    hexagonal.push_back(
        SymmetryOperationFactory::Instance().createSymOp("y,x,-z+1/2"));

    Group two(hexagonal);

    TS_ASSERT_EQUALS(two.getCoordinateSystem(), Group::Hexagonal);
  }

  void testAtomPositionLessThan() {
    AtomPositionsLessThan lessThan(1.e-6);

    V3D v1(0.654321, 0.0, 0.0);
    V3D v2(0.654320, 0.0, 0.0);
    TS_ASSERT(v1 != v2);
    TS_ASSERT(lessThan(v2, v1));

    // 7th digit is not compared.
    V3D v3(0.6543211, 0.0, 0.0);
    TS_ASSERT(v1 == v3);
    TS_ASSERT(!lessThan(v1, v3));
    TS_ASSERT(!lessThan(v3, v1));

    // Same for y
    V3D v4(0.654321, 0.0000010001, 0.0);
    TS_ASSERT(v1 != v4);
    TS_ASSERT(lessThan(v1, v4));

    V3D v5(0.654321, 0.0000001, 0.0);
    TS_ASSERT(v1 == v5);
    TS_ASSERT(!lessThan(v1, v5));
    TS_ASSERT(!lessThan(v5, v1));

    // Same for z
    V3D v6(0.654321, 0.0, 0.0000010001);
    TS_ASSERT(v1 != v6);
    TS_ASSERT(lessThan(v1, v6));

    V3D v7(0.654321, 0.0, 0.0000001);
    TS_ASSERT(v1 == v7);
    TS_ASSERT(!lessThan(v1, v7));
    TS_ASSERT(!lessThan(v7, v1));
  }

  void testAtomPositionEqual() {
    AtomPositionsEqual equal(1.e-6);

    V3D v1(0.654321, 0.0, 0.0);
    V3D v2(0.654320, 0.0, 0.0);
    TS_ASSERT(!equal(v2, v1));

    // 7th digit is not compared.
    V3D v3(0.6543211, 0.0, 0.0);
    TS_ASSERT(equal(v1, v3));
    TS_ASSERT(equal(v3, v1));

    // Same for y
    V3D v4(0.654321, 0.0000010001, 0.0);
    TS_ASSERT(!equal(v1, v4));

    V3D v5(0.654321, 0.0000001, 0.0);
    TS_ASSERT(equal(v5, v1));

    // Same for z
    V3D v6(0.654321, 0.0, 0.0000010001);
    TS_ASSERT(!equal(v1, v6));

    V3D v7(0.654321, 0.0, 0.0000001);
    TS_ASSERT(equal(v1, v7));
  }

  void testContainsOperation() {
    Group group("x,y,z; -x,-y,-z");

    std::vector<SymmetryOperation> ops = group.getSymmetryOperations();
    TS_ASSERT(group.containsOperation(ops[0]));
    TS_ASSERT(group.containsOperation(ops[1]));

    SymmetryOperation mirror =
        SymmetryOperationFactory::Instance().createSymOp("x,y,-z");

    TS_ASSERT(!group.containsOperation(mirror));
  }

  void testGroupAxiomClosure() {
    Group properGroup("x,y,z; -x,-y,-z; x,y,-z; -x,-y,z");
    TS_ASSERT(properGroup.fulfillsAxiom(Group::Closure));

    // Is not closed anymore
    Group noClosure("x,y,z; -x,-y,-z; x,y,-z");
    TS_ASSERT(!noClosure.fulfillsAxiom(Group::Closure));

    // But the other axioms still hold
    TS_ASSERT(noClosure.fulfillsAxiom(Group::Identity));
    TS_ASSERT(noClosure.fulfillsAxiom(Group::Inversion));
    TS_ASSERT(noClosure.fulfillsAxiom(Group::Associativity));
  }

  void testGroupAxiomIdentity() {
    Group properGroup("x,y,z; -x,-y,-z; x,y,-z; -x,-y,z");
    TS_ASSERT(properGroup.fulfillsAxiom(Group::Identity));

    // Is does not have identity anymore
    Group noIdentity("-x,-y,-z; x,y,-z");
    TS_ASSERT(!noIdentity.fulfillsAxiom(Group::Identity));

    // Closure is lost as well
    TS_ASSERT(!noIdentity.fulfillsAxiom(Group::Closure));

    // While the other two still hold
    TS_ASSERT(noIdentity.fulfillsAxiom(Group::Inversion));
    TS_ASSERT(noIdentity.fulfillsAxiom(Group::Associativity));
  }

  void testGroupAxiomInversion() {
    Group properGroup("x,y,z; -x,-y,-z; x,y,-z; -x,-y,z");
    TS_ASSERT(properGroup.fulfillsAxiom(Group::Inversion));

    // Is does not have the identity anymore
    Group noInversion("x,y,z; -y,x,z");
    TS_ASSERT(!noInversion.fulfillsAxiom(Group::Inversion));

    // Closure is lost as well
    TS_ASSERT(!noInversion.fulfillsAxiom(Group::Closure));

    // While the other two still hold
    TS_ASSERT(noInversion.fulfillsAxiom(Group::Associativity));
    TS_ASSERT(noInversion.fulfillsAxiom(Group::Identity));
  }

  void testIsGroup() {
    Group properGroup("x,y,z; -x,-y,-z; x,y,-z; -x,-y,z");
    TS_ASSERT(properGroup.isGroup());

    Group noClosure("x,y,z; -x,-y,-z; x,y,-z");
    TS_ASSERT(!noClosure.isGroup());

    Group noIdentity("-x,-y,-z; x,y,-z; -x,-y,z");
    TS_ASSERT(!noIdentity.isGroup());
  }

  void test_isInvariant_triclinic_monoclinic() {
    UnitCell triclinic(3, 4, 5, 91, 92, 93);
    UnitCell monoclinic(3, 5, 6, 90, 100, 90);

    // Both cells are compatible with triclinic symmetry
    Group p1bar("x,y,z; -x,-y,-z");
    TS_ASSERT(p1bar.isInvariant(triclinic.getG()));
    TS_ASSERT(p1bar.isInvariant(monoclinic.getG()));

    // But not with monoclinic
    Group p2overm("x,y,z; -x,-y,-z; -x,y,-z; x,-y,z");
    TS_ASSERT(p2overm.isInvariant(monoclinic.getG()));
    TS_ASSERT(!p2overm.isInvariant(triclinic.getG()));
  }

  void testSmartPointerOperators() {
    // We take pointgroup -1
    std::vector<SymmetryOperation> inversion;
    inversion.push_back(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    inversion.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

    // And 2 (b-axis unique)
    std::vector<SymmetryOperation> twoFoldY;
    twoFoldY.push_back(
        SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
    twoFoldY.push_back(
        SymmetryOperationFactory::Instance().createSymOp("-x,y,-z"));

    Group_const_sptr one = boost::make_shared<const Group>(inversion);
    Group_const_sptr two = boost::make_shared<const Group>(twoFoldY);

    Group_const_sptr three = one * two;
    TS_ASSERT_EQUALS(three->order(), 4);

    SymmetryOperation mirrorY =
        SymmetryOperationFactory::Instance().createSymOp("x,-y,z");
    std::vector<SymmetryOperation> opsOfThree = three->getSymmetryOperations();

    // Check that it is found in the list of symmetry operations of the new
    // group
    TS_ASSERT_DIFFERS(std::find(opsOfThree.begin(), opsOfThree.end(), mirrorY),
                      opsOfThree.end());

    // Make sure that null-pointer do not work
    Group_const_sptr null;

    TS_ASSERT_THROWS(null * null, const std::invalid_argument &);
    // clang gives a warning if we don't use the result
    GNU_DIAG_OFF("unused-comparison")
    TS_ASSERT_THROWS(null == null, const std::invalid_argument &);
    TS_ASSERT_THROWS(null != null, const std::invalid_argument &);
    GNU_DIAG_ON("unused-comparison")
    TS_ASSERT_THROWS(three * null, const std::invalid_argument &);
    TS_ASSERT_THROWS(null * three, const std::invalid_argument &);

    Mantid::Kernel::V3D coords(0.4, 0.3, 0.1);
    TS_ASSERT_THROWS(null * coords, const std::invalid_argument &);
  }
};

#endif /* MANTID_GEOMETRY_GROUPTEST_H_ */
