#ifndef MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORTEST_H_
#define MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORTEST_H_

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using ::testing::Mock;
using ::testing::_;
using ::testing::Return;

class StructureFactorCalculatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StructureFactorCalculatorTest *createSuite() {
    return new StructureFactorCalculatorTest();
  }
  static void destroySuite(StructureFactorCalculatorTest *suite) {
    delete suite;
  }

  void testCrystalStructureSetHookIsCalled() {
    CrystalStructure cs(UnitCell(1, 2, 3),
                        SpaceGroupFactory::Instance().createSpaceGroup("P -1"),
                        CompositeBraggScatterer::create());

    MockStructureFactorCalculator calculator;
    EXPECT_CALL(calculator, crystalStructureSetHook(_)).Times(1);

    calculator.setCrystalStructure(cs);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&calculator));
  }

  void testGetFSquared() {
    MockStructureFactorCalculator calculator;

    EXPECT_CALL(calculator, getF(_))
        .WillRepeatedly(Return(StructureFactor(2.21, 3.1)));

    // Check that the square of 2.21 + i * 3.1 is returned
    TS_ASSERT_DELTA(calculator.getFSquared(V3D()), 14.4941, 1e-15);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&calculator));
  }

  void testGetFs() {
    MockStructureFactorCalculator calculator;

    int numHKLs = 10;
    EXPECT_CALL(calculator, getF(_))
        .Times(numHKLs)
        .WillRepeatedly(Return(StructureFactor(2.0, 2.0)));

    std::vector<V3D> hkls(numHKLs);
    std::vector<StructureFactor> sfs = calculator.getFs(hkls);

    TS_ASSERT_EQUALS(sfs.size(), hkls.size());

    for (auto sf = sfs.begin(); sf != sfs.end(); ++sf) {
      TS_ASSERT_EQUALS(*sf, StructureFactor(2.0, 2.0));
    }

    TS_ASSERT(Mock::VerifyAndClearExpectations(&calculator));
  }

  void testGetFsSquared() {
    MockStructureFactorCalculator calculator;

    int numHKLs = 10;
    EXPECT_CALL(calculator, getF(_))
        .Times(numHKLs)
        .WillRepeatedly(Return(StructureFactor(2.0, 2.0)));

    std::vector<V3D> hkls(numHKLs);
    std::vector<double> sfsSquared = calculator.getFsSquared(hkls);

    TS_ASSERT_EQUALS(sfsSquared.size(), hkls.size());

    for (auto sf = sfsSquared.begin(); sf != sfsSquared.end(); ++sf) {
      TS_ASSERT_EQUALS(*sf, 8.0);
    }

    TS_ASSERT(Mock::VerifyAndClearExpectations(&calculator));
  }

private:
  /* This MockStructureFactorCalculator helps to test whether the
   * default implementations of the virtual methods work correctly.
   * Furthermore it is used to confirm that crystalStructureSetHook
   * is called appropriately.
   */
  class MockStructureFactorCalculator : public StructureFactorCalculator {
  public:
    DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(getF, StructureFactor(const V3D &hkl));
    MOCK_METHOD1(crystalStructureSetHook,
                 void(const CrystalStructure &crystalStructure));
    DIAG_ON_SUGGEST_OVERRIDE
  };
};

#endif /* MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORTEST_H_ */
