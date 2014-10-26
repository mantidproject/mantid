#ifndef MANTID_GEOMETRY_SCATTERERFACTORYTEST_H_
#define MANTID_GEOMETRY_SCATTERERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/ScattererFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class ScattererFactoryTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static ScattererFactoryTest *createSuite() { return new ScattererFactoryTest(); }
    static void destroySuite( ScattererFactoryTest *suite ) { delete suite; }


    void testSubscribeCreateUnsubscribeGetKeys()
    {
        std::vector<std::string> registered = ScattererFactory::Instance().getKeys();
        TS_ASSERT_EQUALS(std::find(registered.begin(), registered.end(), "MockScatterer"), registered.end());
        TS_ASSERT_THROWS_ANYTHING(ScattererFactory::Instance().createScatterer("MockScatterer"));

        ScattererFactory::Instance().subscribeScatterer<MockScatterer>();

        registered = ScattererFactory::Instance().getKeys();
        TS_ASSERT_DIFFERS(std::find(registered.begin(), registered.end(), "MockScatterer"), registered.end());
        TS_ASSERT_THROWS_NOTHING(ScattererFactory::Instance().createScatterer("MockScatterer"));

        IScatterer_sptr scatterer = ScattererFactory::Instance().createScatterer("MockScatterer");
        TS_ASSERT(scatterer->isInitialized());

        ScattererFactory::Instance().unsubscribe("MockScatterer");
        registered = ScattererFactory::Instance().getKeys();
        TS_ASSERT_EQUALS(std::find(registered.begin(), registered.end(), "MockScatterer"), registered.end());
        TS_ASSERT_THROWS_ANYTHING(ScattererFactory::Instance().createScatterer("MockScatterer"));
    }

private:
    class MockScatterer : public IScatterer
    {
    public:
        MockScatterer() : IScatterer() {}
        ~MockScatterer() { }

        std::string name() const { return "MockScatterer"; }

        MOCK_CONST_METHOD0(clone, IScatterer_sptr());
        MOCK_CONST_METHOD1(calculateStructureFactor, StructureFactor(const V3D &));
    };
};


#endif /* MANTID_GEOMETRY_SCATTERERFACTORYTEST_H_ */
