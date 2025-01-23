// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <cmath>
#include <complex>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ReflectivityMulf : Calculate the ReflectivityMulf from a simple layer model.
 */
class MANTID_CURVEFITTING_DLL ReflectivityMulf : public API::IFunction1D, public API::ParamFunction {
public:
  ReflectivityMulf();

  void init() override;

  /// Overwrite IFunction base class
  std::string name() const override { return "ReflectivityMulf"; }

  const std::string category() const override { return "General"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

private:
  /// ReflectivityMulf layers
  int m_nlayer, m_nlayer_old;
};

using ReflectivityMulf_sptr = std::shared_ptr<ReflectivityMulf>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
