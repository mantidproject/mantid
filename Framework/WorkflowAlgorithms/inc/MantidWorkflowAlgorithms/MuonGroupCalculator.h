#ifndef MANTID_WORKFLOWALGORITHMS_MUONGROUPCALCULATOR_H_
#define MANTID_WORKFLOWALGORITHMS_MUONGROUPCALCULATOR_H_

#include "MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** MuonGroupCalculator : Base class for Muon group counts/asymmetry calculators

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
class DLLExport MuonGroupCalculator : public IMuonAsymmetryCalculator {
public:
  MuonGroupCalculator(const Mantid::API::WorkspaceGroup_sptr inputWS,
                      const std::vector<int> summedPeriods,
                      const std::vector<int> subtractedPeriods,
                      const int groupIndex);
  void setStartEnd(const double start, const double end);

protected:
  /// Workspace index of the group to analyse
  const int m_groupIndex;
  double m_startX;
  double m_endX;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_MUONGROUPCALCULATOR_H_ */