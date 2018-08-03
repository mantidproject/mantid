#ifndef MANTID_ALGORITHMS_POLARIZATIONCORRECTIONWILDES_H_
#define MANTID_ALGORITHMS_POLARIZATIONCORRECTIONWILDES_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace Mantid {
namespace API {
class ISpectrum;
}

namespace Algorithms {

/** PolarizationCorrectionWildes : This algorithm corrects for non-ideal
  component efficiencies in polarized neutron analysis. It is based on
  [A. R. Wildes (2006) Neutron News, 17:2, 17-25,
  DOI: 10.1080/10448630600668738]

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL PolarizationCorrectionWildes
    : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  /// A convenience set of workspaces corresponding flipper configurations.
  struct WorkspaceMap {
    API::MatrixWorkspace_sptr mmWS{nullptr};
    API::MatrixWorkspace_sptr mpWS{nullptr};
    API::MatrixWorkspace_sptr pmWS{nullptr};
    API::MatrixWorkspace_sptr ppWS{nullptr};
    size_t size() const noexcept;
  };

  /// A convenience set of efficiency factors.
  struct EfficiencyMap {
    const API::ISpectrum *P1{nullptr};
    const API::ISpectrum *P2{nullptr};
    const API::ISpectrum *F1{nullptr};
    const API::ISpectrum *F2{nullptr};
  };

  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  void checkConsistentNumberHistograms(const WorkspaceMap &inputs);
  void checkConsistentX(const WorkspaceMap &inputs,
                        const EfficiencyMap &efficiencies);
  EfficiencyMap efficiencyFactors();
  WorkspaceMap directBeamCorrections(const WorkspaceMap &inputs,
                                     const EfficiencyMap &efficiencies);
  WorkspaceMap analyzerlessCorrections(const WorkspaceMap &inputs,
                                       const EfficiencyMap &efficiencies);
  WorkspaceMap twoInputCorrections(const WorkspaceMap &inputs,
                                   const EfficiencyMap &efficiencies);
  WorkspaceMap threeInputCorrections(const WorkspaceMap &inputs,
                                     const EfficiencyMap &efficiencies);
  WorkspaceMap fullCorrections(const WorkspaceMap &inputs,
                               const EfficiencyMap &efficiencies);
  API::WorkspaceGroup_sptr groupOutput(const WorkspaceMap &outputs);
  WorkspaceMap mapInputsToDirections(const std::vector<std::string> &flippers);
  void threeInputsSolve01(WorkspaceMap &inputs,
                          const EfficiencyMap &efficiencies);
  void threeInputsSolve10(WorkspaceMap &inputs,
                          const EfficiencyMap &efficiencies);
  void twoInputsSolve01And10(WorkspaceMap &fullInputs,
                             const WorkspaceMap &inputs,
                             const EfficiencyMap &efficiencies);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONCORRECTIONWILDES_H_ */
