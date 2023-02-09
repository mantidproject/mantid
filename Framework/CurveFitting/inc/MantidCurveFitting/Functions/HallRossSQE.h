// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/Functions/FunctionQDepends.h"

namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
} // namespace API

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
 * @brief Hall-Ross jump diffusion model
 */
class MANTID_CURVEFITTING_DLL HallRossSQE : public FunctionQDepends {

public:
  std::string name() const override { return "HallRossSQE"; }
  const std::string category() const override { return "QuasiElastic"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(Mantid::API::Jacobian *jacobian, const double *xValues, const size_t nData) override;

protected:
  void declareParameters() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
