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

  /// Calculate the value of Phi using the non-magnetic workspace intensities
  MatrixWorkspace_sptr calculatePhi(const MatrixWorkspace_sptr &ws00, const MatrixWorkspace_sptr &ws01,
                                    const MatrixWorkspace_sptr &ws10, const MatrixWorkspace_sptr &ws11);

  /// Calculate the value of Rho from the polarizing flipper efficiency (Fp)
  MatrixWorkspace_sptr calculateRho(const MatrixWorkspace_sptr &wsFp);

  /// Calculate the value of Alpha from the analysing flipper efficiency (Fa)
  MatrixWorkspace_sptr calculateAlpha(const MatrixWorkspace_sptr &wsFa);

  /// Calculate (2p-1) from Phi, Fp, Fa and the magnetic workspace intensities
  MatrixWorkspace_sptr calculateTPMOFromPhi(const WorkspaceGroup_sptr &magWsGrp, const MatrixWorkspace_sptr &wsFp,
                                            const MatrixWorkspace_sptr &wsFa, const MatrixWorkspace_sptr &wsPhi);

  /// Calculate the polarizer and/or analyser efficiencies, as requested
  void calculatePolarizerAndAnalyserEfficiencies(const MatrixWorkspace_sptr &wsFp, const MatrixWorkspace_sptr &wsFa,
                                                 const MatrixWorkspace_sptr &wsPhi, const bool solveForP,
                                                 MatrixWorkspace_sptr &wsP, const bool solveForA,
                                                 MatrixWorkspace_sptr &wsA);

  /// If either the polarizer or the analyser efficiency is known, use the relationship Phi = (2p-1)(2a-1) to solve for
  /// the other efficiency
  MatrixWorkspace_sptr solveForUnknownEfficiency(const MatrixWorkspace_sptr &wsPhi,
                                                 const MatrixWorkspace_sptr &knownEfficiency);

  ///  Set the algorithm outputs
  void setOutputs(const MatrixWorkspace_sptr &wsPhi, const MatrixWorkspace_sptr &wsFp, const MatrixWorkspace_sptr &wsFa,
                  const MatrixWorkspace_sptr &wsP, const MatrixWorkspace_sptr &wsA);
};
} // namespace Mantid::Algorithms
