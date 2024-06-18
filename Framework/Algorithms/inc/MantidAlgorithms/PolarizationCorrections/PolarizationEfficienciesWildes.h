// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid::Algorithms {

class MANTID_ALGORITHMS_DLL PolarizationEfficienciesWildes : public API::Algorithm {
public:
  /// The string identifier for the algorithm. @see Algorithm::name
  std::string const name() const override { return "PolarizationEfficienciesWildes"; }

  /// A summary of the algorithm's purpose. @see Algorithm::summary
  std::string const summary() const override;

  /// The category of the algorithm. @see Algorithm::category
  std::string const category() const override { return "Reflectometry\\PolarizationCorrections"; }

  /// The version number of the algorithm. @see Algorithm::version
  int version() const override { return 1; }

private:
  /// Setup the algorithm's properties and prepare constants.
  void init() override;

  /// Execute the algorithm with the provided properties.
  void exec() override;

  /// Check that the inputs to the algorithm are valid.
  std::map<std::string, std::string> validateInputs() override;
};
} // namespace Mantid::Algorithms
