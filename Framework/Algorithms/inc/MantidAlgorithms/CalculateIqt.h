// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/MersenneTwister.h"

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL CalculateIqt final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  std::string rebinParamsAsString();
  API::MatrixWorkspace_sptr monteCarloErrorCalculation(const API::MatrixWorkspace_sptr &sample,
                                                       const API::MatrixWorkspace_sptr &resolution,
                                                       const std::string &rebinParams, const int seed,
                                                       const bool calculateErrors, const int nIterations,
                                                       const bool enforceNormalization);

  API::MatrixWorkspace_sptr rebin(const API::MatrixWorkspace_sptr &workspace, const std::string &params);
  API::MatrixWorkspace_sptr integration(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr convertToPointData(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr extractFFTSpectrum(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr divide(const API::MatrixWorkspace_sptr &lhsWorkspace,
                                   const API::MatrixWorkspace_sptr &rhsWorkspace);
  API::MatrixWorkspace_sptr cropWorkspace(const API::MatrixWorkspace_sptr &workspace, const double xMax);
  API::MatrixWorkspace_sptr replaceSpecialValues(const API::MatrixWorkspace_sptr &workspace);

  API::MatrixWorkspace_sptr removeInvalidData(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr fourierTransform(API::MatrixWorkspace_sptr workspace, const std::string &rebinParams);
  API::MatrixWorkspace_sptr calculateIqt(API::MatrixWorkspace_sptr workspace,
                                         const API::MatrixWorkspace_sptr &resolutionWorkspace,
                                         const std::string &rebinParams, const bool enforceNormalization);
  API::MatrixWorkspace_sptr doSimulation(API::MatrixWorkspace_sptr sample, const API::MatrixWorkspace_sptr &resolution,
                                         const std::string &rebinParams, Kernel::MersenneTwister &mTwister,
                                         const bool enforceNormalization);
  API::MatrixWorkspace_sptr
  setErrorsToStandardDeviation(const std::vector<API::MatrixWorkspace_sptr> &simulatedWorkspaces);
  API::MatrixWorkspace_sptr setErrorsToZero(const std::vector<API::MatrixWorkspace_sptr> &simulatedWorkspaces);

  API::MatrixWorkspace_sptr m_sampleIntegral;
  API::MatrixWorkspace_sptr m_resolutionIntegral;
};

} // namespace Algorithms
} // namespace Mantid
