#ifndef MANTID_GEOMETRY_POINTGROUPTEST_H_
#define MANTID_GEOMETRY_POINTGROUPTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidKernel/Timer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include <boost/lexical_cast.hpp>
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class PointGroupTest : public CxxTest::TestSuite
{
public:

    void check_point_group(std::string name, V3D hkl, size_t numEquiv, V3D * equiv)
    {
        PointGroup_sptr testedPointGroup = PointGroupFactory::Instance().createPointGroup(name);

        std::vector<V3D> equivalents = testedPointGroup->getEquivalents(hkl);
        // check that the number of equivalent reflections is as expected.
        TSM_ASSERT_EQUALS(name + ": Expected " + boost::lexical_cast<std::string>(numEquiv) + " equivalents, got " + boost::lexical_cast<std::string>(equivalents.size()) + " instead.", equivalents.size(), numEquiv);

        // get reflection family for this hkl
        V3D family = testedPointGroup->getReflectionFamily(hkl);

        for (size_t j=0; j<numEquiv; j++)
        {
            //std::cout << j << std::endl;
            if (!testedPointGroup->isEquivalent(hkl, equiv[j]))
            {
                TSM_ASSERT( name + " : " + hkl.toString() + " is not equivalent to " +  equiv[j].toString(), false);
            }

            // make sure family for equiv[j] is the same as the one for hkl
            TS_ASSERT_EQUALS(testedPointGroup->getReflectionFamily(equiv[j]), family);
            // also make sure that current equivalent is in the collection of equivalents.
            TS_ASSERT_DIFFERS(std::find(equivalents.begin(), equivalents.end(), equiv[j]), equivalents.end());
        }

        return;
    }

  void test_all_point_groups()
  {
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,-3)};
    check_point_group("-1", V3D(1,2,3), 2, equiv); }
    { V3D equiv[] = {V3D(1,2,3), V3D(-1,-2,-3), V3D(-1,2,-3), V3D(1,-2,3)  };
    check_point_group("2/m", V3D(1,2,3), 4, equiv); }
    { V3D equiv[] = {V3D(1,2,3), V3D(-1,-2,3), V3D(-1,-2,-3), V3D(1,2,-3)  };
    check_point_group("112/m", V3D(1,2,3), 4, equiv); }
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


  void testCrystalSystems()
  {
      std::map<std::string, PointGroup::CrystalSystem> crystalSystemsMap;
      crystalSystemsMap["-1 (Triclinic)"] = PointGroup::Triclinic;
      crystalSystemsMap["2/m (Monoclinic, unique axis b)"] = PointGroup::Monoclinic;
      crystalSystemsMap["112/m (Monoclinic, unique axis c)"] = PointGroup::Monoclinic;
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

  void testPerformance()
  {
      PointGroup_sptr pg =PointGroupFactory::Instance().createPointGroup("m-3m");
      checkPointGroupPerformance(pg);
  }

private:
  void checkPointGroupPerformance(const PointGroup_sptr &pointGroup)
  {
      V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-1,2,-3),V3D(1,-2,-3),V3D(3,1,2),V3D(3,-1,-2),V3D(-3,-1,2),V3D(-3,1,-2),V3D(2,3,1),V3D(-2,3,-1),V3D(2,-3,-1),V3D(-2,-3,1),V3D(2,1,-3),V3D(-2,-1,-3),V3D(2,-1,3),V3D(-2,1,3),V3D(1,3,-2),V3D(-1,3,2),V3D(-1,-3,-2),V3D(1,-3,2),V3D(3,2,-1),V3D(3,-2,1),V3D(-3,2,1),V3D(-3,-2,-1),V3D(-1,-2,-3),V3D(1,2,-3),V3D(1,-2,3),V3D(-1,2,3),V3D(-3,-1,-2),V3D(-3,1,2),V3D(3,1,-2),V3D(3,-1,2),V3D(-2,-3,-1),V3D(2,-3,1),V3D(-2,3,1),V3D(2,3,-1),V3D(-2,-1,3),V3D(2,1,3),V3D(-2,1,-3),V3D(2,-1,-3),V3D(-1,-3,2),V3D(1,-3,-2),V3D(1,3,2),V3D(-1,3,-2),V3D(-3,-2,1),V3D(-3,2,-1),V3D(3,-2,-1),V3D(3,2,1)};
      std::vector<V3D> hkls(equiv, equiv + 48);

      Timer t;

      V3D base(1, 2, 3);

      t.reset();
      int h = 0;
      for(size_t i = 0; i < 1000; ++i) {
          for(auto hkl = hkls.begin(); hkl != hkls.end(); ++hkl) {
              bool eq = pointGroup->isEquivalent(base, *hkl);
              if(eq) {
                ++h;
              }
          }
      }

      float time = t.elapsed();

      std::cout << "Eq: " << h << ", Time: " << time / 1000.0 << std::endl;

  }

  /*
  class TestablePointGroup : public PointGroup
  {
      friend class PointGroupTest;

  public:
      TestablePointGroup() : PointGroup("")
      { }
      ~TestablePointGroup() {}

      MOCK_CONST_METHOD0(getName, std::string());
      MOCK_CONST_METHOD2(isEquivalent, bool(const V3D &hkl, const V3D &hkl2));
      MOCK_CONST_METHOD0(crystalSystem, PointGroup::CrystalSystem());

      void init() { }
  };
*/
};


#endif /* MANTID_GEOMETRY_POINTGROUPTEST_H_ */
