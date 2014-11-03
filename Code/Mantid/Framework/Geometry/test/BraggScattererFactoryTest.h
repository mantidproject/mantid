#ifndef MANTID_GEOMETRY_BRAGGSCATTERERFACTORYTEST_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/BraggScattererFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class BraggScattererFactoryTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static BraggScattererFactoryTest *createSuite() { return new BraggScattererFactoryTest(); }
    static void destroySuite( BraggScattererFactoryTest *suite ) { delete suite; }


    void testSubscribeCreateUnsubscribeGetKeys()
    {
        std::vector<std::string> registered = BraggScattererFactory::Instance().getKeys();
        TS_ASSERT_EQUALS(std::find(registered.begin(), registered.end(), "MockScatterer"), registered.end());
        TS_ASSERT_THROWS_ANYTHING(BraggScattererFactory::Instance().createScatterer("MockScatterer"));

        BraggScattererFactory::Instance().subscribeScatterer<MockScatterer>();

        registered = BraggScattererFactory::Instance().getKeys();
        TS_ASSERT_DIFFERS(std::find(registered.begin(), registered.end(), "MockScatterer"), registered.end());
        TS_ASSERT_THROWS_NOTHING(BraggScattererFactory::Instance().createScatterer("MockScatterer"));

        BraggScatterer_sptr scatterer = BraggScattererFactory::Instance().createScatterer("MockScatterer");
        TS_ASSERT(scatterer->isInitialized());

        BraggScattererFactory::Instance().unsubscribe("MockScatterer");
        registered = BraggScattererFactory::Instance().getKeys();
        TS_ASSERT_EQUALS(std::find(registered.begin(), registered.end(), "MockScatterer"), registered.end());
        TS_ASSERT_THROWS_ANYTHING(BraggScattererFactory::Instance().createScatterer("MockScatterer"));
    }

private:
    class MockScatterer : public BraggScatterer
    {
    public:
        MockScatterer() : BraggScatterer() {}
        ~MockScatterer() { }

        std::string name() const { return "MockScatterer"; }

        MOCK_CONST_METHOD0(clone, BraggScatterer_sptr());
        MOCK_CONST_METHOD1(calculateStructureFactor, StructureFactor(const V3D &));
    };
};


#endif /* MANTID_GEOMETRY_BRAGGSCATTERERFACTORYTEST_H_ */
