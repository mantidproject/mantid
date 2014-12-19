//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {

/// Assign this matrix to a product of two other matrices
/// @param mult2 :: Matrix multiplication helper object.
GSLMatrix &GSLMatrix::operator=(const GSLMatrixMult2 &mult2) {
  // sizes of the result matrix
  size_t n1 = mult2.tr1 ? mult2.m_1.size2() : mult2.m_1.size1();
  size_t n2 = mult2.tr2 ? mult2.m_2.size1() : mult2.m_2.size2();

  this->resize(n1, n2);

  CBLAS_TRANSPOSE tr1 = mult2.tr1 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr2 = mult2.tr2 ? CblasTrans : CblasNoTrans;

  // this = m_1 * m_2
  gsl_blas_dgemm(tr1, tr2, 1.0, mult2.m_1.gsl(), mult2.m_2.gsl(), 0.0, gsl());

  return *this;
}

/// Assign this matrix to a product of three other matrices
/// @param mult3 :: Matrix multiplication helper object.
GSLMatrix &GSLMatrix::operator=(const GSLMatrixMult3 &mult3) {
  // sizes of the result matrix
  size_t n1 = mult3.tr1 ? mult3.m_1.size2() : mult3.m_1.size1();
  size_t n2 = mult3.tr3 ? mult3.m_3.size1() : mult3.m_3.size2();

  this->resize(n1, n2);

  // intermediate matrix
  GSLMatrix AB(n1, mult3.m_2.size2());

  CBLAS_TRANSPOSE tr1 = mult3.tr1 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr2 = mult3.tr2 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr3 = mult3.tr3 ? CblasTrans : CblasNoTrans;

  // AB = m_1 * m_2
  gsl_blas_dgemm(tr1, tr2, 1.0, mult3.m_1.gsl(), mult3.m_2.gsl(), 0.0,
                 AB.gsl());

  // this = AB * m_3
  gsl_blas_dgemm(CblasNoTrans, tr3, 1.0, AB.gsl(), mult3.m_3.gsl(), 0.0, gsl());

  return *this;
}

/// Solve system of linear equations M*x == rhs, M is this matrix
/// This matrix is destroyed.
/// @param rhs :: The right-hand-side vector
/// @param x :: The solution vector
void GSLMatrix::solve(const GSLVector &rhs, GSLVector &x) {
  if (size1() != size2()) {
    throw std::runtime_error(
        "System of linear equations: the matrix must be square.");
  }
  size_t n = size1();
  if (rhs.size() != n) {
    throw std::runtime_error(
        "System of linear equations: right-hand side vector has wrong size.");
  }
  x.resize(n);
  int s;
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_LU_decomp(gsl(), p, &s); // matrix is modified at this moment
  gsl_linalg_LU_solve(gsl(), p, rhs.gsl(), x.gsl());
  gsl_permutation_free(p);
}

/// Invert this matrix
void GSLMatrix::invert() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  size_t n = size1();
  int s;
  GSLMatrix LU(*this);
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_LU_decomp(LU.gsl(), p, &s);
  gsl_linalg_LU_invert(LU.gsl(), p, this->gsl());
  gsl_permutation_free(p);
}

/// Calculate the determinant
double GSLMatrix::det() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  size_t n = size1();
  int s;
  GSLMatrix LU(*this);
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_LU_decomp(LU.gsl(), p, &s);
  double res = gsl_linalg_LU_det(LU.gsl(), s);
  gsl_permutation_free(p);
  return res;
}

} // namespace CurveFitting
} // namespace Mantid
