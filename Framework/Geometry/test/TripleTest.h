// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Math/Triple.h"
#include "MantidKernel/Logger.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;

class TripleTest : public CxxTest::TestSuite {

public:
  void testEmptyConstructor() {
    Triple<int> A;
    TS_ASSERT_EQUALS(A[0], 0);
    TS_ASSERT_EQUALS(A[1], 0);
    TS_ASSERT_EQUALS(A[2], 0);
  }

  void testDefaultConstructor() {
    Triple<int> A(1, 2, 3);
    TS_ASSERT_EQUALS(A[0], 1);
    TS_ASSERT_EQUALS(A[1], 2);
    TS_ASSERT_EQUALS(A[2], 3);
  }

  void testTripleConstructor() {
    Triple<int> A(1, 2, 3);
    Triple<int> B(A);
    TS_ASSERT_EQUALS(B[0], 1);
    TS_ASSERT_EQUALS(B[1], 2);
    TS_ASSERT_EQUALS(B[2], 3);
  }

  void testAssignment() {
    Triple<int> A(1, 2, 3);
    Triple<int> B;
    TS_ASSERT_EQUALS(B[0], 0);
    TS_ASSERT_EQUALS(B[1], 0);
    TS_ASSERT_EQUALS(B[2], 0);
    B = A;
    TS_ASSERT_EQUALS(B[0], 1);
    TS_ASSERT_EQUALS(B[1], 2);
    TS_ASSERT_EQUALS(B[2], 3);
  }

  void testLessthan() {
    Triple<int> A(1, 2, 3);
    Triple<int> B(0, 1, 2);
    TS_ASSERT(!(A < B));
    TS_ASSERT(B < A);
  }

  void testGreaterThan() {
    Triple<int> A(1, 2, 3);
    Triple<int> B(0, 1, 2);
    TS_ASSERT(A > B);
    TS_ASSERT(!(B > A));
  }

  void testEquality() {
    Triple<int> A(1, 2, 3);
    Triple<int> B(0, 1, 2);
    Triple<int> C(1, 2, 3);
    TS_ASSERT(!(A == B));
    TS_ASSERT(A == C);
  }

  void testDTriple() {
    DTriple<int, int, std::string> A, B;
    TS_ASSERT(A == B);
    TS_ASSERT(!(A < B));
    TS_ASSERT(!(A > B));
    DTriple<int, int, std::string> C(1, 2, "test");
    A = C;
    TS_ASSERT(A == C);
    TS_ASSERT(!(A == B));
    TS_ASSERT(!(A < B));
    TS_ASSERT(A > B);
    DTriple<int, int, std::string> D(2, 3, "rest");
    B = D;
    TS_ASSERT(B == D);
    TS_ASSERT(!(A == B));
    TS_ASSERT(A < B);
    TS_ASSERT(!(A > B));
  }
};
