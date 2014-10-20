#ifndef MANTID_GEOMETRY_ISCATTERERTEST_H_
#define MANTID_GEOMETRY_ISCATTERERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/IScatterer.h"

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
    }

    void testGetSetPosition()
    {
        MockIScatterer scatterer;

        V3D position(1.0, 1.0, 1.0);

        V3D testPos;
        TS_ASSERT_THROWS_NOTHING(testPos = scatterer.getPosition());
        TS_ASSERT_DIFFERS(testPos, position);

        TS_ASSERT_THROWS_NOTHING(scatterer.setPosition(position));
        TS_ASSERT_EQUALS(scatterer.getPosition(), position);
    }

private:
    class MockIScatterer : public IScatterer
    {
    public:
        MockIScatterer() : IScatterer() { }
        ~MockIScatterer() { }

        MOCK_CONST_METHOD1(calculateStructureFactor, StructureFactor(const V3D&));
    };
};


#endif /* MANTID_GEOMETRY_ISCATTERERTEST_H_ */
