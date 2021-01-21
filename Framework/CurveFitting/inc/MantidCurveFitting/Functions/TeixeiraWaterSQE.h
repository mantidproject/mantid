// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "MantidCurveFitting/Functions/FunctionQDepends.h"
// 3rd party library headers (N/A)
// standard library headers (N/A)

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
@author Jose Borreguero, NScD
@date 25/09/2016
*/

/**
 * @brief Teixeira's model to describe the translational diffusion of water
 */
class MANTID_CURVEFITTING_DLL TeixeiraWaterSQE : public FunctionQDepends {

public:
  std::string name() const override { return "TeixeiraWaterSQE"; }
  const std::string category() const override { return "QuasiElastic"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(Mantid::API::Jacobian *jacobian, const double *xValues, const size_t nData) override;

protected:
  void declareParameters() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
