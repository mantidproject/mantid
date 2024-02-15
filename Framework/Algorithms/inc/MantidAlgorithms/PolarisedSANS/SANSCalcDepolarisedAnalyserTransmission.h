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

class MANTID_ALGORITHMS_DLL SANSCalcDepolarisedAnalyserTransmission : public API::Algorithm {
public:
  /// The string identifier for the algorithm.
  std::string const name() const override { return "SANSCalcDepolarisedAnalyserTransmission"; }

  /// A summary of the algorithm's purpose.
  std::string const summary() const override {
    return "Calculate the transmission rate through a depolarised He3 cell.";
  }

  /// The version number of the algorithm.
  int version() const override { return 1; }

private:
  /// Setup the algorithm's properties and prepare constants.
  void init() override;

  /// Execute the algorithm with the provided properties.
  void exec() override;
};
} // namespace Mantid::Algorithms
