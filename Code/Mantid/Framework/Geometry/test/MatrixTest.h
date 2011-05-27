#ifndef MANTID_TESTMATIX__
#define MANTID_TESTMATIX__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "MantidGeometry/Math/Matrix.h" 
#include "MantidGeometry/V3D.h"

using namespace Mantid;
using namespace Geometry;

class MatrixTest: public CxxTest::TestSuite
{

public:

  void makeMatrix(Matrix<double>& A) const
  {
    A.setMem(3,3);
    A[0][0]=1.0;
    A[1][0]=3.0; 
    A[0][1]=4.0;
    A[0][2]=6.0;
    A[2][0]=5.0;
    A[1][1]=3.0;
    A[2][1]=1.0;
    A[1][2]=6.0;
    A[2][2]=-7.0;
    return;
  }

  /**
  Test that a matrix can be inverted
  */
  void testInvert()
  {
    Matrix<double> A(3,3);

    A[0][0]=1.0;
    A[1][0]=3.0;
    A[0][1]=4.0;
    A[0][2]=6.0;
    A[2][0]=5.0;
    A[1][1]=3.0;
    A[2][1]=1.0;
    A[1][2]=6.0;
    A[2][2]=-7.0;

    TS_ASSERT_DELTA(A.Invert(),105.0,1e-5);
  }

  void testIdent()
  {
    Matrix<double> A(3,3);

    A[0][0]=1.0;
    A[1][0]=0.0;
    A[0][1]=0.0;
    A[0][2]=0.0;
    A[2][0]=0.0;
    A[1][1]=1.0;
    A[2][1]=0.0;
    A[1][2]=0.0;
    A[2][2]=1.0;

    Matrix<double> Ident(3,3);
    TS_ASSERT_DIFFERS(Ident,A);
    Ident.identityMatrix();
    TS_ASSERT_EQUALS(Ident,A);
  }

  /** Test of equals with a user-specified tolerance */
  void test_equals()
  {
    Matrix<double> A(3,3, true);
    Matrix<double> B(3,3, true);
    B[1][1] = 1.1;
    TS_ASSERT( !A.equals(B, 0.05) );
    TS_ASSERT( A.equals(B, 0.15) );
  }

  /**
  Check that we can swap rows and columns
  */
  void testSwapRows()
  {
    Matrix<double> A(3,3);
    makeMatrix(A);
    Matrix<double> B(A);
    A.swapRows(1,2);
    A.swapCols(1,2);
    TS_ASSERT_EQUALS(A[0][0],B[0][0]);
    TS_ASSERT_EQUALS(A[2][2],B[1][1]);
    // Plus all the others..
  }

  void testEigenvectors()
  {
    Matrix<double> Eval;
    Matrix<double> Diag;
    Matrix<double> A(3,3);  // NOTE: A must be symmetric
    A[0][0]=1.0;
    A[1][0]=A[0][1]=4.0;
    A[0][2]=A[2][0]=5.0;
    A[1][1]=3.0;
    A[2][1]=A[1][2]=6.0;
    A[2][2]=-7.0;
    TS_ASSERT(A.Diagonalise(Eval,Diag));

    Matrix<double> MA=A*Eval;
    Matrix<double> MV=Eval*Diag;
    Eval.sortEigen(Diag);
    TS_ASSERT(Diag[0][0]<Diag[1][1]);
    TS_ASSERT(Diag[1][1]<Diag[2][2]);
    TS_ASSERT(MA==MV);

    std::vector<double> X(3);
    X[0]=Eval[0][1];
    X[1]=Eval[1][1];
    X[2]=Eval[2][1];

    std::vector<double> out=A*X;
    transform(X.begin(),X.end(),X.begin(),std::bind2nd(std::multiplies<double>(),Diag[1][1]));
    TS_ASSERT_DELTA(X[0],out[0],0.0001);
    TS_ASSERT_DELTA(X[1],out[1],0.0001);
    TS_ASSERT_DELTA(X[2],out[2],0.0001);

  }

  /**
  Tests the diagonalisation  on a symmetric 2x2 matrix
  */
  void testDiagonalise()
  {
    Matrix<double> Eval;
    Matrix<double> Diag;
    Matrix<double> A(2,2);   // symmetric only
    A[0][0]=1.0;
    A[1][0]=3.0;
    A[0][1]=3.0;
    A[1][1]=4.0;
    TS_ASSERT(A.Diagonalise(Eval,Diag));  // returns 1 or 2

    Matrix<double> EvalT(Eval);
    EvalT.Transpose();
    Eval*=Diag;
    Eval*=EvalT;
    TS_ASSERT(Eval==A);
  }

  void testFromVectorThrows()
  {
	  std::vector<double> data(5,0);

	  TSM_ASSERT_THROWS("building matrix by this construcor and data with wrong number of elements should throw",(Matrix<double>(data)),std::invalid_argument);
  }


  void testFromVectorBuildCorrect()
  {
	  std::vector<int> data(9,0);
	  for(int i=0;i<9;i++){
		  data[i]=i;
	  }
	  Matrix<int> myMat;
	  TSM_ASSERT_THROWS_NOTHING("building matrix by this construcor and data with correct number of elements should not throw",myMat=Matrix<int>(data));

	  // and the range of the elements in the matrix is correct;
	  V3D rez1 = myMat*V3D(1,0,0);
	  V3D rez2 = myMat*V3D(0,1,0);
	  V3D rez3 = myMat*V3D(0,0,1);
	  TSM_ASSERT_EQUALS("The data in a matrix have to be located row-wise, so multiplication by (1,0,0)^T selects 1-st column ",true,V3D(0,3,6)==rez1);
	  TSM_ASSERT_EQUALS("The data in a matrix have to be located row-wise, so multiplication by (0,1,0)^T selects 2-nd column ",true,V3D(1,4,7)==rez2);
	  TSM_ASSERT_EQUALS("The data in a matrix have to be located row-wise, so multiplication by (0,0,1)^T selects 3-rd column ",true,V3D(2,5,8)==rez3);
  }

  void testToRotation()
  {
    /*
    |1  0  0|
    |1  2  0|
    |0  0 -3|
    transforms to
    |-s-s  0|
    |-s s  0|
    |0  0 -1|
    with s=sqrt(0.5) and scaling (-sqrt(2),sqrt(2),3)
    */
    Matrix<double> d(3,3,true);
    d[1][0]=1.0;
    d[1][1]=2.;
    d[2][2]=-3.;
    std::vector<double> v=d.toRotation();

    TS_ASSERT_DELTA(d[0][0],-sqrt(0.5),1e-7);
    TS_ASSERT_DELTA(d[0][1],-sqrt(0.5),1e-7);
    TS_ASSERT_DELTA(d[1][0],-sqrt(0.5),1e-7);
    TS_ASSERT_DELTA(d[1][1],sqrt(0.5),1e-7);
    TS_ASSERT_DELTA(d[2][2],-1.,1e-7);
    TS_ASSERT_DELTA(v[0],-sqrt(2.),1e-7);
    TS_ASSERT_DELTA(v[1],sqrt(2.),1e-7);
    TS_ASSERT_DELTA(v[2],3.,1e-7);
  }

};

#endif
