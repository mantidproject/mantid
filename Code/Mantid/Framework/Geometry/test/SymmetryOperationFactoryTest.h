#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;


class SymmetryOperationFactoryTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SymmetryOperationFactoryTest *createSuite() { return new SymmetryOperationFactoryTest(); }
    static void destroySuite( SymmetryOperationFactoryTest *suite ) { delete suite; }

    SymmetryOperationFactoryTest()
    {
        SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
    }

    ~SymmetryOperationFactoryTest()
    {
        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
    }


    void testCreateSymOp()
    {
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOp("fake2"), Mantid::Kernel::Exception::ParseError);

        // createSymOp also works when an operation is not subscribed
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));

        // it's automatically registered
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), true);
    }

    void testUnsubscribe()
    {
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), true);

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
    }

    void testIsSubscribed()
    {
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), true);
    }

    void testSubscribedSymbols()
    {
        // Clear factory
        std::vector<std::string> allSymbols = SymmetryOperationFactory::Instance().subscribedSymbols();
        for(auto it = allSymbols.begin(); it != allSymbols.end(); ++it) {
            SymmetryOperationFactory::Instance().unsubscribeSymOp(*it);
        }

        // Subscribe two symmetry operations
        SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
        SymmetryOperationFactory::Instance().subscribeSymOp("-x,-y,-z");

        std::vector<std::string> symbols = SymmetryOperationFactory::Instance().subscribedSymbols();

        TS_ASSERT_EQUALS(symbols.size(), 2);
        TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "x,y,z"), symbols.end());
        TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "-x,-y,-z"), symbols.end());

        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
        SymmetryOperationFactory::Instance().unsubscribeSymOp("-x,-y,-z");

        // Restore factory
        for(auto it = allSymbols.begin(); it != allSymbols.end(); ++it) {
            SymmetryOperationFactory::Instance().subscribeSymOp(*it);
        }
    }
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_ */
