// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Matrix.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class ScalarUtilsTest : public CxxTest::TestSuite {
public:
  static Matrix<double> getSiliconNiggliUB() // cubic F
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.147196, -0.141218, 0.304286);
    V3D row_1(0.106643, 0.120339, 0.090515);
    V3D row_2(-0.261275, 0.258430, -0.006186);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getQuartzNiggliUB() // hexagonal P
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(0.122709, 0.006640, 0.144541);
    V3D row_1(0.161964, -0.003276, -0.115259);
    V3D row_2(-0.117973, 0.233336, -0.005870);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getFeSiNiggliUB() // cubic P
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(0.220642, 0.021551, 0.019386);
    V3D row_1(-0.014454, -0.045777, 0.216631);
    V3D row_2(0.024937, -0.216371, -0.044267);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getSapphireNiggliUB() // rhobohedral R
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.189735, 0.175239, 0.101705);
    V3D row_1(0.151181, -0.026369, 0.103045);
    V3D row_2(0.075451, 0.182128, -0.180543);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getBaFeAsNiggliUB() // tetragonal I
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.111463, -0.108301, -0.150253);
    V3D row_1(0.159667, 0.159664, -0.029615);
    V3D row_2(0.176442, -0.178150, -0.001806);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getNatroliteNiggliUB() // orthorhombic F
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.101392, 0.099102, -0.015748);
    V3D row_1(0.127044, 0.015149, -0.083820);
    V3D row_2(-0.050598, -0.043361, -0.064672);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getOxalicAcidNiggliUB() // monoclinic P
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.275165, -0.002206, -0.001983);
    V3D row_1(-0.007265, 0.163243, 0.002560);
    V3D row_2(0.006858, 0.043325, -0.086000);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getTopaz7424UB() // monoclinic C, TOPAZ 7424
  {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.04245285, -0.13219620, 0.23046029);
    V3D row_1(-0.00363704, -0.17688150, -0.02742737);
    V3D row_2(0.20357264, -0.03033733, -0.02246710);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static Matrix<double> getTestNiggliUB() // triclinic P
  {                                       // artifical example
    Matrix<double> UB(3, 3, false);
    V3D row_0(1, .2, 3);
    V3D row_1(4, 5, 0);
    V3D row_2(7, 8, 0);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  void test_GetCells_of_all_types_and_centerings() {
    std::vector<ConventionalCell> list;

    Matrix<double> UB = getSiliconNiggliUB(); // cubic case
    list = ScalarUtils::GetCells(UB, false);  // all types and centerings

    TS_ASSERT_EQUALS(list.size(), 44); // should be 1 for each form
                                       // need to set error limit to
                                       // select reasonable forms

    list = ScalarUtils::GetCells(UB, true); // all types and centerings but
                                            // only one per Bravais lattic

    TS_ASSERT_EQUALS(list.size(), 15); // should be 1 for each Bravais
                                       // lattice plus triclinic
  }

  void test_RemoveHighErrorForms() {
    size_t good_forms[] = {1, 2, 9, 19, 10, 20, 27, 31};
    std::vector<ConventionalCell> list;

    Matrix<double> UB = getSiliconNiggliUB(); // cubic case
    list = ScalarUtils::GetCells(UB, false);  // all types and centerings

    TS_ASSERT_EQUALS(list.size(), 44); // should be 1 for each form
                                       // need to set error limit to
                                       // select reasonable forms

    ScalarUtils::RemoveHighErrorForms(list, 0.2);
    TS_ASSERT_EQUALS(list.size(), 8);

    for (size_t i = 0; i < 8; i++) {
      TS_ASSERT_EQUALS(list[i].GetFormNum(), good_forms[i]);
    }
  }

  void test_GetCellForForm() {
    size_t good_forms[] = {1, 2, 9, 19, 10, 20, 27, 31};
    double errors[] = {0.0074, 0.0129, 0.0042328, 0.0050198, 0.0057, 0.0053090, 0.0050, 0.0000000};

    Matrix<double> UB = getSiliconNiggliUB(); // cubic case
    for (size_t i = 0; i < 8; i++) {
      ConventionalCell info = ScalarUtils::GetCellForForm(UB, good_forms[i]);
      TS_ASSERT_DELTA(info.GetError(), errors[i], 1e-4);
    }
  }

  void test_GetCellBestError() {
    std::vector<ConventionalCell> list;

    Matrix<double> UB = getSapphireNiggliUB(); // rhombohedral case
    list = ScalarUtils::GetCells(UB, ReducedCell::RHOMBOHEDRAL(), ReducedCell::R_CENTERED());

    ConventionalCell info = ScalarUtils::GetCellBestError(list, true);
    TS_ASSERT_EQUALS(info.GetFormNum(), 9);

    info = ScalarUtils::GetCellBestError(list, false);
    TS_ASSERT_EQUALS(info.GetFormNum(), 9);

    list = ScalarUtils::GetCells(UB, false);

    info = ScalarUtils::GetCellBestError(list, true);
    TS_ASSERT_EQUALS(info.GetFormNum(), 31);

    info = ScalarUtils::GetCellBestError(list, false);
    TS_ASSERT_EQUALS(info.GetFormNum(), 10);
  }

  void test_GetCells_given_type_and_centering() {
    std::vector<ConventionalCell> list;

    Matrix<double> UB = getSiliconNiggliUB(); // cubic case
    list = ScalarUtils::GetCells(UB, ReducedCell::CUBIC(), ReducedCell::F_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 1);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 1);

    UB = getQuartzNiggliUB(); // hexagonal case
    list = ScalarUtils::GetCells(UB, ReducedCell::HEXAGONAL(), ReducedCell::P_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 2);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 12);
    TS_ASSERT_EQUALS(list[1].GetFormNum(), 22);

    UB = getSapphireNiggliUB(); // rhombohedral case
    list = ScalarUtils::GetCells(UB, ReducedCell::RHOMBOHEDRAL(), ReducedCell::R_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 4);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 2);
    TS_ASSERT_EQUALS(list[1].GetFormNum(), 4);
    TS_ASSERT_EQUALS(list[2].GetFormNum(), 9);
    TS_ASSERT_EQUALS(list[3].GetFormNum(), 24);

    UB = getBaFeAsNiggliUB(); // tetragonal case
    list = ScalarUtils::GetCells(UB, ReducedCell::TETRAGONAL(), ReducedCell::I_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 4);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 6);
    TS_ASSERT_EQUALS(list[1].GetFormNum(), 7);
    TS_ASSERT_EQUALS(list[2].GetFormNum(), 15);
    TS_ASSERT_EQUALS(list[3].GetFormNum(), 18);

    UB = getNatroliteNiggliUB(); // orthorhombic case
    list = ScalarUtils::GetCells(UB, ReducedCell::ORTHORHOMBIC(), ReducedCell::F_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 2);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 16);
    TS_ASSERT_EQUALS(list[1].GetFormNum(), 26);

    UB = getOxalicAcidNiggliUB(); // monoclinic case
    list = ScalarUtils::GetCells(UB, ReducedCell::MONOCLINIC(), ReducedCell::P_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 3);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 33);
    TS_ASSERT_EQUALS(list[1].GetFormNum(), 34);
    TS_ASSERT_EQUALS(list[2].GetFormNum(), 35);

    UB = getTestNiggliUB(); // triclinic case
    list = ScalarUtils::GetCells(UB, ReducedCell::TRICLINIC(), ReducedCell::P_CENTERED());
    TS_ASSERT_EQUALS(list.size(), 2);
    TS_ASSERT_EQUALS(list[0].GetFormNum(), 31);
    TS_ASSERT_EQUALS(list[1].GetFormNum(), 44);
  }

  void test_GetRelatedUBs() {

    double TOPAZ_7424_mats[6][9] = {
        {-0.042452, -0.132196, 0.230460, -0.003637, -0.176881, -0.027427, 0.203573, -0.030337, -0.022467},
        {0.042452, 0.230460, -0.132196, 0.003637, -0.027427, -0.176881, -0.203573, -0.022467, -0.030337},
        {-0.132196, 0.230460, -0.042452, -0.176881, -0.027427, -0.003637, -0.030337, -0.022467, 0.203573},
        {0.132196, -0.042452, 0.230460, 0.176881, -0.003637, -0.027427, 0.030337, 0.203573, -0.022467},
        {0.230460, -0.042452, -0.132196, -0.027427, -0.003637, -0.176881, -0.022467, 0.203573, -0.030337},
        {-0.230460, -0.132196, -0.042452, 0.027427, -0.176881, -0.003637, 0.022467, -0.030337, 0.203573}};

    double tolerance = 3; // tolerance on angles, in degrees
    double factor = 1.05; // tolerance factor for equal sides

    // Artificial triclinic case. No sides equal, no angles
    // near 90 degrees, so only one cell
    Matrix<double> Triclinic_UB = getTestNiggliUB();
    std::vector<Matrix<double>> Triclinic_list;
    Triclinic_list = ScalarUtils::GetRelatedUBs(Triclinic_UB, factor, tolerance);
    TS_ASSERT_EQUALS(Triclinic_list.size(), 1);

    // Silicon UB.  Three angles 60 degrees, none near 90.  Three equal
    // sides so only six permutations are possible, no reflections.
    Matrix<double> silicon_UB = getSiliconNiggliUB();
    std::vector<Matrix<double>> silicon_list;
    silicon_list = ScalarUtils::GetRelatedUBs(silicon_UB, factor, tolerance);
    TS_ASSERT_EQUALS(silicon_list.size(), 6);

    // Quartz UB. Two 90 degree angles and a=b<c.  Thus two extra
    // reflections are possible and for each of those two orderings
    // are possible, so six related cells in all.
    Matrix<double> quartz_UB = getQuartzNiggliUB();
    std::vector<Matrix<double>> quartz_list;
    quartz_list = ScalarUtils::GetRelatedUBs(quartz_UB, factor, tolerance);
    TS_ASSERT_EQUALS(quartz_list.size(), 6);

    // FeSi UB. Cubic P, so 24 related cells counting three extra
    // reflections of pairs and 6 permutations of sides for
    // each combination of reflections.
    Matrix<double> FeSi_UB = getFeSiNiggliUB();
    std::vector<Matrix<double>> FeSi_list;
    FeSi_list = ScalarUtils::GetRelatedUBs(FeSi_UB, factor, tolerance);
    TS_ASSERT_EQUALS(FeSi_list.size(), 24);

    // No angles near 90 degrees, but three equal sides so only six
    // permutations for related UBs.
    Matrix<double> TOPAZ_7424_UB = getTopaz7424UB();
    std::vector<Matrix<double>> TOPAZ_7424_list;
    TOPAZ_7424_list = ScalarUtils::GetRelatedUBs(TOPAZ_7424_UB, factor, tolerance);
    TS_ASSERT_EQUALS(TOPAZ_7424_list.size(), 6);

    // Check all of the related UBs that are returned in this case
    for (size_t i = 0; i < TOPAZ_7424_list.size(); i++) {
      for (size_t j = 0; j < 9; j++) {
        std::vector<double> entry_list = TOPAZ_7424_list[i].getVector();
        TS_ASSERT_DELTA(entry_list[j], TOPAZ_7424_mats[i][j], 1.0e-4);
      }
    }
  }
};
