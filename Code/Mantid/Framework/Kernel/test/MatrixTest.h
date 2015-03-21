#ifndef MANTID_TESTMATIX__
#define MANTID_TESTMATIX__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>

#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h" 
#include "MantidKernel/V3D.h"

#include <boost/lexical_cast.hpp>

using Mantid::Kernel::Matrix;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

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

  void test_not_equal()
  {
      Matrix<double> A(3, 3, true);
      Matrix<double> B(3, 3, true);

      A[0][0] = -1.0;

      TS_ASSERT(A != B);
      TS_ASSERT(!(A == B));
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

  void test_Transpose_On_Square_Matrix_Matches_TPrime()
  {
    Matrix<double> A(2,2);
    A[0][0]=1.0;
    A[0][1]=2.0;
    A[1][0]=3.0;
    A[1][1]=4.0;

    auto B = A.Tprime(); // new matrix
    TS_ASSERT_EQUALS(1.0, B[0][0]);
    TS_ASSERT_EQUALS(3.0, B[0][1]);
    TS_ASSERT_EQUALS(2.0, B[1][0]);
    TS_ASSERT_EQUALS(4.0, B[1][1]);

    A.Transpose(); // in place
    TS_ASSERT_EQUALS(1.0, A[0][0]);
    TS_ASSERT_EQUALS(3.0, A[0][1]);
    TS_ASSERT_EQUALS(2.0, A[1][0]);
    TS_ASSERT_EQUALS(4.0, A[1][1]);

  }

  void test_Transpose_On_Irregular_Matrix_Matches_TPrime()
  {
    Matrix<double> A(2,3);
    A[0][0]=1.0;
    A[0][1]=2.0;
    A[0][2]=3.0;
    A[1][0]=4.0;
    A[1][1]=5.0;
    A[1][2]=6.0;

    auto B = A.Tprime(); // new matrix
    TS_ASSERT_EQUALS(2, B.numCols());
    TS_ASSERT_EQUALS(3, B.numRows());
    TS_ASSERT_EQUALS(1.0, B[0][0]);
    TS_ASSERT_EQUALS(4.0, B[0][1]);
    TS_ASSERT_EQUALS(2.0, B[1][0]);
    TS_ASSERT_EQUALS(5.0, B[1][1]);
    TS_ASSERT_EQUALS(3.0, B[2][0]);
    TS_ASSERT_EQUALS(6.0, B[2][1]);
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


  void testIsRotation()
  {
    Matrix<double> d(3,3,true);
    TS_ASSERT(d.isRotation());
    d[0][0]=-1;
    TS_ASSERT(!d.isRotation());
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

  void test_Input_Stream_Throws_On_Bad_Input()
  {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matr(3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS(is >> rot, std::invalid_argument);
    is.str("Matrix3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS(is >> rot, std::invalid_argument);
    is.str("Matrix(3,31,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS(is >> rot, std::invalid_argument);
  }


  void test_Input_Stream_On_Square_Matrix()
  {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS_NOTHING(is >> rot);
    TS_ASSERT_EQUALS(rot.numRows(), 3);
    TS_ASSERT_EQUALS(rot.numCols(), 3);
    for( size_t i = 0; i < 3; ++i )
    {
      for( size_t j = 0; j < 3; ++j )
      {
        TS_ASSERT_EQUALS(rot[i][j], static_cast<double>(i*rot.numRows() + j + 1));
      }
    }
  }

   void test_Input_Stream_On_Non_Square_Matrix()
  {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(2,4)0,1,2,3,10,11,12,13");
    TS_ASSERT_THROWS_NOTHING(is >> rot);
    TS_ASSERT_EQUALS(rot.numRows(), 2);
    TS_ASSERT_EQUALS(rot.numCols(), 4);
    for( size_t i = 0; i < 2; ++i )
    {
      for( size_t j = 0; j < 4; ++j )
      {
        if( i < 1 )
        {
          TS_ASSERT_EQUALS(rot[i][j], static_cast<double>(i+j));
        }
        else
        {
          TS_ASSERT_EQUALS(rot[i][j], static_cast<double>(9+i+j));
        }
      }
    }
  }

   void test_fillMatrix_With_Good_Input_Gives_Expected_Matrix()
   {
     DblMatrix rot;
     std::istringstream is;
     is.str("Matrix(3|3)1|2|3|4|5|6|7|8|9");
     TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::fillFromStream(is, rot, '|'));

     checkMatrixHasExpectedValuesForSquareMatrixTest(rot);
   }

   void test_fillMatrix_Accepts_Any_Delimiter_Between_Number_Rows_And_Columns()
   {
     DblMatrix rot;
     std::istringstream is;
     is.str("Matrix(3@3)1|2|3|4|5|6|7|8|9");
     TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::fillFromStream(is, rot, '|'));

     checkMatrixHasExpectedValuesForSquareMatrixTest(rot);
   }

   void test_fillMatrix_With_Mixed_Delimiters_In_Input_Values_Throws()
   {
     DblMatrix rot;
     std::istringstream is;
     is.str("Matrix(3|3)1|2,3|4|5|6|7|8|9");
     TS_ASSERT_THROWS(Mantid::Kernel::fillFromStream(is, rot, '|'), std::invalid_argument);
   }

   void test_Construction_Non_Square_Matrix_From_Output_Stream()
   {
     DblMatrix ref(2,3);
     ref[0][0] = 5;
     ref[0][1] = 10;
     ref[0][2] = 15;
     ref[1][0] = 105;
     ref[1][1] = 110;
     ref[1][2] = 115;

     std::ostringstream os;
     os << ref;
     TS_ASSERT_EQUALS(os.str(), "Matrix(2,3)5,10,15,105,110,115");
   }

   void test_Construction_Square_Matrix_From_Output_Stream()
   {
     DblMatrix square(2,2);
     square[0][0] = 2;
     square[0][1] = 4;
     square[1][0] = 6;
     square[1][1] = 8;

     std::ostringstream os;
     os << square;
     TS_ASSERT_EQUALS(os.str(), "Matrix(2,2)2,4,6,8");
   }

   void test_Dump_Matrix_To_Output_Stream_With_Custom_Delimiter()
   {
     DblMatrix square(2,2);
     square[0][0] = 2;
     square[0][1] = 4;
     square[1][0] = 6;
     square[1][1] = 8;

     std::ostringstream os;
     Mantid::Kernel::dumpToStream(os, square, '|');
     TS_ASSERT_EQUALS(os.str(), "Matrix(2|2)2|4|6|8");
   }

   void test_lexical_cast()
   {
      try
      {
        DblMatrix R = boost::lexical_cast<DblMatrix>("Matrix(2,2)2,4,6,8");
        TS_ASSERT_EQUALS(R.numRows(), 2);
        TS_ASSERT_EQUALS(R.numCols(), 2);
        TS_ASSERT_EQUALS(R[0][0], 2.0);
        TS_ASSERT_EQUALS(R[0][1], 4.0);
        TS_ASSERT_EQUALS(R[1][0], 6.0);
        TS_ASSERT_EQUALS(R[1][1], 8.0);
      }
      catch(boost::bad_lexical_cast & e)
      {
        TS_FAIL(e.what());
      }
   }

   void testMultiplicationWithVector()
   {
       DblMatrix M = boost::lexical_cast<DblMatrix>("Matrix(3,3)-0.23,0.55,5.22,2.96,4.2,0.1,-1.453,3.112,-2.34");

       V3D v(2.3,4.5,-0.45);

       V3D nv = M * v;

       // Results from octave.
       TS_ASSERT_DELTA(nv.X(), -0.403000000000000, 1e-15);
       TS_ASSERT_DELTA(nv.Y(), 25.663000000000000, 1e-15);
       TS_ASSERT_DELTA(nv.Z(), 11.715100000000003, 1e-15);

       DblMatrix M4(4,4, true);
       TS_ASSERT_THROWS(M4.operator *(v), Mantid::Kernel::Exception::MisMatch<size_t>);

       DblMatrix M23 = boost::lexical_cast<DblMatrix>("Matrix(2,3)-0.23,0.55,5.22,2.96,4.2,0.1");
       TS_ASSERT_THROWS_NOTHING(M23.operator *(v));

       nv = M23 * v;

       TS_ASSERT_DELTA(nv.X(), -0.403000000000000, 1e-15);
       TS_ASSERT_DELTA(nv.Y(), 25.663000000000000, 1e-15);
       TS_ASSERT_EQUALS(nv.Z(), 0);
   }

private:

   void checkMatrixHasExpectedValuesForSquareMatrixTest(const DblMatrix & mat)
   {
     TS_ASSERT_EQUALS(mat.numRows(), 3);
     TS_ASSERT_EQUALS(mat.numCols(), 3);
     for( size_t i = 0; i < 3; ++i )
     {
       for( size_t j = 0; j < 3; ++j )
       {
         TS_ASSERT_EQUALS(mat[i][j], static_cast<double>(i*mat.numRows() + j + 1));
       }
     }
   }
};

#endif
