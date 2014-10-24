#ifndef MANTID_GEOMETRY_ISCATTERERTEST_H_
#define MANTID_GEOMETRY_ISCATTERERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/IScatterer.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class IScattererTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static IScattererTest *createSuite() { return new IScattererTest(); }
    static void destroySuite( IScattererTest *suite ) { delete suite; }

    void testConstruction()
    {
        TS_ASSERT_THROWS_NOTHING(MockIScatterer scatterer);

        V3D good(0.5, 0.7, 0.2);
        MockIScatterer goodScatterer(good);

        TS_ASSERT_EQUALS(goodScatterer.getPosition(), good);

        V3D bad(1.2, 4.3, -6.2);
        MockIScatterer badScatterer(bad);
        TS_ASSERT_DIFFERS(badScatterer.getPosition(), bad);
        TS_ASSERT_EQUALS(badScatterer.getPosition(), V3D(0.2, 0.3, 0.8));
    }

    void testGetSetPosition()
    {
        MockIScatterer scatterer;

        V3D goodPosition(0.2, 0.4, 0.3);

        V3D testPos;
        TS_ASSERT_THROWS_NOTHING(testPos = scatterer.getPosition());
        TS_ASSERT_DIFFERS(testPos, goodPosition);

        TS_ASSERT_THROWS_NOTHING(scatterer.setPosition(goodPosition));
        TS_ASSERT_EQUALS(scatterer.getPosition(), goodPosition);

        V3D badPosition(1.2, 4.3, -6.2);
        TS_ASSERT_THROWS_NOTHING(scatterer.setPosition(badPosition));
        TS_ASSERT_DIFFERS(scatterer.getPosition(), badPosition);
        TS_ASSERT_EQUALS(scatterer.getPosition(), V3D(0.2, 0.3, 0.8));
    }

    void testGetSetCell()
    {
        MockIScatterer scatterer;

        UnitCell cell(5.43, 5.43, 5.43);

        TS_ASSERT_THROWS_NOTHING(scatterer.setCell(cell));
        TS_ASSERT_EQUALS(scatterer.getCell().getG(), cell.getG());
    }

    void testGetSetSpaceGroup()
    {
        MockIScatterer scatterer;

        SpaceGroup_const_sptr testGroup = SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");

        TS_ASSERT_THROWS_NOTHING(scatterer.setSpaceGroup(testGroup));
        TS_ASSERT_EQUALS(scatterer.getSpaceGroup(), testGroup);
    }

    void testEquivalentPositions()
    {
        MockIScatterer scatterer;

        V3D generalPosition(0.3, 0.32, 0.45);

        // No space group set - no equivalent positions
        scatterer.setPosition(generalPosition);
        TS_ASSERT_EQUALS(scatterer.getEquivalentPositions().size(), 1);
        TS_ASSERT_EQUALS(scatterer.getEquivalentPositions().front(), generalPosition);

        // Assigning a space group must cause recalculation of equivalent positions
        SpaceGroup_const_sptr testGroup = SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");
        scatterer.setSpaceGroup(testGroup);

        TS_ASSERT_EQUALS(scatterer.getEquivalentPositions().size(), testGroup->order());

        // Re-setting the position also recalculates
        V3D specialPosition(0.0, 0.0, 0.0);

        scatterer.setPosition(specialPosition);
        // Pm-3m does not contain translations, so (0,0,0) is not transformed by any symmetry operation of the group
        TS_ASSERT_EQUALS(scatterer.getEquivalentPositions().size(), 1);
        TS_ASSERT_EQUALS(scatterer.getEquivalentPositions().front(), specialPosition);
    }

    void testUnitCellStringValidator()
    {
        IValidator_sptr validator = boost::make_shared<UnitCellStringValidator>();

        // non-working examples
        TS_ASSERT_DIFFERS(validator->isValid("1.0"), "");
        TS_ASSERT_DIFFERS(validator->isValid("1.0 1.0"), "");
        TS_ASSERT_DIFFERS(validator->isValid("1.0 1.0 1.0 1.0"), "");
        TS_ASSERT_DIFFERS(validator->isValid("1.0 1.0 1.0 1.0 1.0"), "");
        TS_ASSERT_DIFFERS(validator->isValid("1.0.3 1.0 1.0"), "");

        // Working examples
        TS_ASSERT_EQUALS(validator->isValid("1.0 1.0 1.0"), "");
        TS_ASSERT_EQUALS(validator->isValid("1.0 1.0 1.0 90.0 90.0 90.0"), "");
        TS_ASSERT_EQUALS(validator->isValid("1.0 1.0 1.0 90.0 90.0 90.0  "), "");
    }

private:
    class MockIScatterer : public IScatterer
    {
    public:
        MockIScatterer(const V3D &position = V3D(0, 0, 0)) : IScatterer(position) { }
        ~MockIScatterer() { }

        MOCK_CONST_METHOD0(clone, IScatterer_sptr());
        MOCK_CONST_METHOD1(calculateStructureFactor, StructureFactor(const V3D&));
    };
};


#endif /* MANTID_GEOMETRY_ISCATTERERTEST_H_ */
