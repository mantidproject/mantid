#ifndef MANTID_GEOMETRY_PRODUCTGROUPTEST_H_
#define MANTID_GEOMETRY_PRODUCTGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/ProductOfCyclicGroups.h"
#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

using namespace Mantid::Geometry;

class ProductOfCyclicGroupsTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static ProductOfCyclicGroupsTest *createSuite() { return new ProductOfCyclicGroupsTest(); }
    static void destroySuite( ProductOfCyclicGroupsTest *suite ) { delete suite; }


    void testStringConstructor()
    {
        TS_ASSERT_THROWS_NOTHING(ProductOfCyclicGroups group("x,y,z"));

        TS_ASSERT_THROWS_ANYTHING(ProductOfCyclicGroups group("x,y,z; doesnt work"));
        TS_ASSERT_THROWS_ANYTHING(ProductOfCyclicGroups group("x,y,z| z,x,y"));
    }

    void testVectorConstructor()
    {
        std::vector<Group_const_sptr> groups;
        groups.push_back(GroupFactory::create<CyclicGroup>("-x,-y,-z"));
        groups.push_back(GroupFactory::create<CyclicGroup>("x,-y,z"));

        TS_ASSERT_THROWS_NOTHING(ProductOfCyclicGroups group(groups));

        Group_const_sptr null;
        groups.push_back(null);

        TS_ASSERT_THROWS_ANYTHING(ProductOfCyclicGroups group(groups));
    }

    void testGetGeneratedGroup()
    {
        TestableProductOfCyclicGroups group;

        Group_const_sptr generatedGroup = group.getGeneratedGroup("-x,-y,-z; x,-y,z");

        // Inversion generates 1, -1; Mirror 1, m [010] -> results in 1, -1, m [010], 2 [010]
        TS_ASSERT_EQUALS(generatedGroup->order(), 4);
    }

    void testGetFactorGroups()
    {
        TestableProductOfCyclicGroups group;

        std::vector<SymmetryOperation> symmetryOperations = SymmetryOperationFactory::Instance().createSymOps("-x,-y,-z; x,-y,z");
        std::vector<Group_const_sptr> generatedGroup = group.getFactorGroups(symmetryOperations);
        // one group for each symmetry operation
        TS_ASSERT_EQUALS(generatedGroup.size(), 2);
    }

    void testGetProductOfCyclicGroups()
    {
        TestableProductOfCyclicGroups group;

        std::vector<Group_const_sptr> groups;
        groups.push_back(GroupFactory::create<CyclicGroup>("-x,-y,-z"));
        groups.push_back(GroupFactory::create<CyclicGroup>("x,-y,z"));

        Group_const_sptr productGroup = group.getProductOfCyclicGroups(groups);

        TS_ASSERT_EQUALS(productGroup->order(), 4);
    }

private:
    class TestableProductOfCyclicGroups : public ProductOfCyclicGroups
    {
        friend class ProductOfCyclicGroupsTest;
    public:
        TestableProductOfCyclicGroups() :
            ProductOfCyclicGroups("x,y,z") { }
        ~TestableProductOfCyclicGroups() { }
    };

};


#endif /* MANTID_GEOMETRY_PRODUCTGROUPTEST_H_ */
