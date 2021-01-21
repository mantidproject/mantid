// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Jacobian.h"

#include <vector>

namespace Mantid {
namespace CurveFitting {
/**
An implementation of Jacobian using std::vector.

@author Roman Tolchenov
@date 17/02/2012
*/
class Jacobian : public API::Jacobian {
  /// Number of data points
  size_t m_ny;
  /// Number of parameters in a function (== IFunction::nParams())
  size_t m_np;
  /// Storage for the derivatives
  std::vector<double> m_data;

public:
  /// Constructor.
  /// @param ny :: Number of data points
  /// @param np :: Number of parameters
  Jacobian(size_t ny, size_t np) : m_ny(ny), m_np(np) { m_data.resize(ny * np, 0.0); }
  /// overwrite base method
  /// @param value :: the value
  /// @param iP :: the index of the parameter
  ///  @throw runtime_error Thrown if column of Jacobian to add number to does
  ///  not exist
  void addNumberToColumn(const double &value, const size_t &iP) override {
    if (iP < m_np) {
      // add penalty to first and last point and every 10th point in between
      m_data[iP] += value;
      m_data[(m_ny - 1) * m_np + iP] += value;
      for (size_t iY = 9; iY < m_ny; iY += 10)
        m_data[iY * m_np + iP] += value;
    } else {
      throw std::runtime_error("Try to add number to column of Jacobian matrix "
                               "which does not exist.");
    }
  }
  /// overwrite base method
  void set(size_t iY, size_t iP, double value) override {
    if (iY >= m_ny) {
      throw std::out_of_range("Data index in Jacobian is out of range");
    }
    if (iP >= m_np) {
      throw Kernel::Exception::FitSizeWarning(m_np);
    }
    m_data[iY * m_np + iP] = value;
  }
  /// overwrite base method
  double get(size_t iY, size_t iP) override {
    if (iY >= m_ny) {
      throw std::out_of_range("Data index in Jacobian is out of range");
    }
    if (iP >= m_np) {
      throw Kernel::Exception::FitSizeWarning(m_np);
    }
    return m_data[iY * m_np + iP];
  }
  /// overwrite base method
  void zero() override { m_data.assign(m_data.size(), 0.0); }
};

} // namespace CurveFitting
} // namespace Mantid
