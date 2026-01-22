// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/DllConfig.h"

#include <span>

namespace Mantid {

namespace Algorithms {
/** Generates the FlatCell workspace for ISIS SANS reduction of LOQ */
class MANTID_ALGORITHMS_DLL GenerateFlatCellWorkspaceLOQ : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "GenerateFlatCellWorkspaceLOQ"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Generates the FlatCell workspace for ISIS SANS reduction of LOQ.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  struct FlatCellStats {
    double normStdLAB{0.0};
    double normStdHAB{0.0};
  };

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execEvent();
  double mean(std::span<const double> values) const;
  double stddev(std::span<double> values) const;
  void scale(std::span<double> values, double factor) const;
  void createAndSaveMaskWorkspace(const API::MatrixWorkspace_sptr &ws, double normStdLAB, double normStdHAB);
  API::MatrixWorkspace_sptr integrateInput(const API::Workspace_sptr &ws);
  std::vector<double> extractIntegratedValues(const API::MatrixWorkspace_sptr &ws) const;
  FlatCellStats normalizeBanks(std::span<double> values) const;
};

} // namespace Algorithms
} // namespace Mantid
