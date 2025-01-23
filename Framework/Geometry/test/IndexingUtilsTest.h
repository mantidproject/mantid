// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include <utility>

using namespace Mantid::Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class IndexingUtilsTest : public CxxTest::TestSuite {
public:
  static std::vector<V3D> getNatroliteQs() {
    std::vector<V3D> q_vectors{
        {-0.57582, -0.35322, -0.19974}, {-1.41754, -0.78704, -0.75974}, {-1.12030, -0.53578, -0.27559},
        {-0.68911, -0.59397, -0.12716}, {-1.06863, -0.43255, 0.01688},  {-1.82007, -0.49671, -0.06266},
        {-1.10465, -0.73708, -0.01939}, {-0.12747, -0.32380, 0.00821},  {-0.84210, -0.37038, 0.15403},
        {-0.54099, -0.46900, 0.11535},  {-0.90478, -0.50667, 0.51072},  {-0.50387, -0.58561, 0.43502}};
    // Dec 2011: Change convention for Q = 2 pi / wavelength
    for (auto &q_vector : q_vectors)
      q_vector *= (2.0 * M_PI);
    return q_vectors;
  }

  static std::vector<V3D> getNatroliteIndices() {
    std::vector<V3D> correct_indices{{1, 9, -9},  {4, 20, -24}, {2, 18, -14}, {0, 12, -12},
                                     {1, 19, -9}, {3, 31, -13}, {0, 20, -14}, {-1, 3, -5},
                                     {0, 16, -6}, {-1, 11, -7}, {-2, 20, -4}, {-3, 13, -5}};
    return correct_indices;
  }

  static Matrix<double> getNatroliteUB() {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.059660400, -0.049648200, 0.0077539105);
    V3D row_1(0.093009956, -0.007510495, 0.0419835400);
    V3D row_2(-0.104643770, 0.021613428, 0.0322586300);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }

  static void ShowLatticeParameters(Matrix<double> UB) {
    Matrix<double> UB_inv(3, 3, false);
    UB_inv = std::move(UB);
    UB_inv.Invert();
    V3D a_dir(UB_inv[0][0], UB_inv[0][1], UB_inv[0][2]);
    V3D b_dir(UB_inv[1][0], UB_inv[1][1], UB_inv[1][2]);
    V3D c_dir(UB_inv[2][0], UB_inv[2][1], UB_inv[2][2]);
    double alpha = b_dir.angle(c_dir) * 180 / M_PI;
    double beta = c_dir.angle(a_dir) * 180 / M_PI;
    double gamma = a_dir.angle(b_dir) * 180 / M_PI;
    std::cout << "-------------------------------------------\n";
    std::cout << "a = " << a_dir << "   " << a_dir.norm() << '\n';
    std::cout << "b = " << b_dir << "   " << b_dir.norm() << '\n';
    std::cout << "c = " << c_dir << "   " << c_dir.norm() << '\n';
    std::cout << "alpha = " << alpha << '\n';
    std::cout << "beta  = " << beta << '\n';
    std::cout << "gamma = " << gamma << '\n';
    std::cout << "-------------------------------------------\n";
  }

  static void ShowIndexingStats(const Matrix<double> &UB, const std::vector<V3D> &q_vectors,
                                double required_tolerance) {
    std::vector<V3D> miller_indices;
    std::vector<V3D> indexed_qs;
    double ave_error;
    IndexingUtils::GetIndexedPeaks(UB, q_vectors, required_tolerance, miller_indices, indexed_qs, ave_error);

    std::cout << "-------------------------------------------\n";
    std::cout << "UB = " << UB << '\n';
    std::cout << "num indexed  = " << indexed_qs.size() << '\n';
    std::cout << "error = " << ave_error << '\n';
    std::cout << '\n';

    std::cout << "Indexed Qs\n";
    for (size_t i = 0; i < indexed_qs.size(); i++)
      std::cout << "Q = " << indexed_qs[i] << " HKL = " << miller_indices[i] << '\n';
    std::cout << "-------------------------------------------\n";
  }

  void test_Find_UB_given_lattice_parameters() {
    Matrix<double> UB(3, 3, false);

    double correct_UB[] = {-0.1015550, 0.0992964,  -0.0155078, 0.1274830, 0.0150210,
                           -0.0839671, -0.0507717, -0.0432269, -0.0645173};

    std::vector<V3D> q_vectors = getNatroliteQs();

    OrientedLattice lattice(6.6, 9.7, 9.9, 84, 71, 70);
    // test both default case(-1) and
    // case with specified base index(4)
    for (int base_index = -1; base_index < 5; base_index += 5) {
      double required_tolerance = 0.2;
      size_t num_initial = 3;
      double degrees_per_step = 3;

      double error =
          IndexingUtils::Find_UB(UB, q_vectors, lattice, required_tolerance, base_index, num_initial, degrees_per_step);

      //      std::cout << std::endl << "USING LATTICE PARAMETERS\n";
      //      ShowIndexingStats( UB, q_vectors, required_tolerance );
      //      ShowLatticeParameters( UB );

      TS_ASSERT_DELTA(error, 0.00671575, 1e-5);

      std::vector<double> UB_returned = UB.getVector();
      for (size_t i = 0; i < 9; i++) {
        TS_ASSERT_DELTA(UB_returned[i], correct_UB[i], 1e-5);
      }

      int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, required_tolerance);
      TS_ASSERT_EQUALS(num_indexed, 12);
    }
  }

  void test_Find_UB_given_d_min_d_max() {
    Matrix<double> UB(3, 3, false);

    double correct_UB[] = {-0.0177661, -0.0992964, 0.0155078, 0.0585369, -0.0150210,
                           0.0839671,  -0.1585160, 0.0432269, 0.0645173};

    std::vector<V3D> q_vectors = getNatroliteQs();

    double d_min = 6;
    double d_max = 10;
    double required_tolerance = 0.08;
    int num_initial = 8;
    double degrees_per_step = 1;
    // test both default case(-1) and
    // case with specified base index(4)
    for (int base_index = -1; base_index < 5; base_index += 5) {

      double error = IndexingUtils::Find_UB(UB, q_vectors, d_min, d_max, required_tolerance, base_index, num_initial,
                                            degrees_per_step);

      int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, required_tolerance);

      //      std::cout << std::endl << "USING MIN-MAX-D\n";
      //      ShowIndexingStats( UB, q_vectors, required_tolerance );
      //      ShowLatticeParameters( UB );

      TS_ASSERT_EQUALS(num_indexed, 12);

      TS_ASSERT_DELTA(error, 0.000111616, 1e-5);

      std::vector<double> UB_returned = UB.getVector();
      for (size_t i = 0; i < 9; i++) {
        TS_ASSERT_DELTA(UB_returned[i], correct_UB[i], 1e-5);
      }
    }
  }

  void test_Find_UB_using_FFT() {
    Matrix<double> UB(3, 3, false);

    double correct_UB[] = {-0.0177661, -0.0992964, 0.0155078, 0.0585369, -0.0150210,
                           0.0839671,  -0.1585160, 0.0432269, 0.0645173};

    std::vector<V3D> q_vectors = getNatroliteQs();

    double d_min = 6;
    double d_max = 10;
    double required_tolerance = 0.08;
    double degrees_per_step = 1;

    double error = IndexingUtils::Find_UB(UB, q_vectors, d_min, d_max, required_tolerance, degrees_per_step);

    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, required_tolerance);

    //  std::cout << std::endl << "USING FFT\n";
    //  ShowIndexingStats( UB, q_vectors, required_tolerance );
    //  ShowLatticeParameters( UB );

    TS_ASSERT_EQUALS(num_indexed, 12);

    TS_ASSERT_DELTA(error, 0.0102, 1e-3);

    std::vector<double> UB_returned = UB.getVector();
    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(UB_returned[i], correct_UB[i], 1e-5);
    }
  }

  void test_Optimize_UB_given_indexing() {
    std::vector<V3D> q_list = getNatroliteQs();
    std::vector<V3D> hkl_list = getNatroliteIndices();
    Matrix<double> correct_UB = getNatroliteUB();
    Matrix<double> UB(3, 3, false);

    double sum_sq_error = IndexingUtils::Optimize_UB(UB, hkl_list, q_list);

    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        TS_ASSERT_DELTA(UB[i][j], correct_UB[i][j], 1.e-5);

    TS_ASSERT_DELTA(sum_sq_error, 0.000111616, 1e-5);
  }

  void test_Optimize_Direction() {
    std::vector<int> index_values;
    int correct_indices[] = {1, 4, 2, 0, 1, 3, 0, -1, 0, -1, -2, -3};
    for (int correct_index : correct_indices) {
      index_values.emplace_back(correct_index);
    }

    std::vector<V3D> q_vectors = getNatroliteQs();

    V3D best_vec;
    double error = IndexingUtils::Optimize_Direction(best_vec, index_values, q_vectors);
    TS_ASSERT_DELTA(error, 0.00218606, 1e-5);
    TS_ASSERT_DELTA(best_vec[0], -2.58222, 1e-4);
    TS_ASSERT_DELTA(best_vec[1], 3.97345, 1e-4);
    TS_ASSERT_DELTA(best_vec[2], -4.55145, 1e-4);
  }

  void test_ScanFor_UB() {
    double correct_UB[] = {-0.102577,  0.0999725, -0.0136353, 0.123290,  0.0146148,
                           -0.0851386, -0.055154, -0.0427632, -0.0630785};

    Matrix<double> UB(3, 3, false);
    int degrees_per_step = 3;
    double required_tolerance = 0.2;

    UnitCell cell(6.6f, 9.7f, 9.9f, 84, 71, 70);
    std::vector<V3D> q_vectors = getNatroliteQs();

    double error = IndexingUtils::ScanFor_UB(UB, q_vectors, cell, degrees_per_step, required_tolerance);

    TS_ASSERT_DELTA(error, 0.147397, 1.e-5);

    std::vector<double> UB_returned = UB.getVector();
    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(UB_returned[i], correct_UB[i], 1e-5);
    }
  }

  void test_ScanFor_Directions() {
    double vectors[5][3] = {{0.08445961, 9.26951000, 3.4138980},
                            {-2.58222370, 3.97345330, -4.5514464},
                            {2.66668320, 5.29605670, 7.9653444},
                            {7.01297300, 3.23755380, -5.8988633},
                            {-9.59519700, 0.73589927, 1.3474168}};

    std::vector<V3D> directions;
    std::vector<V3D> q_vectors = getNatroliteQs();
    double d_min = 6;
    double d_max = 10;
    double degrees_per_step = 1.0;
    double required_tolerance = 0.12;

    IndexingUtils::ScanFor_Directions(directions, q_vectors, d_min, d_max, required_tolerance, degrees_per_step);

    TS_ASSERT_EQUALS(5, directions.size());

    for (size_t i = 0; i < 3; i++) {
      V3D vec = directions[i];
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(vectors[i][j], vec[j], 1.e-5);
      }
    }
  }

  void test_FFTScanFor_Directions() {
    double vectors[5][3] = {{-2.58222370, 3.97345330, -4.5514464},
                            {-9.59519700, 0.73589927, 1.3474168},
                            {7.01297300, 3.23755380, -5.8988633},
                            {0.08445961, 9.26951000, 3.4138980},
                            {2.66668320, 5.29605670, 7.9653444}};

    std::vector<V3D> directions;
    std::vector<V3D> q_vectors = getNatroliteQs();
    double d_min = 6;
    double d_max = 10;
    double degrees_per_step = 1.0;
    double required_tolerance = 0.12;

    IndexingUtils::FFTScanFor_Directions(directions, q_vectors, d_min, d_max, required_tolerance, degrees_per_step);

    TS_ASSERT_EQUALS(8, directions.size());

    for (size_t i = 0; i < 3; i++) {
      V3D vec = directions[i];
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(vectors[i][j], vec[j], 1.e-5);
      }
    }
  }

  void test_GetMagFFT() {
    constexpr size_t N_FFT_STEPS = 256;
    constexpr size_t HALF_FFT_STEPS = 128;

    double projections[N_FFT_STEPS];
    double magnitude_fft[HALF_FFT_STEPS];

    V3D current_dir(1, 2, -3);
    std::vector<V3D> q_vectors;
    current_dir.normalize();
    for (size_t i = 0; i < 16; i++) {
      V3D vec(current_dir);
      vec *= ((double)i + 0.6);
      vec *= 2.0 * M_PI;
      q_vectors.emplace_back(vec);
    }
    double max_q_magnitude = 16.0;

    double index_factor = ((double)N_FFT_STEPS) / max_q_magnitude;

    double max_mag_fft =
        IndexingUtils::GetMagFFT(q_vectors, current_dir, N_FFT_STEPS, projections, index_factor, magnitude_fft);

    TS_ASSERT_DELTA(max_mag_fft, 16.0, 1e-5);

    for (size_t i = 0; i < N_FFT_STEPS / 2; i++) {
      if (i % 16 == 0) {
        TS_ASSERT_DELTA(magnitude_fft[i], 16.0, 1e-5);
      } else {
        TS_ASSERT_DELTA(magnitude_fft[i], 0.0, 1e-5);
      }
    }
  }

  void test_FormUB_From_abc_Vectors_with_min_angle() {
    Matrix<double> UB(3, 3, false);
    double UB_array[] = {-0.0177703, -0.0993001, 0.0155008, 0.0585436, -0.0150158,
                         0.0839775,  -0.158519,  0.0432281, 0.0645189};

    double vectors[5][3] = {{-2.58222370, 3.97345330, -4.5514464},
                            {-9.59519700, 0.73589927, 1.3474168},
                            {7.01297300, 3.23755380, -5.8988633},
                            {0.08445961, 9.26951000, 3.4138980},
                            {2.66668320, 5.29605670, 7.9653444}};

    std::vector<V3D> directions;
    for (auto &vector : vectors)
      directions.emplace_back(vector[0], vector[1], vector[2]);

    double required_tolerance = 0.12;
    size_t a_index = 0;
    double min_d = 6;
    double max_d = 10;

    IndexingUtils::FormUB_From_abc_Vectors(UB, directions, a_index, min_d, max_d);

    std::vector<V3D> q_vectors = getNatroliteQs();
    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, required_tolerance);
    TS_ASSERT_EQUALS(num_indexed, 12);

    size_t index = 0;
    for (size_t i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(UB[i][j], UB_array[index], 1.e-5);
        index++;
      }
    }
  }

  void test_FormUB_From_abc_Vectors_with_min_volume() {
    Matrix<double> UB(3, 3, false);
    double UB_array[] = {-0.0177703, -0.0993001, 0.0155008, 0.0585436, -0.0150158,
                         0.0839775,  -0.158519,  0.0432281, 0.0645189};

    double vectors[5][3] = {{-2.58222370, 3.97345330, -4.5514464},
                            {-9.59519700, 0.73589927, 1.3474168},
                            {7.01297300, 3.23755380, -5.8988633},
                            {0.08445961, 9.26951000, 3.4138980},
                            {2.66668320, 5.29605670, 7.9653444}};

    std::vector<V3D> directions;
    for (auto &vector : vectors)
      directions.emplace_back(vector[0], vector[1], vector[2]);

    std::vector<V3D> q_vectors = getNatroliteQs();
    double required_tolerance = 0.12;
    double min_vol = 6.0 * 6.0 * 6.0 / 4.0;

    IndexingUtils::FormUB_From_abc_Vectors(UB, directions, q_vectors, required_tolerance, min_vol);

    int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, required_tolerance);
    TS_ASSERT_EQUALS(num_indexed, 12);

    size_t index = 0;
    for (size_t i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        TS_ASSERT_DELTA(UB[i][j], UB_array[index], 1.e-5);
        index++;
      }
    }
  }

  void test_make_c_dir() {
    V3D a_dir(1, 2, 3);
    V3D b_dir(-3, 2, 1);

    const double gamma = a_dir.angle(b_dir) * 180.0 / M_PI;
    const double alpha = 123;
    const double beta = 74;
    const double c_length = 10;

    const double cosAlpha = std::cos(alpha * M_PI / 180);
    const double cosBeta = std::cos(beta * M_PI / 180);
    const double cosGamma = std::cos(gamma * M_PI / 180);
    const double sinGamma = std::sin(gamma * M_PI / 180);

    V3D result = IndexingUtils::makeCDir(a_dir, b_dir, c_length, cosAlpha, cosBeta, cosGamma, sinGamma);

    double alpha_calc = result.angle(b_dir) * 180 / M_PI;
    double beta_calc = result.angle(a_dir) * 180 / M_PI;

    TS_ASSERT_DELTA(result.norm(), c_length, 1e-5);
    TS_ASSERT_DELTA(alpha_calc, alpha, 1e-5);
    TS_ASSERT_DELTA(beta_calc, beta, 1e-5);
  }

  void test_DiscardDuplicates() {
    std::vector<V3D> new_list;
    std::vector<V3D> directions;
    std::vector<V3D> q_vectors = getNatroliteQs();
    double required_tolerance = 0.12;
    double length_tol = 0.05;
    double angle_tol = 3;

    V3D v1(-2.5822200, 3.97345, -4.55145);
    V3D v1b(-2.6000000, 3.98000, -4.56000);
    V3D v1c(-2.8000000, 3.90000, -4.60000);
    V3D v2(-9.5952000, 0.73590, 1.34742);
    V3D v3(7.0129700, 3.23755, -5.89886);
    V3D v3b(7.1129700, 3.53755, -5.99886);
    V3D v4(0.0844598, 9.26951, 3.41390);
    V3D v5(2.6666800, 5.29606, 7.96534);
    V3D v5b(2.7666800, 5.29606, 7.96534);
    V3D v5c(2.8666800, 5.39606, 8.06534);
    V3D v5d(2.9666800, 5.49606, 8.16534);

    directions.emplace_back(v1);
    directions.emplace_back(v1b);
    directions.emplace_back(v1c);
    directions.emplace_back(v2);
    directions.emplace_back(v3);
    directions.emplace_back(v3b);
    directions.emplace_back(v4);
    directions.emplace_back(v5);
    directions.emplace_back(v5b);
    directions.emplace_back(v5c);
    directions.emplace_back(v5d);

    IndexingUtils::DiscardDuplicates(new_list, directions, q_vectors, required_tolerance, length_tol, angle_tol);

    TS_ASSERT_EQUALS(new_list.size(), 5);
    TS_ASSERT_DELTA(new_list[0].norm(), 6.57053, 1e-4);
    TS_ASSERT_DELTA(new_list[1].norm(), 9.71725, 1e-4);
    TS_ASSERT_DELTA(new_list[2].norm(), 9.71905, 1e-4);
    TS_ASSERT_DELTA(new_list[3].norm(), 9.87855, 1e-4);
    TS_ASSERT_DELTA(new_list[4].norm(), 9.93006, 1e-4);
  }

  void test_RoundHKL() {
    V3D hkl(-1.234, 0.345, 7.5765);
    IndexingUtils::RoundHKL(hkl);
    TS_ASSERT_EQUALS(V3D(-1, 0, 8), hkl)
  }

  void test_RoundHKLs() {
    std::vector<V3D> hkls{V3D(-1.234, 0.345, 7.5765), V3D(3.5345, -1.346, 0.2347)};
    IndexingUtils::RoundHKLs(hkls);
    TS_ASSERT_EQUALS(V3D(-1, 0, 8), hkls[0])
    TS_ASSERT_EQUALS(V3D(4, -1, 0), hkls[1])
  }

  void test_ValidIndex() {
    V3D hkl(0, 0, 0);
    TS_ASSERT_EQUALS(IndexingUtils::ValidIndex(hkl, 0.1), false);

    hkl(2.09, -3.09, -2.91);
    TS_ASSERT_EQUALS(IndexingUtils::ValidIndex(hkl, 0.1), true);

    hkl(2.11, -3.09, -2.91);
    TS_ASSERT_EQUALS(IndexingUtils::ValidIndex(hkl, 0.1), false);

    hkl(2.09, -3.11, -2.91);
    TS_ASSERT_EQUALS(IndexingUtils::ValidIndex(hkl, 0.1), false);

    hkl(2.09, -3.09, -2.89);
    TS_ASSERT_EQUALS(IndexingUtils::ValidIndex(hkl, 0.1), false);
  }

  void test_CheckUB() {
    Matrix<double> UB(3, 3, false);
    for (int i = 0; i < 3; i++) // check for matrix and det OK
      for (int j = 0; j < 3; j++)
        if (i == j)
          UB[i][j] = .1;
        else
          UB[i][j] = 0;

    TS_ASSERT_EQUALS(IndexingUtils::CheckUB(UB), true);

    for (int i = 0; i < 3; i++) // check for det too small
      UB[i][i] = 0.00001;

    TS_ASSERT_EQUALS(IndexingUtils::CheckUB(UB), false);

    for (int i = 0; i < 3; i++) // check for det too large
      UB[i][i] = 3;

    TS_ASSERT_EQUALS(IndexingUtils::CheckUB(UB), false);

    for (int i = 0; i < 2; i++) // check for NaN
      UB[i][i] = 0.1;
    UB[2][2] = sqrt(-1.0);

    TS_ASSERT_EQUALS(IndexingUtils::CheckUB(UB), false);

    Matrix<double> UB_4(4, 4, false); // check wrong size
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        if (i == j)
          UB_4[i][j] = .1;
        else
          UB_4[i][j] = 0;

    TS_ASSERT_EQUALS(IndexingUtils::CheckUB(UB_4), false);
  }

  void test_NumberIndexed() {
    std::vector<V3D> q_list = getNatroliteQs();
    Matrix<double> UB = getNatroliteUB();

    int num_indexed;

    num_indexed = IndexingUtils::NumberIndexed(UB, q_list, 0.05);
    TS_ASSERT_EQUALS(num_indexed, 10)
  }

  void test_NumberIndexed_1D() {
    std::vector<V3D> q_list = getNatroliteQs();
    Matrix<double> UB = getNatroliteUB();
    UB.Invert();
    V3D direction(UB[0][0], UB[0][1], UB[0][2]);

    int num_indexed;

    num_indexed = IndexingUtils::NumberIndexed_1D(direction, q_list, 0.10);
    TS_ASSERT_EQUALS(num_indexed, 12);

    num_indexed = IndexingUtils::NumberIndexed_1D(direction, q_list, 0.05);
    TS_ASSERT_EQUALS(num_indexed, 12);

    num_indexed = IndexingUtils::NumberIndexed_1D(direction, q_list, 0.01);
    TS_ASSERT_EQUALS(num_indexed, 8);
  }

  void test_NumberIndexed_3D() {
    std::vector<V3D> q_list = getNatroliteQs();
    Matrix<double> UB = getNatroliteUB();
    UB.Invert();
    V3D a_dir(UB[0][0], UB[0][1], UB[0][2]);
    V3D b_dir(UB[1][0], UB[1][1], UB[1][2]);
    V3D c_dir(UB[2][0], UB[2][1], UB[2][2]);

    int num_indexed;

    num_indexed = IndexingUtils::NumberIndexed_3D(a_dir, b_dir, c_dir, q_list, 0.10);
    TS_ASSERT_EQUALS(num_indexed, 12);

    num_indexed = IndexingUtils::NumberIndexed_3D(a_dir, b_dir, c_dir, q_list, 0.05);
    TS_ASSERT_EQUALS(num_indexed, 10);

    num_indexed = IndexingUtils::NumberIndexed_3D(a_dir, b_dir, c_dir, q_list, 0.01);
    TS_ASSERT_EQUALS(num_indexed, 4);
  }

  void test_CalculateMillerIndices() {
    std::vector<V3D> q_vectors = getNatroliteQs();
    std::vector<V3D> indices = getNatroliteIndices();
    Matrix<double> UB = getNatroliteUB();

    double tolerance = 0.1;
    std::vector<V3D> miller_indices;
    double average_error;

    int num_indexed = IndexingUtils::CalculateMillerIndices(UB, q_vectors, tolerance, miller_indices, average_error);
    TS_ASSERT_EQUALS(num_indexed, 12);

    TS_ASSERT_DELTA(average_error, 0.0185, 1e-3);

    double diff;

    for (size_t i = 0; i < indices.size(); i++) {
      diff = (indices[i] - miller_indices[i]).norm();
      TS_ASSERT_DELTA(diff, 0, 0.1);
    }
  }

  void test_CalculateMillerIndicesSingleQ_for_Valid_Index() {
    const auto q_vectors = getNatroliteQs();
    const auto indices = getNatroliteIndices();
    auto UB = getNatroliteUB();
    UB.Invert();

    const double tolerance = 0.1;
    V3D miller_indices;

    const bool success = IndexingUtils::CalculateMillerIndices(UB, q_vectors[0], tolerance, miller_indices);
    TS_ASSERT(success);
    const auto diff = (indices[0] - miller_indices).norm();
    TS_ASSERT_DELTA(diff, 0, 0.1);
  }

  void test_CalculateMillerIndicesSingleQ_No_Tolerance() {
    const auto q_vectors = getNatroliteQs();
    const auto indices = getNatroliteIndices();
    auto UB = getNatroliteUB();
    UB.Invert();

    const V3D millerIndices = IndexingUtils::CalculateMillerIndices(UB, q_vectors[0]);
    const auto diff = (indices[0] - millerIndices).norm();
    TS_ASSERT_DELTA(diff, 0, 0.1);
  }

  void test_GetIndexedPeaks_1D() {
    int correct_indices[] = {1, 4, 2, 0, 1, 3, 0, -1, 0, -1, -2, -3};

    std::vector<V3D> q_vectors = getNatroliteQs();

    V3D direction(-2.5825930, 3.9741700, -4.5514810);
    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<int> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed =
        IndexingUtils::GetIndexedPeaks_1D(direction, q_vectors, required_tolerance, index_vals, indexed_qs, fit_error);
    TS_ASSERT_EQUALS(num_indexed, 12);
    TS_ASSERT_EQUALS(index_vals.size(), 12);
    TS_ASSERT_EQUALS(indexed_qs.size(), 12);
    TS_ASSERT_DELTA(fit_error, 0.00218634, 1e-5);

    for (size_t i = 0; i < index_vals.size(); i++) {
      TS_ASSERT_EQUALS(index_vals[i], correct_indices[i]);
    }
  }

  void test_GetIndexedPeaks_3D() {
    std::vector<V3D> q_vectors = getNatroliteQs();
    std::vector<V3D> correct_indices = getNatroliteIndices();

    V3D direction_1(-2.5825930, 3.9741700, -4.5514810);
    V3D direction_2(-16.6087800, -2.5005515, 7.2465878);
    V3D direction_3(2.7502847, 14.5671910, 11.3796620);

    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<V3D> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed = IndexingUtils::GetIndexedPeaks_3D(direction_1, direction_2, direction_3, q_vectors,
                                                        required_tolerance, index_vals, indexed_qs, fit_error);
    TS_ASSERT_EQUALS(num_indexed, 12);
    TS_ASSERT_EQUALS(index_vals.size(), 12);
    TS_ASSERT_EQUALS(indexed_qs.size(), 12);
    TS_ASSERT_DELTA(fit_error, 0.023007052, 1e-5);

    for (size_t i = 0; i < index_vals.size(); i++) {
      for (size_t j = 0; j < 3; j++) {
        TS_ASSERT_EQUALS((index_vals[i])[j], (correct_indices[i])[j]);
      }
    }
  }

  void test_GetIndexedPeaks() {
    std::vector<V3D> q_vectors = getNatroliteQs();
    std::vector<V3D> correct_indices = getNatroliteIndices();
    Matrix<double> UB = getNatroliteUB();

    double required_tolerance = 0.1;
    double fit_error = 0;

    std::vector<V3D> index_vals;
    std::vector<V3D> indexed_qs;

    int num_indexed =
        IndexingUtils::GetIndexedPeaks(UB, q_vectors, required_tolerance, index_vals, indexed_qs, fit_error);
    TS_ASSERT_EQUALS(num_indexed, 12);
    TS_ASSERT_EQUALS(index_vals.size(), 12);
    TS_ASSERT_EQUALS(indexed_qs.size(), 12);
    TS_ASSERT_DELTA(fit_error, 0.023007052, 1e-5);

    for (size_t i = 0; i < index_vals.size(); i++) {
      for (size_t j = 0; j < 3; j++) {
        TS_ASSERT_EQUALS((index_vals[i])[j], (correct_indices[i])[j]);
      }
    }
  }

  void test_MakeHemisphereDirections() {
    std::vector<V3D> direction_list = IndexingUtils::MakeHemisphereDirections(5);

    TS_ASSERT_EQUALS(direction_list.size(), 64);

    // check some random entries
    TS_ASSERT_DELTA(direction_list[0].X(), 0, 1e-5);
    TS_ASSERT_DELTA(direction_list[0].Y(), 1, 1e-5);
    TS_ASSERT_DELTA(direction_list[0].Z(), 0, 1e-5);

    TS_ASSERT_DELTA(direction_list[5].X(), -0.154508, 1e-5);
    TS_ASSERT_DELTA(direction_list[5].Y(), 0.951057, 1e-5);
    TS_ASSERT_DELTA(direction_list[5].Z(), -0.267617, 1e-5);

    TS_ASSERT_DELTA(direction_list[10].X(), 0, 1e-5);
    TS_ASSERT_DELTA(direction_list[10].Y(), 0.809017, 1e-5);
    TS_ASSERT_DELTA(direction_list[10].Z(), 0.587785, 1e-5);

    TS_ASSERT_DELTA(direction_list[63].X(), -0.951057, 1e-5);
    TS_ASSERT_DELTA(direction_list[63].Y(), 0, 1e-5);
    TS_ASSERT_DELTA(direction_list[63].Z(), 0.309017, 1e-5);
  }

  void test_MakeCircleDirections() {
    int num_steps = 8;
    V3D axis(1, 1, 1);
    double angle_degrees = 90;

    std::vector<V3D> direction_list = IndexingUtils::MakeCircleDirections(num_steps, axis, angle_degrees);

    TS_ASSERT_EQUALS(direction_list.size(), 8);

    TS_ASSERT_DELTA(direction_list[0].X(), -0.816497, 1e-5);
    TS_ASSERT_DELTA(direction_list[0].Y(), 0.408248, 1e-5);
    TS_ASSERT_DELTA(direction_list[0].Z(), 0.408248, 1e-5);

    TS_ASSERT_DELTA(direction_list[1].X(), -0.577350, 1e-5);
    TS_ASSERT_DELTA(direction_list[1].Y(), -0.211325, 1e-5);
    TS_ASSERT_DELTA(direction_list[1].Z(), 0.788675, 1e-5);

    TS_ASSERT_DELTA(direction_list[7].X(), -0.577350, 1e-5);
    TS_ASSERT_DELTA(direction_list[7].Y(), 0.788675, 1e-5);
    TS_ASSERT_DELTA(direction_list[7].Z(), -0.211325, 1e-5);

    double dot_prod;
    for (const auto &direction : direction_list) {
      dot_prod = axis.scalar_prod(direction);
      TS_ASSERT_DELTA(dot_prod, 0, 1e-10);
    }
  }

  void test_SelectDirection() {
    V3D best_direction;

    std::vector<V3D> q_vectors = getNatroliteQs();

    std::vector<V3D> directions = IndexingUtils::MakeHemisphereDirections(90);

    double plane_spacing = 1.0 / 6.5781;
    double required_tolerance = 0.1;

    int num_indexed =
        IndexingUtils::SelectDirection(best_direction, q_vectors, directions, plane_spacing, required_tolerance);

    TS_ASSERT_DELTA(best_direction[0], -0.399027, 1e-5);
    TS_ASSERT_DELTA(best_direction[1], 0.615661, 1e-5);
    TS_ASSERT_DELTA(best_direction[2], -0.679513, 1e-5);

    TS_ASSERT_EQUALS(num_indexed, 12);
  }

  void test_UB_to_from_V3D() {
    V3D a_dir(2, 0, 0);
    V3D b_dir(0, 3, 0);
    V3D c_dir(0, 0, 4);

    Matrix<double> UB(3, 3, false);

    OrientedLattice::GetUB(UB, a_dir, b_dir, c_dir);

    for (size_t row = 0; row < 3; row++) {
      for (size_t col = 0; col < 3; col++) {
        if (row == col) {
          TS_ASSERT_DELTA(UB[row][col], 1.0 / (double(row + 2)), 1e-10);
        } else {
          TS_ASSERT_DELTA(UB[row][col], 0, 1e-10);
        }
      }
    }

    V3D a;
    V3D b;
    V3D c;

    OrientedLattice::GetABC(UB, a, b, c);

    a = a - a_dir;
    b = b - b_dir;
    c = c - c_dir;

    TS_ASSERT_DELTA(a.norm(), 0, 1e-10);
    TS_ASSERT_DELTA(b.norm(), 0, 1e-10);
    TS_ASSERT_DELTA(c.norm(), 0, 1e-10);
  }

  void test_GetLatticeParameters() {
    double correct_value[7] = {6.5711, 18.2925, 18.6886, 89.9399, 90.4687, 90.0127, 2246.3452};

    Matrix<double> natrolite_UB = getNatroliteUB();

    std::vector<double> lat_par;

    IndexingUtils::GetLatticeParameters(natrolite_UB, lat_par);

    for (size_t i = 0; i < lat_par.size(); i++)
      TS_ASSERT_DELTA(lat_par[i], correct_value[i], 1e-3);
  }
};
