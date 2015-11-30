#ifndef MANTID_WORKFLOWALGORITHMS_MUONPAIRASYMMETRYCALCULATOR_H_
#define MANTID_WORKFLOWALGORITHMS_MUONPAIRASYMMETRYCALCULATOR_H_

#include "MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** MuonPairAsymmetryCalculator : Calculates asymmetry for a given pair of
  groups, given the alpha value.

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
class DLLExport MuonPairAsymmetryCalculator : public IMuonAsymmetryCalculator {
public:
  MuonPairAsymmetryCalculator(const API::WorkspaceGroup_sptr inputWS,
                              const std::vector<int> summedPeriods,
                              const std::vector<int> subtractedPeriods,
                              const int firstPairIndex,
                              const int secondPairIndex,
                              const double alpha = 1);
  virtual ~MuonPairAsymmetryCalculator();

  /// Performs pair asymmetry calculation
  virtual API::MatrixWorkspace_sptr calculate() const override;

private:
  /// Calculate asymmetry for the given workspace
  API::MatrixWorkspace_sptr
  asymmetryCalc(const API::Workspace_sptr &inputWS) const;

  /// Alpha value of the pair
  const double m_alpha;

  /// Workspace index of the first group of the pair
  const int m_firstPairIndex;

  /// Workspace index of the second group of the pair
  const int m_secondPairIndex;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_MUONPAIRASYMMETRYCALCULATOR_H_ */