// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Timer.h"
#include <boost/lexical_cast.hpp>
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class PointGroupTest : public CxxTest::TestSuite {
public:
  void check_point_group(const std::string &name, V3D hkl, size_t numEquiv, V3D *equiv) {
    PointGroup_sptr testedPointGroup = PointGroupFactory::Instance().createPointGroup(name);

    TSM_ASSERT(name + ": Does not fulfill group axioms!", testedPointGroup->isGroup());

    std::vector<V3D> equivalents = testedPointGroup->getEquivalents(hkl);
    // check that the number of equivalent reflections is as expected.
    TSM_ASSERT_EQUALS(name + ": Expected " + boost::lexical_cast<std::string>(numEquiv) + " equivalents, got " +
                          boost::lexical_cast<std::string>(equivalents.size()) + " instead.",
                      equivalents.size(), numEquiv);

    // get reflection family for this hkl
    V3D family = testedPointGroup->getReflectionFamily(hkl);

    for (size_t j = 0; j < numEquiv; j++) {
      // std::cout << j << '\n';
      if (!testedPointGroup->isEquivalent(hkl, equiv[j])) {
        TSM_ASSERT(name + " : " + hkl.toString() + " is not equivalent to " + equiv[j].toString(), false);
      }

      // make sure family for equiv[j] is the same as the one for hkl
      TS_ASSERT_EQUALS(testedPointGroup->getReflectionFamily(equiv[j]), family);
      // also make sure that current equivalent is in the collection of
      // equivalents.
      TS_ASSERT_DIFFERS(std::find(equivalents.begin(), equivalents.end(), equiv[j]), equivalents.end());
    }

    return;
  }

  void test_all_point_groups() {
    {
      V3D equiv[] = {V3D(1, 2, 3), V3D(-1, -2, -3)};
      check_point_group("-1", V3D(1, 2, 3), 2, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3), V3D(-1, -2, -3), V3D(-1, 2, -3), V3D(1, -2, 3)};
      check_point_group("2/m", V3D(1, 2, 3), 4, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3), V3D(-1, -2, 3), V3D(-1, -2, -3), V3D(1, 2, -3)};
      check_point_group("112/m", V3D(1, 2, 3), 4, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),    V3D(-1, -2, 3), V3D(-1, 2, -3), V3D(1, -2, -3),
                     V3D(-1, -2, -3), V3D(1, 2, -3),  V3D(1, -2, 3),  V3D(-1, 2, 3)};
      check_point_group("mmm", V3D(1, 2, 3), 8, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),    V3D(-1, -2, 3), V3D(-2, 1, 3),  V3D(2, -1, 3),
                     V3D(-1, -2, -3), V3D(1, 2, -3),  V3D(2, -1, -3), V3D(-2, 1, -3)};
      check_point_group("4/m", V3D(1, 2, 3), 8, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),  V3D(-1, -2, 3),  V3D(-2, 1, 3),   V3D(2, -1, 3), V3D(-1, 2, -3), V3D(1, -2, -3),
                     V3D(2, 1, -3), V3D(-2, -1, -3), V3D(-1, -2, -3), V3D(1, 2, -3), V3D(2, -1, -3), V3D(-2, 1, -3),
                     V3D(1, -2, 3), V3D(-1, 2, 3),   V3D(-2, -1, 3),  V3D(2, 1, 3)};
      check_point_group("4/mmm", V3D(1, 2, 3), 16, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3), V3D(2, -3, 3), V3D(-3, 1, 3), V3D(-1, -2, -3), V3D(-2, 3, -3), V3D(3, -1, -3)};
      check_point_group("-3", V3D(1, 2, 3), 6, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),    V3D(2, -3, 3),  V3D(-3, 1, 3),  V3D(2, 1, -3),  V3D(1, -3, -3), V3D(-3, 2, -3),
                     V3D(-1, -2, -3), V3D(-2, 3, -3), V3D(3, -1, -3), V3D(-2, -1, 3), V3D(-1, 3, 3),  V3D(3, -2, 3)};
      check_point_group("-3m1", V3D(1, 2, 3), 12, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),    V3D(2, -3, 3),  V3D(-3, 1, 3),  V3D(-2, -1, -3), V3D(-1, 3, -3), V3D(3, -2, -3),
          V3D(-1, -2, -3), V3D(-2, 3, -3), V3D(3, -1, -3), V3D(2, 1, 3),    V3D(1, -3, 3),  V3D(-3, 2, 3),
      };
      check_point_group("-31m", V3D(1, 2, 3), 12, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),    V3D(2, -3, 3),  V3D(-3, 1, 3),  V3D(-1, -2, 3), V3D(-2, 3, 3),  V3D(3, -1, 3),
                     V3D(-1, -2, -3), V3D(-2, 3, -3), V3D(3, -1, -3), V3D(1, 2, -3),  V3D(2, -3, -3), V3D(-3, 1, -3)};
      check_point_group("6/m", V3D(1, 2, 3), 12, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),    V3D(2, -3, 3),  V3D(-3, 1, 3),  V3D(-1, -2, 3),  V3D(-2, 3, 3),  V3D(3, -1, 3),
                     V3D(2, 1, -3),   V3D(1, -3, -3), V3D(-3, 2, -3), V3D(-2, -1, -3), V3D(-1, 3, -3), V3D(3, -2, -3),
                     V3D(-1, -2, -3), V3D(-2, 3, -3), V3D(3, -1, -3), V3D(1, 2, -3),   V3D(2, -3, -3), V3D(-3, 1, -3),
                     V3D(-2, -1, 3),  V3D(-1, 3, 3),  V3D(3, -2, 3),  V3D(2, 1, 3),    V3D(1, -3, 3),  V3D(-3, 2, 3)};
      check_point_group("6/mmm", V3D(1, 2, 3), 24, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3),    V3D(-1, -2, 3), V3D(-1, 2, -3),  V3D(1, -2, -3), V3D(3, 1, 2),    V3D(3, -1, -2),
                     V3D(-3, -1, 2),  V3D(-3, 1, -2), V3D(2, 3, 1),    V3D(-2, 3, -1), V3D(2, -3, -1),  V3D(-2, -3, 1),
                     V3D(-1, -2, -3), V3D(1, 2, -3),  V3D(1, -2, 3),   V3D(-1, 2, 3),  V3D(-3, -1, -2), V3D(-3, 1, 2),
                     V3D(3, 1, -2),   V3D(3, -1, 2),  V3D(-2, -3, -1), V3D(2, -3, 1),  V3D(-2, 3, 1),   V3D(2, 3, -1)};
      check_point_group("m-3", V3D(1, 2, 3), 24, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),    V3D(-1, -2, 3),  V3D(-1, 2, -3),  V3D(1, -2, -3), V3D(3, 1, 2),    V3D(3, -1, -2),
          V3D(-3, -1, 2),  V3D(-3, 1, -2),  V3D(2, 3, 1),    V3D(-2, 3, -1), V3D(2, -3, -1),  V3D(-2, -3, 1),
          V3D(2, 1, -3),   V3D(-2, -1, -3), V3D(2, -1, 3),   V3D(-2, 1, 3),  V3D(1, 3, -2),   V3D(-1, 3, 2),
          V3D(-1, -3, -2), V3D(1, -3, 2),   V3D(3, 2, -1),   V3D(3, -2, 1),  V3D(-3, 2, 1),   V3D(-3, -2, -1),
          V3D(-1, -2, -3), V3D(1, 2, -3),   V3D(1, -2, 3),   V3D(-1, 2, 3),  V3D(-3, -1, -2), V3D(-3, 1, 2),
          V3D(3, 1, -2),   V3D(3, -1, 2),   V3D(-2, -3, -1), V3D(2, -3, 1),  V3D(-2, 3, 1),   V3D(2, 3, -1),
          V3D(-2, -1, 3),  V3D(2, 1, 3),    V3D(-2, 1, -3),  V3D(2, -1, -3), V3D(-1, -3, 2),  V3D(1, -3, -2),
          V3D(1, 3, 2),    V3D(-1, 3, -2),  V3D(-3, -2, 1),  V3D(-3, 2, -1), V3D(3, -2, -1),  V3D(3, 2, 1)};
      check_point_group("m-3m", V3D(1, 2, 3), 48, equiv);
    }

    {
      V3D equiv[] = {V3D(1, 2, 3)};
      check_point_group("1", V3D(1, 2, 3), 1, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3), V3D(-1, 2, -3)};
      check_point_group("2", V3D(1, 2, 3), 2, equiv);
    }
    {
      V3D equiv[] = {V3D(1, 2, 3), V3D(1, -2, 3)};
      check_point_group("m", V3D(1, 2, 3), 2, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),
          V3D(-1, -2, 3),
          V3D(-1, 2, -3),
          V3D(1, -2, -3),
      };
      check_point_group("222", V3D(1, 2, 3), 4, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),
          V3D(-1, -2, 3),
          V3D(1, -2, 3),
          V3D(-1, 2, 3),
      };
      check_point_group("mm2", V3D(1, 2, 3), 4, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),
          V3D(-1, -2, 3),
          V3D(-2, 1, 3),
          V3D(2, -1, 3),
      };
      check_point_group("4", V3D(1, 2, 3), 4, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),
          V3D(-1, -2, 3),
          V3D(2, -1, -3),
          V3D(-2, 1, -3),
      };
      check_point_group("-4", V3D(1, 2, 3), 4, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),   V3D(-1, -2, 3), V3D(-2, 1, 3), V3D(2, -1, 3),
          V3D(-1, 2, -3), V3D(1, -2, -3), V3D(2, 1, -3), V3D(-2, -1, -3),
      };
      check_point_group("422", V3D(1, 2, 3), 8, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),  V3D(-1, -2, 3), V3D(-2, 1, 3),  V3D(2, -1, 3),
          V3D(1, -2, 3), V3D(-1, 2, 3),  V3D(-2, -1, 3), V3D(2, 1, 3),
      };
      check_point_group("4mm", V3D(1, 2, 3), 8, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),  V3D(-1, -2, 3), V3D(-2, 1, 3),  V3D(2, -1, 3),
          V3D(1, -2, 3), V3D(-1, 2, 3),  V3D(-2, -1, 3), V3D(2, 1, 3),
      };
      check_point_group("4mm", V3D(1, 2, 3), 8, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),   V3D(-1, -2, 3), V3D(2, -1, -3), V3D(-2, 1, -3),
          V3D(-1, 2, -3), V3D(1, -2, -3), V3D(-2, -1, 3), V3D(2, 1, 3),
      };
      check_point_group("-42m", V3D(1, 2, 3), 8, equiv);
    }
    {
      V3D equiv[] = {
          V3D(1, 2, 3),  V3D(-1, -2, 3), V3D(2, -1, -3), V3D(-2, 1, -3),
          V3D(1, -2, 3), V3D(-1, 2, 3),  V3D(2, 1, -3),  V3D(-2, -1, -3),
      };
      check_point_group("-4m2", V3D(1, 2, 3), 8, equiv);
    }
  }

  void testCrystalSystems() {
    std::map<std::string, PointGroup::CrystalSystem> crystalSystemsMap;
    crystalSystemsMap["1"] = PointGroup::CrystalSystem::Triclinic;
    crystalSystemsMap["-1"] = PointGroup::CrystalSystem::Triclinic;

    crystalSystemsMap["2"] = PointGroup::CrystalSystem::Monoclinic;
    crystalSystemsMap["112"] = PointGroup::CrystalSystem::Monoclinic;
    crystalSystemsMap["m"] = PointGroup::CrystalSystem::Monoclinic;
    crystalSystemsMap["11m"] = PointGroup::CrystalSystem::Monoclinic;
    crystalSystemsMap["2/m"] = PointGroup::CrystalSystem::Monoclinic;
    crystalSystemsMap["112/m"] = PointGroup::CrystalSystem::Monoclinic;

    crystalSystemsMap["222"] = PointGroup::CrystalSystem::Orthorhombic;
    crystalSystemsMap["mm2"] = PointGroup::CrystalSystem::Orthorhombic;
    crystalSystemsMap["m2m"] = PointGroup::CrystalSystem::Orthorhombic;
    crystalSystemsMap["2mm"] = PointGroup::CrystalSystem::Orthorhombic;
    crystalSystemsMap["mmm"] = PointGroup::CrystalSystem::Orthorhombic;

    crystalSystemsMap["4"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["-4"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["4/m"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["422"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["4mm"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["-42m"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["-4m2"] = PointGroup::CrystalSystem::Tetragonal;
    crystalSystemsMap["4/mmm"] = PointGroup::CrystalSystem::Tetragonal;

    crystalSystemsMap["3"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["-3"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["321"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["32"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["312"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["3m1"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["3m"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["31m"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["-3m1"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["-3m"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["-31m"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["3 r"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["-3 r"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["32 r"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["3m r"] = PointGroup::CrystalSystem::Trigonal;
    crystalSystemsMap["-3m r"] = PointGroup::CrystalSystem::Trigonal;

    crystalSystemsMap["6"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["-6"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["6/m"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["622"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["6mm"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["-62m"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["-6m2"] = PointGroup::CrystalSystem::Hexagonal;
    crystalSystemsMap["6/mmm"] = PointGroup::CrystalSystem::Hexagonal;

    crystalSystemsMap["23"] = PointGroup::CrystalSystem::Cubic;
    crystalSystemsMap["m-3"] = PointGroup::CrystalSystem::Cubic;
    crystalSystemsMap["432"] = PointGroup::CrystalSystem::Cubic;
    crystalSystemsMap["-43m"] = PointGroup::CrystalSystem::Cubic;
    crystalSystemsMap["m-3m"] = PointGroup::CrystalSystem::Cubic;

    std::vector<PointGroup_sptr> pointgroups = getAllPointGroups();

    for (auto &pointgroup : pointgroups) {
      TSM_ASSERT_EQUALS(pointgroup->getSymbol() + ": Unexpected crystal system.", pointgroup->crystalSystem(),
                        crystalSystemsMap[pointgroup->getSymbol()]);
    }
  }

  void testCrystalSystemMap() {
    std::vector<PointGroup_sptr> pointgroups = getAllPointGroups();
    PointGroupCrystalSystemMap pgMap = getPointGroupsByCrystalSystem();

    TS_ASSERT_EQUALS(pointgroups.size(), pgMap.size());

    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Triclinic), 2);

    // 3 * 2 (for unique b- and c-axis
    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Monoclinic), 3 * 2);

    // mmm and 222 + (2mm, m2m, mm2)
    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Orthorhombic), 5);
    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Tetragonal), 8);

    // 5 with rhombohedral axes and 8 with hexagonal and 3 for defaults
    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Trigonal), 5 + 8 + 3);
    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Hexagonal), 8);
    TS_ASSERT_EQUALS(pgMap.count(PointGroup::CrystalSystem::Cubic), 5);
  }

  void testPerformance() {
    PointGroup_sptr pg = PointGroupFactory::Instance().createPointGroup("m-3m");
    checkPointGroupPerformance(pg);
  }

  void testCrystalSystemNames() {
    TS_ASSERT_EQUALS(getCrystalSystemFromString("Cubic"), PointGroup::CrystalSystem::Cubic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("cubic"), PointGroup::CrystalSystem::Cubic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("CUBIC"), PointGroup::CrystalSystem::Cubic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("CuBiC"), PointGroup::CrystalSystem::Cubic);

    TS_ASSERT_EQUALS(getCrystalSystemFromString("Tetragonal"), PointGroup::CrystalSystem::Tetragonal);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("Hexagonal"), PointGroup::CrystalSystem::Hexagonal);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("Trigonal"), PointGroup::CrystalSystem::Trigonal);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("Orthorhombic"), PointGroup::CrystalSystem::Orthorhombic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("Monoclinic"), PointGroup::CrystalSystem::Monoclinic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString("Triclinic"), PointGroup::CrystalSystem::Triclinic);

    TS_ASSERT_THROWS(getCrystalSystemFromString("DoesNotExist"), const std::invalid_argument &);

    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Cubic)),
                     PointGroup::CrystalSystem::Cubic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Tetragonal)),
                     PointGroup::CrystalSystem::Tetragonal);
    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Hexagonal)),
                     PointGroup::CrystalSystem::Hexagonal);
    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Trigonal)),
                     PointGroup::CrystalSystem::Trigonal);
    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Orthorhombic)),
                     PointGroup::CrystalSystem::Orthorhombic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Monoclinic)),
                     PointGroup::CrystalSystem::Monoclinic);
    TS_ASSERT_EQUALS(getCrystalSystemFromString(getCrystalSystemAsString(PointGroup::CrystalSystem::Triclinic)),
                     PointGroup::CrystalSystem::Triclinic);
  }

  void testLatticeSystemNames() {
    TS_ASSERT_EQUALS(getLatticeSystemFromString("Cubic"), PointGroup::LatticeSystem::Cubic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("cubic"), PointGroup::LatticeSystem::Cubic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("CUBIC"), PointGroup::LatticeSystem::Cubic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("CuBiC"), PointGroup::LatticeSystem::Cubic);

    TS_ASSERT_EQUALS(getLatticeSystemFromString("Tetragonal"), PointGroup::LatticeSystem::Tetragonal);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("Hexagonal"), PointGroup::LatticeSystem::Hexagonal);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("Rhombohedral"), PointGroup::LatticeSystem::Rhombohedral);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("Orthorhombic"), PointGroup::LatticeSystem::Orthorhombic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("Monoclinic"), PointGroup::LatticeSystem::Monoclinic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString("Triclinic"), PointGroup::LatticeSystem::Triclinic);

    TS_ASSERT_THROWS(getLatticeSystemFromString("DoesNotExist"), const std::invalid_argument &);

    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Cubic)),
                     PointGroup::LatticeSystem::Cubic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Tetragonal)),
                     PointGroup::LatticeSystem::Tetragonal);
    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Hexagonal)),
                     PointGroup::LatticeSystem::Hexagonal);
    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Rhombohedral)),
                     PointGroup::LatticeSystem::Rhombohedral);
    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Orthorhombic)),
                     PointGroup::LatticeSystem::Orthorhombic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Monoclinic)),
                     PointGroup::LatticeSystem::Monoclinic);
    TS_ASSERT_EQUALS(getLatticeSystemFromString(getLatticeSystemAsString(PointGroup::LatticeSystem::Triclinic)),
                     PointGroup::LatticeSystem::Triclinic);
  }

