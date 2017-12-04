#ifndef MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_
#define MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace Mantid {
namespace HistogramData {
class HistogramY;
}

namespace Algorithms {

/** PolarizationEfficiencyCor : TODO: DESCRIPTION

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL PolarizationEfficiencyCor : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  struct WorkspaceMap {
    API::MatrixWorkspace_sptr mmWS{nullptr};
    API::MatrixWorkspace_sptr mpWS{nullptr};
    API::MatrixWorkspace_sptr pmWS{nullptr};
    API::MatrixWorkspace_sptr ppWS{nullptr};
    size_t size() const noexcept;
  };

  struct EfficiencyMap {
    const HistogramData::HistogramY *P1{nullptr};
    const HistogramData::HistogramY *P2{nullptr};
    const HistogramData::HistogramY *F1{nullptr};
    const HistogramData::HistogramY *F2{nullptr};
  };

  void init() override;
  void exec() override;
  EfficiencyMap efficiencyFactors();
  WorkspaceMap analyzerlessCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies);
  WorkspaceMap threeInputCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies);
  WorkspaceMap fullCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies);
  API::WorkspaceGroup_sptr groupOutput(const WorkspaceMap &outputs);
  WorkspaceMap mapInputsToDirections();
  void solve01(WorkspaceMap &inputs, const EfficiencyMap &efficiencies);
  void solve10(WorkspaceMap &inputs, const EfficiencyMap &efficiencies);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_ */
