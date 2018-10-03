// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_ESTIMATEFITPARAMETERS_H_
#define MANTID_CURVEFITTING_ESTIMATEFITPARAMETERS_H_

#include "MantidCurveFitting/IFittingAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**

  Estimate parameters of a fitting function using a Monte Carlo algorithm.
*/
class DLLExport EstimateFitParameters : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Fit", "EstimatePeakErrors"};
  }
  const std::string summary() const override;

private:
  void initConcrete() override;
  void execConcrete() override;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_ESTIMATEFITPARAMETERS_H_ */
