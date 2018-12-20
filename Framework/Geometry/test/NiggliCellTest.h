#ifndef MANTID_GEOMETRY_NiggliCellTEST_H_
#define MANTID_GEOMETRY_NiggliCellTEST_H_

#include "MantidGeometry/Crystal/NiggliCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class NiggliCellTest : public CxxTest::TestSuite {
public:
  static Matrix<double> getSiliconUB() {
    Matrix<double> UB(3, 3, false);
    V3D row_0(-0.147196, -0.141218, 0.304286);
    V3D row_1(0.106642, 0.120341, 0.090518);
    V3D row_2(-0.261273, 0.258426, -0.006190);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);
    return UB;
  }
  /// test constructors, access to some of the variables
  void test_Simple() {

    NiggliCell u1, u2(3, 4, 5), u3(2, 3, 4, 85., 95., 100), u4;
    u4 = u2;
    TS_ASSERT_EQUALS(u1.a1(), 1);
    TS_ASSERT_EQUALS(u1.alpha(), 90);
    TS_ASSERT_DELTA(u2.b1(), 1. / 3., 1e-10);
    TS_ASSERT_DELTA(u2.alphastar(), 90, 1e-10);
    TS_ASSERT_DELTA(u4.volume(), 1. / u2.recVolume(), 1e-10);
    u2.seta(3);
    TS_ASSERT_DELTA(u2.a(), 3, 1e-10);
  }

  void test_HasNiggleAngles() {
    V3D a(1, 0, 0);
    V3D b(0, 1, 0);
    V3D c(0, 0, 1);

    TS_ASSERT_EQUALS(NiggliCell::HasNiggliAngles(a, b, c, 0.001), true);

    V3D b1(0.1, 1, 0);
    V3D c1(-0.1, 0, 1);

    TS_ASSERT_EQUALS(NiggliCell::HasNiggliAngles(a, b1, c1, 0.001), false);

    V3D a2(1, 0.1, 0.1);
    V3D b2(0.1, 1, 0.1);
    V3D c2(0.1, 0.1, 1);

    TS_ASSERT_EQUALS(NiggliCell::HasNiggliAngles(a2, b2, c2, 0.001), true);

    V3D a3(1, -0.1, -0.1);
    V3D b3(-0.1, 1, -0.1);
    V3D c3(-0.1, -0.1, 1);

    TS_ASSERT_EQUALS(NiggliCell::HasNiggliAngles(a3, b3, c3, 0.001), true);
  }

  void test_MakeNiggliUB() {
    double answer[3][3] = {{-0.147196, -0.141218, 0.304286},
                           {0.106642, 0.120341, 0.090518},
                           {-0.261273, 0.258426, -0.006190}};

    Matrix<double> newUB(3, 3, false);
    Matrix<double> UB = getSiliconUB();
    UB = UB * 1.0;

    TS_ASSERT(NiggliCell::MakeNiggliUB(UB, newUB));

    for (size_t row = 0; row < 3; row++)
      for (size_t col = 0; col < 3; col++)
        TS_ASSERT_DELTA(newUB[row][col], answer[row][col], 1e-5);
  }

  void test_MakeNiggliUB2() {
    // Make a fake UB matrix with:
    // gamma > 90 deg
    // alpha < 90 deg
    Matrix<double> UB(3, 3, true);
    V3D a(10, 0, 0);
    V3D b(-5, 5, 0);
    V3D c(0, 5, 5);
    OrientedLattice::GetUB(UB, a, b, c);

    Matrix<double> newUB(3, 3, false);

    TS_ASSERT(NiggliCell::MakeNiggliUB(UB, newUB));

    // Extract the a,b,c vectors
    V3D a_dir;
    V3D b_dir;
    V3D c_dir;
    OrientedLattice::GetABC(newUB, a_dir, b_dir, c_dir);
    double alpha = b_dir.angle(c_dir) * 180.0 / M_PI;
    double beta = c_dir.angle(a_dir) * 180.0 / M_PI;
    double gamma = a_dir.angle(b_dir) * 180.0 / M_PI;
    // All vectors have two components of length 5.0
    double norm = sqrt(50.0);
    TS_ASSERT_DELTA(a_dir.norm(), norm, 1e-3);
    TS_ASSERT_DELTA(b_dir.norm(), norm, 1e-3);
    TS_ASSERT_DELTA(c_dir.norm(), norm, 1e-3);
    // Angles are 60 degrees
    TS_ASSERT_DELTA(alpha, 60, 1e-1);
    TS_ASSERT_DELTA(beta, 60, 1e-1);
    TS_ASSERT_DELTA(gamma, 60, 1e-1);
  }
};

#endif /* MANTID_GEOMETRY_NiggliCellTEST_H_ */
