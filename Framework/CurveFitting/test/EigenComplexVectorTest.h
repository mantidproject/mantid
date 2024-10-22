// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EigenComplexVector.h"

using namespace Mantid::CurveFitting;

namespace {
const ComplexType v10{5, 0.5};
const ComplexType v11{55, 0.55};
const ComplexType v12{555, 0.555};

const ComplexType v20{3, 0.3};
const ComplexType v21{33, 0.33};
const ComplexType v22{333, 0.333};

const ComplexType v30{1, 0.1};
const ComplexType v31{11, 0.11};
} // namespace

class EigenComplexVectorTest : public CxxTest::TestSuite {
public:
  void test_create_ComplexVector() {
    {
      ComplexVector v;
      TS_ASSERT_EQUALS(v.size(), 1);
    }
    {
      ComplexVector v(1);
      TS_ASSERT_EQUALS(v.size(), 1);
    }
    {
      ComplexVector v(2);
      TS_ASSERT_EQUALS(v.size(), 2);
    }
  }

  void test_copy_constructor_complexVec() {
    auto v = makeVector1_complex();
    ComplexVector gv(v);
    ComplexVector gc(gv);
    TS_ASSERT_EQUALS(gc.size(), 3);
    TS_ASSERT_EQUALS(gc.get(0), v10);
    TS_ASSERT_EQUALS(gc.get(1), v11);
    TS_ASSERT_EQUALS(gc.get(2), v12);
  }

  void test_move_constructor_complexVec() {
    auto v = makeVector1_complex();
    ComplexVector gv(v);
    ComplexVector gm(std::move(gv));
    // test that data has been transferred
    TS_ASSERT_EQUALS(gm.size(), 3);
    TS_ASSERT_EQUALS(gm.get(0), v10);
    TS_ASSERT_EQUALS(gm.get(1), v11);
    TS_ASSERT_EQUALS(gm.get(2), v12);
    // test gv data has been deleted by move
    TS_ASSERT_EQUALS(gv.eigen().size(), 0);
  }

  void test_assignment_operator_complexVec() {
    auto v = makeVector1_complex();
    ComplexVector gv(v);
    ComplexVector gc;
    gc = gv;
    TS_ASSERT_EQUALS(gc.size(), 3);
    TS_ASSERT_EQUALS(gc.get(0), v10);
    TS_ASSERT_EQUALS(gc.get(1), v11);
    TS_ASSERT_EQUALS(gc.get(2), v12);
  }

  void test_move_assignment_operator_complexVec() {
    auto v = makeVector1_complex();
    ComplexVector gv(v);
    ComplexVector gm;
    gm = std::move(gv);
    // test that data has been transferred
    TS_ASSERT_EQUALS(gm.size(), 3);
    TS_ASSERT_EQUALS(gm.get(0), v10);
    TS_ASSERT_EQUALS(gm.get(1), v11);
    TS_ASSERT_EQUALS(gm.get(2), v12);
    // test gv data has been deleted by move
    TS_ASSERT_EQUALS(gv.eigen().size(), 1);
  }

  void test_zero_complexVec() {
    auto v = makeVector1_complex();
    ComplexVector gv(v);
    gv.zero();
    ComplexType z(0, 0);
    TS_ASSERT_EQUALS(gv.get(0), z);
    TS_ASSERT_EQUALS(gv.get(1), z);
    TS_ASSERT_EQUALS(gv.get(2), z);
  }

  void test_set_get_complexVec() {
    ComplexVector gv(3);
    ComplexType a(9, 0.9), b(7, 0.7), c(3, 0.3);
    gv.set(0, a);
    gv.set(1, b);
    gv.set(2, c);
    TS_ASSERT_EQUALS(gv.get(0), a);
    TS_ASSERT_EQUALS(gv.get(1), b);
    TS_ASSERT_EQUALS(gv.get(2), c);
  }

  void test_square_brackets_complexVec() {
    auto v = makeVector1_complex();
    ComplexType a = v[0];
    ComplexType b = v[1];
    ComplexType c = v[2];
    TS_ASSERT_EQUALS(a, v10);
    TS_ASSERT_EQUALS(b, v11);
    TS_ASSERT_EQUALS(c, v12);
    v[0] = v20;
    v[1] = v21;
    v[2] = v22;
    a = v[0];
    b = v[1];
    c = v[2];
    TS_ASSERT_EQUALS(a, v20);
    TS_ASSERT_EQUALS(b, v21);
    TS_ASSERT_EQUALS(c, v22);
  }

