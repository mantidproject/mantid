#ifndef MANTID_KERNEL_MDUNITFACTORYTEST_H_
#define MANTID_KERNEL_MDUNITFACTORYTEST_H_

#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace Mantid::Kernel;
using namespace testing;

/**
 * Helper Mock MDUnit
 */
GNU_DIAG_OFF_SUGGEST_OVERRIDE
class MockMDUnit : public MDUnit {
public:
  MOCK_CONST_METHOD0(getUnitLabel, UnitLabel());
  MOCK_CONST_METHOD1(canConvertTo, bool(const MDUnit &other));
  MOCK_CONST_METHOD0(isQUnit, bool());
  MOCK_CONST_METHOD0(clone, MDUnit *());
};

/**
 * Helper Mock MDUnitFactory
 */
class MockMDUnitFactory : public MDUnitFactory {
public:
  MOCK_CONST_METHOD1(createRaw, MDUnit *(const std::string &));
  MOCK_CONST_METHOD1(canInterpret, bool(const std::string &));
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
class MDUnitFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDUnitFactoryTest *createSuite() { return new MDUnitFactoryTest(); }
  static void destroySuite(MDUnitFactoryTest *suite) { delete suite; }

  void test_checks_before_creates() {
    MockMDUnitFactory factory;

    // We say that the string is interpretable
    EXPECT_CALL(factory, canInterpret(_)).WillOnce(Return(true));
    // So we expect to then be asked to create an instance of the product
    EXPECT_CALL(factory, createRaw(_))
        .Times(1)
        .WillOnce(Return(new MockMDUnit));

    factory.create("");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void test_asks_successor_to_create() {

    MockMDUnitFactory factoryPrimary;
    // We say that the string is uknown on the primary factory
    EXPECT_CALL(factoryPrimary, canInterpret(_)).WillOnce(Return(false));
    // So we DONT expect to then be asked to create an instance of THAT product
    EXPECT_CALL(factoryPrimary, createRaw(_)).Times(0);

    MockMDUnitFactory *factorySecondary = new MockMDUnitFactory;
    // We say that the string is interpretable
    EXPECT_CALL(*factorySecondary, canInterpret(_)).WillOnce(Return(true));
    // So we expect to then be asked to create an instance of the product
    EXPECT_CALL(*factorySecondary, createRaw(_))
        .Times(1)
        .WillOnce(Return(new MockMDUnit));

    factoryPrimary.setSuccessor(
        std::unique_ptr<MDUnitFactory>(factorySecondary));
    factoryPrimary.create("");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&factoryPrimary));
    TS_ASSERT(Mock::VerifyAndClearExpectations(factorySecondary));
  }

  void test_no_successor_throws() {

    MockMDUnitFactory factoryPrimary;
    // We say that the string is uknown on the primary factory
    EXPECT_CALL(factoryPrimary, canInterpret(_)).WillOnce(Return(false));
    // So we DONT expect to then be asked to create an instance of THAT product
    EXPECT_CALL(factoryPrimary, createRaw(_)).Times(0);

    TSM_ASSERT_THROWS("No successor. This has to throw",
                      factoryPrimary.create(""), std::invalid_argument &);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&factoryPrimary));
  }

  void test_label_unit_factory() {
    LabelUnitFactory factory;
    auto product = factory.create("anything");
    TS_ASSERT(dynamic_cast<LabelUnit *>(product.get()));
    TSM_ASSERT("This is not a q-unit", !product->isQUnit());
  }

  void test_label_unit_factory_is_q() {
    LabelUnitFactory factory;

    auto product = factory.create("A^-1");
    TS_ASSERT(dynamic_cast<LabelUnit *>(product.get()));
    TSM_ASSERT("This is a label unit, but denotes Q", product->isQUnit());

    product = factory.create("in 4.436 A^-1");
    TS_ASSERT(dynamic_cast<LabelUnit *>(product.get()));
    TSM_ASSERT("This is a label unit, but denotes Q", product->isQUnit());
  }

  void test_RLU_factory() {
    ReciprocalLatticeUnitFactory factory;
    auto product = factory.create(Units::Symbol::RLU);
    TS_ASSERT(dynamic_cast<ReciprocalLatticeUnit *>(product.get()));
  }

  void test_InverseAngstroms_factory() {
    InverseAngstromsUnitFactory factory;

    auto product = factory.create(Units::Symbol::InverseAngstrom);
    TS_ASSERT(dynamic_cast<InverseAngstromsUnit *>(product.get()));
  }

  void test_make_standard_chain() {
    MDUnitFactory_uptr chain = makeMDUnitFactoryChain();
    // Now lets try the chain of factories out
    TS_ASSERT(dynamic_cast<InverseAngstromsUnit *>(
        chain->create(Units::Symbol::InverseAngstrom).get()));
    TS_ASSERT(dynamic_cast<ReciprocalLatticeUnit *>(
        chain->create(Units::Symbol::RLU).get()));
    TS_ASSERT(dynamic_cast<LabelUnit *>(chain->create("Anything else").get()));
  }
};

#endif /* MANTID_KERNEL_MDUNITFACTORYTEST_H_ */
