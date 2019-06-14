// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_
#define MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

#include "MantidGeometry/Crystal/CyclicGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class SpaceGroupFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpaceGroupFactoryTest *createSuite() {
    return new SpaceGroupFactoryTest();
  }
  static void destroySuite(SpaceGroupFactoryTest *suite) { delete suite; }

  void testInstance() {
    TS_ASSERT_THROWS_NOTHING(SpaceGroupFactory::Instance());
  }

  void testSubscribeGeneratedSpaceGroup() {
    TestableSpaceGroupFactory factory;

    TS_ASSERT(!factory.isSubscribed(2));
    TS_ASSERT(!factory.isSubscribed("P-1"));

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"));

    TS_ASSERT(factory.isSubscribed(2));
    TS_ASSERT(factory.isSubscribed("P-1"));

    // subscribing twice does not work
    TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"),
                     const std::invalid_argument &);

    // but having a different symbol for the same number is ok.
    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeGeneratedSpaceGroup(2, "F-1", "-x,-y,-z"))

    // neither does with a tabulated space group
    TS_ASSERT_THROWS(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"),
        const std::invalid_argument &);

    // Different number with same symbol - does not work
    TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(3, "P-1", "-x,-y,-z"),
                     const std::invalid_argument &);

    // invalid generators are caught before anything is done
    TS_ASSERT_THROWS_ANYTHING(
        factory.subscribeGeneratedSpaceGroup(4, "Fake", "invalid"));

    TS_ASSERT(!factory.isSubscribed(4));
    TS_ASSERT(!factory.isSubscribed("Fake"));
  }

  void testSubscribeTabulatedSpaceGroup() {
    TestableSpaceGroupFactory factory;

    TS_ASSERT(!factory.isSubscribed(2));
    TS_ASSERT(!factory.isSubscribed("P-1"));

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

    TS_ASSERT(factory.isSubscribed(2));
    TS_ASSERT(factory.isSubscribed("P-1"));

    // subscribing twice does not work
    TS_ASSERT_THROWS(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"),
        const std::invalid_argument &);

    // but having a different symbol for the same number is ok.
    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(2, "F-1", "x,y,z; -x,-y,-z"))

    // neither does with a generated space group
    TS_ASSERT_THROWS(factory.subscribeGeneratedSpaceGroup(2, "P-1", "-x,-y,-z"),
                     const std::invalid_argument &);

    // Different number with same symbol - does not work
    TS_ASSERT_THROWS(factory.subscribeTabulatedSpaceGroup(3, "P-1", "-x,-y,-z"),
                     const std::invalid_argument &);

    // invalid generators are caught before anything is done
    TS_ASSERT_THROWS_ANYTHING(
        factory.subscribeTabulatedSpaceGroup(4, "Fake", "invalid"));

    TS_ASSERT(!factory.isSubscribed(4));
    TS_ASSERT(!factory.isSubscribed("Fake"));
  }

  void testIsSubscribed() {
    TestableSpaceGroupFactory factory;

    TS_ASSERT(!factory.isSubscribed(1));

    TS_ASSERT(!factory.isSubscribed(2));
    TS_ASSERT(!factory.isSubscribed("P-1"));

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

    TS_ASSERT(factory.isSubscribed(2));
    TS_ASSERT(factory.isSubscribed("P-1"));

    TS_ASSERT(!factory.isSubscribed(1));
  }

  void testSubscribedSpaceGroupSymbols() {
    TestableSpaceGroupFactory factory;

    TS_ASSERT(factory.subscribedSpaceGroupSymbols().empty());

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

    std::vector<std::string> symbols = factory.subscribedSpaceGroupSymbols();
    TS_ASSERT_EQUALS(symbols.size(), 1);
    TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "P-1"),
                      symbols.end());

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(1, "P1", "x,y,z"));
    symbols = factory.subscribedSpaceGroupSymbols();
    TS_ASSERT_EQUALS(symbols.size(), 2);
    TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "P1"),
                      symbols.end());
  }

  void testSubscribedSpaceGroupNumbers() {
    TestableSpaceGroupFactory factory;

    TS_ASSERT(factory.subscribedSpaceGroupNumbers().empty());

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));

    std::vector<size_t> numbers = factory.subscribedSpaceGroupNumbers();
    TS_ASSERT_EQUALS(numbers.size(), 1);
    TS_ASSERT_DIFFERS(std::find(numbers.begin(), numbers.end(), 2),
                      numbers.end());

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(1, "P1", "x,y,z"));
    numbers = factory.subscribedSpaceGroupNumbers();
    TS_ASSERT_EQUALS(numbers.size(), 2);
    TS_ASSERT_DIFFERS(std::find(numbers.begin(), numbers.end(), 1),
                      numbers.end());

    // Subscribing the same number twice should not influence vector size
    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(1, "F1", "x,y,z"));
    numbers = factory.subscribedSpaceGroupNumbers();
    TS_ASSERT_EQUALS(numbers.size(), 2);
  }

  void testSubscribedSpaceGroupSymbolsForNumber() {
    TestableSpaceGroupFactory factory;
    factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z");
    factory.subscribeTabulatedSpaceGroup(2, "F-1", "x,y,z; -x,-y,-z");
    factory.subscribeTabulatedSpaceGroup(1, "P1", "x,y,z");

    std::vector<std::string> symbols = factory.subscribedSpaceGroupSymbols(1);
    TS_ASSERT_EQUALS(symbols.size(), 1);

    symbols = factory.subscribedSpaceGroupSymbols(2);
    TS_ASSERT_EQUALS(symbols.size(), 2);
  }

  void testSubscribedSpaceGroupSymbolsForPointGroup() {
    PointGroup_sptr pg = PointGroupFactory::Instance().createPointGroup("-1");

    TestableSpaceGroupFactory factory;

    TS_ASSERT_EQUALS(factory.subscribedSpaceGroupSymbols(pg).size(), 0);
    TS_ASSERT(factory.m_pointGroupMap.empty());

    factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z");
    TS_ASSERT_EQUALS(factory.subscribedSpaceGroupSymbols(pg).size(), 1);
    TS_ASSERT(!factory.m_pointGroupMap.empty());

    factory.subscribeTabulatedSpaceGroup(2, "F-1", "x,y,z; -x,-y,-z");
    TS_ASSERT(factory.m_pointGroupMap.empty());
    TS_ASSERT_EQUALS(factory.subscribedSpaceGroupSymbols(pg).size(), 2);
    TS_ASSERT(!factory.m_pointGroupMap.empty());
  }

  void testUnsubscribeSymbol() {
    TestableSpaceGroupFactory factory;

    TS_ASSERT_THROWS(factory.unsubscribeSpaceGroup("P-1"),
                     const std::invalid_argument &);

    TS_ASSERT_THROWS_NOTHING(
        factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z"));
    TS_ASSERT_THROWS_NOTHING(factory.unsubscribeSpaceGroup("P-1"));
  }

  void testUnsubscribeSymbol_multiple_groups_per_number() {
    TestableSpaceGroupFactory factory;

    factory.subscribeTabulatedSpaceGroup(2, "P-1", "x,y,z; -x,-y,-z");
    factory.subscribeTabulatedSpaceGroup(2, "F-1", "x,y,z; -x,-y,-z");

    TS_ASSERT_THROWS_NOTHING(factory.unsubscribeSpaceGroup("F-1"));

    TS_ASSERT(factory.isSubscribed("P-1"));
    TS_ASSERT(!factory.isSubscribed("F-1"));

    TS_ASSERT(factory.isSubscribed(2));

    factory.unsubscribeSpaceGroup("P-1");
    TS_ASSERT(!factory.isSubscribed(2));
  }

  void testAbstractSpaceGroupGenerator() {
    MockSpaceGroupGenerator generator(1, "P 1", "x,y,z");

    TS_ASSERT_EQUALS(generator.getNumber(), 1);
    TS_ASSERT_EQUALS(generator.getHMSymbol(), "P 1");
    TS_ASSERT_EQUALS(generator.getGeneratorString(), "x,y,z");
  }

  void testAbstractSpaceGroupGeneratorPrototypeBehavior() {
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

  void testAlgorithmicSpaceGroupGenerator() {
    AlgorithmicSpaceGroupGenerator generator(1, "P -1", "-x,-y,-z");

    SpaceGroup_const_sptr prototype = generator.getPrototype();

    TS_ASSERT_EQUALS(prototype->order(), 2);
    TS_ASSERT_EQUALS(prototype->hmSymbol(), "P -1");
  }

  void testTabulatedSpaceGroupGenerator() {
    TabulatedSpaceGroupGenerator generator(1, "P -1", "-x,-y,-z");

    SpaceGroup_const_sptr prototype = generator.getPrototype();

    TS_ASSERT_EQUALS(prototype->order(), 1);
  }

  void testTransformationSpaceGroupGenerator_fails_without_registered_base() {
    TransformationSpaceGroupGenerator generator(1, "fake symbol",
                                                "Fake base symbol | -x+z,y,-x");

    TS_ASSERT_THROWS(generator.getPrototype(), const std::invalid_argument &);
  }

  void testTransformationSpaceGroupGenerator_succeeds_with_registered_base() {
    TestableSpaceGroupFactory factory;
    factory.subscribeGeneratedSpaceGroup(1, "Fake base symbol", "x,-y,z+1/2");

    TestableTransformationSpaceGroupGenerator generator(
        1, "fake symbol", "Fake base symbol | -x+z,y,-x", factory);

    // It should not throw because 'Fake base symbol' is registered.
    TS_ASSERT_THROWS_NOTHING(generator.getPrototype());

    // The base symbol is trimmed so white space does not matter.
    TestableTransformationSpaceGroupGenerator generatorWhiteSpace(
        1, "fake symbol other", "  Fake base symbol     | -x+z,y,-x", factory);
    TS_ASSERT_THROWS_NOTHING(generatorWhiteSpace.getPrototype());
  }

  void testTransformationSpaceGroupGenerator_performs_correct_transformation() {
    // The P is required to determine the centering from the first character.
    TestableSpaceGroupFactory factory;
    factory.subscribeGeneratedSpaceGroup(1, "P Fake base symbol", "x,-y,z+1/2");

    TestableTransformationSpaceGroupGenerator generator(
        1, "fake symbol", "P Fake base symbol | -x+z,y,-x", factory);

    SpaceGroup_const_sptr generatedGroup = generator.getPrototype();

    /* The group subscribed to the factory is actually P 1 c 1,
     * after the transformation it is P 1 n 1.
     */
    Group_const_sptr correctGroup =
        GroupFactory::create<Group>("x,y,z; x+1/2,-y,z+1/2");

    TS_ASSERT_EQUALS(*generatedGroup, *correctGroup);
  }

  void test_OperatorSymOpString_too_short() {
    std::vector<std::string> strings{"a", "b"};
    TS_ASSERT_THROWS(SymmetryOperation("x,y,z") * strings,
                     const std::invalid_argument &);
  }

  void test_OperatorSymOpString_correctness() {
    std::vector<std::string> strings{"a", "b", "c"};

    TS_ASSERT(
        vectorsEqual(SymmetryOperation("x,y,z") * strings, {"a", "b", "c"}));
    TS_ASSERT(
        vectorsEqual(SymmetryOperation("-x,-y,-z") * strings, {"a", "b", "c"}));
    TS_ASSERT(
        vectorsEqual(SymmetryOperation("y,x,z") * strings, {"b", "a", "c"}));
    TS_ASSERT(
        vectorsEqual(SymmetryOperation("-z,x,-y") * strings, {"c", "a", "b"}));
  }

  void test_OperatorSymOpString_too_long() {
    std::vector<std::string> strings{"a", "b", "c", ":2", "something"};

    TS_ASSERT(vectorsEqual(SymmetryOperation("x,y,z") * strings,
                           {"a", "b", "c", ":2", "something"}));
    TS_ASSERT(vectorsEqual(SymmetryOperation("-x,-y,-z") * strings,
                           {"a", "b", "c", ":2", "something"}));
    TS_ASSERT(vectorsEqual(SymmetryOperation("y,x,z") * strings,
                           {"b", "a", "c", ":2", "something"}));
    TS_ASSERT(vectorsEqual(SymmetryOperation("-z,x,-y") * strings,
                           {"c", "a", "b", ":2", "something"}));
  }

  void test_OrthorhombicSymbolPermutations() {
    std::vector<std::string> transformations{"y,x,-z", "y,z,x", "z,y,-x",
                                             "z,x,y", "x,z,-y"};

    checkOrthorhombicSymbols(
        "C c c 2", transformations,
        {"C c c 2", "A 2 a a", "A 2 a a", "B b 2 b", "B b 2 b"});
    checkOrthorhombicSymbols(
        "P b c m", transformations,
        {"P c a m", "P m c a", "P m a b", "P b m a", "P c m b"});
  }

private:
  void checkOrthorhombicSymbols(
      const std::string &symbol,
      const std::vector<std::string> &transformations,
      const std::vector<std::string> &expectedTransformations) const {
    TestableSpaceGroupFactory factory;

    for (size_t i = 0; i < transformations.size(); ++i) {
      std::string transformed =
          factory.getTransformedSymbolOrthorhombic(symbol, transformations[i]);

      TSM_ASSERT_EQUALS("Transforming " + symbol + " with " +
                            transformations[i] + " should give " +
                            expectedTransformations[i] + ", but gave " +
                            transformed,
                        transformed, expectedTransformations[i]);
    }
  }

  bool vectorsEqual(const std::vector<std::string> &lhs,
                    const std::vector<std::string> &rhs) {
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
  };

  class TestableSpaceGroupFactory : public SpaceGroupFactoryImpl {
    friend class SpaceGroupFactoryTest;

  public:
    TestableSpaceGroupFactory() : SpaceGroupFactoryImpl() {}
    ~TestableSpaceGroupFactory() override {}
  };

  class MockSpaceGroupGenerator : public AbstractSpaceGroupGenerator {
  public:
    MockSpaceGroupGenerator(size_t number, const std::string &hmSymbol,
                            const std::string &generatorInformation)
        : AbstractSpaceGroupGenerator(number, hmSymbol, generatorInformation) {}
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD0(generateGroup, Group_const_sptr());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };

  /* Testing TransformationSpaceGroupGenerator is a bit hard in the sense that
   * it requires access to the SpaceGroupFactory to generate the base group
   * so that the transformation can be carried out. This is encapsulated in
   * the getBaseSpaceGroup()-method, so that this method can be replaced with
   * a custom one using any factory.
   */
  class TestableTransformationSpaceGroupGenerator
      : public TransformationSpaceGroupGenerator {
  public:
    TestableTransformationSpaceGroupGenerator(
        size_t number, const std::string &hmSymbol,
        const std::string &generatorInformation, SpaceGroupFactoryImpl &factory)
        : TransformationSpaceGroupGenerator(number, hmSymbol,
                                            generatorInformation),
          m_factory(factory) {}

  protected:
    SpaceGroup_const_sptr getBaseSpaceGroup() const override {
      return m_factory.createSpaceGroup(m_baseGroupHMSymbol);
    }

  private:
    SpaceGroupFactoryImpl &m_factory;
  };
};

#endif /* MANTID_GEOMETRY_SPACEGROUPFACTORYTEST_H_ */
