// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction_fwd.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/IFittingAlgorithm.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**
  Calculate chi squared for a function and a data set in a workspace.
*/
class MANTID_CURVEFITTING_DLL CalculateChiSquared : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateCostFunction", "Fit", "ProfileChiSquared1D"};
  }
  const std::string summary() const override;
  static void calcChiSquared(const API::IFunction &fun, size_t nParams, const API::FunctionDomain &domain,
                             API::FunctionValues &values, double &chiSquared, double &chiSquaredWeighted, double &dof);

private:
  void initConcrete() override;
  void execConcrete() override;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
