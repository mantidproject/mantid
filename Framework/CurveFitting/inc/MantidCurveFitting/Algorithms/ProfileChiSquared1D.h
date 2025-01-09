// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/EigenMatrix.h"
#include "MantidCurveFitting/Functions/ChebfunBase.h"
#include "MantidCurveFitting/IFittingAlgorithm.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**
Profiles chi2 about its minimum to find parameter errors
*/
class MANTID_CURVEFITTING_DLL ProfileChiSquared1D : public IFittingAlgorithm {
public:
  ProfileChiSquared1D();
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"CalculateChiSquared", "Fit"}; }
  const std::string summary() const override;

private:
  void initConcrete() override;
  void execConcrete() override;
  void unfixParameters();
  void refixParameters();
  EigenMatrix getCovarianceMatrix();
  std::tuple<double, double> getChiSquaredRoots(const Functions::ChebfunBase_sptr &approximation,
                                                std::vector<double> &coeffs, double qvalue, double rBound,
                                                double lBound) const;
  /// Cache indices of fixed parameters
  std::vector<size_t> m_fixedParameters;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
