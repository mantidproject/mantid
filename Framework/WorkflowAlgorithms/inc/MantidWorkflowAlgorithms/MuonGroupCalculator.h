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

/** MuonGroupCalculator : Base class for Muon group counts/asymmetry calculators
 */
class DLLExport MuonGroupCalculator : public IMuonAsymmetryCalculator {
public:
  MuonGroupCalculator(const Mantid::API::WorkspaceGroup_sptr &inputWS, const std::vector<int> &summedPeriods,
                      const std::vector<int> &subtractedPeriods, const int groupIndex);
  void setStartEnd(const double start, const double end);
  void setWSName(const std::string &wsName);

protected:
  /// Workspace index of the group to analyse
  const int m_groupIndex;
  double m_startX;
  double m_endX;
  std::string m_wsName;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
