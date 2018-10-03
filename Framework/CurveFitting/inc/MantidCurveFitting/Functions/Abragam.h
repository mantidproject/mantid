// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_ABRAGAM_H_
#define MANTID_CURVEFITTING_ABRAGAM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
//#include "MantidAPI/IPeakFunction.h"

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMW.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide Abragam fitting function for muon scientists

 @author Karl Palmen, ISIS, RAL
 @date 21/03/2012
 */

class DLLExport Abragam : public API::ParamFunction,
                          public API::IFunctionMW,
                          public API::IFunction1D {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "Abragam"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "Muon"; }
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian) override;

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void setActiveParameter(size_t i, double value) override;

  /// overwrite IFunction base class method that declares function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ABRAGAM_H_*/
