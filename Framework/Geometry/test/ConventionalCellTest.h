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

using namespace Mantid::Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class ConventionalCellTest : public CxxTest::TestSuite {
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

  void test_CubicCase() {
    Matrix<double> correctNewUB(3, 3, false);
    V3D row_0(0.078545, -0.1442070, 0.081534);
    V3D row_1(0.098579, 0.1134910, 0.105427);
    V3D row_2(-0.133731, -0.0014225, 0.126120);
    correctNewUB.setRow(0, row_0);
    correctNewUB.setRow(1, row_1);
    correctNewUB.setRow(2, row_2);

    Matrix<double> niggliUB = getSiliconNiggliUB();

    ConventionalCell conv_cell(niggliUB, 1);

    Matrix<double> oldUB = conv_cell.GetOriginalUB();
    Matrix<double> newUB = conv_cell.GetNewUB();

    TS_ASSERT_EQUALS(conv_cell.GetFormNum(), 1);
    TS_ASSERT_DELTA(conv_cell.GetError(), 0.00742998, 1e-4);
    TS_ASSERT_EQUALS(conv_cell.GetCellType(), "Cubic");
    TS_ASSERT_EQUALS(conv_cell.GetCentering(), "F");

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(oldUB[row][col], niggliUB[row][col], 1e-10);

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], correctNewUB[row][col], 1e-5);

    TS_ASSERT_DELTA(conv_cell.GetSumOfSides(), 16.3406, 1e-3);
  }

  void test_HexagonalCase() {
    Matrix<double> correctNewUB(3, 3, false);
    V3D row_0(0.122709, 0.006640, 0.144541);
    V3D row_1(0.161964, -0.003276, -0.115259);
    V3D row_2(-0.117973, 0.233336, -0.005870);
    correctNewUB.setRow(0, row_0);
    correctNewUB.setRow(1, row_1);
    correctNewUB.setRow(2, row_2);

    Matrix<double> niggliUB = getQuartzNiggliUB();

    ConventionalCell conv_cell(niggliUB, 12);

    Matrix<double> oldUB = conv_cell.GetOriginalUB();
    Matrix<double> newUB = conv_cell.GetNewUB();

    // NOTE: The error is large. To get a good
    //       match the sides need to be reflected.
    TS_ASSERT_EQUALS(conv_cell.GetFormNum(), 12);
    TS_ASSERT_DELTA(conv_cell.GetError(), 3.55747, 1e-4);
    TS_ASSERT_EQUALS(conv_cell.GetCellType(), "Hexagonal");
    TS_ASSERT_EQUALS(conv_cell.GetCentering(), "P");

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(oldUB[row][col], niggliUB[row][col], 1e-10);

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], correctNewUB[row][col], 1e-5);

    TS_ASSERT_DELTA(conv_cell.GetSumOfSides(), 15.2455, 1e-3);
  }

  void test_RhombohedralCase() {
    Matrix<double> correctNewUB(3, 3, false);
    V3D row_0(0.053308, 0.209141, 0.033902);
    V3D row_1(0.193509, 0.007980, 0.034348);
    V3D row_2(0.137216, 0.121947, -0.060181);
    correctNewUB.setRow(0, row_0);
    correctNewUB.setRow(1, row_1);
    correctNewUB.setRow(2, row_2);

    Matrix<double> niggliUB = getSapphireNiggliUB();

    ConventionalCell conv_cell(niggliUB, 9);

    Matrix<double> oldUB = conv_cell.GetOriginalUB();
    Matrix<double> newUB = conv_cell.GetNewUB();

    TS_ASSERT_EQUALS(conv_cell.GetFormNum(), 9);
    TS_ASSERT_DELTA(conv_cell.GetError(), 0.0474606, 1e-4);
    TS_ASSERT_EQUALS(conv_cell.GetCellType(), "Rhombohedral");
    TS_ASSERT_EQUALS(conv_cell.GetCentering(), "R");

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(oldUB[row][col], niggliUB[row][col], 1e-10);

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], correctNewUB[row][col], 1e-5);

    TS_ASSERT_DELTA(conv_cell.GetSumOfSides(), 22.4781, 1e-3);
  }

  void test_TetragonalCase() {
    Matrix<double> correctNewUB(3, 3, false);
    V3D row_0(-0.036337, -0.033175, -0.075126);
    V3D row_1(0.174474, 0.174471, -0.014808);
    V3D row_2(0.177345, -0.177247, -0.000903);
    correctNewUB.setRow(0, row_0);
    correctNewUB.setRow(1, row_1);
    correctNewUB.setRow(2, row_2);

    Matrix<double> niggliUB = getBaFeAsNiggliUB();

    ConventionalCell conv_cell(niggliUB, 15);

    Matrix<double> oldUB = conv_cell.GetOriginalUB();
    Matrix<double> newUB = conv_cell.GetNewUB();

    TS_ASSERT_EQUALS(conv_cell.GetFormNum(), 15);
    TS_ASSERT_DELTA(conv_cell.GetError(), 0.0152066, 1e-4);
    TS_ASSERT_EQUALS(conv_cell.GetCellType(), "Tetragonal");
    TS_ASSERT_EQUALS(conv_cell.GetCentering(), "I");

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(oldUB[row][col], niggliUB[row][col], 1e-10);

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], correctNewUB[row][col], 1e-5);

    TS_ASSERT_DELTA(conv_cell.GetSumOfSides(), 21.0217, 1e-3);
  }

  void test_OrthorhombicCase() {
    Matrix<double> correctNewUB(3, 3, false);
    V3D row_0(-0.059715, 0.049551, -0.007874);
    V3D row_1(0.092708, 0.007574, -0.041910);
    V3D row_2(-0.104615, -0.021681, -0.032336);
    correctNewUB.setRow(0, row_0);
    correctNewUB.setRow(1, row_1);
    correctNewUB.setRow(2, row_2);

    Matrix<double> niggliUB = getNatroliteNiggliUB();

    ConventionalCell conv_cell(niggliUB, 26);

    Matrix<double> oldUB = conv_cell.GetOriginalUB();
    Matrix<double> newUB = conv_cell.GetNewUB();

    TS_ASSERT_EQUALS(conv_cell.GetFormNum(), 26);
    TS_ASSERT_DELTA(conv_cell.GetError(), 0.0246748, 1e-4);
    TS_ASSERT_EQUALS(conv_cell.GetCellType(), "Orthorhombic");
    TS_ASSERT_EQUALS(conv_cell.GetCentering(), "F");

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(oldUB[row][col], niggliUB[row][col], 1e-10);

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], correctNewUB[row][col], 1e-5);

    TS_ASSERT_DELTA(conv_cell.GetSumOfSides(), 43.575, 1e-3);
  }

  void test_MonoclinicCase() {
    Matrix<double> correctNewUB(3, 3, false);
    V3D row_0(0.002206, 0.275165, 0.001983);
    V3D row_1(-0.163243, 0.007265, -0.002560);
    V3D row_2(-0.043325, -0.006858, 0.086000);
    correctNewUB.setRow(0, row_0);
    correctNewUB.setRow(1, row_1);
    correctNewUB.setRow(2, row_2);

    Matrix<double> niggliUB = getOxalicAcidNiggliUB();

    ConventionalCell conv_cell(niggliUB, 35);

    Matrix<double> oldUB = conv_cell.GetOriginalUB();
    Matrix<double> newUB = conv_cell.GetNewUB();

    // NOTE: The error is large. To get a good
    //       match the sides need to be reflected.
    TS_ASSERT_EQUALS(conv_cell.GetFormNum(), 35);
    TS_ASSERT_DELTA(conv_cell.GetError(), 2.4730, 1e-4);
    TS_ASSERT_EQUALS(conv_cell.GetCellType(), "Monoclinic");
    TS_ASSERT_EQUALS(conv_cell.GetCentering(), "P");

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(oldUB[row][col], niggliUB[row][col], 1e-10);

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], correctNewUB[row][col], 1e-5);

    TS_ASSERT_DELTA(conv_cell.GetSumOfSides(), 21.6434, 1e-3);
  }
};
