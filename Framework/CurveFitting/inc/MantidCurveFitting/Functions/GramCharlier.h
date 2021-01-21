// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
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
  Implements a Gram-Charlier A series expansion.
*/
class MANTID_CURVEFITTING_DLL GramCharlier : public API::ParamFunction, public API::IFunction1D {
protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

public:
  std::string name() const override { return "GramCharlier"; }
  void function1D(double *out, const double *x, const size_t n) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
