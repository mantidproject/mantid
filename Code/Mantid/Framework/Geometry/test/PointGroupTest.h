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
    { V3D equiv[] = {V3D(1,2,3),V3D(2,-3,3),V3D(-3,1,3), V3D(-1,-2,-3),V3D(-2,3,-3),V3D(3,-1,-3)};
    check_point_group("-3", V3D(1,2,3), 6, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(2,-3,3),V3D(-3,1,3),V3D(2,1,-3),V3D(1,-3,-3),V3D(-3,2,-3),V3D(-1,-2,-3),V3D(-2,3,-3),V3D(3,-1,-3),V3D(-2,-1,3),V3D(-1,3,3),V3D(3,-2,3)};
    check_point_group("-3m1", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(2,-3,3),V3D(-3,1,3),V3D(-2,-1,-3),V3D(-1,3,-3),V3D(3,-2,-3),V3D(-1,-2,-3),V3D(-2,3,-3),V3D(3,-1,-3),V3D(2,1,3),V3D(1,-3,3),V3D(-3,2,3),};
    check_point_group("-31m", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(2,-3,3),V3D(-3,1,3),V3D(-1,-2,3),V3D(-2,3,3),V3D(3,-1,3),V3D(-1,-2,-3),V3D(-2,3,-3),V3D(3,-1,-3),V3D(1,2,-3),V3D(2,-3,-3),V3D(-3,1,-3)};
    check_point_group("6/m", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(2,-3,3),V3D(-3,1,3),V3D(-1,-2,3),V3D(-2,3,3),V3D(3,-1,3),V3D(2,1,-3),V3D(1,-3,-3),V3D(-3,2,-3),V3D(-2,-1,-3),V3D(-1,3,-3),V3D(3,-2,-3),V3D(-1,-2,-3),V3D(-2,3,-3),V3D(3,-1,-3),V3D(1,2,-3),V3D(2,-3,-3),V3D(-3,1,-3),V3D(-2,-1,3),V3D(-1,3,3),V3D(3,-2,3),V3D(2,1,3),V3D(1,-3,3),V3D(-3,2,3)};
    check_point_group("6/mmm", V3D(1,2,3), 24, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-1,2,-3),V3D(1,-2,-3),V3D(3,1,2),V3D(3,-1,-2),V3D(-3,-1,2),V3D(-3,1,-2),V3D(2,3,1),V3D(-2,3,-1),V3D(2,-3,-1),V3D(-2,-3,1),V3D(-1,-2,-3),V3D(1,2,-3),V3D(1,-2,3),V3D(-1,2,3),V3D(-3,-1,-2),V3D(-3,1,2),V3D(3,1,-2),V3D(3,-1,2),V3D(-2,-3,-1),V3D(2,-3,1),V3D(-2,3,1),V3D(2,3,-1)};
    check_point_group("m-3", V3D(1,2,3), 24, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-1,-2,3),V3D(-1,2,-3),V3D(1,-2,-3),V3D(3,1,2),V3D(3,-1,-2),V3D(-3,-1,2),V3D(-3,1,-2),V3D(2,3,1),V3D(-2,3,-1),V3D(2,-3,-1),V3D(-2,-3,1),V3D(2,1,-3),V3D(-2,-1,-3),V3D(2,-1,3),V3D(-2,1,3),V3D(1,3,-2),V3D(-1,3,2),V3D(-1,-3,-2),V3D(1,-3,2),V3D(3,2,-1),V3D(3,-2,1),V3D(-3,2,1),V3D(-3,-2,-1),V3D(-1,-2,-3),V3D(1,2,-3),V3D(1,-2,3),V3D(-1,2,3),V3D(-3,-1,-2),V3D(-3,1,2),V3D(3,1,-2),V3D(3,-1,2),V3D(-2,-3,-1),V3D(2,-3,1),V3D(-2,3,1),V3D(2,3,-1),V3D(-2,-1,3),V3D(2,1,3),V3D(-2,1,-3),V3D(2,-1,-3),V3D(-1,-3,2),V3D(1,-3,-2),V3D(1,3,2),V3D(-1,3,-2),V3D(-3,-2,1),V3D(-3,2,-1),V3D(3,-2,-1),V3D(3,2,1)};
    check_point_group("m-3m", V3D(1,2,3), 48, equiv); }

      { V3D equiv[] = {V3D(1,2,3)};
      check_point_group("1", V3D(1,2,3), 1, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(-1,2,-3)};
      check_point_group("2", V3D(1,2,3), 2, equiv); }
      { V3D equiv[] = {V3D(1,2,3),V3D(1,-2,3)};
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


  void testCrystalSystems()
  {
      std::map<std::string, PointGroup::CrystalSystem> crystalSystemsMap;
      crystalSystemsMap["1"] = PointGroup::Triclinic;
      crystalSystemsMap["-1"] = PointGroup::Triclinic;

      crystalSystemsMap["2"] = PointGroup::Monoclinic;
      crystalSystemsMap["m"] = PointGroup::Monoclinic;
      crystalSystemsMap["2/m"] = PointGroup::Monoclinic;
      crystalSystemsMap["112/m"] = PointGroup::Monoclinic;

      crystalSystemsMap["222"] = PointGroup::Orthorhombic;
      crystalSystemsMap["mm2"] = PointGroup::Orthorhombic;
      crystalSystemsMap["mmm"] = PointGroup::Orthorhombic;

      crystalSystemsMap["4"] = PointGroup::Tetragonal;
      crystalSystemsMap["-4"] = PointGroup::Tetragonal;
      crystalSystemsMap["4/m"] = PointGroup::Tetragonal;
      crystalSystemsMap["422"] = PointGroup::Tetragonal;
      crystalSystemsMap["4mm"] = PointGroup::Tetragonal;
      crystalSystemsMap["-42m"] = PointGroup::Tetragonal;
      crystalSystemsMap["-4m2"] = PointGroup::Tetragonal;
      crystalSystemsMap["4/mmm"] = PointGroup::Tetragonal;

      crystalSystemsMap["3"] = PointGroup::Trigonal;
      crystalSystemsMap["-3"] = PointGroup::Trigonal;
      crystalSystemsMap["321"] = PointGroup::Trigonal;
      crystalSystemsMap["32"] = PointGroup::Trigonal;
      crystalSystemsMap["312"] = PointGroup::Trigonal;
      crystalSystemsMap["3m1"] = PointGroup::Trigonal;
      crystalSystemsMap["3m"] = PointGroup::Trigonal;
      crystalSystemsMap["31m"] = PointGroup::Trigonal;
      crystalSystemsMap["-3m1"] = PointGroup::Trigonal;
      crystalSystemsMap["-3m"] = PointGroup::Trigonal;
      crystalSystemsMap["-31m"] = PointGroup::Trigonal;
      crystalSystemsMap["3 r"] = PointGroup::Trigonal;
      crystalSystemsMap["-3 r"] = PointGroup::Trigonal;
      crystalSystemsMap["32 r"] = PointGroup::Trigonal;
      crystalSystemsMap["3m r"] = PointGroup::Trigonal;
      crystalSystemsMap["-3m r"] = PointGroup::Trigonal;

      crystalSystemsMap["6"] = PointGroup::Hexagonal;
      crystalSystemsMap["-6"] = PointGroup::Hexagonal;
      crystalSystemsMap["6/m"] = PointGroup::Hexagonal;
      crystalSystemsMap["622"] = PointGroup::Hexagonal;
      crystalSystemsMap["6mm"] = PointGroup::Hexagonal;
      crystalSystemsMap["-62m"] = PointGroup::Hexagonal;
      crystalSystemsMap["-6m2"] = PointGroup::Hexagonal;
      crystalSystemsMap["6/mmm"] = PointGroup::Hexagonal;

      crystalSystemsMap["23"] = PointGroup::Cubic;
      crystalSystemsMap["m-3"] = PointGroup::Cubic;
      crystalSystemsMap["432"] = PointGroup::Cubic;
      crystalSystemsMap["-43m"] = PointGroup::Cubic;
      crystalSystemsMap["m-3m"] = PointGroup::Cubic;

      std::vector<PointGroup_sptr> pointgroups = getAllPointGroups();

      for(size_t i = 0; i < pointgroups.size(); ++i) {
          TSM_ASSERT_EQUALS(pointgroups[i]->getSymbol() + ": Unexpected crystal system.", pointgroups[i]->crystalSystem(), crystalSystemsMap[pointgroups[i]->getSymbol()]);
      }
  }

  void testCrystalSystemMap()
  {
      std::vector<PointGroup_sptr> pointgroups = getAllPointGroups();
      PointGroupCrystalSystemMap pgMap = getPointGroupsByCrystalSystem();

      TS_ASSERT_EQUALS(pointgroups.size(), pgMap.size());

      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Triclinic), 2);

      // 2/m with axis b and c, so one more
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Monoclinic), 3 + 1);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Orthorhombic), 3);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Tetragonal), 8);

      // 5 with rhombohedral axes and 8 with hexagonal and 3 for defaults
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Trigonal), 5 + 8 + 3);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Hexagonal), 8);
      TS_ASSERT_EQUALS(pgMap.count(PointGroup::Cubic), 5);
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
};


#endif /* MANTID_GEOMETRY_POINTGROUPTEST_H_ */
