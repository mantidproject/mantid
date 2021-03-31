// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/IFittingAlgorithm.h"

namespace Mantid {
namespace CurveFitting {

namespace CostFunctions {
class CostFuncFitting;
}

namespace Algorithms {

/**

  Calculate cost function for a function and a data set in a workspace.
*/
class MANTID_CURVEFITTING_DLL CalculateCostFunction : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"CalculateChiSquared", "Fit"}; }
  const std::string summary() const override;

private:
  void initConcrete() override;
  void execConcrete() override;

  /// Cache for the cost function
  std::shared_ptr<CostFunctions::CostFuncFitting> m_costFunction;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
