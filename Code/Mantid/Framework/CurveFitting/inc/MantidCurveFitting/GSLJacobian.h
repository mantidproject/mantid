#ifndef MANTID_CURVEFITTING_GSLJACOBIAN_H_
#define MANTID_CURVEFITTING_GSLJACOBIAN_H_

#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IFunction.h"
#include <gsl/gsl_matrix.h>

#include <vector>
#include <stdexcept>

namespace Mantid {
namespace CurveFitting {
/**
An implementation of Jacobian using gsl_matrix.

@author Anders Markvardsen, ISIS, RAL
@date 14/05/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class GSLJacobian : public API::Jacobian {
  /// The pointer to the GSL's internal jacobian matrix
  gsl_matrix *m_J;
  /// Maps declared indeces to active. For fixed (tied) parameters holds -1
  std::vector<int> m_index;

public:
  /// Constructor
  /// @param fun :: Function which derivatives to be stored in this Jacobian.
  /// @param ny :: Size of the fitting data.
  GSLJacobian(API::IFunction_const_sptr fun, const size_t ny) {
    m_index.resize(fun->nParams(), -1);
    size_t np = 0; // number of active parameters
    for (size_t i = 0; i < fun->nParams(); ++i) {
      m_index[i] = static_cast<int>(np);
      if (fun->isActive(i))
        ++np;
    }
    m_J = gsl_matrix_alloc(ny, np);
  }

  /// Destructor.
  ~GSLJacobian() { gsl_matrix_free(m_J); }

  /// Get the pointer to the GSL's jacobian
  gsl_matrix *getJ() { return m_J; }

  /// overwrite base method
  /// @param value :: the value
  /// @param iActiveP :: the index of the parameter
  ///  @throw runtime_error Thrown if column of Jacobian to add number to does
  ///  not exist
  void addNumberToColumn(const double &value, const size_t &iActiveP) {
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
  void set(size_t iY, size_t iP, double value) {
    int j = m_index[iP];
    if (j >= 0)
      gsl_matrix_set(m_J, iY, j, value);
  }
  /// overwrite base method
  double get(size_t iY, size_t iP) {
    int j = m_index[iP];
    if (j >= 0)
      return gsl_matrix_get(m_J, iY, j);
    return 0.0;
  }
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLJACOBIAN_H_*/
