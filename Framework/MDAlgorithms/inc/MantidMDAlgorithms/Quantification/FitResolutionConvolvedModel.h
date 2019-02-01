// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_
#define MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace MDAlgorithms {

class DLLExport FitResolutionConvolvedModel : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fits a cuts/slices from an MDEventWorkspace using a resolution "
           "function convolved with a foreground model";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"SimulateResolutionConvolvedModel"};
  }

  int version() const override;
  const std::string category() const override;

protected:
  /// Returns the number of iterations that should be performed
  virtual int niterations() const;
  /// Returns the name of the max iterations property
  std::string maxIterationsPropertyName() const;
  /// Returns the name of the output parameters property
  std::string outputParsPropertyName() const;
  /// Returns the name of the covariance matrix property
  std::string covMatrixPropertyName() const;
  /// Create the function string required by fit
  std::string createFunctionString() const;

  void init() override;
  void exec() override;

private:
  /// Create the fitting Child Algorithm
  API::IAlgorithm_sptr createFittingAlgorithm();
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_ */
