#ifndef MANTID_CURVEFITTING_FORTRANVECTORTEST_H_
#define MANTID_CURVEFITTING_FORTRANVECTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FortranVector.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/ComplexVector.h"


using Mantid::CurveFitting::FortranVector;
using Mantid::CurveFitting::ComplexType;
typedef FortranVector<Mantid::CurveFitting::GSLVector> FortranDoubleVector;
typedef FortranVector<Mantid::CurveFitting::ComplexVector> FortranComplexVector;

class FortranVectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FortranVectorTest *createSuite() { return new FortranVectorTest(); }
  static void destroySuite( FortranVectorTest *suite ) { delete suite; }

  void test_double_c_indexing()
  {
    FortranDoubleVector v(3);
    v[0] = 1;
    v[1] = 2;
    v[2] = 3;

    TS_ASSERT_EQUALS(v[0], 1);
    TS_ASSERT_EQUALS(v[1], 2);
    TS_ASSERT_EQUALS(v[2], 3);

    TS_ASSERT_EQUALS(v.get(0), 1);
    TS_ASSERT_EQUALS(v.get(1), 2);
    TS_ASSERT_EQUALS(v.get(2), 3);
  }

  void test_double_fortran_indexing()
  {
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


  void test_complex_c_indexing()
  {
    ComplexType v1{1, 0.1};
    ComplexType v2{2, 0.2};
    ComplexType v3{3, 0.3};

    FortranComplexVector v(3);
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;

    TS_ASSERT_EQUALS(v[0], v1);
    TS_ASSERT_EQUALS(v[1], v2);
    TS_ASSERT_EQUALS(v[2], v3);

    TS_ASSERT_EQUALS(v(0), v1);
    TS_ASSERT_EQUALS(v(1), v2);
    TS_ASSERT_EQUALS(v(2), v3);

    TS_ASSERT_EQUALS(v.get(0), v1);
    TS_ASSERT_EQUALS(v.get(1), v2);
    TS_ASSERT_EQUALS(v.get(2), v3);
  }

  void test_complex_fortran_indexing()
  {
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

};


#endif /* MANTID_CURVEFITTING_FORTRANVECTORTEST_H_ */