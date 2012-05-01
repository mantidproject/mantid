#ifndef MANTID_GEOMETRY_POINTGROUPTEST_H_
#define MANTID_GEOMETRY_POINTGROUPTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidGeometry/Crystal/PointGroup.h"

using namespace Mantid;
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
        for (size_t j=0; j<numEquiv; j++)
        {
          //std::cout << j << std::endl;
          if (!pgs[i]->isEquivalent(hkl, equiv[j]))
          {
            TSM_ASSERT( name + " : " + hkl.toString() + " is not equivalent to " +  equiv[j].toString(), false);
          }
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
    { V3D equiv[] = {
        V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),
        V3D(-2,-1,-3),V3D(-1+2,2,-3),V3D(1,1-2,-3),
        V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),
        V3D(2,1,3),V3D(1-2,-2,3),V3D(-1,-1+2,3)};
    check_point_group("-31m", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),V3D(2,1,-3),V3D(1-2,-2,-3),V3D(-1,-1+2,-3),V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),V3D(-2,-1,3),V3D(-1+2,2,3),V3D(1,1-2,3)};
    check_point_group("-3m1", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),V3D(-1,-2,3),V3D(2,-1+2,3),V3D(1-2,1,3),V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),V3D(1,2,-3),V3D(-2,1-2,-3),V3D(-1+2,-1,-3)};
    check_point_group("6/m", V3D(1,2,3), 12, equiv); }
    { V3D equiv[] = {V3D(1,2,3),V3D(-2,1-2,3),V3D(-1+2,-1,3),V3D(-1,-2,3),V3D(2,-1+2,3),V3D(1-2,1,3),V3D(2,1,-3),V3D(1-2,-2,-3),V3D(-1,-1+2,-3),V3D(-2,-1,-3),V3D(-1+2,2,-3),V3D(1,1-2,-3),V3D(-1,-2,-3),V3D(2,-1+2,-3),V3D(1-2,1,-3),V3D(1,2,-3),V3D(-2,1-2,-3),V3D(-1+2,-1,-3),V3D(-2,-1,3),V3D(-1+2,2,3),V3D(1,1-2,3),V3D(2,1,3),V3D(1-2,-2,3),V3D(-1,-1+2,3)};
    check_point_group("6/mmm", V3D(1,2,3), 24, equiv); }
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


};


#endif /* MANTID_GEOMETRY_POINTGROUPTEST_H_ */
