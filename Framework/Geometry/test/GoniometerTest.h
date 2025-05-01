// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Quat.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include <string>

using namespace Mantid::Geometry;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

class GoniometerTest : public CxxTest::TestSuite {
public:
  void test_AxisConstructor() {
    GoniometerAxis a("axis1", V3D(1., 0, 0), 3., CW, angRadians);
    TS_ASSERT_EQUALS(a.name, "axis1");
    TS_ASSERT_EQUALS(a.rotationaxis[0], 1.);
    TS_ASSERT_EQUALS(a.angle, 3.);
    TS_ASSERT_EQUALS(a.sense, -1);
    TS_ASSERT_DIFFERS(a.sense, angDegrees);
  }

  void test_Goniometer() {
    Goniometer G;
    DblMatrix M(3, 3);
    // Check simple constructor
    M.identityMatrix();
    TS_ASSERT(!G.isDefined());
    TS_ASSERT_EQUALS(G.getR(), M);
    TS_ASSERT_THROWS(G.setRotationAngle("Axis4", 3), const std::invalid_argument &);
    TS_ASSERT_THROWS_ANYTHING(G.setRotationAngle(1, 2));
    TS_ASSERT_EQUALS((G.axesInfo()).compare("No axis is found\n"), 0);
    TS_ASSERT(!G.isDefined());
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis1", 1., 0., 0., 30));
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis2", 0., 0., 1., 30));
    TS_ASSERT(G.isDefined());
    TS_ASSERT_THROWS(G.pushAxis("Axis2", 0., 0., 1., 30), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(G.setRotationAngle("Axis2", 25));
    TS_ASSERT_THROWS_NOTHING(G.setRotationAngle(0, -17));
    TS_ASSERT_EQUALS(G.getAxis(1).angle, 25.);
    TS_ASSERT_EQUALS(G.getAxis("Axis1").angle, -17);
    TS_ASSERT_DELTA(G.axesInfo().find("-17"), 52, 20);
    M = G.getR();
    // test some matrix elements
    TS_ASSERT_DELTA(M[0][0], 9.063078e-01, 1e-6);
    TS_ASSERT_DELTA(M[0][1], -4.226183e-01, 1e-6);
    TS_ASSERT_DELTA(M[0][2], 0, 1e-6);
    TS_ASSERT_DELTA(M[1][1], 8.667064e-01, 1e-6);
    TS_ASSERT_DELTA(M[1][2], 2.923717e-01, 1e-6);
    // goniometer from a rotation matrix or copied
    Goniometer G1(M), G2(G);
    TS_ASSERT(G1.isDefined());
    TS_ASSERT_EQUALS(M, G1.getR());
    TS_ASSERT_EQUALS(G1.axesInfo(), std::string("Goniometer was initialized from a rotation matrix. No "
                                                "information about axis is available.\n"));
    TS_ASSERT_THROWS_ANYTHING(G.pushAxis("Axis2", 0., 0., 1., 30));
    TS_ASSERT_EQUALS(M, G2.getR());
  }

  void testSense() {
    Goniometer G1, G2, G3;
    TS_ASSERT_THROWS_NOTHING(G1.pushAxis("Axis1", 0., 1., 0., 30));
    TS_ASSERT_THROWS_NOTHING(G2.pushAxis("Axis1", 0., 1., 0., -30));
    TS_ASSERT_THROWS_NOTHING(G3.pushAxis("Axis1", 0., 1., 0., 30, -1));
    DblMatrix M1 = G1.getR();
    DblMatrix M2 = G2.getR();
    DblMatrix M3 = G3.getR();
    TS_ASSERT_DELTA(M1[0][2], -M2[0][2], 1e-6);
    TS_ASSERT_DELTA(M1[0][2], -M3[0][2], 1e-6);
    TS_ASSERT_DELTA(M1[2][0], -M2[2][0], 1e-6);
    TS_ASSERT_DELTA(M1[2][0], -M3[2][0], 1e-6);
  }

  void test_makeUniversalGoniometer() {
    Goniometer G;
    TS_ASSERT(!G.isDefined());
    G.makeUniversalGoniometer();
    TS_ASSERT(G.isDefined());
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(2).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(0).name, "omega");
    TS_ASSERT_EQUALS(G.getConventionFromMotorAxes(), "YZY");
  }

