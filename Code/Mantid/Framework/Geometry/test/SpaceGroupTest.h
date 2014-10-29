#ifndef MANTID_GEOMETRY_SPACEGROUPTEST_H_
#define MANTID_GEOMETRY_SPACEGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class SpaceGroupTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SpaceGroupTest *createSuite() { return new SpaceGroupTest(); }
    static void destroySuite( SpaceGroupTest *suite ) { delete suite; }


    void testConstruction()
    {
        Group_const_sptr inversion = GroupFactory::create<CyclicGroup>("-x,-y,-z");
        SpaceGroup p1bar(2, "P-1", *inversion);

        TS_ASSERT_EQUALS(p1bar.number(), 2);
        TS_ASSERT_EQUALS(p1bar.hmSymbol(), "P-1");
        TS_ASSERT_EQUALS(p1bar.order(), 2);
        TS_ASSERT_EQUALS(p1bar.getSymmetryOperations().size(), 2);
    }

    void testNumber()
    {
        TestableSpaceGroup empty;
        TS_ASSERT_EQUALS(empty.number(), 0);

        empty.m_number = 2;
        TS_ASSERT_EQUALS(empty.number(), 2);
    }

    void testSymbol()
    {
        TestableSpaceGroup empty;
        TS_ASSERT_EQUALS(empty.hmSymbol(), "");

        empty.m_hmSymbol = "Test";
        TS_ASSERT_EQUALS(empty.hmSymbol(), "Test");
    }

    void testAssignmentOperator()
    {
        Group_const_sptr inversion = GroupFactory::create<CyclicGroup>("-x,-y,-z");
        SpaceGroup p1bar(2, "P-1", *inversion);

        SpaceGroup other = p1bar;

        TS_ASSERT_EQUALS(other.number(), p1bar.number());
        TS_ASSERT_EQUALS(other.hmSymbol(), p1bar.hmSymbol());
        TS_ASSERT_EQUALS(other.order(), p1bar.order());
    }

private:
    class TestableSpaceGroup : public SpaceGroup {
        friend class SpaceGroupTest;
    public:
        TestableSpaceGroup() :
            SpaceGroup(0, "", Group())
        { }

        ~TestableSpaceGroup() { }
    };
};


#endif /* MANTID_GEOMETRY_SPACEGROUPTEST_H_ */
