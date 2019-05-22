// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTMATIX__
#define MANTID_TESTMATIX__

#include <algorithm>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <ostream>
#include <vector>

#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

#include <boost/lexical_cast.hpp>

using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::IntMatrix;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

class MatrixTest : public CxxTest::TestSuite {

public:
  void makeMatrix(Matrix<double> &A) const {
    A.setMem(3, 3);
    A[0][0] = 1.0;
    A[1][0] = 3.0;
    A[0][1] = 4.0;
    A[0][2] = 6.0;
    A[2][0] = 5.0;
    A[1][1] = 3.0;
    A[2][1] = 1.0;
    A[1][2] = 6.0;
    A[2][2] = -7.0;
    return;
  }

  /**
  Test that a matrix can be inverted
  */
  void testInvert() {
    // 3x3
    Matrix<double> A(3, 3);
    makeMatrix(A);
    TS_ASSERT_DELTA(A.Invert(), 105.0, 1e-5);

    // 1x1
    Matrix<double> B(1, 1);
    B[0][0] = 2.;
    TS_ASSERT_DELTA(B.Invert(), 2, 1e-5);
    TS_ASSERT_DELTA(B[0][0], 0.5, 1e-5);

    // 2x2
    std::vector<double> data{1, 2, 3, 4}, expected{-2, 1, 1.5, -0.5};
    DblMatrix C(data);
    TS_ASSERT_DELTA(C.Invert(), -2, 1e-5);
    DblMatrix expectedInverse(expected);
    TS_ASSERT(C == expectedInverse);
  }

  void testIdent() {
    Matrix<double> A(3, 3);

    A[0][0] = 1.0;
    A[1][0] = 0.0;
    A[0][1] = 0.0;
    A[0][2] = 0.0;
    A[2][0] = 0.0;
    A[1][1] = 1.0;
    A[2][1] = 0.0;
    A[1][2] = 0.0;
    A[2][2] = 1.0;

    Matrix<double> Ident(3, 3);
    TS_ASSERT_DIFFERS(Ident, A);
    Ident.identityMatrix();
    TS_ASSERT_EQUALS(Ident, A);
  }

  /** Test of equals with a user-specified tolerance */
  void test_equals() {
    Matrix<double> A(3, 3, true);
    Matrix<double> B(3, 3, true);
    B[1][1] = 1.1;
    TS_ASSERT(!A.equals(B, 0.05));
    TS_ASSERT(A.equals(B, 0.15));
  }

  void test_not_equal() {
    Matrix<double> A(3, 3, true);
    Matrix<double> B(3, 3, true);

    A[0][0] = -1.0;

    TS_ASSERT(A != B);
    TS_ASSERT(!(A == B));
  }

  /**
  Check that we can swap rows and columns
  */
  void testSwapRows() {
    Matrix<double> A(3, 3);
    makeMatrix(A);
    Matrix<double> B(A);
    A.swapRows(1, 2);
    A.swapCols(1, 2);
    TS_ASSERT_EQUALS(A[0][0], B[0][0]);
    TS_ASSERT_EQUALS(A[2][2], B[1][1]);
    // Plus all the others..
  }

  void testEigenvectors() {
    Matrix<double> Eval;
    Matrix<double> Diag;
    Matrix<double> A(3, 3); // NOTE: A must be symmetric
    A[0][0] = 1.0;
    A[1][0] = A[0][1] = 4.0;
    A[0][2] = A[2][0] = 5.0;
    A[1][1] = 3.0;
    A[2][1] = A[1][2] = 6.0;
    A[2][2] = -7.0;
    TS_ASSERT(A.Diagonalise(Eval, Diag));

    Matrix<double> MA = A * Eval;
    Matrix<double> MV = Eval * Diag;
    Eval.sortEigen(Diag);
    TS_ASSERT(Diag[0][0] < Diag[1][1]);
    TS_ASSERT(Diag[1][1] < Diag[2][2]);
    TS_ASSERT(MA == MV);

    std::vector<double> X(3);
    X[0] = Eval[0][1];
    X[1] = Eval[1][1];
    X[2] = Eval[2][1];

    std::vector<double> out = A * X;
    transform(X.begin(), X.end(), X.begin(),
              std::bind2nd(std::multiplies<double>(), Diag[1][1]));
    TS_ASSERT_DELTA(X[0], out[0], 0.0001);
    TS_ASSERT_DELTA(X[1], out[1], 0.0001);
    TS_ASSERT_DELTA(X[2], out[2], 0.0001);
  }

