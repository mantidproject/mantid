#ifndef MANTID_GEOMETRY_CENTERINGGROUPTEST_H_
#define MANTID_GEOMETRY_CENTERINGGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/CenteringGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

using namespace Mantid::Geometry;

class CenteringGroupTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static CenteringGroupTest *createSuite() { return new CenteringGroupTest(); }
    static void destroySuite( CenteringGroupTest *suite ) { delete suite; }

    void testValidCenterings()
    {
        testCenteringGroup("P", CenteringGroup::P, "P", SymmetryOperationFactory::Instance().createSymOps("x,y,z"));
        testCenteringGroup("I", CenteringGroup::I, "I", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/2,y+1/2,z+1/2"));
        testCenteringGroup("A", CenteringGroup::A, "A", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x,y+1/2,z+1/2"));
        testCenteringGroup("B", CenteringGroup::B, "B", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/2,y,z+1/2"));
        testCenteringGroup("C", CenteringGroup::C, "C", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/2,y+1/2,z"));
        testCenteringGroup("F", CenteringGroup::F, "F", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x,y+1/2,z+1/2; x+1/2,y,z+1/2; x+1/2,y+1/2,z"));
        testCenteringGroup("R", CenteringGroup::Robv, "R", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/3,y+2/3,z+2/3; x+2/3,y+1/3,z+1/3"));
        testCenteringGroup("Robv", CenteringGroup::Robv, "R", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/3,y+2/3,z+2/3; x+2/3,y+1/3,z+1/3"));
        testCenteringGroup("Rrev", CenteringGroup::Rrev, "R", SymmetryOperationFactory::Instance().createSymOps("x,y,z; x+1/3,y+2/3,z+1/3; x+2/3,y+1/3,z+2/3"));
    }

    void testInvalidCentering()
    {
        TS_ASSERT_THROWS(CenteringGroup group("G"), std::invalid_argument);
        TS_ASSERT_THROWS(CenteringGroup group("f"), std::invalid_argument);
    }

private:
    void testCenteringGroup(const std::string &symbol, CenteringGroup::CenteringType expectedCenteringType, const std::string &expectedSymbol, const std::vector<SymmetryOperation> &expectedOperations)
    {
        TSM_ASSERT_THROWS_NOTHING("Exception when trying to create " + symbol, CenteringGroup rawGroup(symbol));

        Group_const_sptr group = GroupFactory::create<CenteringGroup>(symbol);
        std::vector<SymmetryOperation> ops = group->getSymmetryOperations();
        TSM_ASSERT_EQUALS("Unexpected number of operations for " + symbol, ops.size(), expectedOperations.size());

        for(auto it = expectedOperations.begin(); it != expectedOperations.end(); ++it) {
            TSM_ASSERT("Operation " + (*it).identifier() + " not found in " + symbol, symOpExistsInCollection(*it, ops));
        }

        CenteringGroup_const_sptr centeringGroup = boost::dynamic_pointer_cast<const CenteringGroup>(group);
        TSM_ASSERT("Could not cast to pointer in " + symbol, centeringGroup);
        TSM_ASSERT_EQUALS("CenteringType did not match for " + symbol, centeringGroup->getType(), expectedCenteringType);
        TSM_ASSERT_EQUALS("CenteringString did not match for " + symbol, centeringGroup->getSymbol(), expectedSymbol);
    }


    bool symOpExistsInCollection(const SymmetryOperation &op, const std::vector<SymmetryOperation> &collection)
    {
        return std::find(collection.begin(), collection.end(), op) != collection.end();
    }
};


#endif /* MANTID_GEOMETRY_CENTERINGGROUPTEST_H_ */
