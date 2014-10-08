#ifndef MANTID_GEOMETRY_PRODUCTGROUPTEST_H_
#define MANTID_GEOMETRY_PRODUCTGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/ProductGroup.h"
#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

using namespace Mantid::Geometry;

class ProductGroupTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static ProductGroupTest *createSuite() { return new ProductGroupTest(); }
    static void destroySuite( ProductGroupTest *suite ) { delete suite; }


    void testStringConstructor()
    {
        TS_ASSERT_THROWS_NOTHING(ProductGroup group("x,y,z"));

        TS_ASSERT_THROWS_ANYTHING(ProductGroup group("x,y,z; doesnt work"));
        TS_ASSERT_THROWS_ANYTHING(ProductGroup group("x,y,z| z,x,y"));
    }

    void testVectorConstructor()
    {
        std::vector<Group_const_sptr> groups;
        groups.push_back(GroupFactory::create<CyclicGroup>("-x,-y,-z"));
        groups.push_back(GroupFactory::create<CyclicGroup>("x,-y,z"));

        TS_ASSERT_THROWS_NOTHING(ProductGroup group(groups));

        Group_const_sptr null;
        groups.push_back(null);

        TS_ASSERT_THROWS_ANYTHING(ProductGroup group(groups));
    }

    void testGetGeneratedGroup()
    {
        TestableProductGroup group;

        Group_const_sptr generatedGroup = group.getGeneratedGroup("-x,-y,-z; x,-y,z");

        // Inversion generates 1, -1; Mirror 1, m [010] -> results in 1, -1, m [010], 2 [010]
        TS_ASSERT_EQUALS(generatedGroup->order(), 4);
    }

    void testGetFactorGroups()
    {
        TestableProductGroup group;

        std::vector<SymmetryOperation> symmetryOperations = SymmetryOperationFactory::Instance().createSymOps("-x,-y,-z; x,-y,z");
        std::vector<Group_const_sptr> generatedGroup = group.getFactorGroups(symmetryOperations);
        // one group for each symmetry operation
        TS_ASSERT_EQUALS(generatedGroup.size(), 2);
    }

    void testGetProductGroup()
    {
        TestableProductGroup group;

        std::vector<Group_const_sptr> groups;
        groups.push_back(GroupFactory::create<CyclicGroup>("-x,-y,-z"));
        groups.push_back(GroupFactory::create<CyclicGroup>("x,-y,z"));

        Group_const_sptr productGroup = group.getProductGroup(groups);

        TS_ASSERT_EQUALS(productGroup->order(), 4);
    }

private:
    class TestableProductGroup : public ProductGroup
    {
        friend class ProductGroupTest;
    public:
        TestableProductGroup() :
            ProductGroup("x,y,z") { }
        ~TestableProductGroup() { }
    };

};


#endif /* MANTID_GEOMETRY_PRODUCTGROUPTEST_H_ */
