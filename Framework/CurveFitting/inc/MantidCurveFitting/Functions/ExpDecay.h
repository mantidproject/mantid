// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_EXPDECAY_H_
#define MANTID_CURVEFITTING_EXPDECAY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide exponential decay function: h*exp(-(x-c)/t)

@author Roman Tolchenov, Tessella plc
@date 05/11/2010
*/
class DLLExport ExpDecay : public API::ParamFunction, public API::IFunction1D {
public:
  /// Constructor
  ExpDecay();

  /// overwrite IFunction base class methods
  std::string name() const override { return "ExpDecay"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "General"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_EXPDECAY_H_*/
