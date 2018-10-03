// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_STRETCHEXP_H_
#define MANTID_CURVEFITTING_STRETCHEXP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide Streteched Exponential fitting function: h*exp(-(x/t)^b )

@author Jose Borreguero, NScD
@date 11/14/2011
*/
class DLLExport StretchExp : public API::ParamFunction,
                             public API::IFunction1D {
public:
  /// Constructor
  StretchExp();

  /// overwrite IFunction base class methods
  std::string name() const override { return "StretchExp"; }
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

#endif /*MANTID_CURVEFITTING_STRETCHEXP_H_*/
