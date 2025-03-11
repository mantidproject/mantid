// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidFrameworkTestHelpers/FunctionCreationHelper.h"
#include "MantidKernel/Exception.h"

namespace Mantid::FrameworkTestHelpers {

std::string FunctionChangesNParams::name() const { return "FunctionChangesNParams"; }

FunctionChangesNParams::FunctionChangesNParams() : Mantid::API::IFunction1D(), Mantid::API::ParamFunction() {
  this->declareParameter("A0", 0.0);
}

void FunctionChangesNParams::iterationStarting() { m_canChange = true; }

void FunctionChangesNParams::iterationFinished() {
  auto np = nParams();
  if (m_canChange && np < m_maxNParams) {
    declareParameter("A" + std::to_string(np), 0.0);
    throw Mantid::Kernel::Exception::FitSizeWarning(np, nParams());
  }
  m_canChange = false;
}

void FunctionChangesNParams::function1D(double *out, const double *xValues, const size_t nData) const {
  auto np = nParams();
  for (size_t i = 0; i < nData; ++i) {
    double y = getParameter(np - 1);
    if (np > 1) {
      for (size_t ip = np - 1; ip > 0; --ip) {
        y = getParameter(ip - 1) + xValues[i] * y;
      }
    }
    out[i] = y;
  }
}

void FunctionChangesNParams::functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues, const size_t nData) {
  auto np = nParams();
  for (size_t i = 0; i < nData; ++i) {
    double y = 1.0;
    out->set(i, 0, y);
    if (np > 1) {
      for (size_t ip = 1; ip < np; ++ip) {
        y = xValues[i] * y;
        out->set(i, ip, y);
      }
    }
  }
}

} // namespace Mantid::FrameworkTestHelpers
