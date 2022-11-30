// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EigenComplexVector.h"
#include "MantidCurveFitting/EigenFortranVector.h"
#include "MantidCurveFitting/EigenVector.h"

using Mantid::CurveFitting::ComplexType;
using Mantid::CurveFitting::FortranVector;
using FortranDoubleVector = FortranVector<Mantid::CurveFitting::EigenVector>;
using FortranComplexVector = FortranVector<Mantid::CurveFitting::ComplexVector>;

class EigenFortranVectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EigenFortranVectorTest *createSuite() { return new EigenFortranVectorTest(); }
  static void destroySuite(EigenFortranVectorTest *suite) { delete suite; }

  void test_double_c_indexing() {
    FortranDoubleVector v(3);
    v[1] = 1;
    v[2] = 2;
    v[3] = 3;

    TS_ASSERT_EQUALS(v[1], 1);
    TS_ASSERT_EQUALS(v[2], 2);
    TS_ASSERT_EQUALS(v[3], 3);

    TS_ASSERT_EQUALS(v.get(0), 1);
    TS_ASSERT_EQUALS(v.get(1), 2);
    TS_ASSERT_EQUALS(v.get(2), 3);
  }

  void test_double_fortran_indexing() {
    FortranDoubleVector v(-1, 1);
    v[-1] = 1;
    v[0] = 2;
    v[1] = 3;

    TS_ASSERT_EQUALS(v[-1], 1);
    TS_ASSERT_EQUALS(v[0], 2);
    TS_ASSERT_EQUALS(v[1], 3);

    TS_ASSERT_EQUALS(v(-1), 1);
    TS_ASSERT_EQUALS(v(0), 2);
    TS_ASSERT_EQUALS(v(1), 3);

    TS_ASSERT_EQUALS(v.get(0), 1);
    TS_ASSERT_EQUALS(v.get(1), 2);
    TS_ASSERT_EQUALS(v.get(2), 3);

    v(-1) = 11;
    v(0) = 22;
    v(1) = 33;

    TS_ASSERT_EQUALS(v[-1], 11);
    TS_ASSERT_EQUALS(v[0], 22);
    TS_ASSERT_EQUALS(v[1], 33);

    TS_ASSERT_EQUALS(v(-1), 11);
    TS_ASSERT_EQUALS(v(0), 22);
    TS_ASSERT_EQUALS(v(1), 33);

    TS_ASSERT_EQUALS(v.get(0), 11);
    TS_ASSERT_EQUALS(v.get(1), 22);
    TS_ASSERT_EQUALS(v.get(2), 33);
  }

  void test_complex_c_indexing() {
    ComplexType v1{1, 0.1};
    ComplexType v2{2, 0.2};
    ComplexType v3{3, 0.3};

    FortranComplexVector v(3);
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;

    TS_ASSERT_EQUALS(v[1], v1);
    TS_ASSERT_EQUALS(v[2], v2);
    TS_ASSERT_EQUALS(v[3], v3);

    TS_ASSERT_EQUALS(v(1), v1);
    TS_ASSERT_EQUALS(v(2), v2);
    TS_ASSERT_EQUALS(v(3), v3);

    TS_ASSERT_EQUALS(v.get(0), v1);
    TS_ASSERT_EQUALS(v.get(1), v2);
    TS_ASSERT_EQUALS(v.get(2), v3);
  }

  void test_complex_fortran_indexing() {
    ComplexType v1{1, 0.1};
    ComplexType v2{2, 0.2};
    ComplexType v3{3, 0.3};

    ComplexType v11{11, 0.11};
    ComplexType v22{22, 0.22};
    ComplexType v33{33, 0.33};

    FortranComplexVector v(-1, 1);
    v[-1] = v1;
    v[0] = v2;
    v[1] = v3;

    TS_ASSERT_EQUALS(v[-1], v1);
    TS_ASSERT_EQUALS(v[0], v2);
    TS_ASSERT_EQUALS(v[1], v3);

    TS_ASSERT_EQUALS(v(-1), v1);
    TS_ASSERT_EQUALS(v(0), v2);
    TS_ASSERT_EQUALS(v(1), v3);

    TS_ASSERT_EQUALS(v.get(0), v1);
    TS_ASSERT_EQUALS(v.get(1), v2);
    TS_ASSERT_EQUALS(v.get(2), v3);

    v(-1) = v11;
    v(0) = v22;
    v(1) = v33;

    TS_ASSERT_EQUALS(v[-1], v11);
    TS_ASSERT_EQUALS(v[0], v22);
    TS_ASSERT_EQUALS(v[1], v33);

    TS_ASSERT_EQUALS(v(-1), v11);
    TS_ASSERT_EQUALS(v(0), v22);
    TS_ASSERT_EQUALS(v(1), v33);

    TS_ASSERT_EQUALS(v.get(0), v11);
    TS_ASSERT_EQUALS(v.get(1), v22);
    TS_ASSERT_EQUALS(v.get(2), v33);
  }

  void test_double_move() {
    FortranDoubleVector v(3);
    v[1] = 1;
    v[2] = 2;
    v[3] = 3;
    double *p = &v[1];
    auto vv = v.moveToBaseVector();
    TS_ASSERT_EQUALS(p, &vv[0]);
  }

  void test_complex_move() {
    FortranComplexVector v(3);
    v[1] = ComplexType{1, 0.1};
    v[2] = ComplexType{2, 0.2};
    v[3] = ComplexType{3, 0.3};
    auto p = v.eigen();
    auto vv = v.moveToBaseVector();
    TS_ASSERT_EQUALS(p, vv.eigen());
  }

  void test_allocate_double() {
    FortranDoubleVector v(3);
    v[1] = 0.1;
    v[2] = 0.2;
    v[3] = 0.3;
    v.allocate(2);
    TS_ASSERT_EQUALS(v.size(), 2);
    TS_ASSERT_EQUALS(v(1), 0.1);
    TS_ASSERT_EQUALS(v(2), 0.2);
    v.allocate(5);
    TS_ASSERT_EQUALS(v.size(), 5);
    TS_ASSERT_EQUALS(v(1), 0.1);
    TS_ASSERT_EQUALS(v(2), 0.2);

    v.allocate(2, 5);
    TS_ASSERT_EQUALS(v.size(), 4);
    TS_ASSERT_EQUALS(v(2), 0.1);
    TS_ASSERT_EQUALS(v(3), 0.2);
  }

  void test_allocate_complex() {
    FortranComplexVector v(3);
    v[1] = 0.1;
    v[2] = 0.2;
    v[3] = 0.3;
    v.allocate(2);
    TS_ASSERT_EQUALS(v.size(), 2);
    TS_ASSERT_EQUALS(v(1), 0.1);
    TS_ASSERT_EQUALS(v(2), 0.2);
    v.allocate(5);
    TS_ASSERT_EQUALS(v.size(), 5);
    TS_ASSERT_EQUALS(v(1), 0.1);
    TS_ASSERT_EQUALS(v(2), 0.2);

    v.allocate(2, 5);
    TS_ASSERT_EQUALS(v.size(), 4);
    TS_ASSERT_EQUALS(v(2), 0.1);
    TS_ASSERT_EQUALS(v(3), 0.2);
  }

  void test_add_double() {
    FortranDoubleVector v(3);
    v[1] = 0.1;
    v[2] = 0.2;
    v[3] = 0.3;
    v += 10;
    TS_ASSERT_EQUALS(v(1), 10.1);
    TS_ASSERT_EQUALS(v(2), 10.2);
    TS_ASSERT_EQUALS(v(3), 10.3);
  }

  void test_int_array() {
    using FortranIntVector = FortranVector<std::vector<int>>;
    FortranIntVector ivec(1, 3);
    ivec(1) = 11;
    ivec(2) = 22;
    ivec(3) = 33;

    TS_ASSERT_EQUALS(ivec(1), 11);
    TS_ASSERT_EQUALS(ivec(2), 22);
    TS_ASSERT_EQUALS(ivec(3), 33);

    ivec.allocate(-1, 1);
    TS_ASSERT_EQUALS(ivec(-1), 11);
    TS_ASSERT_EQUALS(ivec(0), 22);
    TS_ASSERT_EQUALS(ivec(1), 33);
  }
};
