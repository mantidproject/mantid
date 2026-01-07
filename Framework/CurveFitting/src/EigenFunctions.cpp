// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EigenFunctions.h"

namespace Mantid::CurveFitting {

/** Mimics gsl_multifit_covar(J, epsrel, covar)
 * @param J :: Jacobian
 * @param epsrel :: relative threshold to decide rank deficiency
 * @return covariance matrix, rows/cols set to zero for dependent params
 */
Eigen::MatrixXd covar_from_jacobian(const map_type &J, double epsrel) {
  if (epsrel < 0.0) {
    throw std::invalid_argument("epsrel must be non-negative");
  }

  const Eigen::Index nr = J.rows(); // nr = num rows
  const Eigen::Index nc = J.cols(); // nc = num cols
  if ((nc == 0) || (nr == 0))
    return Eigen::MatrixXd{};

  // Pivoted QR Decomposition: XP = QR (X is J in this case)
  Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(J);

  // R is (nc x np) in general; the useful part for J^T J is (nc x nc) block
  Eigen::MatrixXd R = qr.matrixR().topLeftCorner(nc, nc).template triangularView<Eigen::Upper>();

  // Determine rank using the same rule as GSL: compare diag entries to |R_11|
  const double r11 = std::abs(R(0, 0));
  Eigen::Index rank = 0;
  if (r11 > 0.0) {
    for (Eigen::Index k = 0; k < nc; ++k) {
      if (std::abs(R(k, k)) > epsrel * r11) { // column considered linerally independant
        ++rank;
      } else {
        break; // with pivoting, following first dependent subsequent cols should also be dependant
      }
    }
  } else {
    // R11 == 0, everything dependent
    rank = 0;
  }

  // Build covariance in the pivoted parameter order
  Eigen::MatrixXd cov_pivot = Eigen::MatrixXd::Zero(nc, nc);

  if (rank > 0) {
    // cov = (R^T R)^{-1} for the independent cols = R^{-1} R^{-T}
    const auto R1 = R.topLeftCorner(rank, rank).template triangularView<Eigen::Upper>();

    Eigen::MatrixXd invR1 = R1.solve(Eigen::MatrixXd::Identity(rank, rank));
    Eigen::MatrixXd cov1 = invR1 * invR1.transpose(); // R^{-1} R^{-T}

    cov_pivot.topLeftCorner(rank, rank) = cov1;
  }

  // Unpivot back to original parameter order:
  // J = Q R P^T  => (J^T J)^{-1} = P (R^T R)^{-1} P^T
  const Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> P = qr.colsPermutation();
  Eigen::MatrixXd cov = P * cov_pivot * P.transpose();

  return cov;
}

} // namespace Mantid::CurveFitting
