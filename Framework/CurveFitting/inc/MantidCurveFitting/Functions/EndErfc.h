// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_ENDERFC_H_
#define MANTID_CURVEFITTING_ENDERFC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide Errore function erfc()for calibrating the end of a tube.

 @author Karl Palmen, ISIS, RAL
 @date 30/04/2012
 */

class DLLExport EndErfc : public API::ParamFunction, public API::IFunction1D {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "EndErfc"; }

  /// overwrite IFunction base class methods
  void setActiveParameter(size_t i, double value) override;
  const std::string category() const override { return "Calibrate"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  /// overwrite IFunction base class method that declares function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ENDERFC_H_*/
