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
            "IsotropicAtomBraggScatterer", "Element=Si;Position=[0,0,0]"));
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

  void testThrowIfRangeUnacceptable() {
    TestableCrystalStructure structure(m_CsCl, m_spaceGroup, m_scatterers);

    TS_ASSERT_THROWS(structure.throwIfRangeUnacceptable(0.0, 1.0),
                     std::invalid_argument);
    TS_ASSERT_THROWS(structure.throwIfRangeUnacceptable(-10.0, 1.0),
                     std::invalid_argument);
    TS_ASSERT_THROWS(structure.throwIfRangeUnacceptable(1.0, 0.0),
                     std::invalid_argument);
    TS_ASSERT_THROWS(structure.throwIfRangeUnacceptable(1.0, -1.0),
                     std::invalid_argument);
    TS_ASSERT_THROWS(structure.throwIfRangeUnacceptable(2.0, 1.0),
                     std::invalid_argument);

    TS_ASSERT_THROWS_NOTHING(structure.throwIfRangeUnacceptable(1.0, 2.0));
  }

  void testGetUniqueHKLsHappyCase() {
    double dMin = 0.55;
    double dMax = 4.0;

    CrystalStructure structure(
        m_CsCl, SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m"),
        m_scatterers);

    TS_ASSERT_THROWS_NOTHING(structure.getUniqueHKLs(dMin, dMax));

    std::vector<V3D> peaks = structure.getUniqueHKLs(dMin, dMax);

    TS_ASSERT_EQUALS(peaks.size(), 68);
    TS_ASSERT_EQUALS(peaks[0], V3D(1, 1, 0));
    TS_ASSERT_EQUALS(peaks[11], V3D(3, 2, 0));
    TS_ASSERT_EQUALS(peaks[67], V3D(7, 2, 1));

    // make d-value list and check that peaks are within limits
    std::vector<double> peaksD = structure.getDValues(peaks);

    std::sort(peaksD.begin(), peaksD.end());

    TS_ASSERT_LESS_THAN_EQUALS(dMin, peaksD.front());
    TS_ASSERT_LESS_THAN_EQUALS(peaksD.back(), dMax);
  }

  void testGetHKLsHappyCase() {
    double dMin = 0.55;
    double dMax = 4.0;

    // make a structure with P-1
    CrystalStructure structure(
        m_CsCl, SpaceGroupFactory::Instance().createSpaceGroup("P -1"),
        m_scatterers);

    std::vector<V3D> unique = structure.getUniqueHKLs(dMin, dMax);
    std::vector<V3D> peaks = structure.getHKLs(dMin, dMax);

    // Because of symmetry -1, each reflection has multiplicity 2.
    TS_ASSERT_EQUALS(peaks.size(), 2 * unique.size());
  }

  void testGetDValues() {
    std::vector<V3D> hkls;
    hkls.push_back(V3D(1, 0, 0));
    hkls.push_back(V3D(0, 1, 0));
    hkls.push_back(V3D(0, 0, 1));

    UnitCell ortho(2.0, 3.0, 5.0);
    CrystalStructure structure(
        ortho, SpaceGroupFactory::Instance().createSpaceGroup("P -1"),
        m_scatterers);

    std::vector<double> dValues = structure.getDValues(hkls);

    TS_ASSERT_EQUALS(dValues.size(), hkls.size());
    TS_ASSERT_EQUALS(dValues[0], 2.0);
    TS_ASSERT_EQUALS(dValues[1], 3.0);
    TS_ASSERT_EQUALS(dValues[2], 5.0);
  }

  void testReflectionConditionMethods() {
    /* This test compares the two methods that are available
     * for testing if a reflection is allowed.
     */

    UnitCell cellSi(5.43, 5.43, 5.43);

    // Crystal structure with cell, space group, scatterers - must be a space
    // group without glides/screws.
    SpaceGroup_const_sptr sgSi =
        SpaceGroupFactory::Instance().createSpaceGroup("F m -3 m");
    // With an atom at (x, x, x) there are no extra conditions.
    CompositeBraggScatterer_sptr scatterers = CompositeBraggScatterer::create();
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=Si;Position=[0.3,0.3,0.3];U=0.05"));

    // Crystal structure with cell, point group, centering
    CrystalStructure siUseCentering(cellSi, sgSi, scatterers);
    std::vector<V3D> hklsCentering =
        siUseCentering.getUniqueHKLs(0.6, 10.0, CrystalStructure::UseCentering);

    CrystalStructure siUseStructureFactors(cellSi, sgSi, scatterers);
    std::vector<V3D> hklsStructureFactors = siUseStructureFactors.getUniqueHKLs(
        0.6, 10.0, CrystalStructure::UseStructureFactor);
    std::vector<V3D> hklsCenteringAlternative =
        siUseStructureFactors.getUniqueHKLs(0.6, 10.0,
                                            CrystalStructure::UseCentering);

    TS_ASSERT_EQUALS(hklsCentering.size(), hklsStructureFactors.size());
    TS_ASSERT_EQUALS(hklsCentering.size(), hklsCenteringAlternative.size());

    for (size_t i = 0; i < hklsCentering.size(); ++i) {
      TS_ASSERT_EQUALS(hklsCentering[i], hklsStructureFactors[i]);
      TS_ASSERT_EQUALS(hklsCentering[i], hklsCenteringAlternative[i]);
    }

    /* Add another scatterer and use setScatterers to replace old scatterers of
     *siUseStructureFactors
     *
     * At this point the advantage of using the structure factor method is very
     *clear. When an atom is
     * added at a different position (for example [0.4, 0.4, 0.4]), some
     *reflections become 0.
     *
     * When the atom is slightly shifted like in the case below, the same
     *reflections as above are
     * allowed.
     */
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=Si;Position=[0.42,0.42,0.42];U=0.05"));
    siUseStructureFactors.setScatterers(scatterers);

    hklsStructureFactors = siUseStructureFactors.getUniqueHKLs(
        0.6, 10.0, CrystalStructure::UseStructureFactor);

    for (size_t i = 0; i < hklsCentering.size(); ++i) {
      TS_ASSERT_EQUALS(hklsCentering[i], hklsStructureFactors[i]);
    }
  }

  void testHexagonal() {
    UnitCell cellMg(3.2094, 3.2094, 5.2108, 90.0, 90.0, 120.0);
    CompositeBraggScatterer_sptr scatterers = CompositeBraggScatterer::create();
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=Mg;Position=[0.333333,0.666667,0.25];U=0.005"));
    SpaceGroup_const_sptr sgMg =
        SpaceGroupFactory::Instance().createSpaceGroup("P 63/m m c");

    CrystalStructure mg(cellMg, sgMg, scatterers);

    std::vector<V3D> hkls =
        mg.getUniqueHKLs(0.5, 10.0, CrystalStructure::UseStructureFactor);
    for (size_t i = 0; i < hkls.size(); ++i) {
      TS_ASSERT_LESS_THAN(0.5, cellMg.d(hkls[i]));
    }

    std::vector<double> dValues = mg.getDValues(hkls);
    for (size_t i = 0; i < hkls.size(); ++i) {
      TS_ASSERT_LESS_THAN(0.5, dValues[i]);
    }
  }

  void testTrigonal() {
    UnitCell cellAl2O3(4.759355, 4.759355, 12.99231, 90.0, 90.0, 120.0);
    CompositeBraggScatterer_sptr scatterers = CompositeBraggScatterer::create();
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=Al;Position=[0,0,0.35217];U=0.005"));
    scatterers->addScatterer(BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer",
        "Element=O;Position=[0.69365,0,0.25];U=0.005"));
    SpaceGroup_const_sptr sgAl2O3 =
        SpaceGroupFactory::Instance().createSpaceGroup("R -3 c");

    // O is on the 18e wyckoff position
    std::vector<V3D> positions = sgAl2O3 * V3D(0.69365000, 0, 0.25000);
    TS_ASSERT_EQUALS(positions.size(), 18);

    CrystalStructure mg(cellAl2O3, sgAl2O3, scatterers);

    std::vector<V3D> hkls =
        mg.getUniqueHKLs(0.885, 10.0, CrystalStructure::UseStructureFactor);

    TS_ASSERT_EQUALS(hkls.size(), 44);
  }

private:
  UnitCell m_CsCl;

  SpaceGroup_const_sptr m_spaceGroup;
  CompositeBraggScatterer_sptr m_scatterers;

  class TestableCrystalStructure : public CrystalStructure {
    friend class CrystalStructureTest;

  public:
    TestableCrystalStructure(const UnitCell &unitCell,
                             const SpaceGroup_const_sptr &spaceGroup,
                             const CompositeBraggScatterer_sptr &scatterers)
        : CrystalStructure(unitCell, spaceGroup, scatterers) {}
  };
};

#endif /* MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_ */
