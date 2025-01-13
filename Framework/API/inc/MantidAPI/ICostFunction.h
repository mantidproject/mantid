// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace API {
/** An interface for specifying the cost function to be used with Fit algorithm
   or minimizers,
    for example, the default being least squares fitting.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/05/2010
*/
class MANTID_API_DLL ICostFunction {
public:
  /// Virtual destructor
  virtual ~ICostFunction() = default;

  /// Get name of minimizer
  virtual std::string name() const = 0;

  /// Get short name of minimizer - useful for say labels in guis
  virtual std::string shortName() const { return "Quality"; }

  /// Get i-th parameter
  /// @param i :: Index of a parameter
  /// @return :: Value of the parameter
  virtual double getParameter(size_t i) const = 0;
  /// Set i-th parameter
  /// @param i :: Index of a parameter
  /// @param value :: New value of the parameter
  virtual void setParameter(size_t i, const double &value) = 0;
  /// Number of parameters
  virtual size_t nParams() const = 0;

  /// Calculate value of cost function
  virtual double val() const = 0;

  /// Calculate the derivatives of the cost function
  /// @param der :: Container to output the derivatives
  virtual void deriv(std::vector<double> &der) const = 0;

  /// Calculate the value and the derivatives of the cost function
  /// @param der :: Container to output the derivatives
  /// @return :: The value of the function
  virtual double valAndDeriv(std::vector<double> &der) const = 0;
};

/// define a shared pointer to a cost function
using ICostFunction_sptr = std::shared_ptr<ICostFunction>;

/**
 * Macro for declaring a new type of cost functions to be used with the
 * CostFunctionFactory
 */
#define DECLARE_COSTFUNCTION(classname, username)                                                                      \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_costfunction_##classname(                                                \
      ((Mantid::API::CostFunctionFactory::Instance().subscribe<classname>(#username)), 0));                            \
  }

} // namespace API
} // namespace Mantid
