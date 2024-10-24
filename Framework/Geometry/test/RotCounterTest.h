// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/Math/RotCounter.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <vector>

using namespace Mantid;
using namespace Geometry;

class RotCounterTest : public CxxTest::TestSuite {
public:
  void testConstructor() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
  }

  void testIncrementOperator() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
    A++;
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:5 ");
    A++;
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:6 ");
  }

  void testDecrementOperator() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
    A--;
    TS_ASSERT_EQUALS(extractString(A), " 2:3:4:5:6 ");
    A--;
    TS_ASSERT_EQUALS(extractString(A), " 1:3:4:5:6 ");
  }

  void testRotConstructor() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
    A++;
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:5 ");
    RotaryCounter B(A);
    TS_ASSERT_EQUALS(extractString(B), " 0:1:2:3:5 ");
  }

  void testAssignment() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
    A++;
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:5 ");
    RotaryCounter B(5, 7);
    TS_ASSERT_EQUALS(extractString(B), " 0:1:2:3:4 ");
    B = A;
    TS_ASSERT_EQUALS(extractString(B), " 0:1:2:3:5 ");
  }

  void testElementOperator() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
    A++;
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:5 ");
    TS_ASSERT_EQUALS(A[0], 0);
    TS_ASSERT_EQUALS(A[1], 1);
    TS_ASSERT_EQUALS(A[2], 2);
    TS_ASSERT_EQUALS(A[3], 3);
    TS_ASSERT_EQUALS(A[4], 5);
  }

  void testComparatorOperator() {
    RotaryCounter A(5, 7);
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:4 ");
    A++;
    TS_ASSERT_EQUALS(extractString(A), " 0:1:2:3:5 ");
    RotaryCounter B(5, 7);
    TS_ASSERT_EQUALS(extractString(B), " 0:1:2:3:4 ");
    B = A;
    TS_ASSERT_EQUALS(extractString(B), " 0:1:2:3:5 ");
    A--;
    TS_ASSERT(A < B);
    TS_ASSERT(B > A);
    A++;
    TS_ASSERT(A == B);
  }

private:
  std::string extractString(RotaryCounter &rc) {
    std::ostringstream output;
    rc.write(output);
    return output.str();
  }
};
