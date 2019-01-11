// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_CALCULATECHISQUARED_H_
#define MANTID_CURVEFITTING_CALCULATECHISQUARED_H_

#include "MantidCurveFitting/IFittingAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**

  Calculate chi squared for a function and a data set in a workspace.
  Optionally outputs slices of the chi^2 along the parameter axes
  and estimates the standard deviations.
*/
class DLLExport CalculateChiSquared : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateCostFunction", "Fit"};
  }
  const std::string summary() const override;

private:
  void initConcrete() override;
  void execConcrete() override;
  void estimateErrors();
  void unfixParameters();
  void refixParameters();

  /// Cache indices of fixed parameters
  std::vector<size_t> m_fixedParameters;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CALCULATECHISQUARED_H_ */
