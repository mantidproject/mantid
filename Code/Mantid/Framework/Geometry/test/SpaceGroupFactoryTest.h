#ifndef MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_
#define MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace  Mantid::Geometry;
using Mantid::Kernel::V3D;

class SpaceGroupFactoryTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SpaceGroupFactoryTest *createSuite() { return new SpaceGroupFactoryTest(); }
    static void destroySuite( SpaceGroupFactoryTest *suite ) { delete suite; }

    void testInstance()
    {
        TS_ASSERT_THROWS_NOTHING(SpaceGroupFactory::Instance());
    }

    void testSubscribeGeneratedSpaceGroup()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT(!factory.isSubscribed(2));
        TS_ASSERT(!factory.isSubscribed("P-1"));

        TS_ASSERT_THROWS_NOTHING(factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"));

        TS_ASSERT(factory.isSubscribed(2));
        TS_ASSERT(factory.isSubscribed("P-1"));

        // subscribing twice does not work
        TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"), std::invalid_argument);

        // neither does with a tabulated space group
        TS_ASSERT_THROWS(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"), std::invalid_argument);

        TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(2, "FAKE", "-x,-y,-z"), std::invalid_argument);
        TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(3, "P-1", "-x,-y,-z"), std::invalid_argument);

        // invalid generators are caught before anything is done
        TS_ASSERT_THROWS_ANYTHING(factory.subscribeGeneratedSpaceGroup(4, "Fake", "invalid"));

        TS_ASSERT(!factory.isSubscribed(4));
        TS_ASSERT(!factory.isSubscribed("Fake"));
    }

    void testSubscribeTabulatedSpaceGroup()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT(!factory.isSubscribed(2));
        TS_ASSERT(!factory.isSubscribed("P-1"));

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

        TS_ASSERT(factory.isSubscribed(2));
        TS_ASSERT(factory.isSubscribed("P-1"));

        // subscribing twice does not work
        TS_ASSERT_THROWS(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"), std::invalid_argument);

        // neither does with a generated space group
        TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"), std::invalid_argument);

        TS_ASSERT_THROWS(factory.subscribeTabulatedSpaceGroup(2, "FAKE", "-x,-y,-z"), std::invalid_argument);
        TS_ASSERT_THROWS(factory.subscribeTabulatedSpaceGroup(3, "P-1", "-x,-y,-z"), std::invalid_argument);

        // invalid generators are caught before anything is done
        TS_ASSERT_THROWS_ANYTHING(factory.subscribeTabulatedSpaceGroup(4, "Fake", "invalid"));

        TS_ASSERT(!factory.isSubscribed(4));
        TS_ASSERT(!factory.isSubscribed("Fake"));
    }

    void testIsSubscribed()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT(!factory.isSubscribed(1));

        TS_ASSERT(!factory.isSubscribed(2));
        TS_ASSERT(!factory.isSubscribed("P-1"));

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

        TS_ASSERT(factory.isSubscribed(2));
        TS_ASSERT(factory.isSubscribed("P-1"));

        TS_ASSERT(!factory.isSubscribed(1));
    }

    void testSubscribedSpaceGroupSymbols()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT(factory.subscribedSpaceGroupSymbols().empty());

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

        std::vector<std::string> symbols = factory.subscribedSpaceGroupSymbols();
        TS_ASSERT_EQUALS(symbols.size(), 1);
        TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "P-1"), symbols.end());

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(1, "P1", "x,y,z"));
        symbols = factory.subscribedSpaceGroupSymbols();
        TS_ASSERT_EQUALS(symbols.size(), 2);
        TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "P1"), symbols.end());
    }

    void testSubscribedSpaceGroupNumbers()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT(factory.subscribedSpaceGroupNumbers().empty());

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

        std::vector<size_t> numbers = factory.subscribedSpaceGroupNumbers();
        TS_ASSERT_EQUALS(numbers.size(), 1);
        TS_ASSERT_DIFFERS(std::find(numbers.begin(), numbers.end(), 2), numbers.end());

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(1, "P1", "x,y,z"));
        numbers = factory.subscribedSpaceGroupNumbers();
        TS_ASSERT_EQUALS(numbers.size(), 2);
        TS_ASSERT_DIFFERS(std::find(numbers.begin(), numbers.end(), 1), numbers.end());
    }

    void testUnsubscribeNumber()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT_THROWS(factory.unsubscribeSpaceGroup(2), std::invalid_argument);

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));
        TS_ASSERT_THROWS_NOTHING(factory.unsubscribeSpaceGroup(2));

        TS_ASSERT_THROWS(factory.unsubscribeSpaceGroup("P-1"), std::invalid_argument);
    }

    void testUnsubscribeSymbol()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT_THROWS(factory.unsubscribeSpaceGroup("P-1"), std::invalid_argument);

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));
        TS_ASSERT_THROWS_NOTHING(factory.unsubscribeSpaceGroup("P-1"));

        TS_ASSERT_THROWS(factory.unsubscribeSpaceGroup(2), std::invalid_argument);
    }

    void testCreateSpaceGroupNumber()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT_THROWS(factory.createSpaceGroup(2), std::invalid_argument);
        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

        TS_ASSERT_THROWS_NOTHING(factory.createSpaceGroup(2));

        SpaceGroup_const_sptr spaceGroup = factory.createSpaceGroup(2);
        TS_ASSERT_EQUALS(spaceGroup->order(), 2);
    }

    void testSpaceGroup()
    {
        SpaceGroup_const_sptr sgBCC = SpaceGroupFactory::Instance().createSpaceGroup(229);

        std::cout << "Space group: " << sgBCC->hmSymbol() << " (" << sgBCC->number() << "):" << std::endl;
        std::cout << "  Order: " << sgBCC->order() << std::endl;

        std::cout << "  Equivalent positions:" << std::endl;
        std::cout << "    96l (";

        V3D general(0.54, 0.43, 0.12);
        std::vector<V3D> equivs = sgBCC->getEquivalentPositions(general);

        std::cout << equivs.size() << " equivalents)." << std::endl;
    }


private:
    class TestableSpaceGroupFactory : public SpaceGroupFactoryImpl
    {
        friend class SpaceGroupFactoryTest;
    public:
        TestableSpaceGroupFactory() : SpaceGroupFactoryImpl() { }
        ~TestableSpaceGroupFactory() { }
    };

};


#endif /* MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_ */
