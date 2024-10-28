// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <ostream>
#include <sstream>
#include <vector>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/SurfaceFactory.h"
#include "MantidKernel/V3D.h"

using namespace Mantid;
using namespace Geometry;

class SurfaceFactoryTest : public CxxTest::TestSuite {
public:
  void testCreateSurface() {
    SurfaceFactory *A;
    A = SurfaceFactory::Instance();
    auto P = A->createSurface("Plane");
    TS_ASSERT_EQUALS(extractString(*P), "-1 px 0\n");
    auto B = A->createSurface("Sphere");
    TS_ASSERT_EQUALS(extractString(*B), "-1 so 0\n");
    auto C = A->createSurface("Cylinder");
    TS_ASSERT_EQUALS(extractString(*C), "-1 cx 0\n");
    auto K = A->createSurface("Cone");
    TS_ASSERT_EQUALS(extractString(*K), "-1  kx 0 0\n");
  }

  void testCreateSurfaceID() {
    SurfaceFactory *A;
    A = SurfaceFactory::Instance();
    auto P = A->createSurfaceID("p");
    TS_ASSERT_EQUALS(extractString(*P), "-1 px 0\n");
    auto B = A->createSurfaceID("s");
    TS_ASSERT_EQUALS(extractString(*B), "-1 so 0\n");
    auto C = A->createSurfaceID("c");
    TS_ASSERT_EQUALS(extractString(*C), "-1 cx 0\n");
    auto K = A->createSurfaceID("k");
    TS_ASSERT_EQUALS(extractString(*K), "-1  kx 0 0\n");
  }

  void testProcessLine() {
    SurfaceFactory *A;
    A = SurfaceFactory::Instance();
    auto P = A->processLine("pz 5");
    Plane tP;
    tP.setSurface("pz 5");
    TS_ASSERT_EQUALS(extractString(*P), extractString(tP));
    auto S = A->processLine("s 1.1 -2.1 1.1 2");
    Sphere tS;
    tS.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_EQUALS(extractString(*S), extractString(tS));
    auto C = A->processLine("c/x 0.5 0.5 1.0");
    Cylinder tC;
    tC.setSurface("c/x 0.5 0.5 1.0");
    TS_ASSERT_EQUALS(extractString(*C), extractString(tC));
    auto K = A->processLine("k/x 1.0 1.0 1.0 1.0");
    Cone tK;
    tK.setSurface("k/x 1.0 1.0 1.0 1.0");
    TS_ASSERT_EQUALS(extractString(*K), extractString(tK));
  }

private:
  std::string extractString(const Surface &pv) {
    // dump output to sting
    std::ostringstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }
};