  void test_gsl_complexVec() {
    ComplexVector gv(3);
    gv.set(0, 9.9);
    gv.set(1, 7.7);
    gv.set(2, 3.3);

    auto gslVec = gv.eigen();
    auto a = gslVec(0);
    auto b = gslVec(1);
    auto c = gslVec(2).real();
    TS_ASSERT_EQUALS(a, 9.9);
    TS_ASSERT_EQUALS(b, 7.7);
    TS_ASSERT_EQUALS(c, 3.3);
  }

  void test_resize_complexVec() {
    ComplexVector gv(3);
    gv.set(0, 9.9);
    gv.set(1, 7.7);
    gv.set(2, 3.3);

    gv.resize(5);
    TS_ASSERT_EQUALS(gv.size(), 5);
    TS_ASSERT_EQUALS(gv.get(0), 9.9);
    TS_ASSERT_EQUALS(gv.get(1), 7.7);
    TS_ASSERT_EQUALS(gv.get(2), 3.3);
    TS_ASSERT_EQUALS(gv.get(3), 0.0);
    TS_ASSERT_EQUALS(gv.get(4), 0.0);

    gv.set(3, ComplexType(22, 0.22));
    gv.set(4, ComplexType(44, 0.44));
    TS_ASSERT_EQUALS(gv.get(3), ComplexType(22, 0.22));
    TS_ASSERT_EQUALS(gv.get(4), ComplexType(44, 0.44));

    gv.resize(2);
    TS_ASSERT_EQUALS(gv.size(), 2);
    TS_ASSERT_EQUALS(gv.get(0), 9.9);
    TS_ASSERT_EQUALS(gv.get(1), 7.7);
  }

  void test_plus_operator_complexVec() {
    auto v1 = makeVector1_complex();
    auto v2 = makeVector2_complex();
    ComplexType res_1 = v1.get(0) + v2.get(0);
    ComplexType res_2 = v1.get(1) + v2.get(1);
    ComplexType res_3 = v1.get(2) + v2.get(2);

    v1 += v2;
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1.get(0), res_1);
    TS_ASSERT_EQUALS(v1.get(1), res_2);
    TS_ASSERT_EQUALS(v1.get(2), res_3);

    TS_ASSERT_THROWS(v1 += makeVector3_complex(), const std::runtime_error &);
  }

  void test_minus_operator_complexVec() {
    auto v1 = makeVector1_complex();
    auto v2 = makeVector2_complex();
    ComplexType res_1 = v1.get(0) - v2.get(0);
    ComplexType res_2 = v1.get(1) - v2.get(1);
    ComplexType res_3 = v1.get(2) - v2.get(2);

    v1 -= v2;
    TS_ASSERT_EQUALS(v1.size(), 3);
    TS_ASSERT_EQUALS(v1.get(0), res_1);
    TS_ASSERT_EQUALS(v1.get(1), res_2);
    TS_ASSERT_EQUALS(v1.get(2), res_3);

    TS_ASSERT_THROWS(v1 -= makeVector3_complex(), const std::runtime_error &);
  }

  void test_times_operator_complexVec() {
    auto v1 = makeVector1_complex();
    double scale = 2.2;
    ComplexType res_1 = v1.get(0) * scale;
    ComplexType res_2 = v1.get(1) * scale;
    ComplexType res_3 = v1.get(2) * scale;

    v1 *= scale;
    TS_ASSERT_EQUALS(v1.size(), 3.0);
    TS_ASSERT_EQUALS(v1.get(0), res_1);
    TS_ASSERT_EQUALS(v1.get(1), res_2);
    TS_ASSERT_EQUALS(v1.get(2), res_3);
  }

  void test_sort_by_indicies() {
    auto v1 = makeVector1_complex();
    std::vector<size_t> indiciesVec = {2, 0, 1};

    v1.sort(indiciesVec);
    TS_ASSERT_EQUALS(v1.size(), 3.0);
    TS_ASSERT_EQUALS(v1.get(0), v12);
    TS_ASSERT_EQUALS(v1.get(1), v10);
    TS_ASSERT_EQUALS(v1.get(2), v11);
  }

  ComplexVector makeVector1_complex() {
    ComplexVector v(3);
    v.set(0, v10);
    v.set(1, v11);
    v.set(2, v12);
    return v;
  }

  ComplexVector makeVector2_complex() {
    ComplexVector v(3);
    v.set(0, v20);
    v.set(1, v21);
    v.set(2, v22);
    return v;
  }

  ComplexVector makeVector3_complex() {
    ComplexVector v(2);
    v.set(0, v30);
    v.set(1, v31);
    return v;
  }
};
