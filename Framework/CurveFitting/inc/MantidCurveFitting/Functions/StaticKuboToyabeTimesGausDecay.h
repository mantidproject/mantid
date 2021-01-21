// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  StaticKuboToyabeTimesGausDecay fitting function.

  Represents multiplication of two other fitting functions: StaticKuboToyabe and
  GausDecay.

  @author Arturs Bekasovs
  @date 27/09/2013
*/
class MANTID_CURVEFITTING_DLL StaticKuboToyabeTimesGausDecay : public API::ParamFunction, public API::IFunction1D {
public:
  std::string name() const override { return "StaticKuboToyabeTimesGausDecay"; }

  const std::string category() const override { return "Muon\\MuonGeneric"; }

protected:
  void function1D(double *out, const double *xValues, const size_t nData) const override;

  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
