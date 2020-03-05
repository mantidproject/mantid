// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEIQT_H_
#define MANTID_ALGORITHMS_CALCULATEIQT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MersenneTwister.h"

namespace Mantid {
namespace Algorithms {

class DLLExport CalculateIqt : public API::Algorithm {
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
  API::MatrixWorkspace_sptr
  monteCarloErrorCalculation(API::MatrixWorkspace_sptr sample,
                             API::MatrixWorkspace_sptr resolution,
                             const std::string &rebinParams, const int seed,
                             const bool calculateErrors, const int nIterations);

  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr workspace,
                                  const std::string &params);
  API::MatrixWorkspace_sptr integration(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  convertToPointData(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  extractFFTSpectrum(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr divide(API::MatrixWorkspace_sptr lhsWorkspace,
                                   API::MatrixWorkspace_sptr rhsWorkspace);
  API::MatrixWorkspace_sptr cropWorkspace(API::MatrixWorkspace_sptr workspace,
                                          const double xMax);
  API::MatrixWorkspace_sptr
  replaceSpecialValues(API::MatrixWorkspace_sptr workspace);

  API::MatrixWorkspace_sptr
  removeInvalidData(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  normalizedFourierTransform(API::MatrixWorkspace_sptr workspace,
                             const std::string &rebinParams);
  API::MatrixWorkspace_sptr
  calculateIqt(API::MatrixWorkspace_sptr workspace,
               API::MatrixWorkspace_sptr resolutionWorkspace,
               const std::string &rebinParams);
  API::MatrixWorkspace_sptr doSimulation(API::MatrixWorkspace_sptr sample,
                                         API::MatrixWorkspace_sptr resolution,
                                         const std::string &rebinParams,
                                         Kernel::MersenneTwister &mTwister);
  API::MatrixWorkspace_sptr setErrorsToStandardDeviation(
      const std::vector<API::MatrixWorkspace_sptr> &simulatedWorkspaces);
  API::MatrixWorkspace_sptr setErrorsToZero(
      const std::vector<API::MatrixWorkspace_sptr> &simulatedWorkspaces);
};

} // namespace Algorithms
} // namespace Mantid
#endif // MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
