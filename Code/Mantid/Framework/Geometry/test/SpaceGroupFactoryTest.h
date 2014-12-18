#ifndef MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_
#define MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/CyclicGroup.h"

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


        // but having a different symbol for the same number is ok.
        TS_ASSERT_THROWS_NOTHING(factory.subscribeGeneratedSpaceGroup(2, "F-1", "-x,-y,-z"))

        // neither does with a tabulated space group
        TS_ASSERT_THROWS(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"), std::invalid_argument);

        // Different number with same symbol - does not work
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

        // but having a different symbol for the same number is ok.
        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "F-1", "x,y,z; -x,-y,-z"))

        // neither does with a generated space group
        TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"), std::invalid_argument);

        // Different number with same symbol - does not work
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

        // Subscribing the same number twice should not influence vector size
        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(1, "F1", "x,y,z"));
        numbers = factory.subscribedSpaceGroupNumbers();
        TS_ASSERT_EQUALS(numbers.size(), 2);
    }

    void testSubscribedSpaceGroupSymbolsForNumber()
    {
        TestableSpaceGroupFactory factory;
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z");
        factory.subscribeTabulatedSpaceGroup(2, "F-1", "x,y,z; -x,-y,-z");
        factory.subscribeTabulatedSpaceGroup(1, "P1", "x,y,z");

        std::vector<std::string> symbols = factory.subscribedSpaceGroupSymbols(1);
        TS_ASSERT_EQUALS(symbols.size(), 1);

        symbols = factory.subscribedSpaceGroupSymbols(2);
        TS_ASSERT_EQUALS(symbols.size(), 2);
    }

    void testUnsubscribeSymbol()
    {
        TestableSpaceGroupFactory factory;

        TS_ASSERT_THROWS(factory.unsubscribeSpaceGroup("P-1"), std::invalid_argument);

        TS_ASSERT_THROWS_NOTHING(factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));
        TS_ASSERT_THROWS_NOTHING(factory.unsubscribeSpaceGroup("P-1"));
    }

    void testAbstractSpaceGroupGenerator()
    {
        MockSpaceGroupGenerator generator(1, "P 1", "x,y,z");

        TS_ASSERT_EQUALS(generator.getNumber(), 1);
        TS_ASSERT_EQUALS(generator.getHMSymbol(), "P 1");
        TS_ASSERT_EQUALS(generator.getGeneratorString(), "x,y,z");
    }

    void testAbstractSpaceGroupGeneratorPrototypeBehavior()
    {
        /* The prototype is generated only once, after that
         * it's stored in the generator.
         */
        MockSpaceGroupGenerator generator(1, "P 1", "x,y,z");

        EXPECT_CALL(generator, generateGroup())
                .Times(1)
                .WillOnce(Return(GroupFactory::create<CyclicGroup>("-x,-y,-z")));

        SpaceGroup_const_sptr prototype = generator.getPrototype();
        TS_ASSERT(prototype);

        SpaceGroup_const_sptr other = generator.getPrototype();

        TS_ASSERT_EQUALS(other.get(), prototype.get());
        TS_ASSERT_EQUALS(other, prototype);
        TS_ASSERT_EQUALS(other->hmSymbol(), prototype->hmSymbol());

        generator.getPrototype();
        generator.getPrototype();
    }

    void testAlgorithmicSpaceGroupGenerator()
    {
        AlgorithmicSpaceGroupGenerator generator(1, "P -1", "-x,-y,-z");

        SpaceGroup_const_sptr prototype = generator.getPrototype();

        TS_ASSERT_EQUALS(prototype->order(), 2);
        TS_ASSERT_EQUALS(prototype->hmSymbol(), "P -1");
    }

    void testTabulatedSpaceGroupGenerator()
    {
        TabulatedSpaceGroupGenerator generator(1, "P -1", "-x,-y,-z");

        SpaceGroup_const_sptr prototype = generator.getPrototype();

        TS_ASSERT_EQUALS(prototype->order(), 1);
    }

private:
    class TestableSpaceGroupFactory : public SpaceGroupFactoryImpl
    {
        friend class SpaceGroupFactoryTest;
    public:
        TestableSpaceGroupFactory() : SpaceGroupFactoryImpl() { }
        ~TestableSpaceGroupFactory() { }
    };

    class MockSpaceGroupGenerator : public AbstractSpaceGroupGenerator
    {
    public:
        MockSpaceGroupGenerator(size_t number, const std::string &hmSymbol, const std::string &generatorInformation) :
            AbstractSpaceGroupGenerator(number, hmSymbol, generatorInformation)
        { }
        ~MockSpaceGroupGenerator() { }

        MOCK_CONST_METHOD0(generateGroup, Group_const_sptr());
    };

};


#endif /* MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_ */
