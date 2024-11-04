// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/Math/BnId.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <vector>

using namespace Mantid;
using namespace Geometry;

class BnIdTest : public CxxTest::TestSuite {
public:
  void testDefaultConstructor() {
    BnId A;
    TS_ASSERT_EQUALS(A.TrueCount(), 0);
    TS_ASSERT_EQUALS(A.Size(), 0);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 0);
    TS_ASSERT_EQUALS(A.intValue(), 0);
    TS_ASSERT_EQUALS(extractString(A), "(0:0)");
  }

  void testConstructor() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(A.intValue(), 224);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
  }

  void testAnotherConstructor() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(A.intValue(), 224);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    BnId B(A);
    TS_ASSERT_EQUALS(B.TrueCount(), 3);
    TS_ASSERT_EQUALS(B.Size(), 8);
    TS_ASSERT_EQUALS(B.PIstatus(), 1);
    TS_ASSERT_EQUALS(B.expressCount(), 8);
    TS_ASSERT_EQUALS(B.intValue(), 224);
    TS_ASSERT_EQUALS(extractString(B), "11100000(3:0)");
  }

  void testItem() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    TS_ASSERT_EQUALS(A[0], -1);
    TS_ASSERT_EQUALS(A[1], -1);
    TS_ASSERT_EQUALS(A[2], -1);
    TS_ASSERT_EQUALS(A[3], -1);
    TS_ASSERT_EQUALS(A[4], -1);
    TS_ASSERT_EQUALS(A[5], 1);
    TS_ASSERT_EQUALS(A[6], 1);
    TS_ASSERT_EQUALS(A[7], 1);
  }

  void testComparing() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    BnId B(A);
    TS_ASSERT_EQUALS(B.TrueCount(), 3);
    TS_ASSERT_EQUALS(B.Size(), 8);
    TS_ASSERT_EQUALS(B.PIstatus(), 1);
    TS_ASSERT_EQUALS(B.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(B), "11100000(3:0)");
    BnId C(8, 240);
    TS_ASSERT_EQUALS(C.TrueCount(), 4);
    TS_ASSERT_EQUALS(C.Size(), 8);
    TS_ASSERT_EQUALS(C.PIstatus(), 1);
    TS_ASSERT_EQUALS(C.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(C), "11110000(4:0)");
    TS_ASSERT(A == B);
    TS_ASSERT(A < C);
    TS_ASSERT(C > B);
    TS_ASSERT_EQUALS(A.equivalent(B), 1);
    TS_ASSERT_EQUALS(A.equivalent(C), 0);
  }

  void testIncrement() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    A++;
    TS_ASSERT_EQUALS(A.TrueCount(), 4);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100001(4:0)");
  }

  void testDecrement() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    A--;
    TS_ASSERT_EQUALS(A.TrueCount(), 7);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11011111(7:0)");
  }

  void testReverse() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    A.reverse();
    TS_ASSERT_EQUALS(A.TrueCount(), 5);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "00011111(5:0)");
  }

  void testMakeCombination() {
    BnId A(8, 224);
    TS_ASSERT_EQUALS(A.TrueCount(), 3);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100000(3:0)");
    BnId B(A);
    BnId C(8, 240);
    TS_ASSERT_EQUALS(C.TrueCount(), 4);
    TS_ASSERT_EQUALS(C.Size(), 8);
    TS_ASSERT_EQUALS(C.PIstatus(), 1);
    TS_ASSERT_EQUALS(C.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(C), "11110000(4:0)");
    std::pair<int, BnId> result = A.makeCombination(B);
    TS_ASSERT_EQUALS(result.first, 0);
    TS_ASSERT(result.second == BnId());
    result = A.makeCombination(C);
    TS_ASSERT_EQUALS(result.first, 1);
    TS_ASSERT_EQUALS(extractString(result.second), "111-0000(3:1)");
    BnId D(8, 158);
    TS_ASSERT_EQUALS(extractString(D), "10011110(5:0)");
    result = A.makeCombination(D);
    TS_ASSERT_EQUALS(result.first, -1);
    TS_ASSERT(result.second == BnId());
    BnId E(9, 240);
    TS_ASSERT_EQUALS(extractString(E), "011110000(4:0)");
    result = A.makeCombination(E);
    TS_ASSERT_EQUALS(result.first, -1);
    TS_ASSERT(result.second == BnId());
  }

  void testMapState() {
    BnId A(8, 225);
    TS_ASSERT_EQUALS(A.TrueCount(), 4);
    TS_ASSERT_EQUALS(A.Size(), 8);
    TS_ASSERT_EQUALS(A.PIstatus(), 1);
    TS_ASSERT_EQUALS(A.expressCount(), 8);
    TS_ASSERT_EQUALS(extractString(A), "11100001(4:0)");
    std::vector<int> index{3, 1, 4, 2, 5, 1, 7};
    std::map<int, int> result;
    A.mapState(index, result);
    TS_ASSERT_EQUALS(result[3], 1);
    TS_ASSERT_EQUALS(result[4], 0);
    TS_ASSERT_EQUALS(result[2], 0);
    TS_ASSERT_EQUALS(result[5], 0);
    TS_ASSERT_EQUALS(result[1], 1);
    TS_ASSERT_EQUALS(result[7], 1);
  }

private:
  std::string extractString(BnId &rc) {
    std::ostringstream output;
    rc.write(output);
    return output.str();
  }
};
