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
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide stetch exponential function for Muon scientists

@author Karl Palmen, ISIS, RAL
@date 12/03/2012
*/
class MANTID_CURVEFITTING_DLL StretchExpMuon : public API::ParamFunction, public API::IFunction1D {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "StretchExpMuon"; }
  const std::string category() const override { return "Muon\\MuonGeneric"; }

protected:
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