  /**
  Tests the diagonalisation  on a symmetric 2x2 matrix
  */
  void testDiagonalise() {
    Matrix<double> Eval;
    Matrix<double> Diag;
    Matrix<double> A(2, 2); // symmetric only
    A[0][0] = 1.0;
    A[1][0] = 3.0;
    A[0][1] = 3.0;
    A[1][1] = 4.0;
    TS_ASSERT(A.Diagonalise(Eval, Diag)); // returns 1 or 2

    Matrix<double> EvalT(Eval);
    EvalT.Transpose();
    Eval *= Diag;
    Eval *= EvalT;
    TS_ASSERT(Eval == A);
  }

  /// Can't diagonalise a non-square or non-symmetric matrix
  void testDiagonaliseThrows() {
    DblMatrix notSquare(2, 3);
    const std::vector<double> data{1, 2, 3, 4};
    DblMatrix notSymm(data);
    DblMatrix eigenVectors, eigenValues;
    TS_ASSERT_EQUALS(0, notSquare.Diagonalise(eigenVectors, eigenValues));
    TS_ASSERT_EQUALS(0, notSymm.Diagonalise(eigenVectors, eigenValues));
  }

  void testFromVectorThrows() {
    std::vector<double> data(5, 0);

    TSM_ASSERT_THROWS("building matrix by this construcor and data with wrong "
                      "number of elements should throw",
                      (Matrix<double>(data)), const std::invalid_argument &);
  }

  void testFromVectorAndDimensions() {
    std::vector<int> data{1, 2, 3, 4, 5, 6};
    TSM_ASSERT_THROWS("building matrix with worng dimension should fail",
                      (Matrix<int>(data, 4, 5)), const std::invalid_argument &);
    Matrix<int> myMat;
    TSM_ASSERT_THROWS_NOTHING("building matrix by this construcor and data "
                              "with correct number of elements should not "
                              "throw",
                              myMat = Matrix<int>(data, 2, 3));
    TS_ASSERT_EQUALS(1, myMat[0][0]);
    TS_ASSERT_EQUALS(2, myMat[0][1]);
    TS_ASSERT_EQUALS(3, myMat[0][2]);
    TS_ASSERT_EQUALS(4, myMat[1][0]);
    TS_ASSERT_EQUALS(5, myMat[1][1]);
    TS_ASSERT_EQUALS(6, myMat[1][2]);
  }

