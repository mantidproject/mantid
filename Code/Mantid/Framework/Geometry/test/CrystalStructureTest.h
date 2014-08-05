#ifndef MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_
#define MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/CrystalStructure.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CrystalStructureTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalStructureTest *createSuite() { return new CrystalStructureTest(); }
  static void destroySuite( CrystalStructureTest *suite ) { delete suite; }

  CrystalStructureTest() :
      m_CsCl(4.126, 4.126, 4.126),
      m_pg(new PointGroupLaue13),
      m_centering(new ReflectionConditionPrimitive)
  {
  }


  void testConstruction()
  {
      // Only cell is really required, for the others there's a default value
      CrystalStructure structure(m_CsCl);

      TS_ASSERT_EQUALS(structure.cell().a(), m_CsCl.a());
      TS_ASSERT(boost::dynamic_pointer_cast<PointGroupLaue1>(structure.pointGroup()));
      TS_ASSERT(boost::dynamic_pointer_cast<ReflectionConditionPrimitive>(structure.centering()));
      TS_ASSERT_THROWS_NOTHING(structure.crystalSystem());
      TS_ASSERT_EQUALS(structure.crystalSystem(), PointGroup::Triclinic);

      CrystalStructure structurePg(m_CsCl, m_pg);
      TS_ASSERT_EQUALS(structurePg.pointGroup(), m_pg);
      TS_ASSERT(boost::dynamic_pointer_cast<ReflectionConditionPrimitive>(structurePg.centering()));
      TS_ASSERT_EQUALS(structurePg.crystalSystem(), m_pg->crystalSystem());

      CrystalStructure structureAll(m_CsCl, m_pg, m_centering);
      TS_ASSERT_EQUALS(structureAll.centering(), m_centering);
  }

  void testCellGetSet()
  {
      CrystalStructure structure(m_CsCl);
      TS_ASSERT_EQUALS(structure.cell().a(), m_CsCl.a());

      UnitCell Si(5.43, 5.43, 5.43);
      structure.setCell(Si);

      TS_ASSERT_EQUALS(structure.cell().a(), Si.a());
  }

  void testPointGroupGetSet()
  {
      CrystalStructure structure(m_CsCl, m_pg);
      TS_ASSERT_EQUALS(structure.pointGroup(), m_pg);
      TS_ASSERT_EQUALS(structure.crystalSystem(), m_pg->crystalSystem());

      PointGroup_sptr newPg = boost::make_shared<PointGroupLaue3>();
      structure.setPointGroup(newPg);

      TS_ASSERT_EQUALS(structure.pointGroup(), newPg);
      TS_ASSERT_EQUALS(structure.crystalSystem(), newPg->crystalSystem());
  }

  void testCenteringGetSet()
  {
      CrystalStructure structure(m_CsCl, m_pg, m_centering);
      TS_ASSERT_EQUALS(structure.centering(), m_centering);

      ReflectionCondition_sptr newCentering = boost::make_shared<ReflectionConditionAFaceCentred>();
      structure.setCentering(newCentering);

      TS_ASSERT_EQUALS(structure.centering(), newCentering);
  }

  void testGetUniqueHKLsHappyCase()
  {
      double dMin = 0.55;
      double dMax = 4.0;

      CrystalStructure structure(m_CsCl, m_pg, m_centering);

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

  void testGetUniqueHKLsExceptions()
  {
      CrystalStructure structure(m_CsCl, m_pg, m_centering);

      TS_ASSERT_THROWS(structure.getUniqueHKLs(0.0, 1.0), std::invalid_argument);
      TS_ASSERT_THROWS(structure.getUniqueHKLs(-1.0, 1.0), std::invalid_argument);
      TS_ASSERT_THROWS(structure.getUniqueHKLs(1.0, 0.3), std::invalid_argument);

      TS_ASSERT_THROWS_NOTHING(structure.getUniqueHKLs(2.0, 3.0));

      structure.setCentering(ReflectionCondition_sptr());
      TS_ASSERT_THROWS(structure.getUniqueHKLs(2.0, 3.0), std::invalid_argument);
      structure.setCentering(m_centering);

      structure.setPointGroup(PointGroup_sptr());
      TS_ASSERT_THROWS(structure.getUniqueHKLs(2.0, 3.0), std::invalid_argument);
      structure.setPointGroup(m_pg);
  }

  void testGetHKLsHappyCase()
  {
      double dMin = 0.55;
      double dMax = 4.0;

      // make a structure with P-1
      CrystalStructure structure(m_CsCl);

      std::vector<V3D> unique = structure.getUniqueHKLs(dMin, dMax);
      std::vector<V3D> peaks = structure.getHKLs(dMin, dMax);

      // Because of symmetry -1, each reflection has multiplicity 2.
      TS_ASSERT_EQUALS(peaks.size(), 2 * unique.size());
  }

  void testGetHKLsExceptions()
  {
      CrystalStructure structure(m_CsCl, m_pg, m_centering);

      TS_ASSERT_THROWS(structure.getHKLs(0.0, 1.0), std::invalid_argument);
      TS_ASSERT_THROWS(structure.getHKLs(-1.0, 1.0), std::invalid_argument);
      TS_ASSERT_THROWS(structure.getHKLs(1.0, 0.3), std::invalid_argument);

      TS_ASSERT_THROWS_NOTHING(structure.getHKLs(2.0, 3.0));

      structure.setCentering(ReflectionCondition_sptr());
      TS_ASSERT_THROWS(structure.getHKLs(2.0, 3.0), std::invalid_argument);
      structure.setCentering(m_centering);

      // no point group information required for this
      structure.setPointGroup(PointGroup_sptr());
      TS_ASSERT_THROWS_NOTHING(structure.getHKLs(2.0, 3.0));
      structure.setPointGroup(m_pg);
  }

  void testGetDValues()
  {
      std::vector<V3D> hkls;
      hkls.push_back(V3D(1, 0, 0));
      hkls.push_back(V3D(0, 1, 0));
      hkls.push_back(V3D(0, 0, 1));

      UnitCell ortho(2.0, 3.0, 5.0);
      CrystalStructure structure(ortho);

      std::vector<double> dValues = structure.getDValues(hkls);

      TS_ASSERT_EQUALS(dValues.size(), hkls.size());
      TS_ASSERT_EQUALS(dValues[0], 2.0);
      TS_ASSERT_EQUALS(dValues[1], 3.0);
      TS_ASSERT_EQUALS(dValues[2], 5.0);
  }

private:
    UnitCell m_CsCl;
    PointGroup_sptr m_pg;
    ReflectionCondition_sptr m_centering;
};


#endif /* MANTID_GEOMETRY_CRYSTALSTRUCTURETEST_H_ */
