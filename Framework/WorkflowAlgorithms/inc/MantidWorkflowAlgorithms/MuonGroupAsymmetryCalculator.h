// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidWorkflowAlgorithms/MuonGroupCalculator.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** MuonGroupAsymmetryCalculator : Calculates asymmetry between given group
  (specified via GroupIndex) and Muon exponential decay
*/
class DLLExport MuonGroupAsymmetryCalculator : public MuonGroupCalculator {
public:
  MuonGroupAsymmetryCalculator(const API::WorkspaceGroup_sptr &inputWS, const std::vector<int> &summedPeriods,
                               const std::vector<int> &subtractedPeriods, const int groupIndex,
                               const double start = 0.0, const double end = 30.0, const std::string &wsName = "");
  /// Performs group asymmetry calculation
  API::MatrixWorkspace_sptr calculate() const override;

private:
  /// Removes exponential decay from the workspace
  API::MatrixWorkspace_sptr removeExpDecay(const API::Workspace_sptr &inputWS, const int index) const;
  API::MatrixWorkspace_sptr estimateAsymmetry(const API::Workspace_sptr &inputWS, const int index) const;
};
double getStoredNorm();
} // namespace WorkflowAlgorithms
} // namespace Mantid
