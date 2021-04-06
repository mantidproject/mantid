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
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"

namespace Mantid {

namespace CurveFitting {
namespace Algorithms {
class Fit;
}
} // namespace CurveFitting
namespace API {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
class FunctionHandler;
/** This is a specialization of IFunction for functions of one real argument.
    It uses FunctionDomain1D as a domain. Implement function1D(...) method in a
   concrete
    function. Implement functionDeriv1D to use analytical derivatives.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009
*/
class MANTID_API_DLL IFunction1D : public virtual IFunction {
public:
  /* Overidden methods */

  void function(const FunctionDomain &domain, FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  virtual void derivative(const FunctionDomain &domain, FunctionValues &values, const size_t order = 1) const;

  /// Function you want to fit to.
  virtual void function1D(double *out, const double *xValues, const size_t nData) const = 0;

  /// Function to calculate the derivatives of the data set
  virtual void derivative1D(double *out, const double *xValues, const size_t nData, const size_t order) const;

  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv1D(Jacobian *jacobian, const double *xValues, const size_t nData);

protected:
  template <typename FunctionType>
  using Function1DMethod = void (FunctionType::*)(double *, const double *, const size_t) const;
  /// Calculate a numerical derivative for the 1D data
  template <typename EvaluationMethod>
  void calcNumericalDerivative1D(Jacobian *jacobian, EvaluationMethod func1D, const double *xValues,
                                 const size_t nData);
  /// Calculate histogram data for the given bin boundaries.
  virtual void histogram1D(double *out, double left, const double *right, const size_t nBins) const;
  /// Derivatives of the histogram1D with respect to active parameters.
  virtual void histogramDerivative1D(Jacobian *jacobian, double left, const double *right, const size_t nBins) const;

  /// Logger instance
  static Kernel::Logger g_log;

  /// Making a friend
  friend class CurveFitting::Algorithms::Fit;
};

using IFunction1D_sptr = std::shared_ptr<IFunction1D>;

} // namespace API
} // namespace Mantid
