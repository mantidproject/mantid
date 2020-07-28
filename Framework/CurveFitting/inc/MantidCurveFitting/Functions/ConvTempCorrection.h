// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
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
Temperature correction used in the convolution fitting tab within the IDA GUI
**/
class MANTID_CURVEFITTING_DLL ConvTempCorrection : public API::ParamFunction,
                                                   public API::IFunction1D {
public:
  std::string name() const override { return "ConvTempCorrection"; }
  const std::string category() const override { return "QuasiElastic"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void init() override;

private:
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
