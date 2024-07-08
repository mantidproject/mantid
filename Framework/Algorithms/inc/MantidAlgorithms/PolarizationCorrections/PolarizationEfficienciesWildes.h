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

  /// Calculate (2p-1) from Phi, Fp, Fa and the magnetic workspace intensities
  MatrixWorkspace_sptr calculateTPMOFromPhi(const WorkspaceGroup_sptr &magWsGrp);

  /// Calculate the polarizer and/or analyser efficiencies, as requested
  void calculatePolarizerAndAnalyserEfficiencies(const bool solveForP, const bool solveForA);

  /// If either the polarizer or the analyser efficiency is known, use the relationship Phi = (2p-1)(2a-1) to solve for
  /// the other efficiency
  MatrixWorkspace_sptr solveForUnknownEfficiency(const MatrixWorkspace_sptr &knownEfficiency);

  /// Solve for the unknown efficiency from either (2p-1) or (2a-1) using the relationship Phi = (2p-1)(2a-1)
  MatrixWorkspace_sptr solveUnknownEfficiencyFromTXMO(const MatrixWorkspace_sptr &wsTXMO);

  ///  Set the algorithm outputs
  void setOutputs();

  MatrixWorkspace_sptr m_wsFp = nullptr;
  MatrixWorkspace_sptr m_wsFa = nullptr;
  MatrixWorkspace_sptr m_wsPhi = nullptr;
  MatrixWorkspace_sptr m_wsP = nullptr;
  MatrixWorkspace_sptr m_wsA = nullptr;
};
} // namespace Mantid::Algorithms
