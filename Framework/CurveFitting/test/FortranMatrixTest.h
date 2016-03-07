#ifndef MANTID_CURVEFITTING_FORTRANMATRIXTEST_H_
#define MANTID_CURVEFITTING_FORTRANMATRIXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ComplexMatrix.h"
#include "MantidCurveFitting/FortranMatrix.h"
#include "MantidCurveFitting/GSLMatrix.h"

using Mantid::CurveFitting::FortranMatrix;
using Mantid::CurveFitting::GSLMatrix;
using Mantid::CurveFitting::ComplexMatrix;
using Mantid::CurveFitting::ComplexType;

typedef FortranMatrix<GSLMatrix> DoubleFortranMatrix;
typedef FortranMatrix<ComplexMatrix> ComplexFortranMatrix;

class FortranMatrixTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FortranMatrixTest *createSuite() { return new FortranMatrixTest(); }
  static void destroySuite(FortranMatrixTest *suite) { delete suite; }

  void test_double_c_indexing() {
    DoubleFortranMatrix m(3, 3);
    m(0, 0) = 0.0;
    m(0, 1) = 1.0;
    m(0, 2) = 2.0;
    m(1, 0) = 10.0;
    m(1, 1) = 11.0;
    m(1, 2) = 12.0;
    m(2, 0) = 20.0;
    m(2, 1) = 21.0;
    m(2, 2) = 22.0;

    TS_ASSERT_EQUALS(m(0, 0), 0.0);
    TS_ASSERT_EQUALS(m(0, 1), 1.0);
    TS_ASSERT_EQUALS(m(0, 2), 2.0);
    TS_ASSERT_EQUALS(m(1, 0), 10.0);
    TS_ASSERT_EQUALS(m(1, 1), 11.0);
    TS_ASSERT_EQUALS(m(1, 2), 12.0);
    TS_ASSERT_EQUALS(m(2, 0), 20.0);
    TS_ASSERT_EQUALS(m(2, 1), 21.0);
    TS_ASSERT_EQUALS(m(2, 2), 22.0);
  }

  void test_double_fortran_indexing() {
    DoubleFortranMatrix m(2, 4, -1, 1);
    m(2,-1) = 0.0;
    m(2, 0) = 1.0;
    m(2, 1) = 2.0;
    m(3,-1) = 10.0;
    m(3, 0) = 11.0;
    m(3, 1) = 12.0;
    m(4,-1) = 20.0;
    m(4, 0) = 21.0;
    m(4, 1) = 22.0;

    TS_ASSERT_EQUALS(m.get(0, 0), 0.0);
    TS_ASSERT_EQUALS(m.get(0, 1), 1.0);
    TS_ASSERT_EQUALS(m.get(0, 2), 2.0);
    TS_ASSERT_EQUALS(m.get(1, 0), 10.0);
    TS_ASSERT_EQUALS(m.get(1, 1), 11.0);
    TS_ASSERT_EQUALS(m.get(1, 2), 12.0);
    TS_ASSERT_EQUALS(m.get(2, 0), 20.0);
    TS_ASSERT_EQUALS(m.get(2, 1), 21.0);
    TS_ASSERT_EQUALS(m.get(2, 2), 22.0);

    TS_ASSERT_EQUALS(m(2,-1), 0.0);
    TS_ASSERT_EQUALS(m(2, 0), 1.0);
    TS_ASSERT_EQUALS(m(2, 1), 2.0);
    TS_ASSERT_EQUALS(m(3,-1), 10.0);
    TS_ASSERT_EQUALS(m(3, 0), 11.0);
    TS_ASSERT_EQUALS(m(3, 1), 12.0);
    TS_ASSERT_EQUALS(m(4,-1), 20.0);
    TS_ASSERT_EQUALS(m(4, 0), 21.0);
    TS_ASSERT_EQUALS(m(4, 1), 22.0);
  }

  void test_complex_c_indexing() {
    ComplexFortranMatrix m(2, 2);
    ComplexType v11{11, 0.11};
    ComplexType v12{12, 0.12};
    ComplexType v21{21, 0.21};
    ComplexType v22{22, 0.22};

    TS_ASSERT_EQUALS(m.size1(), 2);
    TS_ASSERT_EQUALS(m.size2(), 2);
    m(0, 0) = v11;
    m(0, 1) = v12;
    m(1, 0) = v21;
    m(1, 1) = v22;

    TS_ASSERT(m.get(0, 0) == v11);
    TS_ASSERT(m.get(0, 1) == v12);
    TS_ASSERT(m.get(1, 0) == v21);
    TS_ASSERT(m.get(1, 1) == v22);
  }

  void test_complex_fortran_indexing() {
    ComplexFortranMatrix m(2, 3, -2, -1);
    ComplexType v11{11, 0.11};
    ComplexType v12{12, 0.12};
    ComplexType v21{21, 0.21};
    ComplexType v22{22, 0.22};

    TS_ASSERT_EQUALS(m.size1(), 2);
    TS_ASSERT_EQUALS(m.size2(), 2);
    m(2, -2) = v11;
    m(2, -1) = v12;
    m(3, -2) = v21;
    m(3, -1) = v22;

    TS_ASSERT(m(2, -2) == v11);
    TS_ASSERT(m(2, -1) == v12);
    TS_ASSERT(m(3, -2) == v21);
    TS_ASSERT(m(3, -1) == v22);

    TS_ASSERT(m.get(0, 0) == v11);
    TS_ASSERT(m.get(0, 1) == v12);
    TS_ASSERT(m.get(1, 0) == v21);
    TS_ASSERT(m.get(1, 1) == v22);
  }
};

#endif /* MANTID_CURVEFITTING_FORTRANMATRIXTEST_H_ */