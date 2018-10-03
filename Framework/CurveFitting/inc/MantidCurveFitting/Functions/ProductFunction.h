// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PRODUCTFUNCTION_H_
#define MANTID_CURVEFITTING_PRODUCTFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include <boost/shared_array.hpp>
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Allow user to create a fit function which is the product of two or
more other fit functions.

@author Anders Markvardsen, ISIS, RAL
@date 4/4/2011
*/
class DLLExport ProductFunction : public API::CompositeFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "ProductFunction"; }
  /// Function you want to fit to.
  /// @param domain :: The space on which the function acts
  /// @param values :: The buffer for writing the calculated values. Must be big
  /// enough to accept dataSize() values
  void function(const API::FunctionDomain &domain,
                API::FunctionValues &values) const override;
  /// Calculate the derivatives
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian) override;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override{};
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PRODUCTFUNCTIONMW_H_*/
