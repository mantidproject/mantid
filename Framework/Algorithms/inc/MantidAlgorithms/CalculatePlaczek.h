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

/** CalculatePlaczek : Placzek 1st&2nd order correction for inelastic scattering
 */
class MANTID_ALGORITHMS_DLL CalculatePlaczek final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  // Category for quick search and doc page
  const std::string category() const override;
  // Documentation
  const std::string summary() const override;
  // seeAlso (documentation purpose)
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  void exec() override;

  // validator for input parameters
  std::map<std::string, std::string> validateInputs() override;

  double getPackingFraction(const API::MatrixWorkspace_const_sptr &ws);

  double getSampleTemperature();

  std::vector<double> getFluxCoefficient1();
  std::vector<double> getFluxCoefficient2();
  std::vector<double> getEfficiencyCoefficient1();
  std::vector<double> getEfficiencyCoefficient2();
};

} // namespace Algorithms
} // namespace Mantid
