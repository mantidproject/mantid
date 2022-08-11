// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidCurveFitting/EigenMatrix.h"

#include <gsl/gsl_matrix.h>

#include <stdexcept>
#include <vector>

namespace Mantid {
namespace CurveFitting {
/**
Two implementations of Jacobian.
-The first (EigenJacobian) using Eigen::Matrix.
- The second (JacobianImpl1) using gsl_matrix.
@author Anders Markvardsen, ISIS, RAL
@date 14/05/2010
*/
class EigenJacobian : public API::Jacobian {
  /// The pointer to the the internal jacobian matrix
  EigenMatrix m_J;
  /// Maps declared indeces to active. For fixed (tied) parameters holds -1
  std::vector<int> m_index;

public:
  /// Constructor
  /// @param fun :: Function which derivatives to be stored in this Jacobian.
  /// @param ny :: Size of the fitting data.
  EigenJacobian(const API::IFunction &fun, const size_t ny) {
    m_index.resize(fun.nParams(), -1);
    size_t np = 0; // number of active parameters
    for (size_t i = 0; i < fun.nParams(); ++i) {
      m_index[i] = static_cast<int>(np);
      if (fun.isActive(i))
        ++np;
    }
    m_J.resize(ny, np);
  }

  EigenMatrix &matrix() { return m_J; }

  /// Get the pointer to the GSL's jacobian
  map_type &getJ() { return m_J.mutator(); }

  /// overwrite base method
  /// @param value :: the value
  /// @param iActiveP :: the index of the parameter
  ///  @throw runtime_error Thrown if column of Jacobian to add number to does
  ///  not exist
  void addNumberToColumn(const double &value, const size_t &iActiveP) override {
    if (iActiveP < m_J.size2()) {
      // add penalty to first and last point and every 10th point in between
      m_J.mutator().data()[iActiveP] += value;
      m_J.mutator().data()[(m_J.size1() - 1) * m_J.size2() + iActiveP] += value;
      for (size_t iY = 9; iY < m_J.size1() - 1; iY += 10)
        m_J.mutator().data()[iY * m_J.size2() + iActiveP] += value;
    } else {
      throw std::runtime_error("Try to add number to column of Jacobian matrix "
                               "which does not exist.");
    }
  }
  /// overwrite base method
  void set(size_t iY, size_t iP, double value) override {
    int j = m_index[iP];
    if (j >= 0)
      m_J.set(iY, j, value);
  }
  /// overwrite base method
  double get(size_t iY, size_t iP) override {
    int j = m_index[iP];
    if (j >= 0)
      return m_J.get(iY, j);
    return 0.0;
  }
  /// overwrite base method
  void zero() override { m_J.zero(); }
};

/// The implementation of Jacobian
class JacobianImpl1 : public API::Jacobian {
public:
  /// The pointer to the GSL's internal jacobian matrix
  gsl_matrix *m_J;
  /// Maps declared indeces to active. For fixed (tied) parameters holds -1
  std::vector<int> m_index;

  /// Set the pointer to the GSL's jacobian
  void setJ(gsl_matrix *J) { m_J = J; }

  /// overwrite base method
  /// @param value :: the value
  /// @param iActiveP :: the index of the parameter
  ///  @throw runtime_error Thrown if column of Jacobian to add number to does
  ///  not exist
  void addNumberToColumn(const double &value, const size_t &iActiveP) override {
    if (iActiveP < m_J->size2) {
      // add penalty to first and last point and every 10th point in between
      m_J->data[iActiveP] += value;
      m_J->data[(m_J->size1 - 1) * m_J->size2 + iActiveP] += value;
      for (size_t iY = 9; iY < m_J->size1 - 1; iY += 10)
        m_J->data[iY * m_J->size2 + iActiveP] += value;
    } else {
      throw std::runtime_error("Try to add number to column of Jacobian matrix "
                               "which does not exist.");
    }
  }
  /// overwrite base method
  void set(size_t iY, size_t iP, double value) override {
    if (iP >= m_index.size()) {
      // Functions are allowed to change their number of active
      // fitting parameters during the fit (example: the crystal field
      // functions). When it happens after an iteration is finished there is a
      // special exception that signals the minimizer to re-initialize itself.
      // But it can happen during a numeric derivative calculation: a field
      // parameter is incremented, a new peak may appear and it may have a free
      // fitting parameter, so the number of parameters change. You cannot throw
      // and re-initialize the minimizer at this point because the same will
      // happen again. If you ignore these "virtual" extra parameters by the
      // time the derivative calculations finish the number of parameters will
      // return to the original value. Note that the get(...) method doesn't
      // skip any extra indices because if there are any it is an error.
      return;
    }
    int j = m_index[iP];
    if (j >= 0)
      gsl_matrix_set(m_J, iY, j, value);
  }
  /// overwrite base method
  double get(size_t iY, size_t iP) override {
    int j = m_index[iP];
    if (j >= 0)
      return gsl_matrix_get(m_J, iY, j);
    return 0.0;
  }
  /// overwrite base method
  void zero() override { gsl_matrix_set_zero(m_J); }
};

} // namespace CurveFitting
} // namespace Mantid