  void test_Transpose_On_Square_Matrix_Matches_TPrime() {
    Matrix<double> A(2, 2);
    A[0][0] = 1.0;
    A[0][1] = 2.0;
    A[1][0] = 3.0;
    A[1][1] = 4.0;

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

  void test_Transpose_On_Irregular_Matrix_Matches_TPrime() {
    Matrix<double> A(2, 3);
    A[0][0] = 1.0;
    A[0][1] = 2.0;
    A[0][2] = 3.0;
    A[1][0] = 4.0;
    A[1][1] = 5.0;
    A[1][2] = 6.0;

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

  void testFromVectorBuildCorrect() {
    std::vector<int> data(9, 0);
    for (int i = 0; i < 9; i++) {
      data[i] = i;
    }
    Matrix<int> myMat;
    TSM_ASSERT_THROWS_NOTHING("building matrix by this constructor and data "
                              "with correct number of elements should not "
                              "throw",
                              myMat = Matrix<int>(data));

    // and the range of the elements in the matrix is correct;
    V3D rez1 = myMat * V3D(1, 0, 0);
    V3D rez2 = myMat * V3D(0, 1, 0);
    V3D rez3 = myMat * V3D(0, 0, 1);
    TSM_ASSERT_EQUALS("The data in a matrix have to be located row-wise, so "
                      "multiplication by (1,0,0)^T selects 1-st column ",
                      true, V3D(0, 3, 6) == rez1);
    TSM_ASSERT_EQUALS("The data in a matrix have to be located row-wise, so "
                      "multiplication by (0,1,0)^T selects 2-nd column ",
                      true, V3D(1, 4, 7) == rez2);
    TSM_ASSERT_EQUALS("The data in a matrix have to be located row-wise, so "
                      "multiplication by (0,0,1)^T selects 3-rd column ",
                      true, V3D(2, 5, 8) == rez3);
  }

  void testIsRotation() {
    Matrix<double> d(3, 3, true);
    TS_ASSERT(d.isRotation());
    d[0][0] = -1;
    TS_ASSERT(!d.isRotation());
  }

  void testToRotation() {
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
    Matrix<double> d(3, 3, true);
    d[1][0] = 1.0;
    d[1][1] = 2.;
    d[2][2] = -3.;
    std::vector<double> v = d.toRotation();

    TS_ASSERT_DELTA(d[0][0], -sqrt(0.5), 1e-7);
    TS_ASSERT_DELTA(d[0][1], -sqrt(0.5), 1e-7);
    TS_ASSERT_DELTA(d[1][0], -sqrt(0.5), 1e-7);
    TS_ASSERT_DELTA(d[1][1], sqrt(0.5), 1e-7);
    TS_ASSERT_DELTA(d[2][2], -1., 1e-7);
    TS_ASSERT_DELTA(v[0], -M_SQRT2, 1e-7);
    TS_ASSERT_DELTA(v[1], M_SQRT2, 1e-7);
    TS_ASSERT_DELTA(v[2], 3., 1e-7);
  }

  void test_Input_Stream_Throws_On_Bad_Input() {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matr(3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS(is >> rot, const std::invalid_argument &);
    is.str("Matrix3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS(is >> rot, const std::invalid_argument &);
    is.str("Matrix(3,31,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS(is >> rot, const std::invalid_argument &);
  }

  void test_Input_Stream_On_Square_Matrix() {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(3,3)1,2,3,4,5,6,7,8,9");
    TS_ASSERT_THROWS_NOTHING(is >> rot);
    TS_ASSERT_EQUALS(rot.numRows(), 3);
    TS_ASSERT_EQUALS(rot.numCols(), 3);
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        TS_ASSERT_EQUALS(rot[i][j],
                         static_cast<double>(i * rot.numRows() + j + 1));
      }
    }
  }

  void test_Input_Stream_On_Non_Square_Matrix() {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(2,4)0,1,2,3,10,11,12,13");
    TS_ASSERT_THROWS_NOTHING(is >> rot);
    TS_ASSERT_EQUALS(rot.numRows(), 2);
    TS_ASSERT_EQUALS(rot.numCols(), 4);
    for (size_t i = 0; i < 2; ++i) {
      for (size_t j = 0; j < 4; ++j) {
        if (i < 1) {
          TS_ASSERT_EQUALS(rot[i][j], static_cast<double>(i + j));
        } else {
          TS_ASSERT_EQUALS(rot[i][j], static_cast<double>(9 + i + j));
        }
      }
    }
  }

  void test_fillMatrix_With_Good_Input_Gives_Expected_Matrix() {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(3|3)1|2|3|4|5|6|7|8|9");
    TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::fillFromStream(is, rot, '|'));

    checkMatrixHasExpectedValuesForSquareMatrixTest(rot);
  }

  void test_fillMatrix_Accepts_Any_Delimiter_Between_Number_Rows_And_Columns() {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(3@3)1|2|3|4|5|6|7|8|9");
    TS_ASSERT_THROWS_NOTHING(Mantid::Kernel::fillFromStream(is, rot, '|'));

    checkMatrixHasExpectedValuesForSquareMatrixTest(rot);
  }

  void test_fillMatrix_With_Mixed_Delimiters_In_Input_Values_Throws() {
    DblMatrix rot;
    std::istringstream is;
    is.str("Matrix(3|3)1|2,3|4|5|6|7|8|9");
    TS_ASSERT_THROWS(Mantid::Kernel::fillFromStream(is, rot, '|'),
                     const std::invalid_argument &);
  }

  void test_Construction_Non_Square_Matrix_From_Output_Stream() {
    DblMatrix ref(2, 3);
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

  void test_Construction_Square_Matrix_From_Output_Stream() {
    DblMatrix square(2, 2);
    square[0][0] = 2;
    square[0][1] = 4;
    square[1][0] = 6;
    square[1][1] = 8;

    std::ostringstream os;
    os << square;
    TS_ASSERT_EQUALS(os.str(), "Matrix(2,2)2,4,6,8");
  }

  void test_Dump_Matrix_To_Output_Stream_With_Custom_Delimiter() {
    DblMatrix square(2, 2);
    square[0][0] = 2;
    square[0][1] = 4;
    square[1][0] = 6;
    square[1][1] = 8;

    std::ostringstream os;
    Mantid::Kernel::dumpToStream(os, square, '|');
    TS_ASSERT_EQUALS(os.str(), "Matrix(2|2)2|4|6|8");
  }

  void test_lexical_cast() {
    try {
      DblMatrix R = boost::lexical_cast<DblMatrix>("Matrix(2,2)2,4,6,8");
      TS_ASSERT_EQUALS(R.numRows(), 2);
      TS_ASSERT_EQUALS(R.numCols(), 2);
      TS_ASSERT_EQUALS(R[0][0], 2.0);
      TS_ASSERT_EQUALS(R[0][1], 4.0);
      TS_ASSERT_EQUALS(R[1][0], 6.0);
      TS_ASSERT_EQUALS(R[1][1], 8.0);
    } catch (boost::bad_lexical_cast &e) {
      TS_FAIL(e.what());
    }
  }

  /// Tests both V3D and std::vector
  void testMultiplicationWithVector() {
    DblMatrix M = boost::lexical_cast<DblMatrix>(
        "Matrix(3,3)-0.23,0.55,5.22,2.96,4.2,0.1,-1.453,3.112,-2.34");

    V3D v(2.3, 4.5, -0.45);
    std::vector<double> stdvec(v);

    V3D nv = M * v;
    std::vector<double> stdNewVec = M * stdvec;
    std::vector<double> otherStdNewVec;
    M.multiplyPoint(stdvec, otherStdNewVec);

    // Results from octave.
    TS_ASSERT_DELTA(nv.X(), -0.403000000000000, 1e-15);
    TS_ASSERT_DELTA(nv.Y(), 25.663000000000000, 1e-15);
    TS_ASSERT_DELTA(nv.Z(), 11.715100000000003, 1e-15);
    TS_ASSERT_DELTA(stdNewVec[0], -0.403000000000000, 1e-15);
    TS_ASSERT_DELTA(stdNewVec[1], 25.663000000000000, 1e-15);
    TS_ASSERT_DELTA(stdNewVec[2], 11.715100000000003, 1e-15);
    TS_ASSERT_DELTA(otherStdNewVec[0], -0.403000000000000, 1e-15);
    TS_ASSERT_DELTA(otherStdNewVec[1], 25.663000000000000, 1e-15);
    TS_ASSERT_DELTA(otherStdNewVec[2], 11.715100000000003, 1e-15);

    DblMatrix M4(4, 4, true);
    TS_ASSERT_THROWS(M4.operator*(v),
                     const Mantid::Kernel::Exception::MisMatch<size_t> &);
    TS_ASSERT_THROWS(M4.operator*(stdvec),
                     const Mantid::Kernel::Exception::MisMatch<size_t> &);
    TS_ASSERT_THROWS(M4.multiplyPoint(stdvec, otherStdNewVec),
                     const Mantid::Kernel::Exception::MisMatch<size_t> &);

    DblMatrix M23 = boost::lexical_cast<DblMatrix>(
        "Matrix(2,3)-0.23,0.55,5.22,2.96,4.2,0.1");
    TS_ASSERT_THROWS_NOTHING(M23.operator*(v));
    TS_ASSERT_THROWS_NOTHING(M23.operator*(stdvec));
    TS_ASSERT_THROWS_NOTHING(M23.multiplyPoint(stdvec, otherStdNewVec));

    nv = M23 * v;
    stdNewVec = M23 * stdvec;
    M23.multiplyPoint(stdvec, otherStdNewVec);

    TS_ASSERT_DELTA(nv.X(), -0.403000000000000, 1e-15);
    TS_ASSERT_DELTA(nv.Y(), 25.663000000000000, 1e-15);
    TS_ASSERT_EQUALS(nv.Z(), 0);
    TS_ASSERT_DELTA(stdNewVec[0], -0.403000000000000, 1e-15);
    TS_ASSERT_DELTA(stdNewVec[1], 25.663000000000000, 1e-15);
    TS_ASSERT_EQUALS(stdNewVec.size(), 2);
    TS_ASSERT_DELTA(otherStdNewVec[0], -0.403000000000000, 1e-15);
    TS_ASSERT_DELTA(otherStdNewVec[1], 25.663000000000000, 1e-15);
    TS_ASSERT_EQUALS(otherStdNewVec.size(), 2);

    DblMatrix M43 = boost::lexical_cast<DblMatrix>(
        "Matrix(4,3)-0.23,0.55,5.22,2.96,4.2,0.1,-0.23,0.55,5.22,2.96,4.2,0.1");
    TS_ASSERT_THROWS(M43.operator*(v),
                     const Mantid::Kernel::Exception::MisMatch<size_t> &); // V3D only
  }

  /// Test that the constructor taking preset sizes returns a zero matrix
  void testConstructorPresetSizes() {
    constexpr int nRows(2), nCols(3);
    IntMatrix mat(nRows, nCols);
    for (int iRow = 0; iRow < nRows; iRow++) {
      for (int iCol = 0; iCol < nCols; iCol++) {
        TS_ASSERT_EQUALS(mat.item(iRow, iCol), 0);
      }
    }
  }

  /// Test that the 'make identity' option in the preset size constructor works
  void testConstructorPresetSizes_makeIdentity() {
    constexpr int nRowsCols(2);
    constexpr bool makeIdentity(true);
    IntMatrix mat(nRowsCols, nRowsCols, makeIdentity);
    for (int iRow = 0; iRow < nRowsCols; iRow++) {
      for (int iCol = 0; iCol < nRowsCols; iCol++) {
        const int expected = iRow == iCol ? 1 : 0;
        TS_ASSERT_EQUALS(mat.item(iRow, iCol), expected);
      }
    }
  }

  /// Constructor multiplying two vectors
  void testConstructorTwoVectors() {
    const std::vector<int> vecA{1, 2, 3}, vecB{4, 5, 6};
    IntMatrix mat(vecA, vecB);
    const int nRowsCols = static_cast<int>(vecA.size());
    for (int iRow = 0; iRow < nRowsCols; iRow++) {
      for (int iCol = 0; iCol < nRowsCols; iCol++) {
        const int expected = vecA[iRow] * vecB[iCol];
        TS_ASSERT_EQUALS(mat.item(iRow, iCol), expected);
      }
    }
  }

  /// Constructor with missing row or column
  void testConstructorMissingRowColumn() {
    constexpr int nRows(4), nCols(4), missingRow(3), missingCol(1);
    const std::vector<double> data{1, 86,  2, 3, 4,  55,  5,  6,
                                   7, -25, 8, 9, 42, -33, 15, 0};
    DblMatrix original(data);
    TS_ASSERT_THROWS(DblMatrix badMat(original, nRows + 1, missingCol),
                     const Mantid::Kernel::Exception::IndexError &);
    TS_ASSERT_THROWS(DblMatrix badMat(original, missingRow, nCols + 1),
                     const Mantid::Kernel::Exception::IndexError &);
    DblMatrix mat(original, missingRow, missingCol);
    checkMatrixHasExpectedValuesForSquareMatrixTest(mat);
  }

  void testCopyConstructor() {
    const std::vector<double> data{1, 2, 3, 4, 5, 6, 7, 8, 9};
    DblMatrix original(data);
    DblMatrix copy(original);
    checkMatrixHasExpectedValuesForSquareMatrixTest(copy);
  }

  void testAssignment() {
    const std::vector<double> data{1, 2, 3, 4, 5, 6, 7, 8, 9};
    DblMatrix original(data);
    DblMatrix copy = original;
    checkMatrixHasExpectedValuesForSquareMatrixTest(copy);
  }

  /// Test + and +=
  void testAddition() {
    const std::vector<double> data{0, 2, 3, 4, 4, 6, 7, 8, 8};
    DblMatrix mat(data);
    DblMatrix ident(3, 3);
    ident.identityMatrix();
    DblMatrix plus = mat + ident;
    checkMatrixHasExpectedValuesForSquareMatrixTest(plus);
    mat += ident;
    checkMatrixHasExpectedValuesForSquareMatrixTest(mat);
  }

  /// Test - and -=
  void testSubtraction() {
    const std::vector<double> data{2, 2, 3, 4, 6, 6, 7, 8, 10};
    DblMatrix mat(data);
    DblMatrix ident(3, 3);
    ident.identityMatrix();
    DblMatrix minus = mat - ident;
    checkMatrixHasExpectedValuesForSquareMatrixTest(minus);
    mat -= ident;
    checkMatrixHasExpectedValuesForSquareMatrixTest(mat);
  }

  /// Test * and *=
  void testMultiplicationByMatrix() {
    const std::vector<int> dataA{1, 2, 3, 4}, dataB{5, 6, 7, 8};
    IntMatrix matA(dataA), matB(dataB);
    IntMatrix multiplied = matA * matB;
    TS_ASSERT_EQUALS(multiplied[0][0], 19);
    TS_ASSERT_EQUALS(multiplied[0][1], 22);
    TS_ASSERT_EQUALS(multiplied[1][0], 43);
    TS_ASSERT_EQUALS(multiplied[1][1], 50);
    matA *= matB;
    TS_ASSERT_EQUALS(matA[0][0], 19);
    TS_ASSERT_EQUALS(matA[0][1], 22);
    TS_ASSERT_EQUALS(matA[1][0], 43);
    TS_ASSERT_EQUALS(matA[1][1], 50);
  }

  /// Test * and *=
  void testMultiplicationByConstant() {
    const std::vector<int> data{1, 2, 3, 4};
    IntMatrix mat(data);
    IntMatrix multiplied = mat * 2;
    TS_ASSERT_EQUALS(multiplied[0][0], 2);
    TS_ASSERT_EQUALS(multiplied[0][1], 4);
    TS_ASSERT_EQUALS(multiplied[1][0], 6);
    TS_ASSERT_EQUALS(multiplied[1][1], 8);
    mat *= 2;
    TS_ASSERT_EQUALS(mat[0][0], 2);
    TS_ASSERT_EQUALS(mat[0][1], 4);
    TS_ASSERT_EQUALS(mat[1][0], 6);
    TS_ASSERT_EQUALS(mat[1][1], 8);
  }

  void testDivisionByConstant() {
    const std::vector<double> data{2, 4, 6, 8, 10, 12, 14, 16, 18};
    DblMatrix mat(data);
    mat /= 2.0;
    checkMatrixHasExpectedValuesForSquareMatrixTest(mat);
  }

  void testComparisonOperators() {
    constexpr int nRowsCols(3);
    DblMatrix mat(nRowsCols, nRowsCols);
    makeMatrix(mat);
    // self-comparison
    TS_ASSERT_EQUALS(mat < mat, false);
    TS_ASSERT_EQUALS(mat >= mat, true);
    // wrong size
    DblMatrix wrong(nRowsCols - 1, nRowsCols);
    TS_ASSERT_EQUALS(mat < wrong, false);
    TS_ASSERT_EQUALS(mat >= wrong, false);
    // less than
    DblMatrix less(mat);
    for (int iRow = 0; iRow < nRowsCols; iRow++) {
      for (int iCol = 0; iCol < nRowsCols; iCol++) {
        less[iRow][iCol] = mat[iRow][iCol] - 1;
      }
    }
    TS_ASSERT_EQUALS(mat < less, false);
    TS_ASSERT_EQUALS(less < mat, true);
    TS_ASSERT_EQUALS(mat >= less, true);
    TS_ASSERT_EQUALS(less >= mat, false);
    // greater than
    DblMatrix greater(mat);
    for (int iRow = 0; iRow < nRowsCols; iRow++) {
      for (int iCol = 0; iCol < nRowsCols; iCol++) {
        greater[iRow][iCol] = mat[iRow][iCol] + 1;
      }
    }
    TS_ASSERT_EQUALS(mat < greater, true);
    TS_ASSERT_EQUALS(greater < mat, false);
    TS_ASSERT_EQUALS(mat >= greater, false);
    TS_ASSERT_EQUALS(greater >= mat, true);
  }

  void testWrite() {
    std::ostringstream os;
    DblMatrix mat(3, 3);
    makeMatrix(mat);
    mat.write(os, 10);
    std::string output = os.str();
    std::string expected = "1.000000e+00  4.000000e+00  6.000000e+00  "
                           "\n3.000000e+00  3.000000e+00  6.000000e+00  "
                           "\n5.000000e+00  1.000000e+00  -7.000000e+00  \n";
    TS_ASSERT_EQUALS(output, expected);
  }

  void testToString() {
    DblMatrix mat(3, 3);
    makeMatrix(mat);
    std::string output = mat.str();
    std::string expected = "1 4 6 3 3 6 5 1 -7 ";
    TS_ASSERT_EQUALS(output, expected);
  }

  void testToVector() {
    constexpr int nRowsCols(3);
    DblMatrix mat(nRowsCols, nRowsCols);
    makeMatrix(mat);
    std::vector<double> converted = mat.getVector();
    std::vector<double> implicit(mat);
    int iVectorIndex(0);
    for (int iRow = 0; iRow < nRowsCols; iRow++) {
      for (int iCol = 0; iCol < nRowsCols; iCol++) {
        TS_ASSERT_EQUALS(converted[iVectorIndex], mat[iRow][iCol]);
        TS_ASSERT_EQUALS(implicit[iVectorIndex], mat[iRow][iCol]);
        iVectorIndex++;
      }
    }
  }

  void testSetColumn() {
    const std::vector<double> data{1, -2, 3, 4, -5, 6, 7, -8, 9};
    const std::vector<double> newCol{2, 5, 8};
    DblMatrix mat(data);
    size_t badCol(3), goodCol(1);
    TS_ASSERT_THROWS(mat.setColumn(badCol, newCol),
                     const std::invalid_argument &);
    mat.setColumn(goodCol, newCol);
    checkMatrixHasExpectedValuesForSquareMatrixTest(mat);
  }

  void testSetRow() {
    const std::vector<double> data{1, 2, 3, 4, 5, 6, -7, -8, -9};
    const std::vector<double> newRow{7, 8, 9};
    DblMatrix mat(data);
    size_t badRow(3), goodRow(2);
    TS_ASSERT_THROWS(mat.setRow(badRow, newRow), const std::invalid_argument &);
    mat.setRow(goodRow, newRow);
    checkMatrixHasExpectedValuesForSquareMatrixTest(mat);
  }

  void testZeroMatrix() {
    constexpr int nRowCol(3);
    DblMatrix mat(nRowCol, nRowCol);
    makeMatrix(mat);
    TS_ASSERT_DIFFERS(mat[0][0], 0);
    mat.zeroMatrix();
    for (int iRow = 0; iRow < nRowCol; iRow++) {
      for (int iCol = 0; iCol < nRowCol; iCol++) {
        TS_ASSERT_EQUALS(mat[iRow][iCol], 0);
      }
    }
  }

  void testNormVert() {
    constexpr int nRowCol(3);
    DblMatrix mat(nRowCol, nRowCol);
    makeMatrix(mat);
    mat.normVert();
    const std::string expected("0.137361 0.549442 0.824163 0.408248 0.408248 "
                               "0.816497 0.57735 0.11547 -0.80829 ");
    TS_ASSERT_EQUALS(mat.str(), expected);
    TS_ASSERT_DELTA(std::sqrt(mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] +
                              mat[0][2] * mat[0][2]),
                    1.0, 0.001);
    TS_ASSERT_DELTA(std::sqrt(mat[1][0] * mat[1][0] + mat[1][1] * mat[1][1] +
                              mat[1][2] * mat[1][2]),
                    1.0, 0.001);
    TS_ASSERT_DELTA(std::sqrt(mat[2][0] * mat[2][0] + mat[2][1] * mat[2][1] +
                              mat[2][2] * mat[2][2]),
                    1.0, 0.001);
  }

  void testTrace() {
    constexpr int nRowCol(3);
    DblMatrix mat(nRowCol, nRowCol);
    makeMatrix(mat);
    double trace = mat.Trace();
    double expected = 0;
    for (int i = 0; i < nRowCol; i++) {
      expected += mat[i][i];
    }
    TS_ASSERT_EQUALS(trace, expected);
  }

  void testDiagonal() {
    constexpr int nRowCol(3);
    DblMatrix mat(nRowCol, nRowCol);
    makeMatrix(mat);
    std::vector<double> diag = mat.Diagonal();
    TS_ASSERT_EQUALS(diag.size(), nRowCol);
    for (int i = 0; i < nRowCol; i++) {
      TS_ASSERT_EQUALS(diag[i], mat[i][i]);
    }
  }

  void testPreMultiplyDiagonal() {
    const std::vector<double> dataA{1, 2, 3, 4}, dataDiag{5, 6},
        dataBad{5, 6, 7};
    DblMatrix mat(dataA);
    TS_ASSERT_THROWS(mat.preMultiplyByDiagonal(dataBad),
                     const std::runtime_error &);
    DblMatrix result = mat.preMultiplyByDiagonal(dataDiag);
    TS_ASSERT_EQUALS(result[0][0], 5);
    TS_ASSERT_EQUALS(result[0][1], 10);
    TS_ASSERT_EQUALS(result[1][0], 18);
    TS_ASSERT_EQUALS(result[1][1], 24);
  }

  void testPostMultiplyDiagonal() {
    const std::vector<double> dataA{1, 2, 3, 4}, dataDiag{5, 6},
        dataBad{5, 6, 7};
    DblMatrix mat(dataA);
    TS_ASSERT_THROWS(mat.postMultiplyByDiagonal(dataBad),
                     const std::runtime_error &);
    DblMatrix result = mat.postMultiplyByDiagonal(dataDiag);
    TS_ASSERT_EQUALS(result[0][0], 5);
    TS_ASSERT_EQUALS(result[0][1], 12);
    TS_ASSERT_EQUALS(result[1][0], 15);
    TS_ASSERT_EQUALS(result[1][1], 24);
  }

  void testSetMem() {
    DblMatrix mat(3, 3);
    mat.setMem(5, 5);
    double x = 0;
    TS_ASSERT_THROWS_NOTHING(x = mat[4][4]);
    (void)x; // fixes compiler warning
  }

  void testSize() {
    constexpr size_t nRows(5), nCols(4);
    DblMatrix mat(nRows, nCols);
    auto size = mat.size();
    TS_ASSERT_EQUALS(nRows, size.first);
    TS_ASSERT_EQUALS(nCols, size.second);
    TS_ASSERT_EQUALS(std::min(nRows, nCols), mat.Ssize());
  }

  void testAverageSymmetric() {
    const std::vector<double> data{1, 2, 3, 4}, expected{1, 2.5, 2.5, 4};
    DblMatrix mat(data), expectedResult(expected);
    mat.averSymmetric();
    TS_ASSERT(mat == expectedResult);
  }

  void testDeterminant() {
    DblMatrix mat(3, 3);
    makeMatrix(mat);
    double det = mat.determinant();
    TS_ASSERT_EQUALS(det, 105);
  }

  /// Test Gauss-Jordan factorisation
  void testFactor() {
    DblMatrix mat(3, 3);
    makeMatrix(mat);
    TS_ASSERT_EQUALS(mat.factor(), 105);
    const std::vector<double> expectedData{6, 1, 4, 0, 2, -1, 0, 0, 8.75};
    DblMatrix expected(expectedData);
    TS_ASSERT(mat == expected);
  }

  // Test inverting a matrix using Gauss-Jordan method
  void testGaussJordan() {
    constexpr size_t nRowsCols(3);
    DblMatrix mat(nRowsCols, nRowsCols);
    makeMatrix(mat);
    DblMatrix B(nRowsCols, nRowsCols);
    makeMatrix(B);
    DblMatrix expected(mat);
    expected.Invert();
    mat.GaussJordan(B);
    // test the two inverses agree
    TS_ASSERT(mat == expected);
    // B should be an identity matrix
    for (size_t iRow = 0; iRow < nRowsCols; iRow++) {
      for (size_t iCol = 0; iCol < nRowsCols; iCol++) {
        TS_ASSERT_EQUALS(B[iRow][iCol], iRow == iCol ? 1 : 0);
      }
    }
  }

  // sum of squares of all elements
  void testCompSum() {
    DblMatrix mat(3, 3);
    makeMatrix(mat);
    double result = mat.compSum();
    TS_ASSERT_EQUALS(result, 182);
  }

  // test orthogonality - rotations and non-rotational matrices
  void testIsOrthogonal() {
    const std::vector<double> rotationData{0, -1, 1, 0},
        nonRotationData{0, 1, 1, 0};
    DblMatrix rotation(rotationData), reflection(nonRotationData);
    TS_ASSERT(rotation.isRotation());
    TS_ASSERT(rotation.isOrthogonal());
    TS_ASSERT(!reflection.isRotation());
    TS_ASSERT(reflection.isOrthogonal());
  }

private:
  void checkMatrixHasExpectedValuesForSquareMatrixTest(const DblMatrix &mat) {
    TS_ASSERT_EQUALS(mat.numRows(), 3);
    TS_ASSERT_EQUALS(mat.numCols(), 3);
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        TS_ASSERT_EQUALS(mat[i][j],
                         static_cast<double>(i * mat.numRows() + j + 1));
      }
    }
  }
};

#endif
