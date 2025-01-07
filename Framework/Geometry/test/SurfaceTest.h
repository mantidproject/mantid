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

#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/V3D.h"

using namespace Mantid;
using namespace Geometry;

class SurfaceTest : public CxxTest::TestSuite {

public:
  void testConeDistance()
  /**
    Test the distance of a point from the cone
  */
  {
    std::vector<std::string> ConeStr{"kx 0 1", "k/x 0 0 0 1"}; // cone at origin, also cone at origin
    Kernel::V3D P(-1, -1.2, 0);
    const double results[] = {sin(atan(1.2) - M_PI / 4) * sqrt(2.44), sin(atan(1.2) - M_PI / 4) * sqrt(2.44)};

    std::vector<std::string>::const_iterator vc;
    Cone A;
    int cnt(0);
    for (vc = ConeStr.begin(); vc != ConeStr.end(); vc++, cnt++) {
      TS_ASSERT_EQUALS(A.setSurface(*vc), 0);
      TS_ASSERT_EQUALS(extractString(A), "-1  kx 0 1\n");

      const double R = A.distance(P);
      TS_ASSERT_DELTA(R, results[cnt], 1e-5);
    }
  }

private:
  std::string extractString(Surface &pv) {
    // dump output to sting
    std::ostringstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }
};
