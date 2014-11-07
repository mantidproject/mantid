#ifndef MANTID_GEOMETRY_GROUPTEST_H_
#define MANTID_GEOMETRY_GROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;

class GroupTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static GroupTest *createSuite() { return new GroupTest(); }
    static void destroySuite( GroupTest *suite ) { delete suite; }

    void testDefaultConstructor()
    {
        Group group;
        TS_ASSERT_EQUALS(group.order(), 1);
        TS_ASSERT(group.getSymmetryOperations().front().isIdentity());
    }

    void testStringConstructor()
    {
        Group group("x,y,z; -x,-y,-z");

        TS_ASSERT_EQUALS(group.order(), 2);
    }

    void testConstructor()
    {
        std::vector<SymmetryOperation> symOps;
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

        TS_ASSERT_THROWS_NOTHING(Group group(symOps));

        Group group(symOps);

        std::vector<SymmetryOperation> groupOps = group.getSymmetryOperations();
        TS_ASSERT_EQUALS(groupOps.size(), 2);

        std::vector<SymmetryOperation> empty;
        TS_ASSERT_THROWS(Group group(empty), std::invalid_argument);
    }

    void testCopyConstructor()
    {
        std::vector<SymmetryOperation> symOps;
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

        Group group(symOps);
        Group otherGroup(group);

        TS_ASSERT_EQUALS(group.order(), otherGroup.order());
        TS_ASSERT_EQUALS(group.getSymmetryOperations(), otherGroup.getSymmetryOperations());
    }

    void testAssignmentOperator()
    {
        std::vector<SymmetryOperation> symOps;
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

        Group otherGroup(symOps);

        Group group;
        TS_ASSERT_DIFFERS(group.order(), otherGroup.order());
        TS_ASSERT_DIFFERS(group.getSymmetryOperations(), otherGroup.getSymmetryOperations());

        group = otherGroup;
        TS_ASSERT_EQUALS(group.order(), otherGroup.order());
        TS_ASSERT_EQUALS(group.getSymmetryOperations(), otherGroup.getSymmetryOperations());
    }

    void testOrder()
    {
        Group defaultGroup;
        TS_ASSERT_EQUALS(defaultGroup.order(), 1);

        // Making a group of two operations gives order 2
        std::vector<SymmetryOperation> symOps;
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

        Group biggerGroup(symOps);
        TS_ASSERT_EQUALS(biggerGroup.order(), 2);

        // Adding another one results in 3
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,y,z"));
        Group evenBiggerGroup(symOps);
        TS_ASSERT_EQUALS(evenBiggerGroup.order(), 3);

        // Multiple occurences of the same operation do not count
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));
        Group sameAsBefore(symOps);
        TS_ASSERT_EQUALS(sameAsBefore.order(), 3);
    }

    void testComparison()
    {
        std::vector<SymmetryOperation> symOps;
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        symOps.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

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

    void testMultiplicationOperator()
    {
        // We take pointgroup -1
        std::vector<SymmetryOperation> inversion;
        inversion.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        inversion.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

        // And 2 (b-axis unique)
        std::vector<SymmetryOperation> twoFoldY;
        twoFoldY.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        twoFoldY.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,y,-z"));

        Group one(inversion);
        Group two(twoFoldY);

        // Multiplication results in 2/m
        Group three = one * two;
        TS_ASSERT_EQUALS(three.order(), 4);

        // The multiplication created m perpendicular to b (x,-y,-z)
        SymmetryOperation mirrorY = SymmetryOperationFactory::Instance().createSymOp("x,-y,z");
        std::vector<SymmetryOperation> opsOfThree = three.getSymmetryOperations();

        // Check that it is found in the list of symmetry operations of the new group
        TS_ASSERT_DIFFERS(std::find(opsOfThree.begin(), opsOfThree.end(), mirrorY), opsOfThree.end());

        Group four = two * one;
        TS_ASSERT(three == four);
    }

    void testSmartPointerOperators()
    {
        // We take pointgroup -1
        std::vector<SymmetryOperation> inversion;
        inversion.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        inversion.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z"));

        // And 2 (b-axis unique)
        std::vector<SymmetryOperation> twoFoldY;
        twoFoldY.push_back(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        twoFoldY.push_back(SymmetryOperationFactory::Instance().createSymOp("-x,y,-z"));

        Group_const_sptr one = boost::make_shared<const Group>(inversion);
        Group_const_sptr two = boost::make_shared<const Group>(twoFoldY);

        Group_const_sptr three = one * two;
        TS_ASSERT_EQUALS(three->order(), 4);

        SymmetryOperation mirrorY = SymmetryOperationFactory::Instance().createSymOp("x,-y,z");
        std::vector<SymmetryOperation> opsOfThree = three->getSymmetryOperations();

        // Check that it is found in the list of symmetry operations of the new group
        TS_ASSERT_DIFFERS(std::find(opsOfThree.begin(), opsOfThree.end(), mirrorY), opsOfThree.end());

        // Make sure that null-pointer do not work
        Group_const_sptr null;
        
        //AppleClang gives a warning if we don't use the result;
        bool useResult;

        TS_ASSERT_THROWS(null * null, std::invalid_argument);
        TS_ASSERT_THROWS(useResult = (null == null), std::invalid_argument);
        TS_ASSERT_THROWS(useResult = (null != null), std::invalid_argument);
        TS_ASSERT_THROWS(three * null, std::invalid_argument);
        TS_ASSERT_THROWS(null * three, std::invalid_argument);

        Mantid::Kernel::V3D coords(0.4, 0.3, 0.1);
        TS_ASSERT_THROWS(null * coords, std::invalid_argument);
    }


};


#endif /* MANTID_GEOMETRY_GROUPTEST_H_ */
