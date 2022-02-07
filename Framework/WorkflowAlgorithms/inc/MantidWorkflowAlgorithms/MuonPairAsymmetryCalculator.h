// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** MuonPairAsymmetryCalculator : Calculates asymmetry for a given pair of
  groups, given the alpha value.
*/
class DLLExport MuonPairAsymmetryCalculator : public IMuonAsymmetryCalculator {
public:
  MuonPairAsymmetryCalculator(const API::WorkspaceGroup_sptr &inputWS, const std::vector<int> &summedPeriods,
                              const std::vector<int> &subtractedPeriods, const int firstPairIndex,
                              const int secondPairIndex, const double alpha = 1);

  /// Performs pair asymmetry calculation
  API::MatrixWorkspace_sptr calculate() const override;

private:
  /// Calculate asymmetry for the given workspace
  API::MatrixWorkspace_sptr asymmetryCalc(const API::Workspace_sptr &inputWS) const;

  /// Alpha value of the pair
  const double m_alpha;

  /// Workspace index of the first group of the pair
  const int m_firstPairIndex;

  /// Workspace index of the second group of the pair
  const int m_secondPairIndex;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
