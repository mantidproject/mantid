// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace API {
/**
    Represents the Jacobian in IFitFunction::functionDeriv.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class Jacobian {
public:
  /**  Set a value to a Jacobian matrix element.
   *   @param iY :: The index of a data point.
   *   @param iP :: The index of a declared parameter.
   *   @param value :: The derivative value.
   */
  virtual void set(size_t iY, size_t iP, double value) = 0;

  /**  Get the value to a Jacobian matrix element.
   *   @param iY :: The index of a data point.
   *   @param iP :: The index of a declared parameter.
   */
  virtual double get(size_t iY, size_t iP) = 0;

  /** Zero all matrix elements.
   */
  virtual void zero() = 0;

  ///@cond do not document
  /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
   *   @param value :: Value to add
   */
  virtual void addNumberToColumn(const double &value, const size_t &iActiveP) {
    (void)value;
    (void)iActiveP; // Avoid compiler warning
    throw Kernel::Exception::NotImplementedError("No addNumberToColumn() method of Jacobian provided");
  }
  ///@endcond

  /// Virtual destructor
  virtual ~Jacobian() = default;

protected:
};

} // namespace API
} // namespace Mantid
