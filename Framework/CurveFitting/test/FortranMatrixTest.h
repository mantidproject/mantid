#ifndef MANTID_CURVEFITTING_FORTRANMATRIXTEST_H_
#define MANTID_CURVEFITTING_FORTRANMATRIXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FortranMatrix.h"
#include "MantidCurveFitting/GSLMatrix.h"

using Mantid::CurveFitting::FortranMatrix;
using Mantid::CurveFitting::GSLMatrix;

typedef FortranMatrix<GSLMatrix> DoubleMatrix;

class FortranMatrixTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FortranMatrixTest *createSuite() { return new FortranMatrixTest(); }
  static void destroySuite( FortranMatrixTest *suite ) { delete suite; }


  void test_Something()
  {
    DoubleMatrix M(2, 2);
    M(0,0) = 11.;
    std::cerr << M(0,0) << std::endl;
  }


};


#endif /* MANTID_CURVEFITTING_FORTRANMATRIXTEST_H_ */