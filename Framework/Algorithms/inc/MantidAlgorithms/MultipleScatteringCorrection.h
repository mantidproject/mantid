// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** MultipleScatteringCorrection : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL MultipleScatteringCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MultipleScatteringCorrection"; };

  /// Algorithm's version
  int version() const override { return 1; };

  /// Algorithm's category
  const std::string category() const override { return "CorrectionFunctions"; };

  /// Algorithm's summary
  const std::string summary() const override {
    return "Multiple Scattering Correction with the assumption of sample only and elastic scattering only";
  };

  /// Algorithm's see also
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection", "PaalmanPingsAbsorptionCorrection", "PaalmanPingsMonteCarloAbsorption"};
  };

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Algorithms
} // namespace Mantid
