// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid::Algorithms {

class MANTID_ALGORITHMS_DLL FlipperEfficiency : public API::Algorithm {
public:
  /// The string identifier for the algorithm. @see Algorithm::name
  std::string const name() const override { return "FlipperEfficiency"; }

  /// A summary of the algorithm's purpose. @see Algorithm::summary
  std::string const summary() const override;

  /// The category of the algorithm. @see Algorithm::category
  std::string const category() const override { return "SANS\\PolarizationCorrections"; }

  /// The version number of the algorithm. @see Algorithm::version
  int version() const override { return 1; }

private:
  /// Setup the algorithm's properties and prepare constants.
  void init() override;

  /// Execute the algorithm with the provided properties.
  void exec() override;

  /// Check that the inputs to the algorithm are valid.
  std::map<std::string, std::string> validateInputs() override;

  /// Save the given workspace to a given path as Nexus, applying the relevant extension if necessary.
  void saveToFile(API::MatrixWorkspace_sptr const &workspace, std::string const &filePathStr);

  /// Perform the main calculation for determining the efficiency on the given group.
  API::MatrixWorkspace_sptr calculateEfficiency(API::WorkspaceGroup_sptr const &groupWs);
};
} // namespace Mantid::Algorithms