private:
  void checkPointGroupPerformance(const PointGroup_sptr &pointGroup) {
    V3D equiv[] = {V3D(1, 2, 3),    V3D(-1, -2, 3),  V3D(-1, 2, -3),  V3D(1, -2, -3), V3D(3, 1, 2),    V3D(3, -1, -2),
                   V3D(-3, -1, 2),  V3D(-3, 1, -2),  V3D(2, 3, 1),    V3D(-2, 3, -1), V3D(2, -3, -1),  V3D(-2, -3, 1),
                   V3D(2, 1, -3),   V3D(-2, -1, -3), V3D(2, -1, 3),   V3D(-2, 1, 3),  V3D(1, 3, -2),   V3D(-1, 3, 2),
                   V3D(-1, -3, -2), V3D(1, -3, 2),   V3D(3, 2, -1),   V3D(3, -2, 1),  V3D(-3, 2, 1),   V3D(-3, -2, -1),
                   V3D(-1, -2, -3), V3D(1, 2, -3),   V3D(1, -2, 3),   V3D(-1, 2, 3),  V3D(-3, -1, -2), V3D(-3, 1, 2),
                   V3D(3, 1, -2),   V3D(3, -1, 2),   V3D(-2, -3, -1), V3D(2, -3, 1),  V3D(-2, 3, 1),   V3D(2, 3, -1),
                   V3D(-2, -1, 3),  V3D(2, 1, 3),    V3D(-2, 1, -3),  V3D(2, -1, -3), V3D(-1, -3, 2),  V3D(1, -3, -2),
                   V3D(1, 3, 2),    V3D(-1, 3, -2),  V3D(-3, -2, 1),  V3D(-3, 2, -1), V3D(3, -2, -1),  V3D(3, 2, 1)};
    std::vector<V3D> hkls(equiv, equiv + 48);

    Timer t;

    V3D base(1, 2, 3);

    t.reset();
    int h = 0;
    for (size_t i = 0; i < 1000; ++i) {
      for (auto &hkl : hkls) {
        bool eq = pointGroup->isEquivalent(base, hkl);
        if (eq) {
          ++h;
        }
      }
    }

    float time = t.elapsed();

    std::cout << "Eq: " << h << ", Time: " << time / 1000.0 << '\n';
  }
};
