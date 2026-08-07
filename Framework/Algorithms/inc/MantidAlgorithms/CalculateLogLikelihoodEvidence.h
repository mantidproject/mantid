// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Boilerplate for comparing a list of workspaces using chi-squared style
 * metrics.
 */
class MANTID_ALGORITHMS_DLL CalculateLogLikelihoodEvidence final : public API::Algorithm {
public:
  const std::string name() const override { return "CalculateLogLikelihoodEvidence"; }
  const std::string summary() const override {
    return "Compares a list of workspaces and outputs log-likelihood evidence "
           "and relative factors.";
  }
  int version() const override { return 1; }
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  void init() override;
  void exec() override;
  double calculateLogLikelihoodEvidence(const API::MatrixWorkspace_sptr pdfWorkspace) const;
};

} // namespace Algorithms
} // namespace Mantid
