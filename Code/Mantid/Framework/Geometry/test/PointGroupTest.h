#ifndef MANTID_GEOMETRY_POINTGROUPTEST_H_
#define MANTID_GEOMETRY_POINTGROUPTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidKernel/Timer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Crystal/PointGroup.h"
#include <boost/lexical_cast.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class PointGroupTest : public CxxTest::TestSuite
{
public:

  void check_point_group(std::string name, V3D hkl, size_t numEquiv, V3D * equiv)
  {
    std::vector<PointGroup_sptr> pgs = getAllPointGroups();
    for (size_t i=0; i<pgs.size(); i++)
    {
      if (pgs[i]->getName().substr(0, name.size()) == name)
      {
        std::vector<V3D> equivalents = pgs[i]->getEquivalents(hkl);
        // check that the number of equivalent reflections is as expected.
        TSM_ASSERT_EQUALS(name + ": Expected " + boost::lexical_cast<std::string>(numEquiv) + " equivalents, got " + boost::lexical_cast<std::string>(equivalents.size()) + " instead.", equivalents.size(), numEquiv);

        // get reflection family for this hkl
        V3D family = pgs[i]->getReflectionFamily(hkl);

        for (size_t j=0; j<numEquiv; j++)
        {
          //std::cout << j << std::endl;
          if (!pgs[i]->isEquivalent(hkl, equiv[j]))
          {
            TSM_ASSERT( name + " : " + hkl.toString() + " is not equivalent to " +  equiv[j].toString(), false);
          }

          // make sure family for equiv[j] is the same as the one for hkl
          TS_ASSERT_EQUALS(pgs[i]->getReflectionFamily(equiv[j]), family);
          // also make sure that current equivalent is in the collection of equivalents.
          TS_ASSERT_DIFFERS(std::find(equivalents.begin(), equivalents.end(), equiv[j]), equivalents.end());
        }

        return;
      }
    }
    TSM_ASSERT("Point group not found", false);
  }

  void test_all_point_groups()
  {
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,-3)};
    check_point_group("-1", V3D(1,2,3), 2, equiv); }
    { V3D equiv[] = {V3D(1,2,3), V3D(-1,-2,-3), V3D(-1,2,-3), V3D(1,-2,3)  };
    check_point_group("1 2/m 1", V3D(1,2,3), 4, equiv); }
    { V3D equiv[] = {V3D(1,2,3), V3D(-1,-2,3), V3D(-1,-2,-3), V3D(1,2,-3)  };
    check_point_group("1 1 2/m", V3D(1,2,3), 4, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3), V3D(-1,2,-3), V3D(1,-2,-3), V3D(-1,-2,-3), V3D(1,2,-3), V3D(1,-2,3), V3D(-1,2,3)};
    check_point_group("mmm", V3D(1,2,3), 8, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3), V3D(-2,1,3), V3D(2,-1,3), V3D(-1,-2,-3), V3D(1,2,-3), V3D(2,-1,-3), V3D(-2,1,-3)};
    check_point_group("4/m", V3D(1,2,3), 8, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3), V3D(-2,1,3), V3D(2,-1,3), V3D(-1,2,-3), V3D(1,-2,-3), V3D(2,1,-3), V3D(-2,-1,-3), V3D(-1,-2,-3), V3D(1,2,-3), V3D(2,-1,-3), V3D(-2,1,-3), V3D(1,-2,3), V3D(-1,2,3),V3D(-2,-1,3), V3D(2,1,3)};
    check_point_group("4/mmm", V3D(1,2,3), 16, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-2,1-2,3), V3D(-1+2,-1,3), V3D(-1,-2,-3), V3D(2,-1+2,-3), V3D(1-2,1,-3)};
    check_point_group("-3", V3D(1,2,3), 6, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),V3D(2,1,-3),V3D(1-2,-2,-3),V3D(-1,-1+2,-3),V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),V3D(-2,-1,3),V3D(-1+2,2,3),V3D(1,1-2,3)};
    check_point_group("-3m1", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {
        V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),
        V3D(-2,-1,-3),V3D(-1+2,2,-3),V3D(1,1-2,-3),
        V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),
        V3D(2,1,3),V3D(1-2,-2,3),V3D(-1,-1+2,3)};
    check_point_group("-31m", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),V3D(-1,-2,3),V3D(2,-1+2,3),V3D(1-2,1,3),V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),V3D(1,2,-3),V3D(-2,1-2,-3),V3D(-1+2,-1,-3)};
    check_point_group("6/m", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(7,2,3),V3D(-2,7-2,3),V3D(-7+2,-7,3),V3D(-7,-2,3),V3D(2,-7+2,3),V3D(7-2,7,3),V3D(2,7,-3),V3D(7-2,-2,-3),V3D(-7,-7+2,-3),V3D(-2,-7,-3),V3D(-7+2,2,-3),V3D(7,7-2,-3),V3D(-7,-2,-3),V3D(2,-7+2,-3),V3D(7-2,7,-3),V3D(7,2,-3),V3D(-2,7-2,-3),V3D(-7+2,-7,-3),V3D(-2,-7,3),V3D(-7+2,2,3),V3D(7,7-2,3),V3D(2,7,3),V3D(7-2,-2,3),V3D(-7,-7+2,3)};
    check_point_group("6/mmm", V3D(7,2,3), 24, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-1,2,-3),V3D(1,-2,-3),V3D(3,1,2),V3D(3,-1,-2),V3D(-3,-1,2),V3D(-3,1,-2),V3D(2,3,1),V3D(-2,3,-1),V3D(2,-3,-1),V3D(-2,-3,1),V3D(-1,-2,-3),V3D(1,2,-3),V3D(1,-2,3),V3D(-1,2,3),V3D(-3,-1,-2),V3D(-3,1,2),V3D(3,1,-2),V3D(3,-1,2),V3D(-2,-3,-1),V3D(2,-3,1),V3D(-2,3,1),V3D(2,3,-1)};
    check_point_group("m-3", V3D(1,2,3), 24, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-1,2,-3),V3D(1,-2,-3),V3D(3,1,2),V3D(3,-1,-2),V3D(-3,-1,2),V3D(-3,1,-2),V3D(2,3,1),V3D(-2,3,-1),V3D(2,-3,-1),V3D(-2,-3,1),V3D(2,1,-3),V3D(-2,-1,-3),V3D(2,-1,3),V3D(-2,1,3),V3D(1,3,-2),V3D(-1,3,2),V3D(-1,-3,-2),V3D(1,-3,2),V3D(3,2,-1),V3D(3,-2,1),V3D(-3,2,1),V3D(-3,-2,-1),V3D(-1,-2,-3),V3D(1,2,-3),V3D(1,-2,3),V3D(-1,2,3),V3D(-3,-1,-2),V3D(-3,1,2),V3D(3,1,-2),V3D(3,-1,2),V3D(-2,-3,-1),V3D(2,-3,1),V3D(-2,3,1),V3D(2,3,-1),V3D(-2,-1,3),V3D(2,1,3),V3D(-2,1,-3),V3D(2,-1,-3),V3D(-1,-3,2),V3D(1,-3,-2),V3D(1,3,2),V3D(-1,3,-2),V3D(-3,-2,1),V3D(-3,2,-1),V3D(3,-2,-1),V3D(3,2,1)};
    check_point_group("m-3m", V3D(1,2,3), 48, equiv); }

    if (false)
    {
      { V3D equiv[] = {V3D(1,2,3)};
      check_point_group("1", V3D(1,2,3), 1, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,2,-3)};
      check_point_group("2", V3D(1,2,3), 2, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(1,2,-3)};
      check_point_group("m", V3D(1,2,3), 2, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-1,2,-3),V3D(1,-2,-3),};
      check_point_group("222", V3D(1,2,3), 4, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(1,-2,3),V3D(-1,2,3),};
      check_point_group("mm2", V3D(1,2,3), 4, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-2,1,3),V3D(2,-1,3),};
      check_point_group("4", V3D(1,2,3), 4, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(2,-1,-3),V3D(-2,1,-3),};
      check_point_group("-4", V3D(1,2,3), 4, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-2,1,3),V3D(2,-1,3),  V3D(-1,2,-3),V3D(1,-2,-3),V3D(2,1,-3),V3D(-2,-1,-3),};
      check_point_group("422", V3D(1,2,3), 8, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-2,1,3),V3D(2,-1,3),  V3D(1,-2,3),V3D(-1,2,3),V3D(-2,-1,3),V3D(2,1,3),};
      check_point_group("4mm", V3D(1,2,3), 8, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-2,1,3),V3D(2,-1,3),  V3D(1,-2,3),V3D(-1,2,3),V3D(-2,-1,3),V3D(2,1,3),};
      check_point_group("4mm", V3D(1,2,3), 8, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(2,-1,-3),V3D(-2,1,-3),  V3D(-1,2,-3),V3D(1,-2,-3),V3D(-2,-1,3),V3D(2,1,3),};
      check_point_group("-42m", V3D(1,2,3), 8, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(2,-1,-3),V3D(-2,1,-3),  V3D(1,-2,3),V3D(-1,2,3),V3D(2,1,-3),V3D(-2,-1,-3),};
      check_point_group("-4m2", V3D(1,2,3), 8, equiv); }
    }
  }

  void testConstruction()
  {
      TestablePointGroup defaultPointgroup;

      TS_ASSERT_EQUALS(defaultPointgroup.m_symmetryOperations.size(), 0);
      TS_ASSERT_EQUALS(defaultPointgroup.m_transformationMatrices.size(), 0);
  }

  void testAddSymmetryOperation()
  {
      TestablePointGroup pg;

      TS_ASSERT_EQUALS(pg.getSymmetryOperations().size(), 0);

      SymmetryOperation_const_sptr symOp(new SymOpInversion);
      pg.addSymmetryOperation(symOp);

      std::vector<SymmetryOperation_const_sptr> ops = pg.getSymmetryOperations();

      TS_ASSERT_EQUALS(ops.size(), 1);
      TS_ASSERT_EQUALS(ops[0], symOp);
  }

  void testSetTransformationMatrices()
  {
      TestablePointGroup pg;

      std::vector<IntMatrix> matrices(1, IntMatrix(3, 3, true));
      pg.setTransformationMatrices(matrices);

      TS_ASSERT_EQUALS(pg.m_transformationMatrices.size(), 1);
      TS_ASSERT_EQUALS(pg.m_transformationMatrices[0], IntMatrix(3, 3, true));
  }

  void testGenerateTransformationMatrices()
  {
      TestablePointGroup pg;

      SymmetryOperation_const_sptr identity(new SymOpIdentity);
      SymmetryOperation_const_sptr inversion(new SymOpInversion);
      SymmetryOperation_const_sptr mirror(new SymOpMirrorPlaneZ);
      SymmetryOperation_const_sptr twoFold(new SymOpRotationTwoFoldZ);

      pg.addSymmetryOperation(mirror);
      pg.addSymmetryOperation(twoFold);

      std::vector<SymmetryOperation_const_sptr> ops = pg.getSymmetryOperations();
      TS_ASSERT_EQUALS(ops.size(), 2);

      std::vector<IntMatrix> matrices = pg.generateTransformationMatrices(ops);

      // Mirror and 2-fold axis generate inversion, identity is implicit.
      TS_ASSERT_EQUALS(matrices.size(), 4);

      auto matrixVectorBegin = matrices.begin();
      auto matrixVectorEnd = matrices.end();

      IntMatrix identityMatrix(3, 3, true);

      TS_ASSERT_DIFFERS(std::find(matrixVectorBegin, matrixVectorEnd, identity->apply(identityMatrix)), matrixVectorEnd);
      TS_ASSERT_DIFFERS(std::find(matrixVectorBegin, matrixVectorEnd, inversion->apply(identityMatrix)), matrixVectorEnd);
      TS_ASSERT_DIFFERS(std::find(matrixVectorBegin, matrixVectorEnd, mirror->apply(identityMatrix)), matrixVectorEnd);
      TS_ASSERT_DIFFERS(std::find(matrixVectorBegin, matrixVectorEnd, twoFold->apply(identityMatrix)), matrixVectorEnd);

      TS_ASSERT_DIFFERS(matrices[0], matrices[1]);
  }

  void testCrystalSystems()
  {
      std::map<std::string, PointGroup::CrystalSystem> crystalSystemsMap;
      crystalSystemsMap["-1 (Triclinic)"] = PointGroup::Triclinic;
      crystalSystemsMap["1 2/m 1 (Monoclinic, unique axis b)"] = PointGroup::Monoclinic;
      crystalSystemsMap["1 1 2/m (Monoclinic, unique axis c)"] = PointGroup::Monoclinic;
      crystalSystemsMap["mmm (Orthorombic)"] = PointGroup::Orthorhombic;
      crystalSystemsMap["4/m (Tetragonal)"] = PointGroup::Tetragonal;
      crystalSystemsMap["4/mmm (Tetragonal)"] = PointGroup::Tetragonal;
      crystalSystemsMap["-3 (Trigonal - Hexagonal)"] = PointGroup::Trigonal;
      crystalSystemsMap["-3m1 (Trigonal - Rhombohedral)"] = PointGroup::Trigonal;
      crystalSystemsMap["-31m (Trigonal - Rhombohedral)"] = PointGroup::Trigonal;
      crystalSystemsMap["6/m (Hexagonal)"] = PointGroup::Hexagonal;
      crystalSystemsMap["6/mmm (Hexagonal)"] = PointGroup::Hexagonal;
      crystalSystemsMap["m-3 (Cubic)"] = PointGroup::Cubic;
      crystalSystemsMap["m-3m (Cubic)"] = PointGroup::Cubic;

      std::vector<PointGroup_sptr> pointgroups = getAllPointGroups();

      for(size_t i = 0; i < pointgroups.size(); ++i) {
          TSM_ASSERT_EQUALS(pointgroups[i]->getName() + ": Unexpected crystal system.", pointgroups[i]->crystalSystem(), crystalSystemsMap[pointgroups[i]->getName()]);
      }
  }

  void testCrystalSystemMap()
  {
      std::vector<PointGroup_sptr> pointgroups = getAllPointGroups();
      PointGroupCrystalSystemMap pgMap = getPointGroupsByCrystalSystem();

      TS_ASSERT_EQUALS(pointgroups.size(), pgMap.size());

      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Triclinic), 1);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Monoclinic), 2);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Orthorhombic), 1);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Tetragonal), 2);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Trigonal), 3);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Hexagonal), 2);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Cubic), 2);
  }

private:
  class TestablePointGroup : public PointGroup
  {
      friend class PointGroupTest;

  public:
      TestablePointGroup() : PointGroup()
      { }
      ~TestablePointGroup() {}

      MOCK_METHOD0(getName, std::string());
      MOCK_METHOD2(isEquivalent, bool(V3D hkl, V3D hkl2));
      MOCK_CONST_METHOD0(crystalSystem, PointGroup::CrystalSystem());
  };

};


#endif /* MANTID_GEOMETRY_POINTGROUPTEST_H_ */
