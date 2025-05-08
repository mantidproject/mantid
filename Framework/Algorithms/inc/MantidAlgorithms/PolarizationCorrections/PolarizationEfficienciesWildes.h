// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include <unordered_map>

namespace Mantid::Algorithms {

using namespace API;

class MANTID_ALGORITHMS_DLL PolarizationEfficienciesWildes : public API::Algorithm {
public:
  /// The string identifier for the algorithm. @see Algorithm::name
  std::string const name() const override { return "PolarizationEfficienciesWildes"; }

  /// A summary of the algorithm's purpose. @see Algorithm::summary
  std::string const summary() const override;

  /// The category of the algorithm. @see Algorithm::category
  std::string const category() const override { return "Reflectometry\\PolarizationCorrections"; }

  /// Returns related algorithms. @see Algorithm::seeAlso
  const std::vector<std::string> seeAlso() const override { return {"PolarizationCorrectionWildes"}; }

  /// The version number of the algorithm. @see Algorithm::version
  int version() const override { return 1; }

private:
  /// Setup the algorithm's properties and prepare constants.
  void init() override;

  /// Execute the algorithm with the provided properties.
  void exec() override;

  /// Check that the inputs to the algorithm are valid.
  std::map<std::string, std::string> validateInputs() override;

  /// Calculate Fp, Fa and Phi
  void calculateFlipperEfficienciesAndPhi();

  /// Calculate (2p-1) from intensities
  MatrixWorkspace_sptr calculateTPMO();

  /// Calculate the polarizer and/or analyser efficiencies, as requested
  void calculatePolarizerAndAnalyserEfficiencies(const bool solveForP, const bool solveForA);

  /// Set the algorithm outputs
  void setOutputs();

  /// Clear the values for all the algorithm member variables
  void resetMemberVariables();

  /// Sets the property value to its current value. For output workspace properties this will clear any workspaces being
  /// held by the property
  void resetPropertyValue(const std::string &propertyName);

  /// Populates the spin state workspaces map from ws group given key prefix.
  void populateSpinStateWorkspaces(const WorkspaceGroup_sptr &wsGrp, const std::string &keyPrefix = "");

  /// Populates the spin state workspaces map
  void mapSpinStateWorkspaces();

  // Convenience struct for handling of flipper workspaces
  struct FlipperWorkspaces {
    const MatrixWorkspace_sptr &ws00;
    const MatrixWorkspace_sptr &ws01;
    const MatrixWorkspace_sptr &ws10;
    const MatrixWorkspace_sptr &ws11;
  };

  /// Access flipper workspaces in the spin state workspaces map
  FlipperWorkspaces getFlipperWorkspaces(const bool mag = false);

  MatrixWorkspace_sptr m_wsFp = nullptr;
  MatrixWorkspace_sptr m_wsFa = nullptr;
  MatrixWorkspace_sptr m_wsPhi = nullptr;
  MatrixWorkspace_sptr m_wsP = nullptr;
  MatrixWorkspace_sptr m_wsA = nullptr;
  std::unordered_map<std::string, MatrixWorkspace_sptr> m_spinStateWorkspaces;
  bool m_magWsProvided = false;
};
} // namespace Mantid::Algorithms
