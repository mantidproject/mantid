#ifndef MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATOR_H_
#define MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATOR_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AlgorithmManager.h"

#include <vector>

namespace Mantid {
namespace WorkflowAlgorithms {

/** IMuonAsymmetryCalculator : Abstract base class for muon asymmetry
  calculations

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IMuonAsymmetryCalculator {
public:
  IMuonAsymmetryCalculator(const API::WorkspaceGroup_sptr inputWS,
                           const std::vector<int> summedPeriods,
                           const std::vector<int> subtractedPeriods);
  virtual ~IMuonAsymmetryCalculator() = default;
  /// Overridden in derived classes to perform asymmetry calculation
  virtual API::MatrixWorkspace_sptr calculate() const = 0;

protected:
  /// Sums the specified periods in the input workspace group
  API::MatrixWorkspace_sptr
  sumPeriods(const std::vector<int> &periodsToSum) const;

  /// Subtracts one workspace from another (lhs - rhs)
  API::MatrixWorkspace_sptr
  subtractWorkspaces(const API::MatrixWorkspace_sptr &lhs,
                     const API::MatrixWorkspace_sptr &rhs) const;

  /// Extracts a single spectrum from a workspace
  API::MatrixWorkspace_sptr extractSpectrum(const API::Workspace_sptr &inputWS,
                                            const int index) const;

  /// Input workspace
  const API::WorkspaceGroup_sptr m_inputWS;

  /// List of summed periods
  const std::vector<int> m_summedPeriods;

  /// List of subtracted periods
  const std::vector<int> m_subtractedPeriods;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_IMUONASYMMETRYCALCULATOR_H_ */