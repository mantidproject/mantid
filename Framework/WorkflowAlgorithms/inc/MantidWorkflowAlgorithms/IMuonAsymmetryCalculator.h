// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <vector>

namespace Mantid {
namespace WorkflowAlgorithms {

/** IMuonAsymmetryCalculator : Abstract base class for muon asymmetry
  calculations
*/
class DLLExport IMuonAsymmetryCalculator {
public:
  IMuonAsymmetryCalculator(const API::WorkspaceGroup_sptr &inputWS, const std::vector<int> &summedPeriods,
                           const std::vector<int> &subtractedPeriods);
  virtual ~IMuonAsymmetryCalculator() = default;
  /// Overridden in derived classes to perform asymmetry calculation
  virtual API::MatrixWorkspace_sptr calculate() const = 0;

protected:
  /// Sums the specified periods in the input workspace group
  API::MatrixWorkspace_sptr sumPeriods(const std::vector<int> &periodsToSum) const;

  /// Subtracts one workspace from another (lhs - rhs)
  API::MatrixWorkspace_sptr subtractWorkspaces(const API::MatrixWorkspace_sptr &lhs,
                                               const API::MatrixWorkspace_sptr &rhs) const;

  /// Extracts a single spectrum from a workspace
  API::MatrixWorkspace_sptr extractSpectrum(const API::Workspace_sptr &inputWS, const int index) const;

  /// Input workspace
  const API::WorkspaceGroup_sptr m_inputWS;

  /// List of summed periods
  const std::vector<int> m_summedPeriods;

  /// List of subtracted periods
  const std::vector<int> m_subtractedPeriods;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
