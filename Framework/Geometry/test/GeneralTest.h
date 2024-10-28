// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Surfaces/General.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Geometry;

class GeneralTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    General A;
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
  }

  void testSetSurface() {
    General A;
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
    A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1"); // A Sphere equation
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
  }

  void testConstructorGeneral() {
    General A;
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
    A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1");
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
  }

  void testClone() {
    General A;
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
    A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1");
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
    auto B = A.clone();
    TS_ASSERT_EQUALS(extractString(*B), "-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
  }

  void testEqualOperator() {
    General A;
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  0  0  0  0  0  0  0  0  0  0 \n");
    A.setSurface("gq 1 1 1 0 0 0 0 0 0 -1");
    TS_ASSERT_EQUALS(extractString(A), "-1 gq  1  1  1  0  0  0  0  0  0  -1 \n");
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
