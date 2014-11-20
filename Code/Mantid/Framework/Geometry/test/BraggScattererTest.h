#ifndef MANTID_GEOMETRY_BRAGGSCATTERERTEST_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/BraggScatterer.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class BraggScattererTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static BraggScattererTest *createSuite() { return new BraggScattererTest(); }
    static void destroySuite( BraggScattererTest *suite ) { delete suite; }

    void testConstruction()
    {
        TS_ASSERT_THROWS_NOTHING(MockBraggScatterer scatterer);
    }

    void testInitialization()
    {
        BraggScatterer_sptr scatterer = getDefaultScatterer();

        TS_ASSERT(!scatterer->isInitialized());
        TS_ASSERT_THROWS_NOTHING(scatterer->initialize());
        TS_ASSERT(scatterer->isInitialized());
    }

private:
    BraggScatterer_sptr getDefaultScatterer()
    {
        return boost::make_shared<MockBraggScatterer>();
    }

    BraggScatterer_sptr getInitializedScatterer()
    {
        BraggScatterer_sptr raw = getDefaultScatterer();
        raw->initialize();

        return raw;
    }

    class MockBraggScatterer : public BraggScatterer
    {
    public:
        MockBraggScatterer() : BraggScatterer() { }
        ~MockBraggScatterer() { }

        MOCK_CONST_METHOD0(name, std::string());
        MOCK_CONST_METHOD0(clone, BraggScatterer_sptr());
        MOCK_CONST_METHOD1(calculateStructureFactor, StructureFactor(const V3D&));
        MOCK_METHOD1(afterScattererPropertySet, void(const std::string &));
    };
};


#endif /* MANTID_GEOMETRY_BRAGGSCATTERERTEST_H_ */
