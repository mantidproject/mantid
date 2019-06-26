// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_ORIENTEDLATTICETEST_H_
#define MANTID_GEOMETRY_ORIENTEDLATTICETEST_H_

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class OrientedLatticeTest : public CxxTest::TestSuite {
public:
  /// test constructors, access to some of the variables
  void test_Simple() {

    OrientedLattice u1, u2(3, 4, 5), u3(2, 3, 4, 85., 95., 100), u4;
    u4 = u2;
    TS_ASSERT_EQUALS(u1.a1(), 1);
    TS_ASSERT_EQUALS(u1.alpha(), 90);
    TS_ASSERT_DELTA(u2.b1(), 1. / 3., 1e-10);
    TS_ASSERT_DELTA(u2.alphastar(), 90, 1e-10);
    TS_ASSERT_DELTA(u4.volume(), 1. / u2.recVolume(), 1e-10);
    TS_ASSERT_DELTA(u2.getUB()[0][0], 1. / 3., 1e-10);
    u2.seta(13);
    TS_ASSERT_DELTA(u2.a(), 13, 1e-10);
    TS_ASSERT_DELTA(u2.getUB()[0][0], 1. / 13., 1e-10);
  }

  void test_hklFromQ() {
    OrientedLattice u;
    DblMatrix UB(3, 3, true);
    u.setUB(UB);

    // Convert to and from HKL
    V3D hkl = u.hklFromQ(V3D(1.0, 2.0, 3.0));
    double dstar = u.dstar(hkl[0], hkl[1], hkl[2]);
    TS_ASSERT_DELTA(
        dstar, .5 * sqrt(1 + 4.0 + 9.0) / M_PI,
        1e-4); // The d-spacing after a round trip matches the Q we put in
  }

  void test_nexus() {
    NexusTestHelper th(true);
    th.createFile("OrientedLatticeTest.nxs");
    DblMatrix U(3, 3, true);
    OrientedLattice u(1, 2, 3, 90, 89, 88);
    u.saveNexus(th.file.get(), "lattice");
    th.reopenFile();

    OrientedLattice u2;
    u2.loadNexus(th.file.get(), "lattice");
    // Was it reloaded correctly?
    TS_ASSERT_DELTA(u2.a(), 1.0, 1e-5);
    TS_ASSERT_DELTA(u2.b(), 2.0, 1e-5);
    TS_ASSERT_DELTA(u2.c(), 3.0, 1e-5);
    TS_ASSERT_DELTA(u2.alpha(), 90.0, 1e-5);
    TS_ASSERT_DELTA(u2.beta(), 89.0, 1e-5);
    TS_ASSERT_DELTA(u2.gamma(), 88.0, 1e-5);
  }

  /** @author Alex Buts, fixed by Andrei Savici */
  void testUnitRotation() {
    OrientedLattice theCell;
    TSM_ASSERT_THROWS_NOTHING(
        "The unit transformation should not throw",
        theCell.setUFromVectors(V3D(1, 0, 0), V3D(0, 1, 0)));
    const DblMatrix &rot = theCell.getUB();
    /*this should give
      / 0 1 0 \
      | 0 0 1 |
      \ 1 0 0 /
      */
    DblMatrix expected(3, 3);
    expected[0][1] = 1.;
    expected[1][2] = 1.;
    expected[2][0] = 1.;
    TSM_ASSERT("This should produce proper permutation matrix",
               rot.equals(expected, 1e-8));
  }

  /** @author Alex Buts */
  void testParallelProjThrows() {
    OrientedLattice theCell;
    TSM_ASSERT_THROWS("The transformation to plane defined by two parallel "
                      "vectors should throw",
                      theCell.setUFromVectors(V3D(0, 1, 0), V3D(0, 1, 0)),
                      const std::invalid_argument &);
  }

  /** @author Alex Buts, fixed by Andrei Savici */
  void testPermutations() {
    OrientedLattice theCell;
    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(0, 1, 0), V3D(1, 0, 0)));
    const DblMatrix &rot = theCell.getUB();
    /*this should give
      / 1 0 0 \
      | 0 0 -1 |
      \ 0 1 0 /
      */
    DblMatrix expected(3, 3);
    expected[0][0] = 1.;
    expected[1][2] = -1.;
    expected[2][1] = 1.;
    TSM_ASSERT("This should produce proper permutation matrix",
               rot.equals(expected, 1e-8));
  }

  /** @author Alex Buts fixed by Andrei Savici*/
  void testRotations2D() {
    OrientedLattice theCell;
    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(1, 1, 0), V3D(1, -1, 0)));
    const DblMatrix &rot = theCell.getUB();
    V3D dir0(M_SQRT2, 0, 0), rez, expected(1, 0, 1);
    rez = rot * dir0;
    // should be (1,0,1)
    TSM_ASSERT_EQUALS("vector should be (1,0,1)", rez, expected);
  }

  /** @author Alex Buts fixed by Andrei Savici*/
  void testRotations3D() {
    OrientedLattice theCell;
    // two orthogonal vectors
    V3D ort1(M_SQRT2, -1, -1);
    V3D ort2(M_SQRT2, 1, 1);
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",
                              theCell.setUFromVectors(ort1, ort2));
    const DblMatrix &rot = theCell.getUB();

    V3D dir(1, 0, 0), expected(sqrt(0.5), 0, sqrt(0.5));
    V3D result = rot * dir;
    TSM_ASSERT_EQUALS("vector should be (sqrt(0.5),0,sqrt(0.5))", result,
                      expected);
  }

  /** @author Alex Buts */
  void testRotations3DNonOrthogonal() {
    OrientedLattice theCell(1, 2, 3, 30, 60, 45);
    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(1, 0, 0), V3D(0, 1, 0)));
    const DblMatrix &rot = theCell.getUB();

    V3D dir(1, 1, 1);

    std::vector<double> Rot = rot.getVector();
    double x = Rot[0] * dir.X() + Rot[3] * dir.Y() + Rot[6] * dir.Z();
    double y = Rot[1] * dir.X() + Rot[4] * dir.Y() + Rot[7] * dir.Z();
    double z = Rot[2] * dir.X() + Rot[5] * dir.Y() + Rot[8] * dir.Z();
    // this freeses the interface but unclear how to propelry indentify the
    TSM_ASSERT_DELTA("X-coord should be specified correctly",
                     1.4915578672621419, x, 1.e-5);
    TSM_ASSERT_DELTA("Y-coord should be specified correctly",
                     0.18234563931714265, y, 1.e-5);
    TSM_ASSERT_DELTA("Z-coord should be specified correctly",
                     -0.020536948488997286, z, 1.e-5);
  }

  /// Test consistency for setUFromVectors
  void testconsistency() {
    OrientedLattice theCell(2, 2, 2, 90, 90, 90);
    V3D u(1, 2, 0), v(-2, 1, 0), expected1(0, 0, 1), expected2(1, 0, 0), res1,
        res2;

    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",
                              theCell.setUFromVectors(u, v));
    const DblMatrix &rot = theCell.getUB();
    res1 = rot * u;
    res1.normalize();
    res2 = rot * v;
    res2.normalize();
    TSM_ASSERT_EQUALS("Ub*u should be along the beam", res1, expected1);
    TSM_ASSERT_EQUALS("Ub*v should be along the x direction", res2, expected2);
  }

  /// test getting u and v vectors
  void testuvvectors() {
    OrientedLattice theCell(1, 2, 3, 30, 60, 45);

    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(1, 2, 0), V3D(-1, 1, 0)));
    const DblMatrix &rot = theCell.getUB();
    V3D u = theCell.getuVector(), v = theCell.getvVector(), expected1(0, 0, 1),
        expected2(1, 0, 0);
    V3D res1 = rot * u;
    res1.normalize();
    V3D res2 = rot * v;
    res2.normalize();
    TSM_ASSERT_EQUALS("Ub*u should be along the beam", res1, expected1);
    TSM_ASSERT_EQUALS("Ub*v should be along the x direction", res2, expected2);
  }

  void test_UVPerm2() {
    OrientedLattice theCell(1, 3, 4, 35, 60, 70);
    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(1, 0, 0), V3D(0, 1, 0)));
    const DblMatrix &U = theCell.getU();
    V3D ez = U * V3D(1, 0, 0);
    V3D ex = U * V3D(0, 1, 0);
    V3D ey = U * V3D(0, 0, 1);

    TSM_ASSERT_EQUALS("U*u should be along the beam", V3D(0, 0, 1), ez);
    TSM_ASSERT_EQUALS("U*v should be along the x direction", V3D(1, 0, 0), ex);
    TSM_ASSERT_EQUALS(" should be along the y direction", V3D(0, 1, 0), ey);

    std::vector<double> rotMat = U.getVector();
    double qx(1), qy(2), qz(3);
    V3D vc = U * V3D(qx, qy, qz);
    double Qx = (rotMat[0] * qx + rotMat[1] * qy + rotMat[2] * qz);
    double Qy = (rotMat[3] * qx + rotMat[4] * qy + rotMat[5] * qz);
    double Qz = (rotMat[6] * qx + rotMat[7] * qy + rotMat[8] * qz);
    TS_ASSERT_DELTA(vc.X(), Qx, 1.e-5);
    TS_ASSERT_DELTA(vc.Y(), Qy, 1.e-5);
    TS_ASSERT_DELTA(vc.Z(), Qz, 1.e-5);
  }
  void test_UVPerm1() {
    OrientedLattice theCell(1, 1, 1, 90, 90, 90);
    V3D r1(1, 1, 0);
    V3D r2(1, -1, 0);
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",
                              theCell.setUFromVectors(r1, r2));
    DblMatrix U = theCell.getU();

    V3D ez = U * r1;
    ez.normalize();
    V3D ex = U * r2;
    ex.normalize();
    V3D ey = U * r1.cross_prod(r2);
    ey.normalize();

    TSM_ASSERT_EQUALS("U*u should be along the beam", V3D(0, 0, 1), ez);
    TSM_ASSERT_EQUALS("U*v should be along the x direction", V3D(1, 0, 0), ex);
    TSM_ASSERT_EQUALS(" should be along the y direction", V3D(0, 1, 0), ey);

    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(1, 0, 0), V3D(0, 0, 1)));
    U = theCell.getU();
    ez = U * V3D(1, 0, 0);
    ex = U * V3D(0, 0, 1);
    ey = U * V3D(0, -1, 0);

    TSM_ASSERT_EQUALS("U*u should be along the beam", V3D(0, 0, 1), ez);
    TSM_ASSERT_EQUALS("U*v should be along the x direction", V3D(1, 0, 0), ex);
    TSM_ASSERT_EQUALS(" should be along the y direction", V3D(0, 1, 0), ey);

    TSM_ASSERT_THROWS_NOTHING(
        "The permutation transformation should not throw",
        theCell.setUFromVectors(V3D(0, 1, 0), V3D(0, 0, 1)));
    U = theCell.getU();
    ez = U * V3D(0, 1, 0);
    ex = U * V3D(0, 0, 1);
    ey = U * V3D(1, 0, 0);

    TSM_ASSERT_EQUALS("U*u should be along the beam", V3D(0, 0, 1), ez);
    TSM_ASSERT_EQUALS("U*v should be along the x direction", V3D(1, 0, 0), ex);
    TSM_ASSERT_EQUALS(" should be along the y direction", V3D(0, 1, 0), ey);

    std::vector<double> rotMat = U.getVector();
    double qx(1), qy(2), qz(3);
    V3D vc = U * V3D(qx, qy, qz);
    double Qx = (rotMat[0] * qx + rotMat[1] * qy + rotMat[2] * qz);
    double Qy = (rotMat[3] * qx + rotMat[4] * qy + rotMat[5] * qz);
    double Qz = (rotMat[6] * qx + rotMat[7] * qy + rotMat[8] * qz);
    TS_ASSERT_DELTA(vc.X(), Qx, 1.e-5);
    TS_ASSERT_DELTA(vc.Y(), Qy, 1.e-5);
    TS_ASSERT_DELTA(vc.Z(), Qz, 1.e-5);
  }
  void test_UVPerm3() {
    OrientedLattice theCell(2, 1, 2, 90, 90, 90);
    V3D r1(1, 0, 0);
    V3D r2(0, 0, 1);
    TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",
                              theCell.setUFromVectors(r1, r2));
    const DblMatrix &U = theCell.getU();

    V3D ez = U * r1;
    ez.normalize();
    V3D ex = U * r2;
    ex.normalize();
    V3D ey = U * r1.cross_prod(r2);
    ey.normalize();

    V3D eyPrime = r1.cross_prod(r2);
    TSM_ASSERT_EQUALS("U*u should be along the beam", V3D(0, 0, 1), ez);
    TSM_ASSERT_EQUALS("U*v should be along the x direction", V3D(1, 0, 0), ex);
    TSM_ASSERT_EQUALS(" should be along the y direction", V3D(0, 1, 0), ey);
    TSM_ASSERT_EQUALS("y direction is", V3D(0, -1, 0), eyPrime);
  }
};

#endif /* MANTID_GEOMETRY_UNITCELLTEST_H_ */
