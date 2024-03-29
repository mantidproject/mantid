// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {

namespace FrameworkTestHelpers {

class FunctionChangesNParams : public Mantid::API::IFunction1D, public Mantid::API::ParamFunction {
public:
  FunctionChangesNParams();
  std::string name() const override;
  void iterationStarting() override;
  void iterationFinished() override;

protected:
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) override;
  size_t m_maxNParams = 5;
  bool m_canChange = false;
};

} // namespace FrameworkTestHelpers
} // namespace Mantid
