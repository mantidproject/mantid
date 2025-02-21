// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <cmath>
#include <memory>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Allow user to create a fit function which is the product of two or
more other fit functions.

@author Anders Markvardsen, ISIS, RAL
@date 4/4/2011
*/
class MANTID_CURVEFITTING_DLL ProductFunction : public API::CompositeFunction {
public:
  /// override IFunction base class methods
  std::string name() const override;
  /// Function you want to fit to.
  void function(const API::FunctionDomain &domain, API::FunctionValues &values) const override;
  /// Calculate the derivatives
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

protected:
  /// overwrite IFunction base class method, which declares function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
