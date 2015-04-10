#ifndef MANTID_GEOMETRY_BRAGGSCATTERERINCRYSTALSTRUCTURETEST_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERINCRYSTALSTRUCTURETEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/BraggScattererInCrystalStructure.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using ::testing::Return;
using ::testing::A;

class BraggScattererInCrystalStructureTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BraggScattererInCrystalStructureTest *createSuite() {
    return new BraggScattererInCrystalStructureTest();
  }
  static void destroySuite(BraggScattererInCrystalStructureTest *suite) {
    delete suite;
  }

  void testInitialization() {
    BraggScattererInCrystalStructure_sptr scatterer = getDefaultScatterer();

    TS_ASSERT(!scatterer->isInitialized());
    TS_ASSERT_THROWS_NOTHING(scatterer->initialize());
    TS_ASSERT(scatterer->isInitialized());

    TS_ASSERT(scatterer->existsProperty("Position"));
    TS_ASSERT(scatterer->existsProperty("UnitCell"));
    TS_ASSERT(scatterer->existsProperty("SpaceGroup"));
  }

  void testAfterScattererPropertySet() {
    // BraggScattererInCrystalStructure_sptr scatterer =
    // getInitializedScatterer();
    // MockBraggScatterer *mockScatterer = dynamic_cast<MockBraggScatterer
    // *>(scatterer.get());
    // EXPECT_CALL(mockScatterer, afterScattererPropertySet)
  }

  void testGetSetPosition() {
    BraggScattererInCrystalStructure_sptr scatterer = getInitializedScatterer();

    V3D goodPosition(0.2, 0.4, 1./3.);
    scatterer->setProperty("Position", "0.2, 0.4, 1/3");

    V3D testPos;
    TS_ASSERT_THROWS_NOTHING(testPos = scatterer->getPosition());
    TS_ASSERT_EQUALS(testPos, goodPosition);

    V3D badPosition(1.2, 4.3, -6.2);
    TS_ASSERT_THROWS_NOTHING(
        scatterer->setProperty("Position", "[1.2, 4.3, -6.2]"));
    TS_ASSERT_THROWS_NOTHING(testPos = scatterer->getPosition());
    TS_ASSERT_DIFFERS(testPos, badPosition);
    TS_ASSERT_EQUALS(testPos, V3D(0.2, 0.3, 0.8));
  }

  void testGetSetCell() {
    BraggScattererInCrystalStructure_sptr scatterer = getInitializedScatterer();

    UnitCell cell(5.43, 5.43, 5.43);

    TS_ASSERT_THROWS_NOTHING(
        scatterer->setProperty("UnitCell", unitCellToStr(cell)));
    TS_ASSERT_EQUALS(scatterer->getCell().getG(), cell.getG());
  }

  void testGetSetSpaceGroup() {
    BraggScattererInCrystalStructure_sptr scatterer = getInitializedScatterer();

    SpaceGroup_const_sptr testGroup =
        SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");

    TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("SpaceGroup", "P m -3 m"));
    TS_ASSERT_EQUALS(scatterer->getSpaceGroup()->hmSymbol(),
                     testGroup->hmSymbol());
  }

  void testEquivalentPositions() {
    BraggScattererInCrystalStructure_sptr scatterer = getInitializedScatterer();

    V3D generalPosition(0.3, 0.32, 0.45);

    // No space group set - no equivalent positions
    scatterer->setProperty("Position", "[0.3, 0.32, 0.45]");
    TS_ASSERT_EQUALS(scatterer->getEquivalentPositions().size(), 1);
    TS_ASSERT_EQUALS(scatterer->getEquivalentPositions().front(),
                     generalPosition);

    // Assigning a space group must cause recalculation of equivalent positions
    SpaceGroup_const_sptr testGroup =
        SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");
    scatterer->setProperty("SpaceGroup", "P m -3 m");

    TS_ASSERT_EQUALS(scatterer->getEquivalentPositions().size(),
                     testGroup->order());

    // Re-setting the position also recalculates
    V3D specialPosition(0.0, 0.0, 0.0);

    scatterer->setProperty("Position", "[0, 0, 0]");
    // Pm-3m does not contain translations, so (0,0,0) is not transformed by any
    // symmetry operation of the group
    TS_ASSERT_EQUALS(scatterer->getEquivalentPositions().size(), 1);
    TS_ASSERT_EQUALS(scatterer->getEquivalentPositions().front(),
                     specialPosition);
  }

  void testUnitCellStringValidator() {
    IValidator_sptr validator = boost::make_shared<UnitCellStringValidator>();

    // non-working examples
    TS_ASSERT_DIFFERS(validator->isValid("1.0"), "");
    TS_ASSERT_DIFFERS(validator->isValid("1.0 1.0"), "");
    TS_ASSERT_DIFFERS(validator->isValid("1.0 1.0 1.0 1.0"), "");
    TS_ASSERT_DIFFERS(validator->isValid("1.0 1.0 1.0 1.0 1.0"), "");
    TS_ASSERT_DIFFERS(validator->isValid("1.0.3 1.0 1.0"), "");

    // Working examples
    TS_ASSERT_EQUALS(validator->isValid("1.0 1.0 1.0"), "");
    TS_ASSERT_EQUALS(validator->isValid("1.0 1.0 1.0 90.0 90.0 90.0"), "");
    TS_ASSERT_EQUALS(validator->isValid("1 2 3 90 90 90"), "");
    TS_ASSERT_EQUALS(validator->isValid("1.1 2.2 3.2 90 90 90"), "");
    TS_ASSERT_EQUALS(validator->isValid("1.0 1.0 1.0 90.0 90.0 90.0  "), "");
  }

private:
  BraggScattererInCrystalStructure_sptr getDefaultScatterer() {
    boost::shared_ptr<MockBraggScatterer> mockScatterer =
        boost::make_shared<MockBraggScatterer>();
    EXPECT_CALL(*mockScatterer,
                afterScattererPropertySet(A<const std::string &>()))
        .WillRepeatedly(Return());

    return mockScatterer;
  }

  BraggScattererInCrystalStructure_sptr getInitializedScatterer() {
    BraggScattererInCrystalStructure_sptr raw = getDefaultScatterer();
    raw->initialize();

    return raw;
  }

  class MockBraggScatterer : public BraggScattererInCrystalStructure {
  public:
    MockBraggScatterer() : BraggScattererInCrystalStructure() {}
    ~MockBraggScatterer() {}

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_CONST_METHOD0(clone, BraggScatterer_sptr());
    MOCK_CONST_METHOD1(calculateStructureFactor, StructureFactor(const V3D &));
    MOCK_METHOD1(afterScattererPropertySet, void(const std::string &));
  };
};

#endif /* MANTID_GEOMETRY_BRAGGSCATTERERINCRYSTALSTRUCTURETEST_H_ */