  void test_copy() {
    Goniometer G, G1;
    G1.makeUniversalGoniometer();
    G = G1;
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(2).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(0).name, "omega");
  }

  /** Test to make sure the goniometer rotation works as advertised
   * for a simple universal goniometer.
   */
  void test_UniversalGoniometer_getR() {
    Goniometer G;
    V3D init, rot;

    G.makeUniversalGoniometer();
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);

    init = V3D(0, 0, 1.0);
    G.setRotationAngle("phi", 45.0);
    G.setRotationAngle("chi", 0.0);
    G.setRotationAngle("omega", 0.0);

    rot = G.getR() * init;

    TS_ASSERT_DELTA(rot.X(), 0.707, 0.001);
    TS_ASSERT_DELTA(rot.Y(), 0.000, 0.001);
    TS_ASSERT_DELTA(rot.Z(), 0.707, 0.001);

    init = V3D(0, 0, 1.0);
    G.setRotationAngle("phi", 45.0);
    G.setRotationAngle("chi", 90.0);
    G.setRotationAngle("omega", 0.0);
    rot = G.getR() * init;

    TS_ASSERT_DELTA(rot.X(), 0.000, 0.001);
    TS_ASSERT_DELTA(rot.Y(), 0.707, 0.001);
    TS_ASSERT_DELTA(rot.Z(), 0.707, 0.001);

    init = V3D(-1, 0, 0);
    G.setRotationAngle("phi", 90.0);
    G.setRotationAngle("chi", 90.0);
    G.setRotationAngle("omega", 0.0);
    rot = G.getR() * init;
    TS_ASSERT_DELTA(rot.X(), 0.000, 0.001);
    TS_ASSERT_DELTA(rot.Y(), 0.000, 0.001);
    TS_ASSERT_DELTA(rot.Z(), 1.000, 0.001);
  }

  void test_getEulerAngles() {
    Goniometer G;
    DblMatrix rotA;
    G.makeUniversalGoniometer();
    G.setRotationAngle("phi", 45.0);
    G.setRotationAngle("chi", 23.0);
    G.setRotationAngle("omega", 7.0);
    rotA = G.getR();

    std::vector<double> angles = G.getEulerAngles("yzy");

    G.setRotationAngle("phi", angles[2]);
    G.setRotationAngle("chi", angles[1]);
    G.setRotationAngle("omega", angles[0]);

    // Those goniometer angles re-create the initial rotation matrix.
    TS_ASSERT(rotA.equals(G.getR(), 0.0001));
  }

  void test_getEulerAngles2() {
    Goniometer G;
    DblMatrix rotA;
    std::vector<double> angles;
    G.makeUniversalGoniometer();
    for (double phi = -172.; phi <= 180.; phi += 30.)
      for (double chi = -171.; chi <= 180.; chi += 30.)
        for (double omega = -175.3; omega <= 180.; omega += 30.) {
          G.setRotationAngle("phi", phi);
          G.setRotationAngle("chi", chi);
          G.setRotationAngle("omega", omega);
          rotA = G.getR();
          angles = G.getEulerAngles("yzy");
          G.setRotationAngle("phi", angles[2]);
          G.setRotationAngle("chi", angles[1]);
          G.setRotationAngle("omega", angles[0]);
          TS_ASSERT(rotA.equals(G.getR(), 0.0001));
        }
  }

  void test_calcFromQSampleAndWavelength() {
    Goniometer G;
    double wavelength = 2 * M_PI; // k=1
    DblMatrix R;
    V3D Q;

    // 0 degree rotation
    Q = V3D(-1, 0, 1);
    G.calcFromQSampleAndWavelength(Q, wavelength);
    R = G.getR();
    TS_ASSERT_DELTA(R[0][0], 1.0, 0.001);
    TS_ASSERT_DELTA(R[0][1], 0.0, 0.001);
    TS_ASSERT_DELTA(R[0][2], 0.0, 0.001);
    TS_ASSERT_DELTA(R[1][0], 0.0, 0.001);
    TS_ASSERT_DELTA(R[1][1], 1.0, 0.001);
    TS_ASSERT_DELTA(R[1][2], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][0], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][1], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][2], 1.0, 0.001);

    // -90 degree rotation
    Q = V3D(1, 0, 1);
    G.calcFromQSampleAndWavelength(Q, wavelength);
    R = G.getR();
    TS_ASSERT_DELTA(R[0][0], 0.0, 0.001);
    TS_ASSERT_DELTA(R[0][1], 0.0, 0.001);
    TS_ASSERT_DELTA(R[0][2], -1.0, 0.001);
    TS_ASSERT_DELTA(R[1][0], 0.0, 0.001);
    TS_ASSERT_DELTA(R[1][1], 1.0, 0.001);
    TS_ASSERT_DELTA(R[1][2], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][0], 1.0, 0.001);
    TS_ASSERT_DELTA(R[2][1], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][2], 0.0, 0.001);

    // 30 degree rotation
    wavelength = 1.54;
    Q = V3D(-0.63523489, -0.12302677, -0.29517982);
    G.calcFromQSampleAndWavelength(Q, wavelength);
    R = G.getR();
    TS_ASSERT_DELTA(R[0][0], 0.866, 0.001);
    TS_ASSERT_DELTA(R[0][1], 0.0, 0.001);
    TS_ASSERT_DELTA(R[0][2], 0.5, 0.01);
    TS_ASSERT_DELTA(R[1][0], 0.0, 0.001);
    TS_ASSERT_DELTA(R[1][1], 1.0, 0.001);
    TS_ASSERT_DELTA(R[1][2], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][0], -0.5, 0.01);
    TS_ASSERT_DELTA(R[2][1], 0.0, 0.001);
    TS_ASSERT_DELTA(R[2][2], 0.866, 0.001);
  }

  void test_getConventionFromMotorAxes() {
    Goniometer G;
    TS_ASSERT_EQUALS(G.getConventionFromMotorAxes(), "");
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis0", 0., 0., 1., 0));
    TS_ASSERT_EQUALS(G.getConventionFromMotorAxes(), "");
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis1", 0., 1., 0., 0));
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis2", 1., 0., 0., 0));
    TS_ASSERT_EQUALS(G.getConventionFromMotorAxes(), "ZYX");
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis3", 1., 0., 0., 0));
    TS_ASSERT_EQUALS(G.getConventionFromMotorAxes(), "");
  }

  /** Save and load to NXS file */
  void test_nexus() {
    NexusTestHelper th(true);
    th.createFile("GoniometerTest.nxs");

    Goniometer G;
    G.makeUniversalGoniometer();
    G.setRotationAngle("phi", 45.0);
    G.setRotationAngle("chi", 23.0);
    G.setRotationAngle("omega", 7.0);
    G.saveNexus(th.file.get(), "goniometer");

    // Reload from the file
    th.reopenFile();
    Goniometer G2;
    G2.loadNexus(th.file.get(), "goniometer");
    TS_ASSERT_EQUALS(G2.getNumberAxes(), 3);
    // Rotation matrices should be the same after loading
    TS_ASSERT_EQUALS(G2.getR(), G.getR());
  }

  void test_equals_when_identical() {
    Mantid::Kernel::DblMatrix rotation_x{{1, 0, 0, 0, 0, -1, 0, 1, 0}}; // 90 degrees around x axis
    Goniometer a(rotation_x);
    Goniometer b(rotation_x);
    TS_ASSERT_EQUALS(a, b);
    TS_ASSERT(!(a != b));
  }

  void test_not_equals_when_not_identical() {
    Mantid::Kernel::DblMatrix rotation_x{{1, 0, 0, 0, 0, -1, 0, 1, 0}}; // 90 degrees around x axis
    Mantid::Kernel::DblMatrix rotation_y{{0, 0, 1, 0, 1, 0, -1, 0, 0}}; // 90 degrees around y axis
    Goniometer a(rotation_x);
    Goniometer b(rotation_y);
    TS_ASSERT_DIFFERS(a, b);
    TS_ASSERT(!(a == b));
  }
};
