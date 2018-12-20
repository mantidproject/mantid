// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_CALCULATECOSTFUNCTION_H_
#define MANTID_CURVEFITTING_CALCULATECOSTFUNCTION_H_

#include "MantidCurveFitting/IFittingAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {

namespace CostFunctions {
class CostFuncFitting;
}

namespace Algorithms {

/**

  Calculate cost function for a function and a data set in a workspace.
*/
class DLLExport CalculateCostFunction : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateChiSquared", "Fit"};
  }
  const std::string summary() const override;

private:
  void initConcrete() override;
  void execConcrete() override;

  /// Cache for the cost function
  boost::shared_ptr<CostFunctions::CostFuncFitting> m_costFunction;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CALCULATECOSTFUNCTION_H_ */
