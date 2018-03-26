#ifndef MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_
#define MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CrystalStructureTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalStructureTest *createSuite() {
    return new CrystalStructureTest();
  }
  static void destroySuite(CrystalStructureTest *suite) { delete suite; }

  CrystalStructureTest() : m_CsCl(4.126, 4.126, 4.126) {
    m_spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("I m -3 m");

    m_scatterers = CompositeBraggScatterer::create();
    m_scatterers->addScatterer(
        BraggScattererFactory::Instance().createScatterer(
            "IsotropicAtomBraggScatterer",
            R"({"Element":"Si","Position":"0,0,0"})"));
  }

  void testConstructionSpaceGroup() {
    TS_ASSERT_THROWS_NOTHING(
        CrystalStructure structure(m_CsCl, m_spaceGroup, m_scatterers));

    CrystalStructure structure(m_CsCl, m_spaceGroup, m_scatterers);
    TS_ASSERT_EQUALS(structure.cell().getG(), m_CsCl.getG());
    TS_ASSERT_EQUALS(structure.spaceGroup(), m_spaceGroup);
    TS_ASSERT_EQUALS(structure.getScatterers()->nScatterers(),
                     m_scatterers->nScatterers());
  }

  void testSetSpaceGroup() {
    CrystalStructure structure(m_CsCl, m_spaceGroup, m_scatterers);

    TS_ASSERT_EQUALS(structure.spaceGroup()->hmSymbol(),
                     m_spaceGroup->hmSymbol());
    TS_ASSERT_THROWS_NOTHING(structure.setSpaceGroup(
        SpaceGroupFactory::Instance().createSpaceGroup("I a -3 d")));

    // Not null anymore
    TS_ASSERT(structure.spaceGroup());
    TS_ASSERT_EQUALS(structure.spaceGroup()->hmSymbol(), "I a -3 d")
  }

  void testCellGetSet() {
    CrystalStructure structure(m_CsCl, m_spaceGroup, m_scatterers);
    TS_ASSERT_EQUALS(structure.cell().a(), m_CsCl.a());

    UnitCell Si(5.43, 5.43, 5.43);
    structure.setCell(Si);

    TS_ASSERT_EQUALS(structure.cell().a(), Si.a());
  }

  void testCrystalStructureFromStrings() {
    CrystalStructure structure("5.431 5.431 5.431", "F d -3 m",
                               "Si 0 0 0 1.0 0.02");

    TS_ASSERT_EQUALS(structure.cell().a(), 5.431);
    TS_ASSERT_EQUALS(structure.cell().b(), 5.431);
    TS_ASSERT_EQUALS(structure.cell().c(), 5.431);

    TS_ASSERT_EQUALS(structure.spaceGroup()->hmSymbol(), "F d -3 m");
    TS_ASSERT_EQUALS(structure.getScatterers()->nScatterers(), 1);
  }

  void testCopyConstructor() {
    CrystalStructure one("1.2 2.3 3.4", "F d d d", "Fe 1/8 1/8 1/8 1.0 0.001");

    CrystalStructure two(one);
    TS_ASSERT_EQUALS(two.getScatterers()->nScatterers(),
                     one.getScatterers()->nScatterers());
    TS_ASSERT_EQUALS(two.spaceGroup()->hmSymbol(),
                     one.spaceGroup()->hmSymbol());
    TS_ASSERT_EQUALS(unitCellToStr(two.cell()), unitCellToStr(one.cell()));
  }

  void testAssignmentOperator() {
    CrystalStructure one("1.2 2.3 3.4", "F d d d", "Fe 1/8 1/8 1/8 1.0 0.001");

    CrystalStructure two = one;

    TS_ASSERT_EQUALS(two.getScatterers()->nScatterers(),
                     one.getScatterers()->nScatterers());
    TS_ASSERT_EQUALS(two.spaceGroup()->hmSymbol(),
                     one.spaceGroup()->hmSymbol());
    TS_ASSERT_EQUALS(unitCellToStr(two.cell()), unitCellToStr(one.cell()));
  }

private:
  UnitCell m_CsCl;
  SpaceGroup_const_sptr m_spaceGroup;
  CompositeBraggScatterer_sptr m_scatterers;
};

#endif /* MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_ */
